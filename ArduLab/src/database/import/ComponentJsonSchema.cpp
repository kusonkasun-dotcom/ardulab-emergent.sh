#include "database/import/ComponentJsonSchema.h"

namespace ardulab::database {

const QString ComponentJsonSchema::kCanonicalSchemaVersion = QStringLiteral("1.0");

int ComponentJsonSchema::majorOf(const QString& version)
{
    if (version.isEmpty()) {
        return -1;
    }
    const QString major = version.section(QLatin1Char('.'), 0, 0);
    bool ok = false;
    const int value = major.toInt(&ok);
    return ok ? value : -1;
}

bool ComponentJsonSchema::isSupported(const QString& schemaVersion)
{
    return majorOf(schemaVersion) == kSupportedMajor;
}

SchemaDetection ComponentJsonSchema::detect(const QJsonObject& root)
{
    SchemaDetection detection;
    detection.schemaVersion = root.value(QStringLiteral("schema_version")).toString();
    detection.componentSchemaVersion = root.value(QStringLiteral("component_schema_version")).toString();

    if (detection.schemaVersion.isEmpty()) {
        detection.supported = false;
        detection.reason = QStringLiteral("missing 'schema_version'");
        return detection;
    }
    if (!isSupported(detection.schemaVersion)) {
        detection.supported = false;
        detection.reason = QStringLiteral("unsupported schema major version '%1'; this build reads major %2")
                               .arg(detection.schemaVersion)
                               .arg(kSupportedMajor);
        return detection;
    }
    // component_schema_version is copied from the document, not inferred; when
    // absent it defaults to the envelope version.
    if (detection.componentSchemaVersion.isEmpty()) {
        detection.componentSchemaVersion = detection.schemaVersion;
    }
    detection.supported = true;
    return detection;
}

} // namespace ardulab::database
