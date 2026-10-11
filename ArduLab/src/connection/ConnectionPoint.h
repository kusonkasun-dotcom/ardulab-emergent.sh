#pragma once

// ConnectionPoint — DECLARATION ONLY (Plan §4.8).
//
// A project-local electrical attachment point created by Connection Core
// (v0.1.4) from a placed instance + CatalogPinRef. Persisted in .FAL under
// "connections.points"; never a catalog row.
//
// Each connection point has its own stable identity so that wires and nets
// can reference electrical attachment points without relying on duplicated
// instance/pin strings.
//
// No behaviour is implemented in the foundation phase.

#include "connection/CatalogPinRef.h"
#include "core/Identifiers.h"
#include "core/Units.h"

namespace ardulab::connection {


struct ConnectionPoint final
{
    /**
     * Stable identity of this project-local electrical point.
     *
     * Example:
     * "CP-U1-GPIO23"
     */
    core::ConnectionPointId pointId;


    /**
     * Placed component this point belongs to.
     */
    core::InstanceId instanceId;


    /**
     * Exact catalog pin reference.
     */
    CatalogPinRef pin;


    /**
     * Resolved PinAnchor position on the sheet, mm.
     */
    core::PointMm sheetPosition;
};


} // namespace ardulab::connection