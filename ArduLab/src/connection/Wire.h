#pragma once

// Wire — DECLARATION ONLY (Plan §4.8).
//
// A project-local polyline between connection points / junctions, owned by
// Connection Core (v0.1.4). Persisted in .FAL under "connections.wires".
//
// A wire connects two electrical attachment points identified by
// ConnectionPointId. The vertices describe only the visual routing geometry.
//
// No routing, editing, or junction logic exists in the foundation phase.

#include "core/Identifiers.h"
#include "core/Units.h"

#include <vector>

namespace ardulab::connection {


namespace tags { struct Wire {}; }

using WireId = core::TypedId<tags::Wire>;


struct Wire final
{
    /**
     * Stable wire identity.
     */
    WireId wireId;


    /**
     * Electrical source endpoint.
     *
     * References ConnectionPoint::pointId.
     */
    core::ConnectionPointId startPointId;


    /**
     * Electrical destination endpoint.
     *
     * References ConnectionPoint::pointId.
     */
    core::ConnectionPointId endPointId;


    /**
     * Visual routing geometry.
     *
     * Stored in sheet millimeters.
     */
    std::vector<core::PointMm> vertices;


    /**
     * Assigned after Net System resolution.
     */
    core::NetId netId;
};


} // namespace ardulab::connection