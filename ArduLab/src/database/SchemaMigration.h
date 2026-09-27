#pragma once

// SchemaMigration — one immutable, ordered catalog schema step (Plan §4.6, ADR §7).
//
// A migration carries a numeric id (application order), a description, a
// stable checksum of its definition, and the apply() operation. It contains
// no UI behavior and receives an open QSqlDatabase from the migrator.

#include "core/Result.h"

#include <QSqlDatabase>
#include <QString>

namespace ardulab::database {

class SchemaMigration
{
public:
    virtual ~SchemaMigration() = default;

    /// Numeric application order (1, 2, 3 …). Must be unique within a registry.
    [[nodiscard]] virtual int id() const = 0;

    [[nodiscard]] virtual QString description() const = 0;

    /// Stable checksum of the migration definition; a change is detectable so
    /// an already-applied migration whose definition was edited is caught.
    [[nodiscard]] virtual QString checksum() const = 0;

    /// Apply the migration's DDL/seed statements on the given connection.
    /// Runs inside a transaction owned by the migrator.
    [[nodiscard]] virtual core::Status apply(QSqlDatabase& db) const = 0;
};

} // namespace ardulab::database
