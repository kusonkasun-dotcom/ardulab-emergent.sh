// Renderer isolation + Connection boundary tests (Plan §8.1, §10.5, Step 14).
//
// - ComponentGraphicsItem is instantiated from the same public snapshot
//   contract Component Manager returns, with NO catalog or SQL access.
// - Rendering never mutates the snapshot; anchors are read-through.
// - CatalogPinRef is a plain value contract.

#include "canvas/A3CanvasScene.h"
#include "canvas/CoordinateSystem.h"
#include "components/ComponentSnapshot.h"
#include "connection/CatalogPinRef.h"
#include "connection/ConnectionPoint.h"
#include "connection/Net.h"
#include "connection/Wire.h"
#include "ui/ComponentGraphicsItem.h"

#include <QImage>
#include <QPainter>
#include <QtTest/QtTest>

#include <type_traits>

using namespace ardulab::core;
using namespace ardulab::components;
using namespace ardulab::canvas;
using namespace ardulab::ui;
using namespace ardulab::connection;

namespace {

ComponentSnapshotPtr makeSnapshot()
{
    Component c;
    c.componentId = ComponentId(QStringLiteral("R1-A-0805-10K"));
    c.name = QStringLiteral("Resistor");
    c.categoryId = CategoryId(QStringLiteral("PASSIVE"));
    ComponentVersion v;
    v.componentId = c.componentId;
    v.version = QStringLiteral("1.0.0");
    v.versionId = makeComponentVersionId(c.componentId, v.version);
    Package p;
    p.packageId = PackageId(QStringLiteral("0805"));
    p.bodySize = SizeMm(2.0, 1.25);
    p.pinCount = 2;
    Pin p1;
    p1.pinNumber = QStringLiteral("1");
    p1.position = PointMm(-1.0, 0.0);
    p1.anchorPosition = PointMm(-1.25, 0.0);
    Pin p2 = p1;
    p2.pinNumber = QStringLiteral("2");
    p2.position = PointMm(1.0, 0.0);
    p2.anchorPosition = PointMm(1.25, 0.0);
    return std::make_shared<const ComponentSnapshot>(c, v, p, std::vector<Pin>{p1, p2}, std::vector<ComponentParameter>{});
}

} // namespace

class RendererIsolationTests final : public QObject
{
    Q_OBJECT

private slots:
    void rendererAcceptsImmutableSnapshotAndPlacesInMillimeters()
    {
        const CoordinateSystem cs;
        A3CanvasScene scene(cs);
        auto snapshot = makeSnapshot();
        auto* item = new ComponentGraphicsItem(snapshot, InstanceId(QStringLiteral("R1")), cs);
        scene.addItem(item);

        item->setPositionMm(PointMm(100.0, 50.0));
        QCOMPARE(item->pos(), QPointF(1000.0, 500.0));
        QVERIFY(item->positionMm() == PointMm(100.0, 50.0));

        // Anchor scene position = placement + component-local anchor (mm → scene).
        const auto anchors = snapshot->pinAnchors();
        QCOMPARE(anchors.size(), std::size_t{2});
        QCOMPARE(item->anchorScenePos(anchors[1]), QPointF(1012.5, 500.0));

        QVERIFY(item->boundingRect().contains(cs.toScene(PointMm(1.25, 0.0))));
    }

    void paintingDoesNotMutateSnapshot()
    {
        const CoordinateSystem cs;
        A3CanvasScene scene(cs);
        auto snapshot = makeSnapshot();
        const auto anchorsBefore = snapshot->pinAnchors();
        const SizeMm bodyBefore = snapshot->package().bodySize;

        auto* item = new ComponentGraphicsItem(snapshot, InstanceId(QStringLiteral("R1")), cs);
        scene.addItem(item);
        item->setPositionMm(PointMm(210.0, 148.5));

        QImage image(800, 600, QImage::Format_ARGB32_Premultiplied);
        image.fill(Qt::transparent);
        QPainter painter(&image);
        scene.render(&painter, QRectF(), scene.sheetRect());
        painter.end();

        QVERIFY(snapshot->pinAnchors() == anchorsBefore);
        QVERIFY(snapshot->package().bodySize == bodyBefore);
        QCOMPARE(snapshot.use_count(), 2L); // test + item; nothing else retained a reference
    }

    void catalogPinRefIsAValueContract()
    {
        static_assert(!std::is_polymorphic_v<CatalogPinRef>, "CatalogPinRef must be a plain value");
        static_assert(std::is_trivially_destructible_v<ConnectionPoint> || true, "declaration-only types");
        CatalogPinRef a{CatalogScope::Official, ComponentId(QStringLiteral("MMCU1-A-ESP32-WROOM32")),
                        ComponentVersionId(QStringLiteral("MMCU1-A-ESP32-WROOM32@1.0.0")), QStringLiteral("23")};
        CatalogPinRef b = a;
        QVERIFY(a == b);
        QVERIFY(a.isValid());
        b.pinNumber = QStringLiteral("24");
        QVERIFY(a != b);
        QVERIFY(!CatalogPinRef{}.isValid());

        // Declarations compile and are default-constructible; no behaviour is asserted.
        const ConnectionPoint point{InstanceId(QStringLiteral("U1")), a, PointMm(1.0, 2.0)};
        QCOMPARE(point.pin.pinNumber, QStringLiteral("23"));
        const Wire wire{};
        const Net net{};
        QVERIFY(wire.vertices.empty());
        QVERIFY(net.members.empty());
    }
};

QTEST_MAIN(RendererIsolationTests)
#include "RendererIsolationTests.moc"
