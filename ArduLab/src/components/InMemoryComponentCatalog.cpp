#include "components/InMemoryComponentCatalog.h"

#include <algorithm>

namespace ardulab::components {

namespace {

bool matchesScope(const ComponentSearchCriteria& c, CatalogScope scope)
{
    return c.scopes.empty() || std::find(c.scopes.begin(), c.scopes.end(), scope) != c.scopes.end();
}

bool matchesStatus(const ComponentSearchCriteria& c, LifecycleStatus status)
{
    if (status == LifecycleStatus::Deprecated && !c.includeDeprecated) {
        return false;
    }
    return c.statuses.empty() || std::find(c.statuses.begin(), c.statuses.end(), status) != c.statuses.end();
}

bool matchesText(const ComponentSearchCriteria& c, const Component& comp)
{
    if (c.text.isEmpty()) {
        return true;
    }
    return comp.componentId.value().contains(c.text, Qt::CaseInsensitive)
        || comp.name.contains(c.text, Qt::CaseInsensitive)
        || comp.partNumber.contains(c.text, Qt::CaseInsensitive);
}

} // namespace

core::Result<ComponentSnapshotPtr> InMemoryComponentCatalog::loadExact(const ComponentKey& key) const
{
    const auto it = m_entries.find(Key{key.scope, key.componentId.value(), key.versionId.value()});
    if (it != m_entries.end()) {
        return it->second;
    }
    // Distinguish "component unknown" from "component known, version unknown".
    const auto anyVersion = m_activeVersion.find({key.scope, key.componentId.value()});
    if (anyVersion == m_activeVersion.end()) {
        return core::Error(core::ErrorCode::ComponentNotFound,
                           QStringLiteral("component not found in %1 catalog").arg(QLatin1String(catalogScopeName(key.scope))),
                           key.componentId.value());
    }
    return core::Error(core::ErrorCode::ComponentVersionNotFound,
                       QStringLiteral("exact component version not found; no substitution performed"),
                       key.versionId.value());
}

core::Result<core::ComponentVersionId>
InMemoryComponentCatalog::activeVersionOf(CatalogScope scope, const core::ComponentId& componentId) const
{
    const auto it = m_activeVersion.find({scope, componentId.value()});
    if (it == m_activeVersion.end()) {
        return core::Error(core::ErrorCode::ComponentNotFound, QStringLiteral("component not found"), componentId.value());
    }
    return core::ComponentVersionId(it->second);
}

core::Result<std::vector<ComponentSummary>> InMemoryComponentCatalog::search(const ComponentSearchCriteria& criteria) const
{
    std::vector<ComponentSummary> results;
    for (const auto& [activeKey, versionId] : m_activeVersion) {
        const auto it = m_entries.find(Key{activeKey.first, activeKey.second, versionId});
        if (it == m_entries.end()) {
            continue;
        }
        const ComponentSnapshot& s = *it->second;
        const Component& c = s.component();
        if (!matchesScope(criteria, c.scope) || !matchesStatus(criteria, c.status) || !matchesText(criteria, c)) {
            continue;
        }
        if (criteria.categoryId && *criteria.categoryId != c.categoryId) continue;
        if (criteria.manufacturerId && *criteria.manufacturerId != c.manufacturerId) continue;
        if (criteria.packageId && *criteria.packageId != s.package().packageId) continue;

        ComponentSummary summary;
        summary.componentId = c.componentId;
        summary.activeVersionId = s.version().versionId;
        summary.name = c.name;
        summary.categoryId = c.categoryId;
        summary.manufacturerId = c.manufacturerId;
        summary.partNumber = c.partNumber;
        summary.packageId = s.package().packageId;
        summary.status = c.status;
        summary.scope = c.scope;
        summary.version = s.version().version;
        results.push_back(std::move(summary));

        if (criteria.limit > 0 && static_cast<int>(results.size()) >= criteria.limit) {
            break;
        }
    }
    return results;
}

core::Result<std::vector<ComponentVersionSummary>>
InMemoryComponentCatalog::listVersions(CatalogScope scope, const core::ComponentId& componentId) const
{
    std::vector<ComponentVersionSummary> versions;
    const auto active = m_activeVersion.find({scope, componentId.value()});
    for (const auto& [key, snapshot] : m_entries) {
        if (key.scope != scope || key.componentId != componentId.value()) {
            continue;
        }
        ComponentVersionSummary v;
        v.versionId = snapshot->version().versionId;
        v.version = snapshot->version().version;
        v.contentHash = snapshot->version().contentHash;
        v.validationState = snapshot->version().validationState;
        v.isActive = active != m_activeVersion.end() && active->second == key.versionId;
        versions.push_back(std::move(v));
    }
    if (versions.empty()) {
        return core::Error(core::ErrorCode::ComponentNotFound, QStringLiteral("component not found"), componentId.value());
    }
    std::reverse(versions.begin(), versions.end()); // newest (highest) first by insertion order
    return versions;
}

core::Status InMemoryComponentCatalog::store(const ComponentSnapshot& snapshot)
{
    const ComponentKey key = snapshot.key();
    const Key storageKey{key.scope, key.componentId.value(), key.versionId.value()};
    if (m_entries.count(storageKey) != 0) {
        return core::Status::failure(core::ErrorCode::AlreadyExists,
                                     QStringLiteral("component version already exists; versions are immutable"),
                                     key.versionId.value());
    }
    m_entries.emplace(storageKey, std::make_shared<const ComponentSnapshot>(snapshot));
    // Foundation policy: the most recently stored version becomes active.
    // Lifecycle activation rules arrive with the validation phase.
    m_activeVersion[{key.scope, key.componentId.value()}] = key.versionId.value();
    return core::Status::success();
}

} // namespace ardulab::components
