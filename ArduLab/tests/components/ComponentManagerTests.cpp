// Component Engine tests: exact-version loading, search delegation, immutable
// snapshots, validation of candidates, and event publication — verified
// against the in-memory catalog double (no SQLite).

#include "components/ComponentManager.h"
#include "components/InMemoryComponentCatalog.h"
#include "core/EventBus.h"

#include <QtTest/QtTest>

#include <type_traits>

using namespace ardulab::core;
using namespace ardulab::components;

namespace {

ComponentSnapshot makeResistor(const QString& version, CatalogScope scope = CatalogScope::User)
{
    Component c;
    c.componentId = ComponentId(QStringLiteral("R1-A-0805-10K"));
    c.name = QStringLiteral("Resistor 10k 0805");
    c.categoryId = CategoryId(QStringLiteral("PASSIVE"));
    c.scope = scope;
    c.status = LifecycleStatus::Draft;
    c.version = version;
    c.componentSchemaVersion = kCurrentComponentSchemaVersion;

    ComponentVersion v;
    v.componentId = c.componentId;
    v.version = version;
    v.versionId = makeComponentVersionId(c.componentId, version);
    v.contentHash = QStringLiteral("sha256:test-") + version;
    v.sourceType = SourceType::Manual;

    Package p;
    p.packageId = PackageId(QStringLiteral("0805"));
    p.packageType = QStringLiteral("0805");
    p.bodySize = SizeMm(2.0, 1.25);
    p.pinCount = 2;

    Pin p1;
    p1.pinNumber = QStringLiteral("1");
    p1.pinName = QStringLiteral("P1");
    p1.type = PinType::Passive;
    p1.direction = PinDirection::Passive;
    p1.side = PinSide::Left;
    p1.position = PointMm(-1.0, 0.0);
    p1.anchorPosition = PointMm(-1.0, 0.0);

    Pin p2 = p1;
    p2.pinNumber = QStringLiteral("2");
    p2.pinName = QStringLiteral("P2");
    p2.side = PinSide::Right;
    p2.position = PointMm(1.0, 0.0);
    p2.anchorPosition = PointMm(1.0, 0.0);

    std::vector<ComponentParameter> params{
        ComponentParameter{QStringLiteral("resistance"), QStringLiteral("10000"), QStringLiteral("Ohm"), ParameterType::Real},
    };

    return ComponentSnapshot(c, v, p, {p1, p2}, params);
}

} // namespace

class ComponentManagerTests final : public QObject
{
    Q_OBJECT

private slots:
    void snapshotIsImmutableAggregate()
    {
        static_assert(std::is_same_v<ComponentSnapshotPtr, std::shared_ptr<const ComponentSnapshot>>,
                      "consumers must receive const snapshots");
        const auto s = makeResistor(QStringLiteral("1.0.0"));
        QCOMPARE(s.key().versionId.value(), QStringLiteral("R1-A-0805-10K@1.0.0"));
        QCOMPARE(s.pins().size(), std::size_t{2});
        QVERIFY(s.findPin(QStringLiteral("2")).has_value());
        QVERIFY(!s.findPin(QStringLiteral("3")).has_value());
    }

    void registerThenLoadExactVersion()
    {
        InMemoryComponentCatalog catalog;
        ComponentManager manager(catalog, nullptr);

        QVERIFY(manager.registerSnapshot(makeResistor(QStringLiteral("1.0.0"))).isSuccess());
        QCOMPARE(catalog.size(), std::size_t{1});

        const auto loaded = manager.load(CatalogScope::User, ComponentId(QStringLiteral("R1-A-0805-10K")),
                                         ComponentVersionId(QStringLiteral("R1-A-0805-10K@1.0.0")));
        QVERIFY(loaded.isSuccess());
        QCOMPARE(loaded.value()->component().name, QStringLiteral("Resistor 10k 0805"));
        QCOMPARE(loaded.value()->package().bodySize, SizeMm(2.0, 1.25));
    }

    void missingVersionIsNeverSubstituted()
    {
        InMemoryComponentCatalog catalog;
        ComponentManager manager(catalog, nullptr);
        QVERIFY(manager.registerSnapshot(makeResistor(QStringLiteral("1.0.0"))).isSuccess());

        const auto r = manager.load(CatalogScope::User, ComponentId(QStringLiteral("R1-A-0805-10K")),
                                    ComponentVersionId(QStringLiteral("R1-A-0805-10K@2.0.0")));
        QVERIFY(r.isFailure());
        QCOMPARE(r.error().code(), ErrorCode::ComponentVersionNotFound);

        const auto unknown = manager.load(CatalogScope::User, ComponentId(QStringLiteral("NOPE")),
                                          ComponentVersionId(QStringLiteral("NOPE@1.0.0")));
        QCOMPARE(unknown.error().code(), ErrorCode::ComponentNotFound);
    }

    void scopeIsPartOfLookupIdentity()
    {
        InMemoryComponentCatalog catalog;
        ComponentManager manager(catalog, nullptr);
        QVERIFY(manager.registerSnapshot(makeResistor(QStringLiteral("1.0.0"), CatalogScope::Official)).isSuccess());

        const auto asUser = manager.load(CatalogScope::User, ComponentId(QStringLiteral("R1-A-0805-10K")),
                                         ComponentVersionId(QStringLiteral("R1-A-0805-10K@1.0.0")));
        QVERIFY(asUser.isFailure());

        const auto asOfficial = manager.load(CatalogScope::Official, ComponentId(QStringLiteral("R1-A-0805-10K")),
                                             ComponentVersionId(QStringLiteral("R1-A-0805-10K@1.0.0")));
        QVERIFY(asOfficial.isSuccess());
    }

