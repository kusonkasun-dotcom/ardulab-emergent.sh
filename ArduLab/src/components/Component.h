#pragma once

// Component — stable logical catalog identity (Plan §4.5, ADR §3.2, §9).
//
// Mutable lifecycle metadata lives here; immutable definition payloads live in
// ComponentVersion and the versioned Package/Pin/Parameter records that are
// aggregated into ComponentSnapshot.

#include "core/Identifiers.h"

#include <QDateTime>
#include <QString>

namespace ardulab::components {

/// Lifecycle status (ADR §9.1).
enum class LifecycleStatus {
    Draft,
    Verified,
    Community,
    Official,
    Deprecated,
};

/// Ownership/source boundary (ADR §9.4). Part of lookup identity.
enum class CatalogScope {
    User,
    Community,
    Official,
};

constexpr const char* lifecycleStatusName(LifecycleStatus s) noexcept
{
    switch (s) {
    case LifecycleStatus::Draft:      return "DRAFT";
    case LifecycleStatus::Verified:   return "VERIFIED";
    case LifecycleStatus::Community:  return "COMMUNITY";
    case LifecycleStatus::Official:   return "OFFICIAL";
    case LifecycleStatus::Deprecated: return "DEPRECATED";
    }
    return "DRAFT";
}

constexpr const char* catalogScopeName(CatalogScope s) noexcept
{
    switch (s) {
    case CatalogScope::User:      return "USER";
    case CatalogScope::Community: return "COMMUNITY";
    case CatalogScope::Official:  return "OFFICIAL";
    }
    return "USER";
}

struct Component final
{
    core::ComponentId componentId;         ///< CATEGORY-ID-VARIANT-PART-PACKAGE
    QString name;
    core::CategoryId categoryId;
    core::ManufacturerId manufacturerId;   ///< Optional; empty allowed for DRAFT.
    QString partNumber;                    ///< Optional; required before OFFICIAL.
    QString description;
    LifecycleStatus status = LifecycleStatus::Draft;
    CatalogScope scope = CatalogScope::User;
    QString version;                       ///< Active semantic version label.
    QString componentSchemaVersion;        ///< Data contract version ("1.0").
    core::ComponentVersionId activeVersionId;
    QString contentHash;
    core::ComponentId replacementComponentId; ///< For DEPRECATED records.
    QDateTime createdUtc;
    QDateTime updatedUtc;
};

/// Exact lookup key (ADR §9.4: "scope is part of lookup identity").
struct ComponentKey final
{
    CatalogScope scope = CatalogScope::User;
    core::ComponentId componentId;
    core::ComponentVersionId versionId;

    friend bool operator==(const ComponentKey& a, const ComponentKey& b) noexcept
    {
        return a.scope == b.scope && a.componentId == b.componentId && a.versionId == b.versionId;
    }
    friend bool operator!=(const ComponentKey& a, const ComponentKey& b) noexcept { return !(a == b); }
};

} // namespace ardulab::components
