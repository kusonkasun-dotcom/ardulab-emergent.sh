// Core Engine tests: Result/Error, typed identifiers, millimeter units, EventBus.

#include "core/Error.h"
#include "core/EventBus.h"
#include "core/Identifiers.h"
#include "core/Result.h"
#include "core/Units.h"

#include <QtTest/QtTest>

#include <type_traits>

using namespace ardulab::core;

namespace {

struct ProjectOpened final : TypedEvent<ProjectOpened>
{
    ARDULAB_EVENT_NAME("ProjectOpened");
    QString path;
    explicit ProjectOpened(QString p) : path(std::move(p)) {}
};

struct ProjectClosed final : TypedEvent<ProjectClosed>
{
    ARDULAB_EVENT_NAME("ProjectClosed");
};

Result<int> parsePositive(int v)
{
    if (v <= 0) {
        return Error(ErrorCode::InvalidArgument, QStringLiteral("value must be positive"), QString::number(v));
    }
    return v;
}

} // namespace

class CoreTests final : public QObject
{
    Q_OBJECT

private slots:
    // ---- Result / Error ----------------------------------------------------
    void resultCarriesValueOnSuccess()
    {
        const Result<int> r = parsePositive(5);
        QVERIFY(r.isSuccess());
        QVERIFY(static_cast<bool>(r));
        QCOMPARE(r.value(), 5);
        QVERIFY(r.error().isNone());
    }

    void resultCarriesErrorOnFailure()
    {
        const Result<int> r = parsePositive(-1);
        QVERIFY(r.isFailure());
        QCOMPARE(r.error().code(), ErrorCode::InvalidArgument);
        QCOMPARE(r.error().context(), QStringLiteral("-1"));
        QCOMPARE(r.valueOr(99), 99);
        QCOMPARE(r.error().toString(), QStringLiteral("INVALID_ARGUMENT: value must be positive [-1]"));
    }

    void statusDefaultsToSuccess()
    {
        const Status ok;
        QVERIFY(ok.isSuccess());
        const Status bad = Status::failure(ErrorCode::IoFailure, QStringLiteral("disk"));
        QVERIFY(bad.isFailure());
        QCOMPARE(QString::fromLatin1(errorCodeName(bad.error().code())), QStringLiteral("IO_FAILURE"));
    }

    // ---- Identifiers -------------------------------------------------------
    void identifiersAreDistinctTypes()
    {
        static_assert(!std::is_same_v<ComponentId, InstanceId>, "catalog and instance IDs must differ");
        static_assert(!std::is_same_v<ComponentId, ComponentVersionId>, "component and version IDs must differ");
        static_assert(!std::is_convertible_v<ComponentId, InstanceId>, "no implicit interchange");
        static_assert(!std::is_convertible_v<QString, ComponentId>, "no implicit construction from raw text");

        const ComponentId a(QStringLiteral("R1-A-0805-10K"));
        const ComponentId b(QStringLiteral("R1-A-0805-10K"));
        const ComponentId c(QStringLiteral("C1-A-0603-100nF"));
        QVERIFY(a == b);
        QVERIFY(a != c);
        QVERIFY(a.isValid());
        QVERIFY(ComponentId().isEmpty());
        QCOMPARE(qHash(a), qHash(b));
    }

    void componentVersionIdFollowsAdrConvention()
    {
        const auto v = makeComponentVersionId(ComponentId(QStringLiteral("MMCU1-A-ESP32-WROOM32")), QStringLiteral("1.0.0"));
        QCOMPARE(v.value(), QStringLiteral("MMCU1-A-ESP32-WROOM32@1.0.0"));
    }

    // ---- Units -------------------------------------------------------------
    void millimeterArithmeticAndTolerance()
    {
        const Millimeters a(1.0);
        const Millimeters b(2.5);
        QVERIFY((a + b) == Millimeters(3.5));
        QVERIFY((b - a) == Millimeters(1.5));
        QVERIFY(Millimeters(0.1 + 0.2) == Millimeters(0.3)); // tolerant compare
        QVERIFY(Millimeters(1.0) != Millimeters(1.001));
    }

    void a3SheetDimensions()
    {
        const SizeMm a3 = sheet::a3Landscape();
        QCOMPARE(a3.width, 420.0);
        QCOMPARE(a3.height, 297.0);
        const RectMm r(PointMm(0.0, 0.0), a3);
        QVERIFY(r.contains(PointMm(420.0, 297.0)));
        QVERIFY(!r.contains(PointMm(420.1, 0.0)));
        QVERIFY(!r.contains(PointMm(-0.1, 0.0)));
    }

    // ---- EventBus ----------------------------------------------------------
    void eventBusDeliversTypedEventsSynchronously()
    {
        auto bus = EventBus::create();
        QString received;
        int closedCount = 0;

        Subscription s1 = bus->subscribe<ProjectOpened>([&](const ProjectOpened& e) { received = e.path; });
        Subscription s2 = bus->subscribe<ProjectClosed>([&](const ProjectClosed&) { ++closedCount; });

        bus->publishEvent<ProjectOpened>(QStringLiteral("demo.FAL"));
        QCOMPARE(received, QStringLiteral("demo.FAL")); // synchronous
        QCOMPARE(closedCount, 0);                      // type-filtered

        bus->publishEvent<ProjectClosed>();
        QCOMPARE(closedCount, 1);
        QCOMPARE(bus->subscriberCount<ProjectOpened>(), std::size_t{1});
    }

    void subscriptionTokenUnsubscribesOnDestruction()
    {
        auto bus = EventBus::create();
        int count = 0;
        {
            Subscription s = bus->subscribe<ProjectClosed>([&](const ProjectClosed&) { ++count; });
            bus->publishEvent<ProjectClosed>();
            QCOMPARE(count, 1);
            QCOMPARE(bus->subscriberCount<ProjectClosed>(), std::size_t{1});
        }
        bus->publishEvent<ProjectClosed>();
        QCOMPARE(count, 1); // token destroyed -> no delivery
        QCOMPARE(bus->subscriberCount<ProjectClosed>(), std::size_t{0});
    }

    void subscriptionOutlivingBusIsSafe()
    {
        Subscription s;
        {
            auto bus = EventBus::create();
            s = bus->subscribe<ProjectClosed>([](const ProjectClosed&) {});
            QVERIFY(s.isActive());
        }
        QVERIFY(!s.isActive());
        s.reset(); // must not crash
    }

    void reentrantUnsubscribeDuringDeliveryIsSafe()
    {
        auto bus = EventBus::create();
        int hits = 0;
        Subscription self;
        self = bus->subscribe<ProjectClosed>([&](const ProjectClosed&) {
            ++hits;
            self.reset(); // unsubscribe from within the handler
        });
        Subscription other = bus->subscribe<ProjectClosed>([&](const ProjectClosed&) { ++hits; });

        bus->publishEvent<ProjectClosed>();
        QCOMPARE(hits, 2);
        bus->publishEvent<ProjectClosed>();
        QCOMPARE(hits, 3); // only `other` remains
    }

    void eventNameIsStable()
    {
        const ProjectOpened e(QStringLiteral("x"));
        QCOMPARE(e.name(), QStringLiteral("ProjectOpened"));
        QVERIFY(e.typeId() == eventTypeOf<ProjectOpened>());
        QVERIFY(e.typeId() != eventTypeOf<ProjectClosed>());
    }
};

QTEST_APPLESS_MAIN(CoreTests)
#include "EventBusTests.moc"
