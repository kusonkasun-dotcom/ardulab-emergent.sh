#include "project/ProjectService.h"

#include "project/FalSerializer.h"

#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QSaveFile>
#include <QUuid>

namespace ardulab::project {

ProjectService::ProjectService(components::IComponentManager* componentManager, std::shared_ptr<core::EventBus> eventBus)
    : m_componentManager(componentManager)
    , m_eventBus(std::move(eventBus))
{
}

core::Status ProjectService::createNew(const QString& name)
{
    if (m_project) {
        return core::Status::failure(core::ErrorCode::ProjectAlreadyOpen, QStringLiteral("close the current project first"));
    }
    ProjectMetadata meta;
    meta.projectId = core::ProjectId(QUuid::createUuid().toString(QUuid::WithoutBraces));
    meta.name = name.isEmpty() ? QStringLiteral("Untitled Project") : name;
    meta.version = QStringLiteral("0.1");
    meta.falFormatVersion = kCurrentFalFormatVersion;

    m_project.emplace(std::move(meta));
    m_filePath.clear();

    if (m_eventBus) {
        m_eventBus->publishEvent<ProjectOpenedEvent>(QString(), m_project->metadata().name);
    }
    return core::Status::success();
}

core::Result<ProjectOpenResult> ProjectService::open(const QString& filePath)
{
    // Transactional open: the file is read, parsed and resolved into a local
    // Project first. Any failure (I/O, corrupt JSON, unsupported version)
    // returns an error with the currently open document untouched — a broken
    // file can never replace the active project.
    QFile file(filePath);
    if (!file.open(QIODevice::ReadOnly)) {
        return core::Error(core::ErrorCode::IoFailure, QStringLiteral("cannot open .FAL file: %1").arg(file.errorString()), filePath);
    }
    const QByteArray bytes = file.readAll();
    file.close();

    auto read = FalSerializer::read(bytes);
    if (!read) {
        core::Error err = read.error();
        return core::Error(err.code(), err.message(), err.context().isEmpty() ? filePath : err.context());
    }

    ProjectOpenResult result;
    result.report = std::move(read.value().report);
    Project project = std::move(read.value().project);
    resolveReferences(project, result);
    project.markClean();

    // Only now is the previous document released.
    close();
    m_project.emplace(std::move(project));
    m_filePath = QFileInfo(filePath).absoluteFilePath();

    if (m_eventBus) {
        m_eventBus->publishEvent<ProjectOpenedEvent>(m_filePath, m_project->metadata().name);
    }
    return result;
}

void ProjectService::resolveReferences(Project& project, ProjectOpenResult& result) const
{
    for (ComponentInstance& instance : project.instances()) {
        if (!instance.catalogReference || !instance.catalogReference->isComplete()) {
            instance.referenceState = ReferenceState::Unreferenced;
            continue;
        }
        instance.referenceState = ReferenceState::Unresolved;
        if (m_componentManager != nullptr) {
            const CatalogReference& ref = *instance.catalogReference;
            // Exact version only. A missing version stays Unresolved (ADR §11.5).
            const auto loaded = m_componentManager->load(ref.scope, ref.componentId, ref.versionId);
            if (loaded) {
                instance.referenceState = ReferenceState::Resolved;
            }
        }
        if (instance.referenceState == ReferenceState::Resolved) {
            ++result.resolvedReferences;
        } else {
            ++result.unresolvedReferences;
            result.report.warnings << QStringLiteral("%1: catalog version %2 not available; instance opened read-only (no substitution)")
                                          .arg(instance.instanceId.value(), instance.catalogReference->versionId.value());
        }
    }
}

core::Status ProjectService::save()
{
    if (!m_project) {
        return core::Status::failure(core::ErrorCode::ProjectNotOpen, QStringLiteral("no project is open"));
    }
    if (m_filePath.isEmpty()) {
        return core::Status::failure(core::ErrorCode::InvalidState, QStringLiteral("project has no file path; use Save As"));
    }
    return saveAs(m_filePath);
}

core::Status ProjectService::saveAs(const QString& filePath)
{
    if (!m_project) {
        return core::Status::failure(core::ErrorCode::ProjectNotOpen, QStringLiteral("no project is open"));
    }
    if (filePath.isEmpty()) {
        return core::Status::failure(core::ErrorCode::InvalidArgument, QStringLiteral("file path is required"));
    }
    const QByteArray bytes = FalSerializer::write(*m_project);
    if (const auto written = writeAtomically(filePath, bytes); !written) {
        return written;
    }
    m_filePath = QFileInfo(filePath).absoluteFilePath();
    m_project->metadata().falFormatVersion = kCurrentFalFormatVersion;
    m_project->markClean();
    if (m_eventBus) {
        m_eventBus->publishEvent<ProjectSavedEvent>(m_filePath);
    }
    return core::Status::success();
}

core::Status ProjectService::writeAtomically(const QString& filePath, const QByteArray& bytes)
{
    const QFileInfo info(filePath);
    if (!info.dir().exists() && !QDir().mkpath(info.dir().absolutePath())) {
        return core::Status::failure(core::ErrorCode::IoFailure, QStringLiteral("cannot create directory"), info.dir().absolutePath());
    }
    // QSaveFile writes to a temporary file and atomically replaces the target on commit.
    QSaveFile file(filePath);
    if (!file.open(QIODevice::WriteOnly)) {
        return core::Status::failure(core::ErrorCode::IoFailure, QStringLiteral("cannot write .FAL file: %1").arg(file.errorString()), filePath);
    }
    if (file.write(bytes) != bytes.size()) {
        file.cancelWriting();
        return core::Status::failure(core::ErrorCode::IoFailure, QStringLiteral("short write to .FAL file"), filePath);
    }
    if (!file.commit()) {
        return core::Status::failure(core::ErrorCode::IoFailure, QStringLiteral("cannot commit .FAL file: %1").arg(file.errorString()), filePath);
    }
    return core::Status::success();
}

void ProjectService::close()
{
    if (!m_project) {
        return;
    }
    m_project.reset();
    m_filePath.clear();
    if (m_eventBus) {
        m_eventBus->publishEvent<ProjectClosedEvent>();
    }
}

} // namespace ardulab::project
