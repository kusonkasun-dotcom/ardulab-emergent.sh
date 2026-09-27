#pragma once

// InMemoryComponentCatalog — IComponentCatalog test double / foundation
// adapter (Plan §9 Step 4: "use a test double to validate exact version
// loading, search delegation, and immutable snapshots before SQLite exists").
//
// It is also used by ApplicationBootstrap until the SQLite adapter lands in
// the database phase, so the application has a real catalog boundary from
// day one. No persistence; contents live for the process lifetime.

#include "components/IComponentCatalog.h"

#include <map>
#include <vector>

namespace ardulab::components {

class InMemoryComponentCatalog final : public IComponentCatalog
{
public:
    [[nodiscard]] QString backendName() const override { return QStringLiteral("in-memory"); }

    [[nodiscard]] core::Result<ComponentSnapshotPtr> loadExact(const ComponentKey& key) const override;

    [[nodiscard]] core::Result<core::ComponentVersionId>
    activeVersionOf(CatalogScope scope, const core::ComponentId& componentId) const override;

    [[nodiscard]] core::Result<std::vector<ComponentSummary>> search(const ComponentSearchCriteria& criteria) const override;

    [[nodiscard]] core::Result<std::vector<ComponentVersionSummary>>
    listVersions(CatalogScope scope, const core::ComponentId& componentId) const override;

    [[nodiscard]] core::Status store(const ComponentSnapshot& snapshot) override;

    [[nodiscard]] std::size_t size() const noexcept { return m_entries.size(); }

private:
    struct Key
    {
        CatalogScope scope;
        QString componentId;
        QString versionId;
        bool operator<(const Key& o) const noexcept
        {
            if (scope != o.scope) return scope < o.scope;
            if (componentId != o.componentId) return componentId < o.componentId;
            return versionId < o.versionId;
        }
    };

    std::map<Key, ComponentSnapshotPtr> m_entries;
    std::map<std::pair<CatalogScope, QString>, QString> m_activeVersion; // (scope, componentId) -> versionId
};

} // namespace ardulab::components
