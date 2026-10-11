#pragma once

// Strongly-typed identifiers (Plan §4.2, ADR §3.1).
//
// Each identity concept is a distinct type so that a catalog ID can never be
// passed where a project-instance ID is expected. All IDs are text-based
// because the approved catalog uses human-readable stable IDs
// (e.g. "MMCU1-A-ESP32-WROOM32", "MMCU1-A-ESP32-WROOM32@1.0.0", "U1").

#include <QHash>
#include <QString>

namespace ardulab::core {

/// CRTP-free tagged string ID. Tag is an empty struct that names the concept.
template <typename Tag>
class TypedId final
{
public:
    TypedId() = default;
    explicit TypedId(QString value)
        : m_value(std::move(value))
    {
    }

    [[nodiscard]] const QString& value() const noexcept { return m_value; }
    [[nodiscard]] bool isEmpty() const noexcept { return m_value.isEmpty(); }
    [[nodiscard]] bool isValid() const noexcept { return !m_value.isEmpty(); }

    friend bool operator==(const TypedId& a, const TypedId& b) noexcept { return a.m_value == b.m_value; }
    friend bool operator!=(const TypedId& a, const TypedId& b) noexcept { return a.m_value != b.m_value; }
    friend bool operator<(const TypedId& a, const TypedId& b) noexcept { return a.m_value < b.m_value; }

    friend size_t qHash(const TypedId& id, size_t seed = 0) noexcept { return ::qHash(id.m_value, seed); }

private:
    QString m_value;
};

namespace tags {
struct Component {};
struct ComponentVersion {};
struct ProjectInstance {};
struct Pin {};
struct Package {};
struct Project {};
struct Net {};
struct ConnectionPoint {};
struct Category {};
struct Manufacturer {};
} // namespace tags


/// Stable logical catalog identity — "R1-A-0805-10K".
using ComponentId = TypedId<tags::Component>;

/// Immutable catalog revision.
using ComponentVersionId = TypedId<tags::ComponentVersion>;

/// Project-local placed occurrence.
using InstanceId = TypedId<tags::ProjectInstance>;

/// Stable pin record identity.
using PinId = TypedId<tags::Pin>;

/// Stable package identity.
using PackageId = TypedId<tags::Package>;

/// Project identity.
using ProjectId = TypedId<tags::Project>;

/// Electrical network identity.
using NetId = TypedId<tags::Net>;

/// Electrical connection point identity.
using ConnectionPointId = TypedId<tags::ConnectionPoint>;

/// Category code.
using CategoryId = TypedId<tags::Category>;

/// Manufacturer identity.
using ManufacturerId = TypedId<tags::Manufacturer>;

/// Build the canonical version ID: "<component_id>@<semantic_version>".
inline ComponentVersionId makeComponentVersionId(const ComponentId& componentId, const QString& semanticVersion)
{
    return ComponentVersionId(componentId.value() + QLatin1Char('@') + semanticVersion);
}

} // namespace ardulab::core
