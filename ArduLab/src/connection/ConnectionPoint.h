#pragma once

// ConnectionPoint — DECLARATION ONLY (Plan §4.8).
//
// A project-local electrical attachment point created by Connection Core
// (v0.1.4) from a placed instance + CatalogPinRef. Persisted in .FAL under
// "connections.points"; never a catalog row.
//
// No behaviour is implemented in the foundation phase.

#include "connection/CatalogPinRef.h"
#include "core/Identifiers.h"
#include "core/Units.h"

namespace ardulab::connection {

struct ConnectionPoint final
{
    core::InstanceId instanceId;   ///< Placed component this point belongs to.
    CatalogPinRef pin;             ///< Exact catalog pin.
    core::PointMm sheetPosition;   ///< Resolved PinAnchor position on the sheet, mm.
};

} // namespace ardulab::connection
