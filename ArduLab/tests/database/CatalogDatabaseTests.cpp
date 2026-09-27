// SqliteComponentCatalog + Migration 001 integration tests (Plan Step 5/7/8,
// ADR §11.3). Uses an in-memory SQLite database per test (no user catalog).

#include "database/CatalogDatabase.h"
#include "database/SchemaMigrationRegistry.h"
#include "database/SchemaMigrator.h"
#include "database/SqliteComponentCatalog.h"

#include <QtSql/QSqlQuery>
#include <QtTest/QtTest>

#include <memory>

using namespace ardulab::core;
using namespace ardulab::components;
using namespace ardulab::database;

namespace {

struct Fixture
{
    std::unique_ptr<CatalogDatabase> db;
    std::unique_ptr<SqliteComponentCatalog> catalog;
};

Fixture makeCatalog()
{
    auto db = std::make_unique<CatalogDatabase>(QStringLiteral(":memory:"));
    if (!db->open()) {
        return {};
    }
    const SchemaMigrationRegistry registry = SchemaMigrationRegistry::foundation();
    SchemaMigrator migrator(*db, registry, QStringLiteral("test"));
    (void)migrator.migrate();
    auto catalog = std::make_unique<SqliteComponentCatalog>(*db);
    return {std::move(db), std::move(catalog)};
}

ComponentSnapshot makeResistor(const QString& version, const QString& category = QStringLiteral("PASSIVE"),
                               CatalogScope scope = CatalogScope::User)
{
    Component c;
    c.componentId = ComponentId(QStringLiteral("R1-A-0805-10K"));
    c.name = QStringLiteral("Resistor 10k 0805");
    c.categoryId = CategoryId(category);
    c.scope = scope;
    c.status = LifecycleStatus::Draft;
    c.version = version;
    c.componentSchemaVersion = kCurrentComponentSchemaVersion;
    c.contentHash = QStringLiteral("sha256:test-") + version;

    ComponentVersion v;
    v.componentId = c.componentId;
    v.version = version;
    v.versionId = makeComponentVersionId(c.componentId, version);
    v.contentHash = c.contentHash;
    v.sourceType = SourceType::Manual;
    v.validationState = ValidationState::Passed;

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
    p1.anchorPosition = PointMm(-1.0, 0.25);

    Pin p2 = p1;
    p2.pinNumber = QStringLiteral("2");
    p2.pinName = QStringLiteral("P2");
    p2.side = PinSide::Right;
    p2.position = PointMm(1.0, 0.0);
    p2.anchorPosition = PointMm(1.0, -0.25);

    std::vector<ComponentParameter> params{
        ComponentParameter{QStringLiteral("resistance"), QStringLiteral("10000"), QStringLiteral("Ohm"), ParameterType::Real},
    };
    return ComponentSnapshot(c, v, p, {p1, p2}, params);
}

} // namespace

class CatalogDatabaseTests final : public QObject
{
    Q_OBJECT

private slots:
    void migrationCreatesFoundationTables()
    {
        Fixture fx = makeCatalog();
        QVERIFY(fx.db && fx.db->isOpen());
        QSqlDatabase db = fx.db->database();
        const QStringList expected = {QStringLiteral("categories"),          QStringLiteral("manufacturers"),
                                      QStringLiteral("packages"),            QStringLiteral("components"),
                                      QStringLiteral("component_versions"),  QStringLiteral("pins"),
                                      QStringLiteral("component_parameters"),QStringLiteral("schema_migrations")};
        for (const QString& table : expected) {
            QSqlQuery q(db);
            q.prepare(QStringLiteral("SELECT name FROM sqlite_master WHERE type='table' AND name=?"));
            q.addBindValue(table);
            QVERIFY(q.exec());
            QVERIFY2(q.next(), qPrintable(QStringLiteral("missing table %1").arg(table)));
        }
        // System categories are seeded.
        QSqlQuery cat(db);
        QVERIFY(cat.exec(QStringLiteral("SELECT COUNT(*) FROM categories WHERE is_system=1")));
        QVERIFY(cat.next());
        QCOMPARE(cat.value(0).toInt(), 9);
    }

