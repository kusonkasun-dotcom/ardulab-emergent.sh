#pragma once

// ProjectMetadata — project identity block (Plan §4.3, Arch Doc §7).

#include "core/Identifiers.h"

#include <QString>

namespace ardulab::project {

/// Current .FAL format version written by this build (Arch Doc §7.2).
inline const QString kCurrentFalFormatVersion = QStringLiteral("1.0");

struct ProjectMetadata final
{
    core::ProjectId projectId;   ///< UUID text; generated when a legacy file lacks one.
    QString name;
    QString version;             ///< Project revision label, e.g. "0.1".
    QString falFormatVersion;    ///< Format version the document was read from / will be written as.
};

/// Canvas sheet settings persisted with the project (Arch Doc §7.2 "canvas").
/// Physical values only — visual scale is presentation state, kept separately.
struct CanvasSettings final
{
    double widthMm = 420.0;
    double heightMm = 297.0;
    double gridMm = 1.0;
    double visualScale = 10.0;   ///< Preserved for compatibility; never applied to physical coordinates.
};

} // namespace ardulab::project
