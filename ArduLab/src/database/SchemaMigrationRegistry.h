#pragma once

// SchemaMigrationRegistry — ordered set of migrations (Plan §4.6, ADR §7.2).
//
// Returns migrations in strictly increasing id order and rejects duplicate
// ids. The foundation registry contains Migration001InitialCatalog.

#include "core/Result.h"
#include "database/SchemaMigration.h"

#include <memory>
#include <vector>

namespace ardulab::database {

class SchemaMigrationRegistry
{
public:
    SchemaMigrationRegistry() = default;

    /// The v0.1.3 foundation registry (Migration 001 only).
    [[nodiscard]] static SchemaMigrationRegistry foundation();

    /// Register a migration. Fails with AlreadyExists on a duplicate id.
    [[nodiscard]] core::Status add(std::unique_ptr<SchemaMigration> migration);

    /// Migrations in ascending id order.
    [[nodiscard]] const std::vector<std::unique_ptr<SchemaMigration>>& migrations() const noexcept { return m_migrations; }

    /// Highest migration id (the schema version the application targets); 0 if empty.
    [[nodiscard]] int latestSchemaVersion() const noexcept;

    [[nodiscard]] bool isEmpty() const noexcept { return m_migrations.empty(); }

private:
    std::vector<std::unique_ptr<SchemaMigration>> m_migrations;
};

} // namespace ardulab::database
