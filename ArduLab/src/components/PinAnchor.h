#pragma once

// PinAnchor — electrical connection anchor (Plan §4.5, §8.2).
//
// A PinAnchor is a *value object* expressed in component-local millimeter
// coordinates (origin = package body origin, +X right, +Y down).
//
// It is the stable handoff contract to the future Connection Core:
//   - It does NOT create wires, connection points, or nets.
//   - It has no SQL, JSON, scene, or widget behavior.
//   - Adapters (JSON/SQLite) map *to* it; they never define it.
//   - Component Manager returns it unchanged inside ComponentSnapshot.

#include "core/Units.h"

#include <QString>

namespace ardulab::components {

class PinAnchor final
{
public:
    PinAnchor() = default;

    PinAnchor(QString pinNumber, core::PointMm positionMm)
        : m_pinNumber(std::move(pinNumber))
        , m_position(positionMm)
    {
    }

    /// Package pin number / pad identifier this anchor belongs to ("1", "A3").
    [[nodiscard]] const QString& pinNumber() const noexcept { return m_pinNumber; }

    /// Electrical connection point in component-local millimeters.
    [[nodiscard]] core::PointMm position() const noexcept { return m_position; }

    [[nodiscard]] bool isValid() const noexcept { return !m_pinNumber.isEmpty(); }

    friend bool operator==(const PinAnchor& a, const PinAnchor& b) noexcept
    {
        return a.m_pinNumber == b.m_pinNumber && a.m_position == b.m_position;
    }
    friend bool operator!=(const PinAnchor& a, const PinAnchor& b) noexcept { return !(a == b); }

private:
    QString m_pinNumber;
    core::PointMm m_position;
};

} // namespace ardulab::components
