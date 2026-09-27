#pragma once

// ComponentManager — IComponentManager over an IComponentCatalog (Plan §4.5).
//
// Independent of Renderer, Canvas, Connection Core, MainWindow, and SQLite.
// Publishes domain events through the Core EventBus.

#include "components/IComponentCatalog.h"
#include "components/IComponentManager.h"
#include "core/Event.h"
#include "core/EventBus.h"

#include <memory>

namespace ardulab::components {

/// Published after a snapshot was stored as a new immutable version.
struct ComponentRegisteredEvent final : core::TypedEvent<ComponentRegisteredEvent>
{
    ARDULAB_EVENT_NAME("ComponentRegistered");
    ComponentKey key;
    explicit ComponentRegisteredEvent(ComponentKey k) : key(std::move(k)) {}
};

class ComponentManager final : public IComponentManager
{
public:
    /// `catalog` must outlive the manager. `eventBus` may be null (no publishing).
    ComponentManager(IComponentCatalog& catalog, std::shared_ptr<core::EventBus> eventBus);

    [[nodiscard]] core::Result<ComponentSnapshotPtr>
    load(CatalogScope scope, const core::ComponentId& componentId, const core::ComponentVersionId& versionId) const override;

    [[nodiscard]] core::Result<ComponentSnapshotPtr>
    loadActive(CatalogScope scope, const core::ComponentId& componentId) const override;

    [[nodiscard]] core::Result<std::vector<ComponentSummary>>
    search(const ComponentSearchCriteria& criteria) const override;

    [[nodiscard]] core::Result<std::vector<ComponentVersionSummary>>
    listVersions(CatalogScope scope, const core::ComponentId& componentId) const override;

    [[nodiscard]] core::Result<std::vector<Pin>>
    pins(CatalogScope scope, const core::ComponentId& componentId, const core::ComponentVersionId& versionId) const override;

    [[nodiscard]] core::Status registerSnapshot(const ComponentSnapshot& snapshot) override;

private:
    IComponentCatalog& m_catalog;
    std::shared_ptr<core::EventBus> m_eventBus;
};

} // namespace ardulab::components
