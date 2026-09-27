#pragma once

// SchemaMigrator — read schema version and apply pending migrations (ADR §7.3).
//
// Reads the current schema version from schema_migrations, refuses a catalog
// newer than the application supports, and applies each pending migration in
// a transaction — recording id, checksum, application version, and timestamp.

#include "core/Result.h"

#include <QString>

namespace ardulab::database {

class CatalogDatabase;
class SchemaMigrationRegistry;

class SchemaMigrator
{
public:
    SchemaMigrator(CatalogDatabase& database, const SchemaMigrationRegistry& registry, QString applicationVersion);

    /// Current applied schema version: 0 when the catalog is brand new.
    [[nodiscard]] core::Result<int> currentSchemaVersion() const;

    /// Apply all pending migrations. Idempotent: a fully-migrated catalog is a
    /// no-op. Fails if the catalog schema is newer than this application.
    [[nodiscard]] core::Status migrate();

private:
    [[nodiscard]] bool schemaMigrationsTableExists() const;

    CatalogDatabase& m_database;
    const SchemaMigrationRegistry& m_registry;
    QString m_applicationVersion;
};

} // namespace ardulab::database
