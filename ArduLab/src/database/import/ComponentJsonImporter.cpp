#include "database/import/ComponentJsonImporter.h"

#include "database/import/ComponentJsonDocument.h"

#include <QCryptographicHash>
#include <QDateTime>
#include <QFile>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>

#include <algorithm>
#include <set>

namespace ardulab::database {

using namespace ardulab::components;

namespace {

// The system category codes seeded by Migration 001. An import referencing an
// unknown category is rejected (VAL008) because the FK would otherwise fail.
const std::set<QString>& knownCategories()
{
    static const std::set<QString> cats = {
        QStringLiteral("MCU"),     QStringLiteral("MCU_MODULE"), QStringLiteral("PASSIVE"),
        QStringLiteral("DIODE"),   QStringLiteral("ACTIVE"),     QStringLiteral("POWER"),
        QStringLiteral("SENSOR"),  QStringLiteral("ACTUATOR"),   QStringLiteral("CONNECTOR"),
    };
    return cats;
}

PinType parsePinType(const QString& s, bool& ok)
{
    ok = true;
    if (s == QLatin1String("POWER")) return PinType::Power;
    if (s == QLatin1String("GROUND")) return PinType::Ground;
    if (s == QLatin1String("GPIO")) return PinType::Gpio;
    if (s == QLatin1String("ANALOG")) return PinType::Analog;
    if (s == QLatin1String("DIGITAL")) return PinType::Digital;
    if (s == QLatin1String("PASSIVE")) return PinType::Passive;
    if (s == QLatin1String("NC")) return PinType::NotConnected;
    if (s == QLatin1String("OTHER")) return PinType::Other;
    ok = false;
    return PinType::Other;
}

PinDirection parsePinDirection(const QString& s, bool& ok)
{
    ok = true;
    if (s == QLatin1String("INPUT")) return PinDirection::Input;
    if (s == QLatin1String("OUTPUT")) return PinDirection::Output;
    if (s == QLatin1String("INPUT_OUTPUT")) return PinDirection::InputOutput;
    if (s == QLatin1String("POWER_IN")) return PinDirection::PowerIn;
    if (s == QLatin1String("POWER_OUT")) return PinDirection::PowerOut;
    if (s == QLatin1String("PASSIVE")) return PinDirection::Passive;
    if (s == QLatin1String("NC")) return PinDirection::NotConnected;
    ok = false;
    return PinDirection::Passive;
}

PinSide parsePinSide(const QString& s, bool& ok)
{
    ok = true;
    if (s == QLatin1String("LEFT")) return PinSide::Left;
    if (s == QLatin1String("RIGHT")) return PinSide::Right;
    if (s == QLatin1String("TOP")) return PinSide::Top;
    if (s == QLatin1String("BOTTOM")) return PinSide::Bottom;
    if (s == QLatin1String("PACKAGE_DEFINED")) return PinSide::PackageDefined;
    ok = false;
    return PinSide::PackageDefined;
}

ParameterType parseParameterType(const QString& s)
{
    if (s == QLatin1String("INTEGER")) return ParameterType::Integer;
    if (s == QLatin1String("REAL")) return ParameterType::Real;
    if (s == QLatin1String("BOOLEAN")) return ParameterType::Boolean;
    if (s == QLatin1String("ENUM")) return ParameterType::Enumeration;
    return ParameterType::Text;
}

} // namespace

ComponentJsonImporter::ComponentJsonImporter(IComponentManager& manager)
    : m_manager(manager)
{
}

QString ComponentJsonImporter::computeContentHash(const ComponentSnapshot& snapshot)
{
    const Component& c = snapshot.component();
    const ComponentVersion& v = snapshot.version();
    const Package& pkg = snapshot.package();

    QJsonObject root;
    root[QStringLiteral("component_id")] = c.componentId.value();
    root[QStringLiteral("version")] = v.version;
    root[QStringLiteral("schema")] = v.componentSchemaVersion;
    root[QStringLiteral("category")] = c.categoryId.value();

    QJsonObject pkgObj;
    pkgObj[QStringLiteral("id")] = pkg.packageId.value();
    pkgObj[QStringLiteral("type")] = pkg.packageType;
    pkgObj[QStringLiteral("w")] = pkg.bodySize.width;
    pkgObj[QStringLiteral("h")] = pkg.bodySize.height;
    pkgObj[QStringLiteral("pins")] = pkg.pinCount;
    pkgObj[QStringLiteral("pitch")] = pkg.pitch ? QJsonValue(pkg.pitch->value) : QJsonValue();
    root[QStringLiteral("package")] = pkgObj;

    // Pins sorted by pin number for a stable canonical order.
    std::vector<Pin> pins = snapshot.pins();
    std::sort(pins.begin(), pins.end(), [](const Pin& a, const Pin& b) { return a.pinNumber < b.pinNumber; });
    QJsonArray pinArray;
    for (const Pin& pin : pins) {
        QJsonObject po;
        po[QStringLiteral("num")] = pin.pinNumber;
        po[QStringLiteral("name")] = pin.pinName;
        po[QStringLiteral("type")] = QString::fromLatin1(pinTypeName(pin.type));
        po[QStringLiteral("dir")] = QString::fromLatin1(pinDirectionName(pin.direction));
        po[QStringLiteral("side")] = QString::fromLatin1(pinSideName(pin.side));
        po[QStringLiteral("px")] = pin.position.x;
        po[QStringLiteral("py")] = pin.position.y;
        po[QStringLiteral("ax")] = pin.anchorPosition.x;
        po[QStringLiteral("ay")] = pin.anchorPosition.y;
        po[QStringLiteral("v")] = pin.voltage;
        po[QStringLiteral("fn")] = pin.functions.join(QLatin1Char('|'));
        pinArray.append(po);
    }
    root[QStringLiteral("pins")] = pinArray;

    std::vector<ComponentParameter> params = snapshot.parameters();
    std::sort(params.begin(), params.end(), [](const ComponentParameter& a, const ComponentParameter& b) {
        return a.key < b.key;
    });
    QJsonArray paramArray;
    for (const ComponentParameter& p : params) {
        QJsonObject po;
        po[QStringLiteral("key")] = p.key;
        po[QStringLiteral("value")] = p.value;
        po[QStringLiteral("unit")] = p.unit;
        po[QStringLiteral("type")] = QString::fromLatin1(parameterTypeName(p.type));
        paramArray.append(po);
    }
    root[QStringLiteral("parameters")] = paramArray;

    const QByteArray bytes = QJsonDocument(root).toJson(QJsonDocument::Compact);
    return QStringLiteral("sha256:")
        + QString::fromLatin1(QCryptographicHash::hash(bytes, QCryptographicHash::Sha256).toHex());
}

core::Result<ImportReport>
ComponentJsonImporter::importFromFile(const QString& filePath, CatalogScope scope)
{
    QFile file(filePath);
    if (!file.open(QIODevice::ReadOnly)) {
        return core::Error(core::ErrorCode::IoFailure, QStringLiteral("could not open component file"), filePath);
    }
    const QByteArray bytes = file.readAll();
    return importFromJson(bytes, scope, filePath);
}

core::Result<ImportReport>
ComponentJsonImporter::importFromJson(const QByteArray& bytes, CatalogScope scope, const QString& sourceReference)
{
    ImportReport report;
    report.scope = scope;
    report.sourceReference = sourceReference;

    // 1. Parse.
    QJsonParseError parseError{};
    const QJsonDocument doc = QJsonDocument::fromJson(bytes, &parseError);
    if (parseError.error != QJsonParseError::NoError || !doc.isObject()) {
        report.outcome = ImportOutcome::Rejected;
        report.add(QString(), ImportSeverity::Error,
                   QStringLiteral("invalid JSON: %1").arg(parseError.errorString()), sourceReference);
        return report;
    }
    const QJsonObject root = doc.object();

    // 2. Schema detection (VAL016).
    const SchemaDetection schema = ComponentJsonSchema::detect(root);
    if (!schema.supported) {
        report.outcome = ImportOutcome::Rejected;
        report.add(QStringLiteral("VAL016"), ImportSeverity::Error, schema.reason,
                   QStringLiteral("schema_version"));
        return report;
    }

    // 3. Map canonical fields into a candidate snapshot.
    Component component;
    component.componentId = core::ComponentId(root.value(QStringLiteral("component_id")).toString());
    component.name = root.value(QStringLiteral("name")).toString();
    component.categoryId = core::CategoryId(root.value(QStringLiteral("category_id")).toString());
    component.manufacturerId = core::ManufacturerId(root.value(QStringLiteral("manufacturer_id")).toString());
    component.partNumber = root.value(QStringLiteral("part_number")).toString();
    component.description = root.value(QStringLiteral("description")).toString();
    component.status = LifecycleStatus::Draft;  // imports always enter DRAFT (ADR §9.2)
    component.scope = scope;
    component.version = root.value(QStringLiteral("version")).toString();
    component.componentSchemaVersion = schema.componentSchemaVersion;

    report.componentId = component.componentId;

    // Package.
    Package package;
    const QJsonObject pkgObj = root.value(QStringLiteral("package")).toObject();
    package.packageId = core::PackageId(pkgObj.value(QStringLiteral("package_id")).toString());
    package.packageType = pkgObj.value(QStringLiteral("package_type")).toString();
    package.bodySize = core::SizeMm(pkgObj.value(QStringLiteral("width_mm")).toDouble(),
                                    pkgObj.value(QStringLiteral("height_mm")).toDouble());
    package.pinCount = pkgObj.value(QStringLiteral("pin_count")).toInt();
    if (pkgObj.contains(QStringLiteral("pitch_mm")) && !pkgObj.value(QStringLiteral("pitch_mm")).isNull()) {
        package.pitch = core::Millimeters(pkgObj.value(QStringLiteral("pitch_mm")).toDouble());
    }

    // Parameters.
    std::vector<ComponentParameter> parameters;
    for (const QJsonValue& pv : root.value(QStringLiteral("parameters")).toArray()) {
        const QJsonObject po = pv.toObject();
        ComponentParameter p;
        p.key = po.value(QStringLiteral("key")).toString();
        p.value = po.value(QStringLiteral("value")).toString();
        p.unit = po.value(QStringLiteral("unit")).toString();
        p.type = parseParameterType(po.value(QStringLiteral("type")).toString());
        parameters.push_back(std::move(p));
    }

    // Pins.
    std::vector<Pin> pins;
    const QJsonArray pinArray = root.value(QStringLiteral("pins")).toArray();
    int pinIndex = 0;
    for (const QJsonValue& pv : pinArray) {
        const QJsonObject po = pv.toObject();
        Pin pin;
        pin.pinNumber = po.value(QStringLiteral("pin_number")).toString();
        pin.pinName = po.value(QStringLiteral("pin_name")).toString();

        bool okType = false, okDir = false, okSide = false;
        pin.type = parsePinType(po.value(QStringLiteral("pin_type")).toString(), okType);
        pin.direction = parsePinDirection(po.value(QStringLiteral("direction")).toString(), okDir);
        pin.side = parsePinSide(po.value(QStringLiteral("side")).toString(), okSide);
        const QString path = QStringLiteral("pins[%1]").arg(pinIndex);
        if (!okType) {
            report.add(QStringLiteral("VAL014"), ImportSeverity::Warning,
                       QStringLiteral("unknown pin_type '%1' mapped to OTHER")
                           .arg(po.value(QStringLiteral("pin_type")).toString()), path);
        }
        if (!okDir) {
            report.add(QStringLiteral("VAL014"), ImportSeverity::Warning,
                       QStringLiteral("unknown direction mapped to PASSIVE"), path);
        }
        if (!okSide) {
            report.add(QStringLiteral("VAL014"), ImportSeverity::Warning,
                       QStringLiteral("unknown side mapped to PACKAGE_DEFINED"), path);
        }

        pin.voltage = po.value(QStringLiteral("voltage")).toString();
        for (const QJsonValue& fn : po.value(QStringLiteral("function")).toArray()) {
            pin.functions << fn.toString();
        }
        const QJsonObject pos = po.value(QStringLiteral("position")).toObject();
        pin.position = core::PointMm(pos.value(QStringLiteral("x_mm")).toDouble(),
                                     pos.value(QStringLiteral("y_mm")).toDouble());
        if (!po.contains(QStringLiteral("anchor_position"))) {
            report.add(QStringLiteral("VAL006"), ImportSeverity::Error,
                       QStringLiteral("pin is missing anchor_position"), path);
        }
        const QJsonObject anchor = po.value(QStringLiteral("anchor_position")).toObject();
        pin.anchorPosition = core::PointMm(anchor.value(QStringLiteral("x_mm")).toDouble(),
                                           anchor.value(QStringLiteral("y_mm")).toDouble());
        pins.push_back(std::move(pin));
        ++pinIndex;
    }

    // 4. Structural validation.
    if (component.componentId.isEmpty()) {
        report.add(QStringLiteral("VAL007"), ImportSeverity::Error, QStringLiteral("component_id is required"),
                   QStringLiteral("component_id"));
    }
    if (component.name.isEmpty()) {
        report.add(QString(), ImportSeverity::Error, QStringLiteral("name is required"), QStringLiteral("name"));
    }
    if (component.categoryId.isEmpty()) {
        report.add(QStringLiteral("VAL008"), ImportSeverity::Error, QStringLiteral("category_id is required"),
                   QStringLiteral("category_id"));
    } else if (knownCategories().find(component.categoryId.value()) == knownCategories().end()) {
        report.add(QStringLiteral("VAL008"), ImportSeverity::Error,
                   QStringLiteral("unknown category '%1'").arg(component.categoryId.value()),
                   QStringLiteral("category_id"));
    }
    if (component.version.isEmpty()) {
        report.add(QString(), ImportSeverity::Error, QStringLiteral("version is required"), QStringLiteral("version"));
    }
    if (package.packageId.isEmpty()) {
        report.add(QString(), ImportSeverity::Error, QStringLiteral("package.package_id is required"),
                   QStringLiteral("package"));
    }
    if (package.bodySize.width < 0.0 || package.bodySize.height < 0.0
        || (package.pitch && package.pitch->value < 0.0)) {
        report.add(QStringLiteral("VAL013"), ImportSeverity::Error,
                   QStringLiteral("package dimensions/pitch must be non-negative"), QStringLiteral("package"));
    }

    // Pin numbers: present + unique (VAL005).
    std::set<QString> seenNumbers;
    for (int i = 0; i < static_cast<int>(pins.size()); ++i) {
        const QString path = QStringLiteral("pins[%1].pin_number").arg(i);
        if (pins[static_cast<std::size_t>(i)].pinNumber.isEmpty()) {
            report.add(QStringLiteral("VAL005"), ImportSeverity::Error, QStringLiteral("pin_number is required"), path);
            continue;
        }
        if (!seenNumbers.insert(pins[static_cast<std::size_t>(i)].pinNumber).second) {
            report.add(QStringLiteral("VAL005"), ImportSeverity::Error,
                       QStringLiteral("duplicate pin_number '%1'").arg(pins[static_cast<std::size_t>(i)].pinNumber), path);
        }
    }
    if (pins.empty()) {
        report.add(QStringLiteral("VAL005"), ImportSeverity::Warning,
                   QStringLiteral("component defines no pins; not usable by Connection Core"), QStringLiteral("pins"));
    }
    if (package.pinCount != static_cast<int>(pins.size())) {
        report.add(QStringLiteral("VAL003"), ImportSeverity::Warning,
                   QStringLiteral("package.pin_count (%1) does not match defined pins (%2)")
                       .arg(package.pinCount)
                       .arg(pins.size()),
                   QStringLiteral("package.pin_count"));
    }
    if (component.manufacturerId.isEmpty() || component.partNumber.isEmpty()) {
        report.add(QStringLiteral("VAL009"), ImportSeverity::Warning,
                   QStringLiteral("manufacturer or part_number is missing; allowed for DRAFT, blocks OFFICIAL"),
                   QStringLiteral("manufacturer_id"));
    }

    if (report.hasErrors()) {
        report.outcome = ImportOutcome::Rejected;
        return report;
    }

    // 5. Build the version record.
    ComponentVersion version;
    version.componentId = component.componentId;
    version.version = component.version;
    version.versionId = core::makeComponentVersionId(component.componentId, component.version);
    version.componentSchemaVersion = schema.componentSchemaVersion;
    version.sourceType = SourceType::JsonImport;
    version.sourceReference = sourceReference;
    version.validationState = report.warningCount() > 0 ? ValidationState::PassedWithWarnings : ValidationState::Passed;
    version.createdUtc = QDateTime::currentDateTimeUtc();
    version.createdBy = QStringLiteral("json-import");

    ComponentSnapshot candidate(component, version, package, pins, parameters);
    const QString contentHash = computeContentHash(candidate);

    // Re-stamp the content hash onto the persisted records.
    Component stampedComponent = component;
    stampedComponent.contentHash = contentHash;
    stampedComponent.activeVersionId = version.versionId;
    ComponentVersion stampedVersion = version;
    stampedVersion.contentHash = contentHash;
    ComponentSnapshot stamped(stampedComponent, stampedVersion, package, pins, parameters);

    report.versionId = stampedVersion.versionId;
    report.contentHash = contentHash;

    // 6. Duplicate detection — never overwrite an existing version (ADR §6.5).
    const auto existing = m_manager.load(scope, component.componentId, stampedVersion.versionId);
    if (existing) {
        report.outcome = ImportOutcome::Skipped;
        report.add(QString(), ImportSeverity::Info,
                   QStringLiteral("version already present in the %1 catalog; skipped (versions are immutable)")
                       .arg(QString::fromLatin1(catalogScopeName(scope))),
                   stampedVersion.versionId.value());
        return report;
    }
    if (existing.error().code() != core::ErrorCode::ComponentNotFound
        && existing.error().code() != core::ErrorCode::ComponentVersionNotFound) {
        return existing.error();
    }

    // 7. Transactional store through the Component Manager.
    const auto stored = m_manager.registerSnapshot(stamped);
    if (!stored) {
        if (stored.error().code() == core::ErrorCode::AlreadyExists) {
            report.outcome = ImportOutcome::Skipped;
            report.add(QString(), ImportSeverity::Info, QStringLiteral("version already present; skipped"),
                       stampedVersion.versionId.value());
            return report;
        }
        return stored.error();
    }

    report.outcome = ImportOutcome::Imported;
    return report;
}

} // namespace ardulab::database
