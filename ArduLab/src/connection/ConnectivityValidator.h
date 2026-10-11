#pragma once

#include "connection/ConnectivityGraph.h"
#include "connection/ValidationResult.h"


namespace ardulab::connection {


class ConnectivityValidator final
{
public:

    ValidationResult validate(
        const ConnectivityGraph& graph
        ) const;

};


}