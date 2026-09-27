#pragma once

// ComponentJsonSchema — recognise the component JSON contract version (ADR §6.2/§6.3).
//
// The canonical envelope declares `schema_version` and `component_schema_version`.
// v0.1.3 reads canonical major version 1 (e.g. "1.0"). An unsupported major
// version is rejected with actionable guidance (validation rule VAL016).

#include <QJsonObject>
#include <QString>

namespace ardulab::database {

struct SchemaDetection final
{
    QString schemaVersion;            ///< Envelope/document version, e.g. "1.0".
    QString componentSchemaVersion;   ///< Component contract version, e.g. "1.0".
    bool supported = false;
    QString reason;                   ///< Populated when !supported.
};

class ComponentJsonSchema
{
public:
    static constexpr int kSupportedMajor = 1;
    static const QString kCanonicalSchemaVersion; ///< "1.0"

    [[nodiscard]] static SchemaDetection detect(const QJsonObject& root);

    /// True when the major version equals the supported canonical major.
    [[nodiscard]] static bool isSupported(const QString& schemaVersion);

    [[nodiscard]] static int majorOf(const QString& version);
};

} // namespace ardulab::database
