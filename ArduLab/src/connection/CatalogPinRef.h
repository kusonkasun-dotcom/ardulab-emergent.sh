#pragma once

// CatalogPinRef — immutable reference from the future Connection Core to a
// catalog pin (ADR §3.6):
//
//   CatalogPinRef = catalog_scope + component_id + component_version_id + pin_number
//
// Contract only. Connection Core behaviour is NOT implemented in this phase.

#include "components/Component.h"
#include "core/Identifiers.h"

#include <QString>

namespace ardulab::connection {

struct CatalogPinRef final
{
    components::CatalogScope scope = components::CatalogScope::User;
    core::ComponentId componentId;
    core::ComponentVersionId versionId;
    QString pinNumber;

    [[nodiscard]] bool isValid() const noexcept
    {
        return componentId.isValid() && versionId.isValid() && !pinNumber.isEmpty();
    }

    friend bool operator==(const CatalogPinRef& a, const CatalogPinRef& b) noexcept
    {
        return a.scope == b.scope && a.componentId == b.componentId && a.versionId == b.versionId && a.pinNumber == b.pinNumber;
    }
    friend bool operator!=(const CatalogPinRef& a, const CatalogPinRef& b) noexcept { return !(a == b); }
};

} // namespace ardulab::connection