    void storeThenLoadPreservesPinsAndAnchors()
    {
        Fixture fx = makeCatalog();
        const ComponentSnapshot snap = makeResistor(QStringLiteral("1.0.0"));
        QVERIFY(fx.catalog->store(snap).isSuccess());

        const auto loaded = fx.catalog->loadExact(snap.key());
        QVERIFY2(loaded.isSuccess(), qPrintable(loaded.error().toString()));
        QCOMPARE(loaded.value()->pins().size(), std::size_t{2});
        const auto pin1 = loaded.value()->findPin(QStringLiteral("1"));
        QVERIFY(pin1.has_value());
        QCOMPARE(pin1->anchorPosition.x, -1.0);
        QCOMPARE(pin1->anchorPosition.y, 0.25);
        QCOMPARE(loaded.value()->package().bodySize.width, 2.0);
        QCOMPARE(loaded.value()->component().status, LifecycleStatus::Draft);
        QCOMPARE(loaded.value()->parameters().size(), std::size_t{1});
    }

    void duplicateVersionIsRejected()
    {
        Fixture fx = makeCatalog();
        const ComponentSnapshot snap = makeResistor(QStringLiteral("1.0.0"));
        QVERIFY(fx.catalog->store(snap).isSuccess());
        const auto again = fx.catalog->store(snap);
        QVERIFY(again.isFailure());
        QCOMPARE(again.error().code(), ErrorCode::AlreadyExists);
    }

    void unknownCategoryViolatesForeignKey()
    {
        Fixture fx = makeCatalog();
        const ComponentSnapshot snap = makeResistor(QStringLiteral("1.0.0"), QStringLiteral("NONEXISTENT"));
        const auto stored = fx.catalog->store(snap);
        QVERIFY2(stored.isFailure(), "storing a component with an unknown category must fail the FK");
    }

    void searchFiltersByCategoryAndText()
    {
        Fixture fx = makeCatalog();
        QVERIFY(fx.catalog->store(makeResistor(QStringLiteral("1.0.0"))).isSuccess());

        ComponentSearchCriteria all;
        const auto r1 = fx.catalog->search(all);
        QVERIFY(r1.isSuccess());
        QCOMPARE(r1.value().size(), std::size_t{1});
        QCOMPARE(r1.value().front().packageId.value(), QStringLiteral("0805"));

        ComponentSearchCriteria byCat;
        byCat.categoryId = CategoryId(QStringLiteral("PASSIVE"));
        QCOMPARE(fx.catalog->search(byCat).value().size(), std::size_t{1});

        ComponentSearchCriteria wrongCat;
        wrongCat.categoryId = CategoryId(QStringLiteral("MCU"));
        QCOMPARE(fx.catalog->search(wrongCat).value().size(), std::size_t{0});

        ComponentSearchCriteria byText;
        byText.text = QStringLiteral("10k");
        QCOMPARE(fx.catalog->search(byText).value().size(), std::size_t{1});
    }

    void listVersionsReturnsNewestFirstAndMarksActive()
    {
        Fixture fx = makeCatalog();
        QVERIFY(fx.catalog->store(makeResistor(QStringLiteral("1.0.0"))).isSuccess());
        QVERIFY(fx.catalog->store(makeResistor(QStringLiteral("1.1.0"))).isSuccess());

        const auto versions = fx.catalog->listVersions(CatalogScope::User, ComponentId(QStringLiteral("R1-A-0805-10K")));
        QVERIFY(versions.isSuccess());
        QCOMPARE(versions.value().size(), std::size_t{2});
        QCOMPARE(versions.value().front().version, QStringLiteral("1.1.0"));
        QVERIFY(versions.value().front().isActive); // newest stored is active

        const auto active = fx.catalog->activeVersionOf(CatalogScope::User, ComponentId(QStringLiteral("R1-A-0805-10K")));
        QVERIFY(active.isSuccess());
        QCOMPARE(active.value().value(), QStringLiteral("R1-A-0805-10K@1.1.0"));
    }

    void exactLoadNeverSubstitutesMissingVersion()
    {
        Fixture fx = makeCatalog();
        QVERIFY(fx.catalog->store(makeResistor(QStringLiteral("1.0.0"))).isSuccess());
        ComponentKey missing{CatalogScope::User, ComponentId(QStringLiteral("R1-A-0805-10K")),
                             ComponentVersionId(QStringLiteral("R1-A-0805-10K@9.9.9"))};
        const auto loaded = fx.catalog->loadExact(missing);
        QVERIFY(loaded.isFailure());
        QCOMPARE(loaded.error().code(), ErrorCode::ComponentVersionNotFound);
    }
};

QTEST_GUILESS_MAIN(CatalogDatabaseTests)
#include "CatalogDatabaseTests.moc"
