// JSON import pipeline tests (Plan Step 9/10, ADR §6/§8/§11.2/§11.4).
// Canonical v1.0 import → DRAFT, duplicate skip, unsupported-schema and
// structural rejection, and deterministic content hashing.

#include "components/ComponentManager.h"
#include "database/CatalogDatabase.h"
#include "database/SchemaMigrationRegistry.h"
#include "database/SchemaMigrator.h"
#include "database/SqliteComponentCatalog.h"
#include "database/import/ComponentJsonImporter.h"

#include <QtTest/QtTest>

#include <memory>

using namespace ardulab::core;
using namespace ardulab::components;
using namespace ardulab::database;

namespace {

QString fixture(const char* name)
{
    return QString::fromLatin1(ARDULAB_TEST_FIXTURES_DIR) + QLatin1Char('/') + QString::fromLatin1(name);
}

struct Harness
{
    std::unique_ptr<CatalogDatabase> db;
    std::unique_ptr<SqliteComponentCatalog> catalog;
    std::unique_ptr<ComponentManager> manager;
    std::unique_ptr<ComponentJsonImporter> importer;
};

Harness makeHarness()
{
    auto db = std::make_unique<CatalogDatabase>(QStringLiteral(":memory:"));
    (void)db->open();
    (void)SchemaMigrator(*db, SchemaMigrationRegistry::foundation(), QStringLiteral("test")).migrate();
    auto catalog = std::make_unique<SqliteComponentCatalog>(*db);
    auto manager = std::make_unique<ComponentManager>(*catalog, nullptr);
    auto importer = std::make_unique<ComponentJsonImporter>(*manager);
    return {std::move(db), std::move(catalog), std::move(manager), std::move(importer)};
}

bool hasRule(const ImportReport& r, const QString& ruleId)
{
    for (const ImportMessage& m : r.messages) {
        if (m.ruleId == ruleId) {
            return true;
        }
    }
    return false;
}

} // namespace

class ComponentJsonImporterTests final : public QObject
{
    Q_OBJECT

private slots:
    void canonicalImportCreatesDraftAndIsLoadable()
    {
        Harness h = makeHarness();
        const auto result = h.importer->importFromFile(fixture("canonical-resistor-1.0.json"), CatalogScope::User);
        QVERIFY2(result.isSuccess(), qPrintable(result.error().toString()));
        const ImportReport& report = result.value();
        QCOMPARE(report.outcome, ImportOutcome::Imported);
        QCOMPARE(report.componentId.value(), QStringLiteral("R1-A-0805-10K"));
        QVERIFY(report.contentHash.startsWith(QStringLiteral("sha256:")));

        // Loadable through the manager as a DRAFT with preserved anchors.
        const auto loaded = h.manager->load(CatalogScope::User, ComponentId(QStringLiteral("R1-A-0805-10K")),
                                            ComponentVersionId(QStringLiteral("R1-A-0805-10K@1.0.0")));
        QVERIFY(loaded.isSuccess());
        QCOMPARE(loaded.value()->component().status, LifecycleStatus::Draft);
        QCOMPARE(loaded.value()->version().sourceType, SourceType::JsonImport);
        QCOMPARE(loaded.value()->pins().size(), std::size_t{2});
        QCOMPARE(loaded.value()->findPin(QStringLiteral("1"))->anchorPosition.x, -1.0);
    }

    void reimportIsSkippedNotOverwritten()
    {
        Harness h = makeHarness();
        QCOMPARE(h.importer->importFromFile(fixture("canonical-resistor-1.0.json"), CatalogScope::User).value().outcome,
                 ImportOutcome::Imported);
        const auto again = h.importer->importFromFile(fixture("canonical-resistor-1.0.json"), CatalogScope::User);
        QVERIFY(again.isSuccess());
        QCOMPARE(again.value().outcome, ImportOutcome::Skipped);

        // Only one version exists.
        const auto versions = h.catalog->listVersions(CatalogScope::User, ComponentId(QStringLiteral("R1-A-0805-10K")));
        QVERIFY(versions.isSuccess());
        QCOMPARE(versions.value().size(), std::size_t{1});
    }

    void unsupportedSchemaIsRejected()
    {
        Harness h = makeHarness();
        const auto result = h.importer->importFromFile(fixture("unsupported-schema.json"), CatalogScope::User);
        QVERIFY(result.isSuccess()); // reported, not a hard error
        QCOMPARE(result.value().outcome, ImportOutcome::Rejected);
        QVERIFY(hasRule(result.value(), QStringLiteral("VAL016")));
    }

    void missingPinNumberIsRejected()
    {
        Harness h = makeHarness();
        const auto result = h.importer->importFromFile(fixture("incomplete-pins.json"), CatalogScope::User);
        QVERIFY(result.isSuccess());
        QCOMPARE(result.value().outcome, ImportOutcome::Rejected);
        QVERIFY(hasRule(result.value(), QStringLiteral("VAL005")));
        // Nothing was stored.
        const auto loaded = h.catalog->loadExact(ComponentKey{CatalogScope::User,
                                                              ComponentId(QStringLiteral("R2-A-0805-4K7")),
                                                              ComponentVersionId(QStringLiteral("R2-A-0805-4K7@1.0.0"))});
        QVERIFY(loaded.isFailure());
    }

    void contentHashIsDeterministic()
    {
        Harness a = makeHarness();
        Harness b = makeHarness();
        const auto ra = a.importer->importFromFile(fixture("canonical-resistor-1.0.json"), CatalogScope::User);
        const auto rb = b.importer->importFromFile(fixture("canonical-resistor-1.0.json"), CatalogScope::User);
        QVERIFY(ra.isSuccess() && rb.isSuccess());
        QVERIFY(!ra.value().contentHash.isEmpty());
        QCOMPARE(ra.value().contentHash, rb.value().contentHash);
    }
};

QTEST_GUILESS_MAIN(ComponentJsonImporterTests)
#include "ComponentJsonImporterTests.moc"
