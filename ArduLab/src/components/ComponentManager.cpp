#include "components/ComponentManager.h"

namespace ardulab::components {

namespace {

core::Status validateCandidate(const ComponentSnapshot& snapshot)
{
    using core::ErrorCode;
    const Component& c = snapshot.component();
    const ComponentVersion& v = snapshot.version();

    if (c.componentId.isEmpty()) {
        return core::Status::failure(ErrorCode::ValidationFailed, QStringLiteral("component_id is required"));
    }
    if (c.name.isEmpty()) {
        return core::Status::failure(ErrorCode::ValidationFailed, QStringLiteral("name is required"), c.componentId.value());
    }
    if (c.categoryId.isEmpty()) {
        return core::Status::failure(ErrorCode::ValidationFailed, QStringLiteral("category_id is required"), c.componentId.value());
    }
    if (v.version.isEmpty()) {
        return core::Status::failure(ErrorCode::ValidationFailed, QStringLiteral("version is required"), c.componentId.value());
    }
    if (v.componentId != c.componentId) {
        return core::Status::failure(ErrorCode::ValidationFailed,
                                     QStringLiteral("version.component_id does not match component_id"),
                                     c.componentId.value());
    }
    const core::ComponentVersionId expected = core::makeComponentVersionId(c.componentId, v.version);
    if (v.versionId != expected) {
        return core::Status::failure(ErrorCode::ValidationFailed,
                                     QStringLiteral("component_version_id must equal '<component_id>@<version>'"),
                                     v.versionId.value());
    }
    // Pin numbers must be unique within a version (ADR §3.6: stable pin identity).
    const auto& pins = snapshot.pins();
    for (std::size_t i = 0; i < pins.size(); ++i) {
        if (pins[i].pinNumber.isEmpty()) {
            return core::Status::failure(ErrorCode::ValidationFailed, QStringLiteral("pin_number is required for every pin"),
                                         c.componentId.value());
        }
        for (std::size_t j = i + 1; j < pins.size(); ++j) {
            if (pins[i].pinNumber == pins[j].pinNumber) {
                return core::Status::failure(ErrorCode::ValidationFailed,
                                             QStringLiteral("duplicate pin_number '%1'").arg(pins[i].pinNumber),
                                             c.componentId.value());
            }
        }
    }
    return core::Status::success();
}

} // namespace

ComponentManager::ComponentManager(IComponentCatalog& catalog, std::shared_ptr<core::EventBus> eventBus)
    : m_catalog(catalog)
    , m_eventBus(std::move(eventBus))
{
}

core::Result<ComponentSnapshotPtr>
ComponentManager::load(CatalogScope scope, const core::ComponentId& componentId, const core::ComponentVersionId& versionId) const
{
    if (componentId.isEmpty() || versionId.isEmpty()) {
        return core::Error(core::ErrorCode::InvalidArgument, QStringLiteral("component_id and component_version_id are required"));
    }
    // Exact resolution only — no fallback to another version (ADR §9.3).
    return m_catalog.loadExact(ComponentKey{scope, componentId, versionId});
}

core::Result<ComponentSnapshotPtr>
ComponentManager::loadActive(CatalogScope scope, const core::ComponentId& componentId) const
{
    const auto active = m_catalog.activeVersionOf(scope, componentId);
    if (!active) {
        return active.error();
    }
    return m_catalog.loadExact(ComponentKey{scope, componentId, active.value()});
}

core::Result<std::vector<ComponentSummary>> ComponentManager::search(const ComponentSearchCriteria& criteria) const
{
    return m_catalog.search(criteria);
}

core::Result<std::vector<ComponentVersionSummary>>
ComponentManager::listVersions(CatalogScope scope, const core::ComponentId& componentId) const
{
    return m_catalog.listVersions(scope, componentId);
}

core::Result<std::vector<Pin>>
ComponentManager::pins(CatalogScope scope, const core::ComponentId& componentId, const core::ComponentVersionId& versionId) const
{
    const auto snapshot = load(scope, componentId, versionId);
    if (!snapshot) {
        return snapshot.error();
    }
    return snapshot.value()->pins(); // copy of immutable values
}

core::Status ComponentManager::registerSnapshot(const ComponentSnapshot& snapshot)
{
    if (const auto valid = validateCandidate(snapshot); !valid) {
        return valid;
    }
    const auto stored = m_catalog.store(snapshot);
    if (!stored) {
        return stored;
    }
    if (m_eventBus) {
        m_eventBus->publishEvent<ComponentRegisteredEvent>(snapshot.key());
    }
    return core::Status::success();
}

} // namespace ardulab::components
