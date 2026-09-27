#include "database/migrations/Migration001InitialCatalog.h"

#include "database/CatalogDatabase.h"

#include <QCryptographicHash>
#include <QDateTime>
#include <QSqlError>
#include <QSqlQuery>
#include <QStringList>

namespace ardulab::database {

namespace {

// The DDL and seed statements. Kept as a static list so checksum() can hash
// the exact definition and detect later edits (ADR §7.5 / §11.3).
const QStringList& statements()
{
    static const QStringList sql = {
        // Applied-migration ledger.
        QStringLiteral(
            "CREATE TABLE schema_migrations ("
            "  migration_id        INTEGER PRIMARY KEY,"
            "  description         TEXT NOT NULL,"
            "  checksum            TEXT NOT NULL,"
            "  application_version TEXT NOT NULL,"
            "  applied_utc         TEXT NOT NULL"
            ")"),

        // Free-form catalog metadata (schema identity, catalog id, etc.).
        QStringLiteral(
            "CREATE TABLE catalog_metadata ("
            "  meta_key   TEXT PRIMARY KEY,"
            "  meta_value TEXT NOT NULL"
            ")"),

        // Taxonomy.
        QStringLiteral(
            "CREATE TABLE categories ("
            "  category_id        TEXT PRIMARY KEY,"
            "  name               TEXT NOT NULL,"
            "  parent_category_id TEXT REFERENCES categories(category_id),"
            "  description        TEXT,"
            "  is_system          INTEGER NOT NULL DEFAULT 0"
            ")"),
        QStringLiteral(
            "CREATE TABLE manufacturers ("
            "  manufacturer_id TEXT PRIMARY KEY,"
            "  name            TEXT NOT NULL,"
            "  normalized_name TEXT,"
            "  website         TEXT,"
            "  country         TEXT,"
            "  created_utc     TEXT,"
            "  updated_utc     TEXT"
            ")"),
        QStringLiteral(
            "CREATE TABLE packages ("
            "  package_id       TEXT PRIMARY KEY,"
            "  package_type     TEXT NOT NULL,"
            "  width_mm         REAL NOT NULL DEFAULT 0,"
            "  height_mm        REAL NOT NULL DEFAULT 0,"
            "  pin_count        INTEGER NOT NULL DEFAULT 0,"
            "  pitch_mm         REAL,"
            "  body_outline_ref TEXT,"
            "  CHECK (width_mm >= 0 AND height_mm >= 0 AND pin_count >= 0 AND (pitch_mm IS NULL OR pitch_mm >= 0))"
            ")"),

        // Logical component identity (mutable lifecycle metadata).
        QStringLiteral(
            "CREATE TABLE components ("
            "  catalog_scope            TEXT NOT NULL,"
            "  component_id             TEXT NOT NULL,"
            "  name                     TEXT NOT NULL,"
            "  category_id              TEXT NOT NULL REFERENCES categories(category_id),"
            "  manufacturer_id          TEXT REFERENCES manufacturers(manufacturer_id),"
            "  part_number              TEXT,"
            "  description              TEXT,"
            "  status                   TEXT NOT NULL,"
            "  version                  TEXT NOT NULL,"
            "  component_schema_version TEXT NOT NULL,"
            "  active_version_id        TEXT,"
            "  content_hash             TEXT NOT NULL,"
            "  replacement_component_id TEXT,"
            "  created_utc              TEXT NOT NULL,"
            "  updated_utc              TEXT NOT NULL,"
            "  PRIMARY KEY (catalog_scope, component_id)"
            ")"),

        // Immutable versions.
        QStringLiteral(
            "CREATE TABLE component_versions ("
            "  catalog_scope            TEXT NOT NULL,"
            "  component_version_id     TEXT NOT NULL,"
            "  component_id             TEXT NOT NULL,"
            "  version                  TEXT NOT NULL,"
            "  component_schema_version TEXT NOT NULL,"
            "  content_hash             TEXT NOT NULL,"
            "  source_type              TEXT NOT NULL,"
            "  source_reference         TEXT,"
            "  validation_state         TEXT NOT NULL,"
            "  created_utc              TEXT NOT NULL,"
            "  activated_utc            TEXT,"
            "  created_by               TEXT,"
            "  package_id               TEXT NOT NULL REFERENCES packages(package_id),"
            "  PRIMARY KEY (catalog_scope, component_version_id),"
            "  UNIQUE (catalog_scope, component_id, version),"
            "  FOREIGN KEY (catalog_scope, component_id)"
            "    REFERENCES components(catalog_scope, component_id) ON DELETE CASCADE"
            ")"),

        // Versioned pins (Connection Core boundary). Unique pin number per version.
        QStringLiteral(
            "CREATE TABLE pins ("
            "  pin_id               TEXT NOT NULL,"
            "  catalog_scope        TEXT NOT NULL,"
            "  component_version_id TEXT NOT NULL,"
            "  component_id         TEXT NOT NULL,"
            "  pin_number           TEXT NOT NULL,"
            "  pin_name             TEXT NOT NULL,"
            "  pin_type             TEXT NOT NULL,"
            "  direction            TEXT NOT NULL,"
            "  side                 TEXT NOT NULL,"
            "  voltage              TEXT,"
            "  functions            TEXT,"
            "  position_x_mm        REAL NOT NULL,"
            "  position_y_mm        REAL NOT NULL,"
            "  anchor_x_mm          REAL NOT NULL,"
            "  anchor_y_mm          REAL NOT NULL,"
            "  PRIMARY KEY (catalog_scope, component_version_id, pin_number),"
            "  FOREIGN KEY (catalog_scope, component_version_id)"
            "    REFERENCES component_versions(catalog_scope, component_version_id) ON DELETE CASCADE"
            ")"),

        // Versioned engineering parameters.
        QStringLiteral(
            "CREATE TABLE component_parameters ("
            "  catalog_scope        TEXT NOT NULL,"
            "  component_version_id TEXT NOT NULL,"
            "  param_key            TEXT NOT NULL,"
            "  value                TEXT,"
            "  unit                 TEXT,"
            "  value_type           TEXT NOT NULL,"
            "  PRIMARY KEY (catalog_scope, component_version_id, param_key),"
            "  FOREIGN KEY (catalog_scope, component_version_id)"
            "    REFERENCES component_versions(catalog_scope, component_version_id) ON DELETE CASCADE"
            ")"),

        QStringLiteral("CREATE INDEX idx_components_category ON components(category_id)"),
        QStringLiteral("CREATE INDEX idx_components_status ON components(status)"),
        QStringLiteral("CREATE INDEX idx_versions_component ON component_versions(catalog_scope, component_id)"),
        QStringLiteral("CREATE INDEX idx_pins_version ON pins(catalog_scope, component_version_id)"),
    };
    return sql;
}

// Required initial system categories (ADR §3.4).
struct SeedCategory
{
    const char* id;
    const char* name;
    const char* description;
};

const std::vector<SeedCategory>& seedCategories()
{
    static const std::vector<SeedCategory> cats = {
        {"MCU", "Microcontroller", "Bare microcontroller IC"},
        {"MCU_MODULE", "Microcontroller Module", "MCU carrier or radio module"},
        {"PASSIVE", "Passive", "Resistors, capacitors, inductors"},
        {"DIODE", "Diode", "Rectifier, Zener, Schottky, ESD, LED"},
        {"ACTIVE", "Active", "Transistors, op-amps, logic and other ICs"},
        {"POWER", "Power", "Regulators, converters, protection"},
        {"SENSOR", "Sensor", "Environmental, motion, optical sensors"},
        {"ACTUATOR", "Actuator", "Relays, motors, drivers"},
        {"CONNECTOR", "Connector", "Headers, terminals, USB"},
    };
    return cats;
}

} // namespace

QString Migration001InitialCatalog::checksum() const
{
    QByteArray payload;
    for (const QString& s : statements()) {
        payload += s.toUtf8();
        payload += '\n';
    }
    for (const SeedCategory& c : seedCategories()) {
        payload += c.id;
        payload += '|';
    }
    return QStringLiteral("sha256:")
        + QString::fromLatin1(QCryptographicHash::hash(payload, QCryptographicHash::Sha256).toHex());
}

core::Status Migration001InitialCatalog::apply(QSqlDatabase& db) const
{
    QSqlQuery query(db);
    for (const QString& sql : statements()) {
        if (!query.exec(sql)) {
            return CatalogDatabase::translate(QStringLiteral("migration 001 failed"), query.lastError());
        }
    }

    // Seed system categories.
    for (const SeedCategory& c : seedCategories()) {
        QSqlQuery insert(db);
        insert.prepare(QStringLiteral(
            "INSERT INTO categories (category_id, name, parent_category_id, description, is_system) "
            "VALUES (?, ?, NULL, ?, 1)"));
        insert.addBindValue(QString::fromLatin1(c.id));
        insert.addBindValue(QString::fromLatin1(c.name));
        insert.addBindValue(QString::fromLatin1(c.description));
        if (!insert.exec()) {
            return CatalogDatabase::translate(QStringLiteral("migration 001 category seed failed"), insert.lastError());
        }
    }

    // Catalog metadata.
    const QDateTime now = QDateTime::currentDateTimeUtc();
    const std::vector<std::pair<QString, QString>> meta = {
        {QStringLiteral("catalog_schema_version"), QStringLiteral("1")},
        {QStringLiteral("component_schema_version"), QStringLiteral("1.0")},
        {QStringLiteral("created_utc"), now.toString(Qt::ISODateWithMs)},
    };
    for (const auto& [key, value] : meta) {
        QSqlQuery insert(db);
        insert.prepare(QStringLiteral("INSERT INTO catalog_metadata (meta_key, meta_value) VALUES (?, ?)"));
        insert.addBindValue(key);
        insert.addBindValue(value);
        if (!insert.exec()) {
            return CatalogDatabase::translate(QStringLiteral("migration 001 metadata seed failed"), insert.lastError());
        }
    }

    return core::Status::success();
}

} // namespace ardulab::database
