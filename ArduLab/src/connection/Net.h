#pragma once

// Net — DECLARATION ONLY (Plan §4.8).
//
// A project-local electrical equivalence class of connection points and
// wires, owned by Connection Core (v0.1.4). Persisted in .FAL under
// "connections.nets". Consumed later by Schematic ERC, Simulation, and PCB.
//
// No net construction or graph algorithm exists in the foundation phase.

#include "connection/CatalogPinRef.h"
#include "connection/Wire.h"
#include "core/Identifiers.h"

#include <QString>

#include <vector>

namespace ardulab::connection {

struct NetMember final
{
    core::InstanceId instanceId;
    CatalogPinRef pin;
};

struct Net final
{
    core::NetId netId;
    QString name;                  ///< Optional user label (net label arrives with Schematic Engine).
    std::vector<NetMember> members;
    std::vector<WireId> wires;
};

} // namespace ardulab::connection
