#pragma once

// Package — physical package definition (Plan §4.5, ADR §3.5).
//
// All dimensions are millimeters. Visual scale is a Canvas/Renderer concern
// and is never stored here.

#include "core/Identifiers.h"
#include "core/Units.h"

#include <QString>

#include <optional>

namespace ardulab::components {

struct Package final
{
    core::PackageId packageId;
    QString packageType;                 ///< QFN, TQFP, MODULE, 0805 …
    core::SizeMm bodySize;               ///< Physical width × height in mm.
    int pinCount = 0;
    std::optional<core::Millimeters> pitch; ///< Nullable for non-pinned parts.
    QString bodyOutlineRef;              ///< Optional geometry source reference.

    [[nodiscard]] bool hasGeometry() const noexcept { return bodySize.isValid(); }
};

} // namespace ardulab::components
