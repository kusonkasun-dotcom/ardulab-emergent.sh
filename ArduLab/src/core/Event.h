#pragma once

// Event foundation (Plan §4.2).
//
// Events are plain value types derived from Event. Each concrete event type
// carries a compile-time type key so EventBus can dispatch without RTTI or
// QObject/moc. Events are immutable once published.

#include <QString>

#include <cstddef>

namespace ardulab::core {

/// Opaque identity of an event type. Obtained via eventTypeOf<T>().
using EventTypeId = const void*;

/// Base class for all events. Not intended to be instantiated directly.
class Event
{
public:
    virtual ~Event() = default;

    /// Runtime type key — matches eventTypeOf<Derived>() for the concrete type.
    [[nodiscard]] virtual EventTypeId typeId() const noexcept = 0;

    /// Stable, human-readable event name for logs/diagnostics.
    [[nodiscard]] virtual QString name() const = 0;

protected:
    Event() = default;
    Event(const Event&) = default;
    Event& operator=(const Event&) = default;
};

namespace detail {
template <typename T>
struct EventTypeTag final
{
    static constexpr char kKey = 0;
};
} // namespace detail

/// Compile-time unique key per event type (address of a per-type static).
template <typename T>
constexpr EventTypeId eventTypeOf() noexcept
{
    return static_cast<EventTypeId>(&detail::EventTypeTag<T>::kKey);
}

/// Helper base: `struct MyEvent : TypedEvent<MyEvent> { ... }` implements typeId()/name().
template <typename Derived>
class TypedEvent : public Event
{
public:
    [[nodiscard]] EventTypeId typeId() const noexcept final { return eventTypeOf<Derived>(); }
    [[nodiscard]] QString name() const override { return QString::fromLatin1(Derived::kName); }
};

} // namespace ardulab::core

// Convenience: declare the stable event name inside a TypedEvent subclass.
#define ARDULAB_EVENT_NAME(literal) static constexpr const char* kName = literal
