#include "database/SchemaMigrator.h"

#include "database/CatalogDatabase.h"
#include "database/CatalogTransaction.h"
#include "database/SchemaMigrationRegistry.h"

#include <QDateTime>
#include <QSqlError>
#include <QSqlQuery>
#include <QVariant>

namespace ardulab::database {

SchemaMigrator::SchemaMigrator(CatalogDatabase& database, const SchemaMigrationRegistry& registry,
                               QString applicationVersion)
    : m_database(database)
    , m_registry(registry)
    , m_applicationVersion(std::move(applicationVersion))
{
}

bool SchemaMigrator::schemaMigrationsTableExists() const
{
    QSqlDatabase db = m_database.database();
    QSqlQuery query(db);
    query.prepare(QStringLiteral(
        "SELECT name FROM sqlite_master WHERE type='table' AND name='schema_migrations'"));
    if (!query.exec()) {
        return false;
    }
    return query.next();
}

core::Result<int> SchemaMigrator::currentSchemaVersion() const
{
    if (!m_database.isOpen()) {
        return core::Error(core::ErrorCode::CatalogUnavailable, QStringLiteral("catalog database is not open"));
    }
    if (!schemaMigrationsTableExists()) {
        return 0; // brand-new catalog
    }
    QSqlDatabase db = m_database.database();
    QSqlQuery query(db);
    if (!query.exec(QStringLiteral("SELECT COALESCE(MAX(migration_id), 0) FROM schema_migrations"))) {
        return CatalogDatabase::translate(QStringLiteral("could not read schema version"), query.lastError());
    }
    if (query.next()) {
        return query.value(0).toInt();
    }
    return 0;
}

core::Status SchemaMigrator::migrate()
{
    if (!m_database.isOpen()) {
        return core::Error(core::ErrorCode::CatalogUnavailable, QStringLiteral("catalog database is not open"));
    }

    const auto current = currentSchemaVersion();
    if (!current) {
        return current.error();
    }
    const int currentVersion = current.value();
    const int latest = m_registry.latestSchemaVersion();

    if (currentVersion > latest) {
        return core::Error(core::ErrorCode::UnsupportedFormatVersion,
                           QStringLiteral("catalog schema version %1 is newer than this application supports (%2)")
                               .arg(currentVersion)
                               .arg(latest));
    }

    QSqlDatabase db = m_database.database();
    for (const auto& migration : m_registry.migrations()) {
        if (migration->id() <= currentVersion) {
            continue; // already applied
        }

        CatalogTransaction tx(db);
        if (const auto begun = tx.begin(); !begun) {
            return begun;
        }

        if (const auto applied = migration->apply(db); !applied) {
            tx.rollback();
            return applied;
        }

        // Record the applied migration (schema_migrations is created by
        // Migration 001, so it exists by the time we record it).
        QSqlQuery record(db);
        record.prepare(QStringLiteral(
            "INSERT INTO schema_migrations (migration_id, description, checksum, application_version, applied_utc) "
            "VALUES (?, ?, ?, ?, ?)"));
        record.addBindValue(migration->id());
        record.addBindValue(migration->description());
        record.addBindValue(migration->checksum());
        record.addBindValue(m_applicationVersion);
        record.addBindValue(QDateTime::currentDateTimeUtc().toString(Qt::ISODateWithMs));
        if (!record.exec()) {
            const core::Error error = CatalogDatabase::translate(
                QStringLiteral("could not record migration %1").arg(migration->id()), record.lastError());
            tx.rollback();
            return error;
        }

        if (const auto committed = tx.commit(); !committed) {
            return committed;
        }
    }

    return core::Status::success();
}

} // namespace ardulab::database
