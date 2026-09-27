#pragma once

// IComponentManager — application-facing component service contract
// (Plan §4.5, ADR §10).
//
// Phase 1 foundation exposes load/search/list/register. Import/export/
// validate/activate/deprecate are added with the database and JSON phases
// and are intentionally absent here so no caller can depend on them early.

#include "components/Component.h"
#include "components/ComponentSearchCriteria.h"
#include "components/ComponentSnapshot.h"
#include "core/Result.h"

#include <vector>

namespace ardulab::components {

class IComponentManager
{
public:
    virtual ~IComponentManager() = default;

    /// load(component_id, version) → ComponentSnapshot (exact version).
    [[nodiscard]] virtual core::Result<ComponentSnapshotPtr>
    load(CatalogScope scope, const core::ComponentId& componentId, const core::ComponentVersionId& versionId) const = 0;

    /// Load the currently active version of a component. Explicit opt-in — the
    /// caller acknowledges the version is resolved at call time.
    [[nodiscard]] virtual core::Result<ComponentSnapshotPtr>
    loadActive(CatalogScope scope, const core::ComponentId& componentId) const = 0;

    /// search(criteria) → ComponentSummary[]
    [[nodiscard]] virtual core::Result<std::vector<ComponentSummary>>
    search(const ComponentSearchCriteria& criteria) const = 0;

    /// list_versions(component_id) → ComponentVersionSummary[]
    [[nodiscard]] virtual core::Result<std::vector<ComponentVersionSummary>>
    listVersions(CatalogScope scope, const core::ComponentId& componentId) const = 0;

    /// get_pins(component_version_id) → PinSnapshot[]  (via exact load)
    [[nodiscard]] virtual core::Result<std::vector<Pin>>
    pins(CatalogScope scope, const core::ComponentId& componentId, const core::ComponentVersionId& versionId) const = 0;

    /// Register an in-memory candidate as a new immutable version.
    /// Foundation entry point used by tests and the bootstrap; the JSON
    /// importer will call this in the import phase.
    [[nodiscard]] virtual core::Status registerSnapshot(const ComponentSnapshot& snapshot) = 0;
};

} // namespace ardulab::components
