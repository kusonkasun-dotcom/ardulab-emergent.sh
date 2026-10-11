#pragma once

// NetIdentity — deterministic electrical net identity foundation.
//
// Responsibility:
// - Generate stable NetId from electrical connection members.
// - Same electrical members must always produce the same NetId.
//
// Current phase:
// - Foundation only.
// - Not connected to ConnectivityGraph yet.
// - No persistence logic.
//
// Future:
// - Replace simple connection keys with full electrical signatures
//   (InstanceId + ComponentId + Pin identity).

#include "core/Identifiers.h"

#include <QString>
#include <vector>


namespace ardulab::connection {


struct NetIdentityInput final
{
    std::vector<QString> connectionKeys;
};


class NetIdentity final
{
public:

    /**
     * Generate deterministic NetId.
     *
     * The same normalized input must always
     * generate the same identifier.
     */
    static core::NetId generate(
        const NetIdentityInput& input
        );


private:

    static QString normalize(
        const NetIdentityInput& input
        );

};


} // namespace ardulab::connection