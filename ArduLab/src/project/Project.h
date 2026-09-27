#pragma once

// Project — project domain aggregate (Plan §4.3).
//
// Independent of QGraphicsScene and SQL rows. Holds identity, component
// instances with exact catalog references, and preserved extension data for
// future sections (connections, schematic, …) that this phase does not
// interpret but must round-trip.

#include "components/Component.h"
#include "core/Identifiers.h"
#include "core/Units.h"
#include "project/ProjectMetadata.h"

#include <QJsonObject>
#include <QJsonValue>
#include <QString>
#include <QStringList>

#include <optional>
#include <vector>

namespace ardulab::project {

/// Additive exact catalog reference (ADR §11.5). Optional on legacy instances.
struct CatalogReference final
{
    components::CatalogScope scope = components::CatalogScope::User;
    core::ComponentId componentId;
    core::ComponentVersionId versionId;
    QString contentHash;

    /// Read-only recovery/display cache; never an editable second catalog.
    struct FallbackSnapshot final
    {
        QString name;
        QString referencePrefix;
        core::PackageId packageId;
        QStringList pinNumbers;
    };
    std::optional<FallbackSnapshot> fallback;

    [[nodiscard]] bool isComplete() const noexcept { return componentId.isValid() && versionId.isValid(); }
};

/// Resolution state of a placed instance against the catalog (ADR §11.5).
enum class ReferenceState {
    Unreferenced, ///< Legacy instance with no catalog reference block.
    Resolved,     ///< Exact version found in catalog.
    Unresolved,   ///< Reference present but exact version missing — opened read-only, never substituted.
};

struct ComponentInstance final
{
    core::InstanceId instanceId;              ///< "U1", "R3" — preserved exactly.
    QString libraryId;                        ///< Legacy `id`/library field, preserved verbatim.
    QString displayName;
    QString value;                            ///< Engineering value label ("10k", "100nF"); free text.
    core::PointMm position;                   ///< Placement on the A3 sheet, mm.
    double rotationDegrees = 0.0;
    std::optional<CatalogReference> catalogReference;
    ReferenceState referenceState = ReferenceState::Unreferenced;
    QJsonObject extra;                        ///< Unknown instance fields, preserved for round trip.
};

class Project final
{
public:
    Project() = default;
    explicit Project(ProjectMetadata metadata);

    [[nodiscard]] const ProjectMetadata& metadata() const noexcept { return m_metadata; }
    [[nodiscard]] ProjectMetadata& metadata() noexcept { return m_metadata; }

    [[nodiscard]] const CanvasSettings& canvas() const noexcept { return m_canvas; }
    [[nodiscard]] CanvasSettings& canvas() noexcept { return m_canvas; }

    [[nodiscard]] const std::vector<ComponentInstance>& instances() const noexcept { return m_instances; }
    [[nodiscard]] std::vector<ComponentInstance>& instances() noexcept { return m_instances; }

    /// Top-level document sections not interpreted by this phase
    /// (connections, schematic, simulation, …) preserved verbatim.
    [[nodiscard]] const QJsonObject& preservedSections() const noexcept { return m_preservedSections; }
    [[nodiscard]] QJsonObject& preservedSections() noexcept { return m_preservedSections; }

    /// Catalog version label recorded in the "library" section.
    [[nodiscard]] const QString& catalogVersion() const noexcept { return m_catalogVersion; }
    void setCatalogVersion(QString v) { m_catalogVersion = std::move(v); }

    [[nodiscard]] const ComponentInstance* findInstance(const core::InstanceId& id) const noexcept;

    /// Mutable lookup used by interactive edits (move/rotate/property change).
    [[nodiscard]] ComponentInstance* findInstanceMutable(const core::InstanceId& id) noexcept;

    /// Adds an instance; fails with AlreadyExists on duplicate instance ID.
    [[nodiscard]] bool addInstance(ComponentInstance instance);

    /// Removes an instance by ID; returns false if it was not present.
    [[nodiscard]] bool removeInstance(const core::InstanceId& id);

    [[nodiscard]] std::size_t unresolvedReferenceCount() const noexcept;

    [[nodiscard]] bool isDirty() const noexcept { return m_dirty; }
    void markDirty() noexcept { m_dirty = true; }
    void markClean() noexcept { m_dirty = false; }

private:
    ProjectMetadata m_metadata;
    CanvasSettings m_canvas;
    QString m_catalogVersion;
    std::vector<ComponentInstance> m_instances;
    QJsonObject m_preservedSections;
    bool m_dirty = false;
};

} // namespace ardulab::project
