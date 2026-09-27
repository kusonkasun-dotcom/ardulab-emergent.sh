// Schema migration framework tests (Plan Step 6, ADR §7). Verifies version
// tracking, idempotent re-application, newer-schema rejection, and duplicate
// migration-id rejection.

#include "database/CatalogDatabase.h"
#include "database/SchemaMigrationRegistry.h"
#include "database/SchemaMigrator.h"
#include "database/migrations/Migration001InitialCatalog.h"

#include <QtSql/QSqlQuery>
#include <QtTest/QtTest>

#include <memory>

using namespace ardulab::core;
using namespace ardulab::database;

class SchemaMigratorTests final : public QObject
{
    Q_OBJECT

private slots:
    void freshCatalogIsVersionZeroThenMigratesToOne()
    {
        CatalogDatabase db(QStringLiteral(":memory:"));
        QVERIFY(db.open().isSuccess());

        const SchemaMigrationRegistry registry = SchemaMigrationRegistry::foundation();
        SchemaMigrator migrator(db, registry, QStringLiteral("test"));

        const auto before = migrator.currentSchemaVersion();
        QVERIFY(before.isSuccess());
        QCOMPARE(before.value(), 0);

        QVERIFY2(migrator.migrate().isSuccess(), "initial migration should succeed");

        const auto after = migrator.currentSchemaVersion();
        QVERIFY(after.isSuccess());
        QCOMPARE(after.value(), 1);
    }

    void reMigrationIsIdempotent()
    {
        CatalogDatabase db(QStringLiteral(":memory:"));
        QVERIFY(db.open().isSuccess());
        const SchemaMigrationRegistry registry = SchemaMigrationRegistry::foundation();
        SchemaMigrator migrator(db, registry, QStringLiteral("test"));

        QVERIFY(migrator.migrate().isSuccess());
        QVERIFY(migrator.migrate().isSuccess()); // no-op second pass

        QSqlQuery q(db.database());
        QVERIFY(q.exec(QStringLiteral("SELECT COUNT(*) FROM schema_migrations")));
        QVERIFY(q.next());
        QCOMPARE(q.value(0).toInt(), 1); // Migration 001 applied exactly once
    }

    void newerCatalogSchemaIsRejected()
    {
        CatalogDatabase db(QStringLiteral(":memory:"));
        QVERIFY(db.open().isSuccess());
        const SchemaMigrationRegistry registry = SchemaMigrationRegistry::foundation();
        SchemaMigrator migrator(db, registry, QStringLiteral("test"));
        QVERIFY(migrator.migrate().isSuccess());

        // Simulate a catalog written by a future application version.
        QSqlQuery insert(db.database());
        insert.prepare(QStringLiteral(
            "INSERT INTO schema_migrations (migration_id, description, checksum, application_version, applied_utc) "
            "VALUES (99, 'future', 'x', '9.9', '2099-01-01T00:00:00.000Z')"));
        QVERIFY(insert.exec());

        SchemaMigrator reopened(db, registry, QStringLiteral("test"));
        const auto version = reopened.currentSchemaVersion();
        QVERIFY(version.isSuccess());
        QCOMPARE(version.value(), 99);

        const auto migrated = reopened.migrate();
        QVERIFY(migrated.isFailure());
        QCOMPARE(migrated.error().code(), ErrorCode::UnsupportedFormatVersion);
    }

    void registryRejectsDuplicateMigrationId()
    {
        SchemaMigrationRegistry registry;
        QVERIFY(registry.add(std::make_unique<Migration001InitialCatalog>()).isSuccess());
        const auto dup = registry.add(std::make_unique<Migration001InitialCatalog>());
        QVERIFY(dup.isFailure());
        QCOMPARE(dup.error().code(), ErrorCode::AlreadyExists);
        QCOMPARE(registry.latestSchemaVersion(), 1);
    }
};

QTEST_GUILESS_MAIN(SchemaMigratorTests)
#include "SchemaMigratorTests.moc"
