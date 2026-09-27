#include "project/FalSerializer.h"

#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonParseError>
#include <QSet>
#include <QUuid>

namespace ardulab::project {

namespace {

using components::CatalogScope;

const QString kInstanceKeyId = QStringLiteral("instance_id");
const QString kInstanceKeyLibraryId = QStringLiteral("id");
const QString kInstanceKeyName = QStringLiteral("name");
const QString kInstanceKeyPosition = QStringLiteral("position");
const QString kInstanceKeyRotation = QStringLiteral("rotation_deg");
const QString kInstanceKeyCatalogRef = QStringLiteral("catalog_reference");

const QSet<QString>& interpretedInstanceKeys()
{
    static const QSet<QString> keys{kInstanceKeyId, kInstanceKeyLibraryId, kInstanceKeyName,
                                    kInstanceKeyPosition, kInstanceKeyRotation, kInstanceKeyCatalogRef};
    return keys;
}

const QSet<QString>& interpretedRootKeys()
{
    static const QSet<QString> keys{fal_keys::kFormatVersion, fal_keys::kProject, fal_keys::kCanvas,
                                    fal_keys::kLibrary, fal_keys::kComponents};
    return keys;
}

CatalogScope scopeFromText(const QString& text, bool* ok)
{
    *ok = true;
    if (text == QLatin1String("OFFICIAL")) return CatalogScope::Official;
    if (text == QLatin1String("COMMUNITY")) return CatalogScope::Community;
    if (text == QLatin1String("USER") || text.isEmpty()) return CatalogScope::User;
    *ok = false;
    return CatalogScope::User;
}

std::optional<CatalogReference> readCatalogReference(const QJsonObject& obj, const QString& instanceLabel, QStringList& warnings)
{
    if (obj.isEmpty()) {
        return std::nullopt;
    }
    CatalogReference ref;
    bool scopeOk = true;
    ref.scope = scopeFromText(obj.value(QStringLiteral("catalog_scope")).toString(), &scopeOk);
    if (!scopeOk) {
        warnings << QStringLiteral("%1: unknown catalog_scope '%2' treated as USER")
                        .arg(instanceLabel, obj.value(QStringLiteral("catalog_scope")).toString());
    }
    ref.componentId = core::ComponentId(obj.value(QStringLiteral("component_id")).toString());
    ref.versionId = core::ComponentVersionId(obj.value(QStringLiteral("component_version_id")).toString());
    ref.contentHash = obj.value(QStringLiteral("content_hash")).toString();

    const QJsonObject fb = obj.value(QStringLiteral("fallback_snapshot")).toObject();
    if (!fb.isEmpty()) {
        CatalogReference::FallbackSnapshot snapshot;
        snapshot.name = fb.value(QStringLiteral("name")).toString();
        snapshot.referencePrefix = fb.value(QStringLiteral("reference_prefix")).toString();
        snapshot.packageId = core::PackageId(fb.value(QStringLiteral("package_id")).toString());
        for (const QJsonValue& v : fb.value(QStringLiteral("pin_numbers")).toArray()) {
            snapshot.pinNumbers << v.toString();
        }
        ref.fallback = std::move(snapshot);
    }
    if (!ref.isComplete()) {
        warnings << QStringLiteral("%1: incomplete catalog_reference (component_id/component_version_id missing)").arg(instanceLabel);
    }
    return ref;
}

QJsonObject writeCatalogReference(const CatalogReference& ref)
{
    QJsonObject obj;
    obj.insert(QStringLiteral("catalog_scope"), QString::fromLatin1(components::catalogScopeName(ref.scope)));
    obj.insert(QStringLiteral("component_id"), ref.componentId.value());
    obj.insert(QStringLiteral("component_version_id"), ref.versionId.value());
    obj.insert(QStringLiteral("content_hash"), ref.contentHash);
    if (ref.fallback) {
        QJsonObject fb;
        fb.insert(QStringLiteral("name"), ref.fallback->name);
        fb.insert(QStringLiteral("reference_prefix"), ref.fallback->referencePrefix);
        fb.insert(QStringLiteral("package_id"), ref.fallback->packageId.value());
        QJsonArray pins;
        for (const QString& p : ref.fallback->pinNumbers) {
            pins.append(p);
        }
        fb.insert(QStringLiteral("pin_numbers"), pins);
        obj.insert(QStringLiteral("fallback_snapshot"), fb);
    }
    return obj;
}

ComponentInstance readInstance(const QJsonObject& obj, int index, QStringList& warnings)
{
    ComponentInstance instance;
    instance.instanceId = core::InstanceId(obj.value(kInstanceKeyId).toString());
    instance.libraryId = obj.value(kInstanceKeyLibraryId).toString();
    instance.displayName = obj.value(kInstanceKeyName).toString();

    const QString label = instance.instanceId.isValid()
        ? instance.instanceId.value()
        : QStringLiteral("components[%1]").arg(index);

    const QJsonObject pos = obj.value(kInstanceKeyPosition).toObject();
    if (!pos.isEmpty()) {
        instance.position = core::PointMm(pos.value(QStringLiteral("x_mm")).toDouble(),
                                          pos.value(QStringLiteral("y_mm")).toDouble());
    }
    instance.rotationDegrees = obj.value(kInstanceKeyRotation).toDouble(0.0);

    instance.catalogReference = readCatalogReference(obj.value(kInstanceKeyCatalogRef).toObject(), label, warnings);
    instance.referenceState = instance.catalogReference && instance.catalogReference->isComplete()
        ? ReferenceState::Unresolved // resolved later by ProjectService against the catalog
        : ReferenceState::Unreferenced;

    for (auto it = obj.begin(); it != obj.end(); ++it) {
        if (!interpretedInstanceKeys().contains(it.key())) {
            instance.extra.insert(it.key(), it.value());
        }
    }
    return instance;
}

QJsonObject writeInstance(const ComponentInstance& instance)
{
    QJsonObject obj = instance.extra; // unknown fields first; interpreted keys overwrite
    obj.insert(kInstanceKeyId, instance.instanceId.value());
    if (!instance.libraryId.isEmpty()) {
        obj.insert(kInstanceKeyLibraryId, instance.libraryId);
    }
    if (!instance.displayName.isEmpty()) {
        obj.insert(kInstanceKeyName, instance.displayName);
    }
    QJsonObject pos;
    pos.insert(QStringLiteral("x_mm"), instance.position.x);
    pos.insert(QStringLiteral("y_mm"), instance.position.y);
    obj.insert(kInstanceKeyPosition, pos);
    obj.insert(kInstanceKeyRotation, instance.rotationDegrees);
    if (instance.catalogReference) {
        obj.insert(kInstanceKeyCatalogRef, writeCatalogReference(*instance.catalogReference));
    }
    return obj;
}

} // namespace

bool FalSerializer::canRead(const QString& formatVersion)
{
    if (formatVersion.isEmpty()) {
        return true; // legacy minimal shape
    }
    // Same major version is readable (additive evolution).
    const QString major = formatVersion.section(QLatin1Char('.'), 0, 0);
    return major == kCurrentFalFormatVersion.section(QLatin1Char('.'), 0, 0);
}

core::Result<FalDocument> FalSerializer::parse(const QByteArray& bytes)
{
    QJsonParseError parseError{};
    const QJsonDocument json = QJsonDocument::fromJson(bytes, &parseError);
    if (parseError.error != QJsonParseError::NoError) {
        return core::Error(core::ErrorCode::ParseFailure,
                           QStringLiteral("invalid .FAL JSON: %1").arg(parseError.errorString()),
                           QStringLiteral("offset %1").arg(parseError.offset));
    }
    if (!json.isObject()) {
        return core::Error(core::ErrorCode::ParseFailure, QStringLiteral(".FAL root must be a JSON object"));
    }
    return FalDocument{json.object()};
}

core::Result<FalReadResult> FalSerializer::read(const QByteArray& bytes)
{
    const auto document = parse(bytes);
    if (!document) {
        return document.error();
    }
    return fromDocument(document.value());
}

core::Result<FalReadResult> FalSerializer::fromDocument(const FalDocument& document)
{
    const QJsonObject& root = document.root;
    FalReadResult result;
    FalReadReport& report = result.report;

    report.detectedFormatVersion = document.formatVersion();
    report.isLegacyShape = document.hasLegacyShapeOnly();

    if (!canRead(report.detectedFormatVersion)) {
        return core::Error(core::ErrorCode::UnsupportedFormatVersion,
                           QStringLiteral("this build reads .FAL format %1.x; file declares %2")
                               .arg(kCurrentFalFormatVersion.section(QLatin1Char('.'), 0, 0), report.detectedFormatVersion));
    }
    if (!root.contains(fal_keys::kProject)) {
        return core::Error(core::ErrorCode::ParseFailure, QStringLiteral(".FAL document has no 'project' section"));
    }

    // --- project -----------------------------------------------------------
    const QJsonObject projectObj = root.value(fal_keys::kProject).toObject();
    ProjectMetadata meta;
    meta.name = projectObj.value(QStringLiteral("name")).toString();
    meta.version = projectObj.value(QStringLiteral("version")).toString();
    meta.falFormatVersion = report.detectedFormatVersion.isEmpty() ? kCurrentFalFormatVersion : report.detectedFormatVersion;
    const QString idText = projectObj.value(QStringLiteral("id")).toString();
    if (idText.isEmpty()) {
        meta.projectId = core::ProjectId(QUuid::createUuid().toString(QUuid::WithoutBraces));
        report.warnings << QStringLiteral("project.id absent; generated a new project id (legacy document)");
    } else {
        meta.projectId = core::ProjectId(idText);
    }
    if (report.isLegacyShape) {
        report.warnings << QStringLiteral("legacy .FAL shape detected; missing sections defaulted");
    }
    result.project = Project(std::move(meta));
    Project& project = result.project;

    // --- canvas (optional, defaults) --------------------------------------
    const QJsonObject canvasObj = root.value(fal_keys::kCanvas).toObject();
    if (!canvasObj.isEmpty()) {
        CanvasSettings& c = project.canvas();
        c.widthMm = canvasObj.value(QStringLiteral("width_mm")).toDouble(c.widthMm);
        c.heightMm = canvasObj.value(QStringLiteral("height_mm")).toDouble(c.heightMm);
        c.gridMm = canvasObj.value(QStringLiteral("grid_mm")).toDouble(c.gridMm);
        c.visualScale = canvasObj.value(QStringLiteral("visual_scale")).toDouble(c.visualScale);
    }

    // --- library (optional) -------------------------------------------------
    const QJsonObject libraryObj = root.value(fal_keys::kLibrary).toObject();
    project.setCatalogVersion(libraryObj.value(QStringLiteral("catalog_version")).toString());

    // --- components ---------------------------------------------------------
    const QJsonArray componentsArr = root.value(fal_keys::kComponents).toArray();
    int index = 0;
    for (const QJsonValue& v : componentsArr) {
        if (!v.isObject()) {
            report.warnings << QStringLiteral("components[%1] is not an object; skipped").arg(index);
            ++index;
            continue;
        }
        ComponentInstance instance = readInstance(v.toObject(), index, report.warnings);
        if (!project.addInstance(std::move(instance))) {
            report.warnings << QStringLiteral("components[%1] duplicates an existing instance_id; skipped").arg(index);
        }
        ++index;
    }
    project.markClean();

    // --- preserve everything else ------------------------------------------
    for (auto it = root.begin(); it != root.end(); ++it) {
        if (!interpretedRootKeys().contains(it.key())) {
            project.preservedSections().insert(it.key(), it.value());
            report.preservedUnknownKeys << it.key();
        }
    }

    return result;
}

FalDocument FalSerializer::toDocument(const Project& project)
{
    QJsonObject root = project.preservedSections(); // future/unknown sections first

    root.insert(fal_keys::kFormatVersion, kCurrentFalFormatVersion);

    QJsonObject projectObj;
    projectObj.insert(QStringLiteral("id"), project.metadata().projectId.value());
    projectObj.insert(QStringLiteral("name"), project.metadata().name);
    projectObj.insert(QStringLiteral("version"), project.metadata().version);
    root.insert(fal_keys::kProject, projectObj);

    const CanvasSettings& c = project.canvas();
    QJsonObject canvasObj;
    canvasObj.insert(QStringLiteral("width_mm"), c.widthMm);
    canvasObj.insert(QStringLiteral("height_mm"), c.heightMm);
    canvasObj.insert(QStringLiteral("grid_mm"), c.gridMm);
    canvasObj.insert(QStringLiteral("visual_scale"), c.visualScale);
    root.insert(fal_keys::kCanvas, canvasObj);

    QJsonObject libraryObj;
    libraryObj.insert(QStringLiteral("catalog_version"), project.catalogVersion());
    QJsonArray references;
    for (const ComponentInstance& instance : project.instances()) {
        if (instance.catalogReference && instance.catalogReference->isComplete()) {
            references.append(instance.catalogReference->versionId.value());
        }
    }
    libraryObj.insert(QStringLiteral("component_references"), references);
    root.insert(fal_keys::kLibrary, libraryObj);

    QJsonArray componentsArr;
    for (const ComponentInstance& instance : project.instances()) {
        componentsArr.append(writeInstance(instance));
    }
    root.insert(fal_keys::kComponents, componentsArr);

    return FalDocument{root};
}

QByteArray FalSerializer::write(const Project& project)
{
    return QJsonDocument(toDocument(project).root).toJson(QJsonDocument::Indented);
}

} // namespace ardulab::project
