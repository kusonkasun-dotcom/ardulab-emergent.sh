#pragma once

// ComponentParameter — typed engineering parameter (Plan §4.5, ADR §3.12).
//
// Values are stored as text plus a declared type and unit so that future
// ERC/simulation consumers can interpret them without lossy conversion.

#include <QString>

namespace ardulab::components {

enum class ParameterType {
    Text,
    Integer,
    Real,
    Boolean,
    Enumeration,
};

constexpr const char* parameterTypeName(ParameterType t) noexcept
{
    switch (t) {
    case ParameterType::Text:        return "TEXT";
    case ParameterType::Integer:     return "INTEGER";
    case ParameterType::Real:        return "REAL";
    case ParameterType::Boolean:     return "BOOLEAN";
    case ParameterType::Enumeration: return "ENUM";
    }
    return "TEXT";
}

struct ComponentParameter final
{
    QString key;     ///< Stable parameter key, e.g. "resistance".
    QString value;   ///< Canonical textual value, e.g. "10000".
    QString unit;    ///< SI or domain unit, e.g. "Ohm". Empty when unitless.
    ParameterType type = ParameterType::Text;
};

} // namespace ardulab::components
