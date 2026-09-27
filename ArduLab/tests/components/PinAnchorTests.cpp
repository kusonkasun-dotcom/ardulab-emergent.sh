// PinAnchor stability tests (Plan §8.2): value semantics, millimeter
// preservation through the snapshot path, and freedom from any SQL/JSON/
// scene/wire behavior.

#include "components/ComponentSnapshot.h"
#include "components/PinAnchor.h"

#include <QtTest/QtTest>

#include <type_traits>

using namespace ardulab::core;
using namespace ardulab::components;

class PinAnchorTests final : public QObject
{
    Q_OBJECT

private slots:
    void isPlainValueObject()
    {
        static_assert(std::is_copy_constructible_v<PinAnchor>, "PinAnchor must be copyable");
        static_assert(std::is_nothrow_move_constructible_v<PinAnchor>, "PinAnchor must be cheaply movable");
        static_assert(!std::is_polymorphic_v<PinAnchor>, "PinAnchor must not be polymorphic");
        QVERIFY(!PinAnchor().isValid());
    }

    void equalityIsByPinNumberAndMillimeterPosition()
    {
        const PinAnchor a(QStringLiteral("1"), PointMm(-1.0, 0.0));
        const PinAnchor b(QStringLiteral("1"), PointMm(-1.0, 0.0));
        const PinAnchor c(QStringLiteral("2"), PointMm(-1.0, 0.0));
        const PinAnchor d(QStringLiteral("1"), PointMm(-1.0, 0.5));
        QVERIFY(a == b);
        QVERIFY(a != c);
        QVERIFY(a != d);
    }

    void anchorPreservesExactMillimeterValuesFromPin()
    {
        Pin pin;
        pin.pinNumber = QStringLiteral("A3");
        pin.pinName = QStringLiteral("GPIO23");
        pin.position = PointMm(12.345678, -0.5);
        pin.anchorPosition = PointMm(13.0, -0.5); // anchor may differ from body position

        const PinAnchor anchor = pin.anchor();
        QCOMPARE(anchor.pinNumber(), QStringLiteral("A3"));
        QCOMPARE(anchor.position().x, 13.0);
        QCOMPARE(anchor.position().y, -0.5);
        QVERIFY(anchor.position() != pin.position);
    }

    void snapshotReturnsAnchorsUnchanged()
    {
        Component c;
        c.componentId = ComponentId(QStringLiteral("MMCU1-A-ESP32-WROOM32"));
        c.name = QStringLiteral("ESP32-WROOM-32");
        c.categoryId = CategoryId(QStringLiteral("MCU_MODULE"));
        ComponentVersion v;
        v.componentId = c.componentId;
        v.version = QStringLiteral("1.0.0");
        v.versionId = makeComponentVersionId(c.componentId, v.version);

        std::vector<Pin> pins(3);
        for (int i = 0; i < 3; ++i) {
            pins[static_cast<std::size_t>(i)].pinNumber = QString::number(i + 1);
            pins[static_cast<std::size_t>(i)].anchorPosition = PointMm(-9.0, 1.27 * i);
        }

        const ComponentSnapshot snapshot(c, v, Package{}, pins, {});
        const auto anchors = snapshot.pinAnchors();
        QCOMPARE(anchors.size(), std::size_t{3});
        QCOMPARE(anchors[2].pinNumber(), QStringLiteral("3"));
        QVERIFY(anchors[2].position() == PointMm(-9.0, 2.54));
    }

    void pinWithoutNumberIsNotConnectable()
    {
        Pin pin;
        pin.pinName = QStringLiteral("unnamed");
        QVERIFY(!pin.isConnectable());
        pin.pinNumber = QStringLiteral("7");
        QVERIFY(pin.isConnectable());
    }
};

QTEST_APPLESS_MAIN(PinAnchorTests)
#include "PinAnchorTests.moc"
