#pragma once

// ComponentJsonImporter — canonical v1.0 JSON → catalog (Plan §9/§10, ADR §6/§8).
//
// Parses the canonical component envelope, detects the schema version,
// performs structural validation, computes a deterministic content hash, and
// stores the candidate as a new immutable DRAFT version through the Component
// Manager. Duplicate (scope, component, version) imports are skipped and
// reported — never silently overwritten (ADR §6.5).
//
// This class lives in the database/import layer but stores only through the
// IComponentManager boundary, so it never bypasses validation.

#include "components/IComponentImporter.h"
#include "components/IComponentManager.h"

namespace ardulab::database {

class ComponentJsonImporter final : public components::IComponentImporter
{
public:
    explicit ComponentJsonImporter(components::IComponentManager& manager);

    [[nodiscard]] core::Result<components::ImportReport>
    importFromFile(const QString& filePath, components::CatalogScope scope) override;

    [[nodiscard]] core::Result<components::ImportReport>
    importFromJson(const QByteArray& bytes, components::CatalogScope scope, const QString& sourceReference) override;

    /// Deterministic canonical content hash ("sha256:...") of a candidate.
    [[nodiscard]] static QString computeContentHash(const components::ComponentSnapshot& snapshot);

private:
    components::IComponentManager& m_manager;
};

} // namespace ardulab::database
