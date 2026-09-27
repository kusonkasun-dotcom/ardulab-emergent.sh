#pragma once

// Wire — DECLARATION ONLY (Plan §4.8).
//
// A project-local polyline between connection points / junctions, owned by
// Connection Core (v0.1.4). Persisted in .FAL under "connections.wires".
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
    WireId wireId;
    std::vector<core::PointMm> vertices; ///< Sheet millimeters.
    core::NetId netId;                   ///< Assigned by the Net System.
};

} // namespace ardulab::connection