    void versionsAreImmutable()
    {
        InMemoryComponentCatalog catalog;
        ComponentManager manager(catalog, nullptr);
        QVERIFY(manager.registerSnapshot(makeResistor(QStringLiteral("1.0.0"))).isSuccess());
        const auto again = manager.registerSnapshot(makeResistor(QStringLiteral("1.0.0")));
        QVERIFY(again.isFailure());
        QCOMPARE(again.error().code(), ErrorCode::AlreadyExists);
    }

    void listVersionsAndActiveResolution()
    {
        InMemoryComponentCatalog catalog;
        ComponentManager manager(catalog, nullptr);
        QVERIFY(manager.registerSnapshot(makeResistor(QStringLiteral("1.0.0"))).isSuccess());
        QVERIFY(manager.registerSnapshot(makeResistor(QStringLiteral("1.1.0"))).isSuccess());

        const auto versions = manager.listVersions(CatalogScope::User, ComponentId(QStringLiteral("R1-A-0805-10K")));
        QVERIFY(versions.isSuccess());
        QCOMPARE(versions.value().size(), std::size_t{2});

        const auto active = manager.loadActive(CatalogScope::User, ComponentId(QStringLiteral("R1-A-0805-10K")));
        QVERIFY(active.isSuccess());
        QCOMPARE(active.value()->version().version, QStringLiteral("1.1.0"));

        // The explicit exact load of the older version still works — no upgrade.
        const auto old = manager.load(CatalogScope::User, ComponentId(QStringLiteral("R1-A-0805-10K")),
                                      ComponentVersionId(QStringLiteral("R1-A-0805-10K@1.0.0")));
        QVERIFY(old.isSuccess());
        QCOMPARE(old.value()->version().version, QStringLiteral("1.0.0"));
    }

    void searchDelegatesToCatalog()
    {
        InMemoryComponentCatalog catalog;
        ComponentManager manager(catalog, nullptr);
        QVERIFY(manager.registerSnapshot(makeResistor(QStringLiteral("1.0.0"))).isSuccess());

        ComponentSearchCriteria byText;
        byText.text = QStringLiteral("10k");
        const auto hits = manager.search(byText);
        QVERIFY(hits.isSuccess());
        QCOMPARE(hits.value().size(), std::size_t{1});
        QCOMPARE(hits.value().front().packageId.value(), QStringLiteral("0805"));
        QCOMPARE(hits.value().front().scope, CatalogScope::User);
        QCOMPARE(hits.value().front().status, LifecycleStatus::Draft);

        ComponentSearchCriteria byCategory;
        byCategory.categoryId = CategoryId(QStringLiteral("MCU"));
        QCOMPARE(manager.search(byCategory).value().size(), std::size_t{0});
    }

    void candidateValidationRejectsBrokenSnapshots()
    {
        InMemoryComponentCatalog catalog;
        ComponentManager manager(catalog, nullptr);

        // version id does not follow "<id>@<version>"
        auto bad = makeResistor(QStringLiteral("1.0.0"));
        ComponentVersion v = bad.version();
        v.versionId = ComponentVersionId(QStringLiteral("R1-A-0805-10K@9.9.9"));
        const ComponentSnapshot mismatched(bad.component(), v, bad.package(), bad.pins(), bad.parameters());
        const auto r1 = manager.registerSnapshot(mismatched);
        QVERIFY(r1.isFailure());
        QCOMPARE(r1.error().code(), ErrorCode::ValidationFailed);

        // duplicate pin numbers
        auto pins = bad.pins();
        pins[1].pinNumber = QStringLiteral("1");
        const ComponentSnapshot dupPins(bad.component(), bad.version(), bad.package(), pins, bad.parameters());
        const auto r2 = manager.registerSnapshot(dupPins);
        QVERIFY(r2.isFailure());
        QVERIFY(r2.error().message().contains(QStringLiteral("duplicate pin_number")));
        QCOMPARE(catalog.size(), std::size_t{0}); // nothing stored
    }

    void registrationPublishesEvent()
    {
        auto bus = EventBus::create();
        InMemoryComponentCatalog catalog;
        ComponentManager manager(catalog, bus);

        ComponentKey received;
        Subscription sub = bus->subscribe<ComponentRegisteredEvent>([&](const ComponentRegisteredEvent& e) { received = e.key; });
        QVERIFY(manager.registerSnapshot(makeResistor(QStringLiteral("1.0.0"))).isSuccess());
        QCOMPARE(received.versionId.value(), QStringLiteral("R1-A-0805-10K@1.0.0"));
    }

    void pinsAccessorReturnsExactVersionPins()
    {
        InMemoryComponentCatalog catalog;
        ComponentManager manager(catalog, nullptr);
        QVERIFY(manager.registerSnapshot(makeResistor(QStringLiteral("1.0.0"))).isSuccess());
        const auto pins = manager.pins(CatalogScope::User, ComponentId(QStringLiteral("R1-A-0805-10K")),
                                       ComponentVersionId(QStringLiteral("R1-A-0805-10K@1.0.0")));
        QVERIFY(pins.isSuccess());
        QCOMPARE(pins.value().size(), std::size_t{2});
        QCOMPARE(pins.value()[1].anchorPosition, PointMm(1.0, 0.0));
    }
};

QTEST_APPLESS_MAIN(ComponentManagerTests)
#include "ComponentManagerTests.moc"
