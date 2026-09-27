#pragma once

// Pin — catalog pin definition (Plan §4.5, ADR §3.6).
//
// Value type. Position and anchor are physical millimeters relative to the
// package origin. Enumerations mirror the approved ADR vocabularies.

#include "components/PinAnchor.h"
#include "core/Identifiers.h"
#include "core/Units.h"

#include <QString>
#include <QStringList>

namespace ardulab::components {

enum class PinType {
    Power,
    Ground,
    Gpio,
    Analog,
    Digital,
    Passive,
    NotConnected,
    Other,
};

enum class PinDirection {
    Input,
    Output,
    InputOutput,
    PowerIn,
    PowerOut,
    Passive,
    NotConnected,
};

enum class PinSide {
    Left,
    Right,
    Top,
    Bottom,
    PackageDefined,
};

constexpr const char* pinTypeName(PinType t) noexcept
{
    switch (t) {
    case PinType::Power:        return "POWER";
    case PinType::Ground:       return "GROUND";
    case PinType::Gpio:         return "GPIO";
    case PinType::Analog:       return "ANALOG";
    case PinType::Digital:      return "DIGITAL";
    case PinType::Passive:      return "PASSIVE";
    case PinType::NotConnected: return "NC";
    case PinType::Other:        return "OTHER";
    }
    return "OTHER";
}

constexpr const char* pinDirectionName(PinDirection d) noexcept
{
    switch (d) {
    case PinDirection::Input:        return "INPUT";
    case PinDirection::Output:       return "OUTPUT";
    case PinDirection::InputOutput:  return "INPUT_OUTPUT";
    case PinDirection::PowerIn:      return "POWER_IN";
    case PinDirection::PowerOut:     return "POWER_OUT";
    case PinDirection::Passive:      return "PASSIVE";
    case PinDirection::NotConnected: return "NC";
    }
    return "PASSIVE";
}

constexpr const char* pinSideName(PinSide s) noexcept
{
    switch (s) {
    case PinSide::Left:           return "LEFT";
    case PinSide::Right:          return "RIGHT";
    case PinSide::Top:            return "TOP";
    case PinSide::Bottom:         return "BOTTOM";
    case PinSide::PackageDefined: return "PACKAGE_DEFINED";
    }
    return "PACKAGE_DEFINED";
}

struct Pin final
{
    core::PinId pinId;
    QString pinNumber;            ///< Required. Package pin number or pad identifier.
    QString pinName;              ///< Required. e.g. "GPIO23".
    PinType type = PinType::Other;
    PinDirection direction = PinDirection::Passive;
    PinSide side = PinSide::PackageDefined;
    QString voltage;              ///< Optional nominal/allowed voltage text.
    QStringList functions;        ///< Optional alternate functions (I2C_SCL, PWM…).
    core::PointMm position;       ///< Physical pin position, mm, package origin.
    core::PointMm anchorPosition; ///< Electrical connection point, mm.

    /// The stable value handed to Connection Core.
    [[nodiscard]] PinAnchor anchor() const { return PinAnchor(pinNumber, anchorPosition); }

    /// A pin without a trustworthy number cannot be used by Connection Core (ADR §3.6).
    [[nodiscard]] bool isConnectable() const noexcept { return !pinNumber.isEmpty(); }
};

} // namespace ardulab::components
