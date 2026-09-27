#pragma once

// ComponentSnapshot — immutable aggregate handed to consumers (Plan §4.5, §8.1).
//
// UI, Renderer, PinAnchor consumers, and future engines receive a
// std::shared_ptr<const ComponentSnapshot>. They can never retain a mutable
// database record, a raw SQL object, or a mutable domain reference.

#include "components/Component.h"
#include "components/ComponentParameter.h"
#include "components/ComponentVersion.h"
#include "components/Package.h"
#include "components/Pin.h"
#include "components/PinAnchor.h"

#include <memory>
#include <optional>
#include <vector>

namespace ardulab::components {

class ComponentSnapshot final
{
public:
    ComponentSnapshot(Component component,
                      ComponentVersion version,
                      Package package,
                      std::vector<Pin> pins,
                      std::vector<ComponentParameter> parameters)
        : m_component(std::move(component))
        , m_version(std::move(version))
        , m_package(std::move(package))
        , m_pins(std::move(pins))
        , m_parameters(std::move(parameters))
    {
    }

    [[nodiscard]] const Component& component() const noexcept { return m_component; }
    [[nodiscard]] const ComponentVersion& version() const noexcept { return m_version; }
    [[nodiscard]] const Package& package() const noexcept { return m_package; }
    [[nodiscard]] const std::vector<Pin>& pins() const noexcept { return m_pins; }
    [[nodiscard]] const std::vector<ComponentParameter>& parameters() const noexcept { return m_parameters; }

    [[nodiscard]] ComponentKey key() const
    {
        return ComponentKey{m_component.scope, m_component.componentId, m_version.versionId};
    }

    /// Anchors derived from pins — the Connection Core handoff values (unchanged).
    [[nodiscard]] std::vector<PinAnchor> pinAnchors() const
    {
        std::vector<PinAnchor> anchors;
        anchors.reserve(m_pins.size());
        for (const Pin& pin : m_pins) {
            anchors.push_back(pin.anchor());
        }
        return anchors;
    }

    [[nodiscard]] std::optional<Pin> findPin(const QString& pinNumber) const
    {
        for (const Pin& pin : m_pins) {
            if (pin.pinNumber == pinNumber) {
                return pin;
            }
        }
        return std::nullopt;
    }

private:
    Component m_component;
    ComponentVersion m_version;
    Package m_package;
    std::vector<Pin> m_pins;
    std::vector<ComponentParameter> m_parameters;
};

using ComponentSnapshotPtr = std::shared_ptr<const ComponentSnapshot>;

/// Lightweight search-result row (ADR §10.2 `search(criteria) → ComponentSummary[]`).
struct ComponentSummary final
{
    core::ComponentId componentId;
    core::ComponentVersionId activeVersionId;
    QString name;
    core::CategoryId categoryId;
    core::ManufacturerId manufacturerId;
    QString partNumber;
    core::PackageId packageId;
    LifecycleStatus status = LifecycleStatus::Draft;
    CatalogScope scope = CatalogScope::User;
    QString version;
};

/// `list_versions(component_id) → ComponentVersionSummary[]`.
struct ComponentVersionSummary final
{
    core::ComponentVersionId versionId;
    QString version;
    QString contentHash;
    ValidationState validationState = ValidationState::NotValidated;
    bool isActive = false;
};

} // namespace ardulab::components
