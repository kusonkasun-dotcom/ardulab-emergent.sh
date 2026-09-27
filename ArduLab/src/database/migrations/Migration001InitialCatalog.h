#pragma once

// Migration001InitialCatalog — foundation catalog schema (Plan §9 Step 7).
//
// Creates the applied-migrations table, catalog metadata, taxonomy
// (categories, manufacturers, packages), the logical components table, the
// immutable component_versions table, and the versioned pins / parameters
// tables. Seeds the required system category codes (ADR §3.4). Foreign keys
// and unique constraints enforce the ADR §8.1 structural invariants.

#include "database/SchemaMigration.h"

namespace ardulab::database {

class Migration001InitialCatalog final : public SchemaMigration
{
public:
    [[nodiscard]] int id() const override { return 1; }
    [[nodiscard]] QString description() const override { return QStringLiteral("Initial catalog foundation schema"); }
    [[nodiscard]] QString checksum() const override;
    [[nodiscard]] core::Status apply(QSqlDatabase& db) const override;
};

} // namespace ardulab::database
