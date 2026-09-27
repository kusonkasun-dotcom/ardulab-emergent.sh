#pragma once

// FalDocument — transport-level .FAL JSON document (Plan §4.3).
//
// Separates the on-disk JSON shape from the Project domain object. Keeps the
// full root object so unknown additive fields survive read → write.

#include <QJsonObject>
#include <QString>
#include <QStringList>

namespace ardulab::project {

/// Warnings produced while reading a document (legacy defaults applied,
/// unknown sections preserved, unresolved references, …).
struct FalReadReport final
{
    QString detectedFormatVersion;     ///< "" for the legacy minimal shape.
    bool isLegacyShape = false;        ///< true when only project/components were present.
    QStringList warnings;
    QStringList preservedUnknownKeys;  ///< Root keys carried through untouched.
};

struct FalDocument final
{
    /// Complete parsed root object, exactly as read (or as about to be written).
    QJsonObject root;

    [[nodiscard]] QString formatVersion() const
    {
        return root.value(QStringLiteral("fal_format_version")).toString();
    }

    [[nodiscard]] bool hasLegacyShapeOnly() const
    {
        return !root.contains(QStringLiteral("fal_format_version"))
            && root.contains(QStringLiteral("project"));
    }
};

/// Root keys interpreted by this phase. Anything else is preserved verbatim.
namespace fal_keys {
inline const QString kFormatVersion = QStringLiteral("fal_format_version");
inline const QString kProject = QStringLiteral("project");
inline const QString kCanvas = QStringLiteral("canvas");
inline const QString kLibrary = QStringLiteral("library");
inline const QString kComponents = QStringLiteral("components");
} // namespace fal_keys

} // namespace ardulab::project
