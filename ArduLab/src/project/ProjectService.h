#pragma once

// ProjectService — project lifecycle use cases (Plan §4.3).
//
//   create / open / save / saveAs / close
//
// Resolves catalog references through IComponentManager (exact version only;
// missing versions become Unresolved diagnostics, never substitutions) and
// publishes lifecycle events on the Core EventBus.
//
// Saving uses write-to-temp + atomic rename (Arch Doc §7.3).

#include "components/IComponentManager.h"
#include "core/Event.h"
#include "core/EventBus.h"
#include "core/Result.h"
#include "project/FalDocument.h"
#include "project/Project.h"

#include <QString>

#include <memory>
#include <optional>

namespace ardulab::project {

// ---- lifecycle events ------------------------------------------------------

struct ProjectOpenedEvent final : core::TypedEvent<ProjectOpenedEvent>
{
    ARDULAB_EVENT_NAME("ProjectOpened");
    QString filePath;   ///< Empty for a new unsaved project.
    QString projectName;
    ProjectOpenedEvent(QString path, QString name) : filePath(std::move(path)), projectName(std::move(name)) {}
};

struct ProjectSavedEvent final : core::TypedEvent<ProjectSavedEvent>
{
    ARDULAB_EVENT_NAME("ProjectSaved");
    QString filePath;
    explicit ProjectSavedEvent(QString path) : filePath(std::move(path)) {}
};

struct ProjectClosedEvent final : core::TypedEvent<ProjectClosedEvent>
{
    ARDULAB_EVENT_NAME("ProjectClosed");
};

// ---- service ---------------------------------------------------------------

struct ProjectOpenResult final
{
    FalReadReport report;
    std::size_t resolvedReferences = 0;
    std::size_t unresolvedReferences = 0;
};

class ProjectService final
{
public:
    /// `componentManager` may be null in this foundation phase; then all
    /// catalog references stay Unresolved. `eventBus` may be null.
    ProjectService(components::IComponentManager* componentManager, std::shared_ptr<core::EventBus> eventBus);

    [[nodiscard]] bool hasOpenProject() const noexcept { return m_project.has_value(); }
    [[nodiscard]] const Project* currentProject() const noexcept { return m_project ? &*m_project : nullptr; }
    [[nodiscard]] Project* currentProject() noexcept { return m_project ? &*m_project : nullptr; }
    [[nodiscard]] const QString& currentFilePath() const noexcept { return m_filePath; }

    /// Create a new empty A3 project. Fails with ProjectAlreadyOpen if one is open.
    [[nodiscard]] core::Status createNew(const QString& name);

    /// Open a .FAL file. Fails with ProjectAlreadyOpen if one is open.
    [[nodiscard]] core::Result<ProjectOpenResult> open(const QString& filePath);

    /// Save to the current path (fails with InvalidState if never saved).
    [[nodiscard]] core::Status save();

    /// Save to a new path and adopt it as the current path.
    [[nodiscard]] core::Status saveAs(const QString& filePath);

    /// Close without saving. No-op if nothing is open.
    void close();

private:
    void resolveReferences(Project& project, ProjectOpenResult& result) const;
    [[nodiscard]] static core::Status writeAtomically(const QString& filePath, const QByteArray& bytes);

    components::IComponentManager* m_componentManager;
    std::shared_ptr<core::EventBus> m_eventBus;
    std::optional<Project> m_project;
    QString m_filePath;
};

} // namespace ardulab::project
