#pragma once

#include "core/Identifiers.h"
#include "core/Units.h"

namespace ardulab::connection {

namespace tags {
struct Junction {};
}

using JunctionId = core::TypedId<tags::Junction>;

struct Junction final
{
    JunctionId junctionId;

    core::PointMm position;

    [[nodiscard]]
    bool isValid() const noexcept
    {
        return junctionId.isValid();
    }
};

} // namespace ardulab::connection