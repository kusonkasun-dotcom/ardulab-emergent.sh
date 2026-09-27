#pragma once

// SqliteComponentCatalog — IComponentCatalog over SQLite (Plan §4.6, ADR §3, §10).
//
// Converts database rows to immutable ComponentSnapshots, enforces exact
// (scope, component, version) resolution, and stores new versions inside a
// transaction. The Component Engine sees only the IComponentCatalog contract;
// Qt Sql never leaks past this class.

#include "components/IComponentCatalog.h"

namespace ardulab::database {

class CatalogDatabase;

class SqliteComponentCatalog final : public components::IComponentCatalog
{
public:
    /// `database` must be open and migrated, and must outlive this catalog.
    explicit SqliteComponentCatalog(CatalogDatabase& database);

    [[nodiscard]] QString backendName() const override { return QStringLiteral("sqlite"); }

    [[nodiscard]] core::Result<components::ComponentSnapshotPtr>
    loadExact(const components::ComponentKey& key) const override;

    [[nodiscard]] core::Result<core::ComponentVersionId>
    activeVersionOf(components::CatalogScope scope, const core::ComponentId& componentId) const override;

    [[nodiscard]] core::Result<std::vector<components::ComponentSummary>>
    search(const components::ComponentSearchCriteria& criteria) const override;

    [[nodiscard]] core::Result<std::vector<components::ComponentVersionSummary>>
    listVersions(components::CatalogScope scope, const core::ComponentId& componentId) const override;

    [[nodiscard]] core::Status store(const components::ComponentSnapshot& snapshot) override;

private:
    CatalogDatabase& m_database;
};

} // namespace ardulab::database
