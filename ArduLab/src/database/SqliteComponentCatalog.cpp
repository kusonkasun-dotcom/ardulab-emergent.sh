#include "database/SqliteComponentCatalog.h"

#include "database/CatalogDatabase.h"
#include "database/CatalogTransaction.h"

#include <QDateTime>
#include <QSqlError>
#include <QSqlQuery>
#include <QVariant>

#include <optional>

namespace ardulab::database {

using namespace ardulab::components;

namespace {

QString nowIso()
{
    return QDateTime::currentDateTimeUtc().toString(Qt::ISODateWithMs);
}

QString isoOr(const QDateTime& dt)
{
    return dt.isValid() ? dt.toUTC().toString(Qt::ISODateWithMs) : nowIso();
}

// ---- enum <-> text ------------------------------------------------------

CatalogScope parseScope(const QString& s)
{
    if (s == QLatin1String("OFFICIAL")) return CatalogScope::Official;
    if (s == QLatin1String("COMMUNITY")) return CatalogScope::Community;
    return CatalogScope::User;
}

LifecycleStatus parseStatus(const QString& s)
{
    if (s == QLatin1String("VERIFIED")) return LifecycleStatus::Verified;
    if (s == QLatin1String("COMMUNITY")) return LifecycleStatus::Community;
    if (s == QLatin1String("OFFICIAL")) return LifecycleStatus::Official;
    if (s == QLatin1String("DEPRECATED")) return LifecycleStatus::Deprecated;
    return LifecycleStatus::Draft;
}

SourceType parseSourceType(const QString& s)
{
    if (s == QLatin1String("JSON_IMPORT")) return SourceType::JsonImport;
    if (s == QLatin1String("LEGACY_IMPORT")) return SourceType::LegacyImport;
    if (s == QLatin1String("AI_PROPOSAL")) return SourceType::AiProposal;
    if (s == QLatin1String("COMMUNITY")) return SourceType::Community;
    return SourceType::Manual;
}

ValidationState parseValidationState(const QString& s)
{
    if (s == QLatin1String("PASSED")) return ValidationState::Passed;
    if (s == QLatin1String("PASSED_WITH_WARNINGS")) return ValidationState::PassedWithWarnings;
    if (s == QLatin1String("FAILED")) return ValidationState::Failed;
    return ValidationState::NotValidated;
}

PinType parsePinType(const QString& s)
{
    if (s == QLatin1String("POWER")) return PinType::Power;
    if (s == QLatin1String("GROUND")) return PinType::Ground;
    if (s == QLatin1String("GPIO")) return PinType::Gpio;
    if (s == QLatin1String("ANALOG")) return PinType::Analog;
    if (s == QLatin1String("DIGITAL")) return PinType::Digital;
    if (s == QLatin1String("PASSIVE")) return PinType::Passive;
    if (s == QLatin1String("NC")) return PinType::NotConnected;
    return PinType::Other;
}

PinDirection parsePinDirection(const QString& s)
{
    if (s == QLatin1String("INPUT")) return PinDirection::Input;
    if (s == QLatin1String("OUTPUT")) return PinDirection::Output;
    if (s == QLatin1String("INPUT_OUTPUT")) return PinDirection::InputOutput;
    if (s == QLatin1String("POWER_IN")) return PinDirection::PowerIn;
    if (s == QLatin1String("POWER_OUT")) return PinDirection::PowerOut;
    if (s == QLatin1String("NC")) return PinDirection::NotConnected;
    return PinDirection::Passive;
}

PinSide parsePinSide(const QString& s)
{
    if (s == QLatin1String("LEFT")) return PinSide::Left;
    if (s == QLatin1String("RIGHT")) return PinSide::Right;
    if (s == QLatin1String("TOP")) return PinSide::Top;
    if (s == QLatin1String("BOTTOM")) return PinSide::Bottom;
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

QString latin1(const char* s) { return QString::fromLatin1(s); }

} // namespace

SqliteComponentCatalog::SqliteComponentCatalog(CatalogDatabase& database)
    : m_database(database)
{
}

core::Result<ComponentSnapshotPtr> SqliteComponentCatalog::loadExact(const ComponentKey& key) const
{
    QSqlDatabase db = m_database.database();
    const QString scope = latin1(catalogScopeName(key.scope));

    // Version row.
    QSqlQuery vq(db);
    vq.prepare(QStringLiteral(
        "SELECT version, component_schema_version, content_hash, source_type, source_reference, "
        "       validation_state, created_utc, activated_utc, created_by, package_id "
        "FROM component_versions WHERE catalog_scope = ? AND component_version_id = ?"));
    vq.addBindValue(scope);
    vq.addBindValue(key.versionId.value());
    if (!vq.exec()) {
        return CatalogDatabase::translate(QStringLiteral("load version failed"), vq.lastError());
    }
    if (!vq.next()) {
        // Distinguish unknown component from unknown version.
        QSqlQuery cq(db);
        cq.prepare(QStringLiteral("SELECT 1 FROM components WHERE catalog_scope = ? AND component_id = ?"));
        cq.addBindValue(scope);
        cq.addBindValue(key.componentId.value());
        if (cq.exec() && cq.next()) {
            return core::Error(core::ErrorCode::ComponentVersionNotFound,
                               QStringLiteral("exact component version not found; no substitution performed"),
                               key.versionId.value());
        }
        return core::Error(core::ErrorCode::ComponentNotFound,
                           QStringLiteral("component not found in %1 catalog").arg(scope), key.componentId.value());
    }

    ComponentVersion version;
    version.versionId = key.versionId;
    version.componentId = key.componentId;
    version.version = vq.value(0).toString();
    version.componentSchemaVersion = vq.value(1).toString();
    version.contentHash = vq.value(2).toString();
    version.sourceType = parseSourceType(vq.value(3).toString());
    version.sourceReference = vq.value(4).toString();
    version.validationState = parseValidationState(vq.value(5).toString());
    version.createdUtc = QDateTime::fromString(vq.value(6).toString(), Qt::ISODateWithMs);
    version.activatedUtc = QDateTime::fromString(vq.value(7).toString(), Qt::ISODateWithMs);
    version.createdBy = vq.value(8).toString();
    const QString packageId = vq.value(9).toString();

    // Component (logical) row.
    QSqlQuery cq(db);
    cq.prepare(QStringLiteral(
        "SELECT name, category_id, manufacturer_id, part_number, description, status, "
        "       component_schema_version, active_version_id, content_hash, replacement_component_id, "
        "       created_utc, updated_utc "
        "FROM components WHERE catalog_scope = ? AND component_id = ?"));
    cq.addBindValue(scope);
    cq.addBindValue(key.componentId.value());
    if (!cq.exec() || !cq.next()) {
        return core::Error(core::ErrorCode::ComponentNotFound, QStringLiteral("component row missing"),
                           key.componentId.value());
    }

    Component component;
    component.componentId = key.componentId;
    component.scope = key.scope;
    component.name = cq.value(0).toString();
    component.categoryId = core::CategoryId(cq.value(1).toString());
    component.manufacturerId = core::ManufacturerId(cq.value(2).toString());
    component.partNumber = cq.value(3).toString();
    component.description = cq.value(4).toString();
    component.status = parseStatus(cq.value(5).toString());
    component.componentSchemaVersion = cq.value(6).toString();
    component.activeVersionId = core::ComponentVersionId(cq.value(7).toString());
    component.contentHash = cq.value(8).toString();
    component.replacementComponentId = core::ComponentId(cq.value(9).toString());
    component.createdUtc = QDateTime::fromString(cq.value(10).toString(), Qt::ISODateWithMs);
    component.updatedUtc = QDateTime::fromString(cq.value(11).toString(), Qt::ISODateWithMs);
    component.version = version.version; // reflect the loaded version

    // Package.
    Package package;
    package.packageId = core::PackageId(packageId);
    QSqlQuery pq(db);
    pq.prepare(QStringLiteral(
        "SELECT package_type, width_mm, height_mm, pin_count, pitch_mm, body_outline_ref "
        "FROM packages WHERE package_id = ?"));
    pq.addBindValue(packageId);
    if (pq.exec() && pq.next()) {
        package.packageType = pq.value(0).toString();
        package.bodySize = core::SizeMm(pq.value(1).toDouble(), pq.value(2).toDouble());
        package.pinCount = pq.value(3).toInt();
        if (!pq.value(4).isNull()) {
            package.pitch = core::Millimeters(pq.value(4).toDouble());
        }
        package.bodyOutlineRef = pq.value(5).toString();
    }

    // Pins.
    std::vector<Pin> pins;
    QSqlQuery pinq(db);
    pinq.prepare(QStringLiteral(
        "SELECT pin_id, pin_number, pin_name, pin_type, direction, side, voltage, functions, "
        "       position_x_mm, position_y_mm, anchor_x_mm, anchor_y_mm "
        "FROM pins WHERE catalog_scope = ? AND component_version_id = ? ORDER BY rowid"));
    pinq.addBindValue(scope);
    pinq.addBindValue(key.versionId.value());
    if (!pinq.exec()) {
        return CatalogDatabase::translate(QStringLiteral("load pins failed"), pinq.lastError());
    }
    while (pinq.next()) {
        Pin pin;
        pin.pinId = core::PinId(pinq.value(0).toString());
        pin.pinNumber = pinq.value(1).toString();
        pin.pinName = pinq.value(2).toString();
        pin.type = parsePinType(pinq.value(3).toString());
        pin.direction = parsePinDirection(pinq.value(4).toString());
        pin.side = parsePinSide(pinq.value(5).toString());
        pin.voltage = pinq.value(6).toString();
        const QString functions = pinq.value(7).toString();
        if (!functions.isEmpty()) {
            pin.functions = functions.split(QLatin1Char(','), Qt::SkipEmptyParts);
        }
        pin.position = core::PointMm(pinq.value(8).toDouble(), pinq.value(9).toDouble());
        pin.anchorPosition = core::PointMm(pinq.value(10).toDouble(), pinq.value(11).toDouble());
        pins.push_back(std::move(pin));
    }

    // Parameters.
    std::vector<ComponentParameter> params;
    QSqlQuery paramq(db);
    paramq.prepare(QStringLiteral(
        "SELECT param_key, value, unit, value_type FROM component_parameters "
        "WHERE catalog_scope = ? AND component_version_id = ? ORDER BY param_key"));
    paramq.addBindValue(scope);
    paramq.addBindValue(key.versionId.value());
    if (paramq.exec()) {
        while (paramq.next()) {
            ComponentParameter p;
            p.key = paramq.value(0).toString();
            p.value = paramq.value(1).toString();
            p.unit = paramq.value(2).toString();
            p.type = parseParameterType(paramq.value(3).toString());
            params.push_back(std::move(p));
        }
    }

    return std::make_shared<const ComponentSnapshot>(std::move(component), std::move(version), std::move(package),
                                                      std::move(pins), std::move(params));
}

core::Result<core::ComponentVersionId>
SqliteComponentCatalog::activeVersionOf(CatalogScope scope, const core::ComponentId& componentId) const
{
    QSqlDatabase db = m_database.database();
    QSqlQuery q(db);
    q.prepare(QStringLiteral("SELECT active_version_id FROM components WHERE catalog_scope = ? AND component_id = ?"));
    q.addBindValue(latin1(catalogScopeName(scope)));
    q.addBindValue(componentId.value());
    if (!q.exec()) {
        return CatalogDatabase::translate(QStringLiteral("active version lookup failed"), q.lastError());
    }
    if (!q.next()) {
        return core::Error(core::ErrorCode::ComponentNotFound, QStringLiteral("component not found"), componentId.value());
    }
    const QString active = q.value(0).toString();
    if (active.isEmpty()) {
        return core::Error(core::ErrorCode::ComponentVersionNotFound, QStringLiteral("component has no active version"),
                           componentId.value());
    }
    return core::ComponentVersionId(active);
}

core::Result<std::vector<ComponentSummary>>
SqliteComponentCatalog::search(const ComponentSearchCriteria& criteria) const
{
    QSqlDatabase db = m_database.database();

    QString sql = QStringLiteral(
        "SELECT c.component_id, c.active_version_id, c.name, c.category_id, c.manufacturer_id, "
        "       c.part_number, c.status, c.version, c.catalog_scope, cv.package_id "
        "FROM components c "
        "LEFT JOIN component_versions cv "
        "  ON cv.catalog_scope = c.catalog_scope AND cv.component_version_id = c.active_version_id ");

    QStringList where;
    QVariantList binds;

    if (!criteria.text.isEmpty()) {
        where << QStringLiteral("(c.component_id LIKE ? OR c.name LIKE ? OR c.part_number LIKE ?)");
        const QString like = QStringLiteral("%%%1%%").arg(criteria.text);
        binds << like << like << like;
    }
    if (criteria.categoryId) {
        where << QStringLiteral("c.category_id = ?");
        binds << criteria.categoryId->value();
    }
    if (criteria.manufacturerId) {
        where << QStringLiteral("c.manufacturer_id = ?");
        binds << criteria.manufacturerId->value();
    }
    if (criteria.packageId) {
        where << QStringLiteral("cv.package_id = ?");
        binds << criteria.packageId->value();
    }
    if (!criteria.scopes.empty()) {
        QStringList marks;
        for (const CatalogScope s : criteria.scopes) {
            marks << QStringLiteral("?");
            binds << latin1(catalogScopeName(s));
        }
        where << QStringLiteral("c.catalog_scope IN (%1)").arg(marks.join(QStringLiteral(", ")));
    }
    if (!criteria.statuses.empty()) {
        QStringList marks;
        for (const LifecycleStatus s : criteria.statuses) {
            marks << QStringLiteral("?");
            binds << latin1(lifecycleStatusName(s));
        }
        where << QStringLiteral("c.status IN (%1)").arg(marks.join(QStringLiteral(", ")));
    } else if (!criteria.includeDeprecated) {
        where << QStringLiteral("c.status != 'DEPRECATED'");
    }

    if (!where.isEmpty()) {
        sql += QStringLiteral("WHERE ") + where.join(QStringLiteral(" AND ")) + QLatin1Char(' ');
    }
    sql += QStringLiteral("ORDER BY c.component_id, c.catalog_scope ");
    if (criteria.limit > 0) {
        sql += QStringLiteral("LIMIT ?");
        binds << criteria.limit;
    }

    QSqlQuery q(db);
    q.prepare(sql);
    for (const QVariant& b : binds) {
        q.addBindValue(b);
    }
    if (!q.exec()) {
        return CatalogDatabase::translate(QStringLiteral("search failed"), q.lastError());
    }

    std::vector<ComponentSummary> results;
    while (q.next()) {
        ComponentSummary s;
        s.componentId = core::ComponentId(q.value(0).toString());
        s.activeVersionId = core::ComponentVersionId(q.value(1).toString());
        s.name = q.value(2).toString();
        s.categoryId = core::CategoryId(q.value(3).toString());
        s.manufacturerId = core::ManufacturerId(q.value(4).toString());
        s.partNumber = q.value(5).toString();
        s.status = parseStatus(q.value(6).toString());
        s.version = q.value(7).toString();
        s.scope = parseScope(q.value(8).toString());
        s.packageId = core::PackageId(q.value(9).toString());
        results.push_back(std::move(s));
    }
    return results;
}

core::Result<std::vector<ComponentVersionSummary>>
SqliteComponentCatalog::listVersions(CatalogScope scope, const core::ComponentId& componentId) const
{
    QSqlDatabase db = m_database.database();

    QString activeVersion;
    {
        QSqlQuery aq(db);
        aq.prepare(QStringLiteral("SELECT active_version_id FROM components WHERE catalog_scope = ? AND component_id = ?"));
        aq.addBindValue(latin1(catalogScopeName(scope)));
        aq.addBindValue(componentId.value());
        if (aq.exec() && aq.next()) {
            activeVersion = aq.value(0).toString();
        }
    }

    QSqlQuery q(db);
    q.prepare(QStringLiteral(
        "SELECT component_version_id, version, content_hash, validation_state "
        "FROM component_versions WHERE catalog_scope = ? AND component_id = ? "
        "ORDER BY created_utc DESC, rowid DESC"));
    q.addBindValue(latin1(catalogScopeName(scope)));
    q.addBindValue(componentId.value());
    if (!q.exec()) {
        return CatalogDatabase::translate(QStringLiteral("list versions failed"), q.lastError());
    }

    std::vector<ComponentVersionSummary> versions;
    while (q.next()) {
        ComponentVersionSummary v;
        v.versionId = core::ComponentVersionId(q.value(0).toString());
        v.version = q.value(1).toString();
        v.contentHash = q.value(2).toString();
        v.validationState = parseValidationState(q.value(3).toString());
        v.isActive = !activeVersion.isEmpty() && activeVersion == v.versionId.value();
        versions.push_back(std::move(v));
    }
    if (versions.empty()) {
        return core::Error(core::ErrorCode::ComponentNotFound, QStringLiteral("component not found"), componentId.value());
    }
    return versions;
}

core::Status SqliteComponentCatalog::store(const ComponentSnapshot& snapshot)
{
    QSqlDatabase db = m_database.database();
    const ComponentKey key = snapshot.key();
    const QString scope = latin1(catalogScopeName(key.scope));
    const Component& c = snapshot.component();
    const ComponentVersion& v = snapshot.version();
    const Package& pkg = snapshot.package();

    CatalogTransaction tx(db);
    if (const auto begun = tx.begin(); !begun) {
        return begun;
    }

    // Reject an existing immutable version up front (ADR §3.3).
    {
        QSqlQuery exists(db);
        exists.prepare(QStringLiteral(
            "SELECT 1 FROM component_versions WHERE catalog_scope = ? AND component_version_id = ?"));
        exists.addBindValue(scope);
        exists.addBindValue(key.versionId.value());
        if (!exists.exec()) {
            return CatalogDatabase::translate(QStringLiteral("version existence check failed"), exists.lastError());
        }
        if (exists.next()) {
            return core::Error(core::ErrorCode::AlreadyExists,
                               QStringLiteral("component version already exists; versions are immutable"),
                               key.versionId.value());
        }
    }

    // Package (shared, upserted).
    {
        QSqlQuery q(db);
        q.prepare(QStringLiteral(
            "INSERT INTO packages (package_id, package_type, width_mm, height_mm, pin_count, pitch_mm, body_outline_ref) "
            "VALUES (?, ?, ?, ?, ?, ?, ?) "
            "ON CONFLICT(package_id) DO UPDATE SET package_type=excluded.package_type, width_mm=excluded.width_mm, "
            "  height_mm=excluded.height_mm, pin_count=excluded.pin_count, pitch_mm=excluded.pitch_mm, "
            "  body_outline_ref=excluded.body_outline_ref"));
        q.addBindValue(pkg.packageId.value());
        q.addBindValue(pkg.packageType);
        q.addBindValue(pkg.bodySize.width);
        q.addBindValue(pkg.bodySize.height);
        q.addBindValue(pkg.pinCount);
        q.addBindValue(pkg.pitch ? QVariant(pkg.pitch->value) : QVariant());
        q.addBindValue(pkg.bodyOutlineRef.isEmpty() ? QVariant() : QVariant(pkg.bodyOutlineRef));
        if (!q.exec()) {
            return CatalogDatabase::translate(QStringLiteral("store package failed"), q.lastError());
        }
    }

    // Manufacturer (optional).
    if (!c.manufacturerId.isEmpty()) {
        QSqlQuery q(db);
        q.prepare(QStringLiteral(
            "INSERT INTO manufacturers (manufacturer_id, name, normalized_name, created_utc, updated_utc) "
            "VALUES (?, ?, ?, ?, ?) ON CONFLICT(manufacturer_id) DO NOTHING"));
        q.addBindValue(c.manufacturerId.value());
        q.addBindValue(c.manufacturerId.value());
        q.addBindValue(c.manufacturerId.value().toLower());
        q.addBindValue(nowIso());
        q.addBindValue(nowIso());
        if (!q.exec()) {
            return CatalogDatabase::translate(QStringLiteral("store manufacturer failed"), q.lastError());
        }
    }

    // Logical component (upsert; foundation policy: newest stored version is active).
    {
        QSqlQuery q(db);
        q.prepare(QStringLiteral(
            "INSERT INTO components (catalog_scope, component_id, name, category_id, manufacturer_id, part_number, "
            "  description, status, version, component_schema_version, active_version_id, content_hash, "
            "  replacement_component_id, created_utc, updated_utc) "
            "VALUES (?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?) "
            "ON CONFLICT(catalog_scope, component_id) DO UPDATE SET name=excluded.name, category_id=excluded.category_id, "
            "  manufacturer_id=excluded.manufacturer_id, part_number=excluded.part_number, description=excluded.description, "
            "  status=excluded.status, version=excluded.version, component_schema_version=excluded.component_schema_version, "
            "  active_version_id=excluded.active_version_id, content_hash=excluded.content_hash, "
            "  replacement_component_id=excluded.replacement_component_id, updated_utc=excluded.updated_utc"));
        q.addBindValue(scope);
        q.addBindValue(c.componentId.value());
        q.addBindValue(c.name);
        q.addBindValue(c.categoryId.value());
        q.addBindValue(c.manufacturerId.isEmpty() ? QVariant() : QVariant(c.manufacturerId.value()));
        q.addBindValue(c.partNumber.isEmpty() ? QVariant() : QVariant(c.partNumber));
        q.addBindValue(c.description);
        q.addBindValue(latin1(lifecycleStatusName(c.status)));
        q.addBindValue(v.version);
        q.addBindValue(c.componentSchemaVersion.isEmpty() ? v.componentSchemaVersion : c.componentSchemaVersion);
        q.addBindValue(v.versionId.value());
        q.addBindValue(v.contentHash);
        q.addBindValue(c.replacementComponentId.isEmpty() ? QVariant() : QVariant(c.replacementComponentId.value()));
        q.addBindValue(isoOr(c.createdUtc));
        q.addBindValue(nowIso());
        if (!q.exec()) {
            return CatalogDatabase::translate(QStringLiteral("store component failed"), q.lastError());
        }
    }

    // Immutable version.
    {
        QSqlQuery q(db);
        q.prepare(QStringLiteral(
            "INSERT INTO component_versions (catalog_scope, component_version_id, component_id, version, "
            "  component_schema_version, content_hash, source_type, source_reference, validation_state, "
            "  created_utc, activated_utc, created_by, package_id) "
            "VALUES (?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?)"));
        q.addBindValue(scope);
        q.addBindValue(v.versionId.value());
        q.addBindValue(c.componentId.value());
        q.addBindValue(v.version);
        q.addBindValue(v.componentSchemaVersion);
        q.addBindValue(v.contentHash);
        q.addBindValue(latin1(sourceTypeName(v.sourceType)));
        q.addBindValue(v.sourceReference.isEmpty() ? QVariant() : QVariant(v.sourceReference));
        q.addBindValue(latin1(validationStateName(v.validationState)));
        q.addBindValue(isoOr(v.createdUtc));
        q.addBindValue(v.activatedUtc.isValid() ? QVariant(v.activatedUtc.toUTC().toString(Qt::ISODateWithMs)) : QVariant());
        q.addBindValue(v.createdBy.isEmpty() ? QVariant() : QVariant(v.createdBy));
        q.addBindValue(pkg.packageId.value());
        if (!q.exec()) {
            return CatalogDatabase::translate(QStringLiteral("store version failed"), q.lastError());
        }
    }

    // Pins.
    for (const Pin& pin : snapshot.pins()) {
        QSqlQuery q(db);
        q.prepare(QStringLiteral(
            "INSERT INTO pins (pin_id, catalog_scope, component_version_id, component_id, pin_number, pin_name, "
            "  pin_type, direction, side, voltage, functions, position_x_mm, position_y_mm, anchor_x_mm, anchor_y_mm) "
            "VALUES (?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?)"));
        const QString pinId = pin.pinId.isEmpty()
            ? (v.versionId.value() + QLatin1Char('#') + pin.pinNumber)
            : pin.pinId.value();
        q.addBindValue(pinId);
        q.addBindValue(scope);
        q.addBindValue(v.versionId.value());
        q.addBindValue(c.componentId.value());
        q.addBindValue(pin.pinNumber);
        q.addBindValue(pin.pinName);
        q.addBindValue(latin1(pinTypeName(pin.type)));
        q.addBindValue(latin1(pinDirectionName(pin.direction)));
        q.addBindValue(latin1(pinSideName(pin.side)));
        q.addBindValue(pin.voltage.isEmpty() ? QVariant() : QVariant(pin.voltage));
        q.addBindValue(pin.functions.isEmpty() ? QVariant() : QVariant(pin.functions.join(QLatin1Char(','))));
        q.addBindValue(pin.position.x);
        q.addBindValue(pin.position.y);
        q.addBindValue(pin.anchorPosition.x);
        q.addBindValue(pin.anchorPosition.y);
        if (!q.exec()) {
            return CatalogDatabase::translate(QStringLiteral("store pin failed"), q.lastError());
        }
    }

    // Parameters.
    for (const ComponentParameter& param : snapshot.parameters()) {
        QSqlQuery q(db);
        q.prepare(QStringLiteral(
            "INSERT INTO component_parameters (catalog_scope, component_version_id, param_key, value, unit, value_type) "
            "VALUES (?, ?, ?, ?, ?, ?)"));
        q.addBindValue(scope);
        q.addBindValue(v.versionId.value());
        q.addBindValue(param.key);
        q.addBindValue(param.value);
        q.addBindValue(param.unit);
        q.addBindValue(latin1(parameterTypeName(param.type)));
        if (!q.exec()) {
            return CatalogDatabase::translate(QStringLiteral("store parameter failed"), q.lastError());
        }
    }

    return tx.commit();
}

} // namespace ardulab::database
