#pragma once

// IComponentCatalog — persistence port used by Component Manager (Plan §4.5).
//
// Hides SQLite and Qt Sql from the Component Engine. The SQLite adapter
// (database phase) and in-memory test doubles implement this interface.
//
// Every operation is exact: (scope, componentId, versionId). Implementations
// must NEVER resolve a different version than the one requested.

#include "components/Component.h"
#include "components/ComponentSearchCriteria.h"
#include "components/ComponentSnapshot.h"
#include "core/Result.h"

#include <vector>

namespace ardulab::components {

class IComponentCatalog
{
public:
    virtual ~IComponentCatalog() = default;

    /// Human-readable adapter identity for diagnostics ("in-memory", "sqlite").
    [[nodiscard]] virtual QString backendName() const = 0;

    /// Load the exact (scope, component, version) definition.
    /// Fails with ComponentNotFound / ComponentVersionNotFound; never substitutes.
    [[nodiscard]] virtual core::Result<ComponentSnapshotPtr> loadExact(const ComponentKey& key) const = 0;

    /// Resolve the active version ID of a component within a scope.
    [[nodiscard]] virtual core::Result<core::ComponentVersionId>
    activeVersionOf(CatalogScope scope, const core::ComponentId& componentId) const = 0;

    /// Metadata-only search.
    [[nodiscard]] virtual core::Result<std::vector<ComponentSummary>>
    search(const ComponentSearchCriteria& criteria) const = 0;

    /// All versions of a component within a scope, newest first.
    [[nodiscard]] virtual core::Result<std::vector<ComponentVersionSummary>>
    listVersions(CatalogScope scope, const core::ComponentId& componentId) const = 0;

    /// Store a complete snapshot as a new immutable version. Implementations
    /// wrap this in a transaction; failure leaves the catalog unchanged.
    /// Fails with AlreadyExists if (scope, componentId, versionId) exists.
    [[nodiscard]] virtual core::Status store(const ComponentSnapshot& snapshot) = 0;
};

} // namespace ardulab::components
