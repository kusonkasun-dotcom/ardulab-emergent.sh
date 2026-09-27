#include "database/SchemaMigrationRegistry.h"

#include "database/migrations/Migration001InitialCatalog.h"

namespace ardulab::database {

SchemaMigrationRegistry SchemaMigrationRegistry::foundation()
{
    SchemaMigrationRegistry registry;
    // The Clean Foundation Implementation Plan (Step 7) consolidates the
    // foundation tables into a single Migration 001; the ADR's Migration_001..006
    // are design-level identifiers, not separate files for this phase.
    (void)registry.add(std::make_unique<Migration001InitialCatalog>());
    return registry;
}

core::Status SchemaMigrationRegistry::add(std::unique_ptr<SchemaMigration> migration)
{
    if (!migration) {
        return core::Error(core::ErrorCode::InvalidArgument, QStringLiteral("null migration"));
    }
    const int newId = migration->id();
    for (const auto& existing : m_migrations) {
        if (existing->id() == newId) {
            return core::Error(core::ErrorCode::AlreadyExists,
                               QStringLiteral("duplicate migration id"),
                               QString::number(newId));
        }
    }
    // Insert keeping ascending id order.
    auto pos = m_migrations.begin();
    while (pos != m_migrations.end() && (*pos)->id() < newId) {
        ++pos;
    }
    m_migrations.insert(pos, std::move(migration));
    return core::Status::success();
}

int SchemaMigrationRegistry::latestSchemaVersion() const noexcept
{
    int latest = 0;
    for (const auto& m : m_migrations) {
        if (m->id() > latest) {
            latest = m->id();
        }
    }
    return latest;
}

} // namespace ardulab::database
