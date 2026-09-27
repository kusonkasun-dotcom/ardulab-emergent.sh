#pragma once

// ComponentSearchCriteria — catalog metadata search filter (ADR §10.1).
//
// All fields are optional; an empty criteria matches every component.
// Search is metadata-only — it never loads full definitions.

#include "components/Component.h"
#include "core/Identifiers.h"

#include <QString>

#include <optional>
#include <vector>

namespace ardulab::components {

struct ComponentSearchCriteria final
{
    QString text;                                   ///< Case-insensitive match on ID, name, part number.
    std::optional<core::CategoryId> categoryId;
    std::optional<core::ManufacturerId> manufacturerId;
    std::optional<core::PackageId> packageId;
    std::vector<CatalogScope> scopes;               ///< Empty = all scopes.
    std::vector<LifecycleStatus> statuses;          ///< Empty = all statuses.
    bool includeDeprecated = false;
    int limit = 200;

    [[nodiscard]] bool isEmpty() const noexcept
    {
        return text.isEmpty() && !categoryId && !manufacturerId && !packageId && scopes.empty() && statuses.empty();
    }
};

} // namespace ardulab::components
