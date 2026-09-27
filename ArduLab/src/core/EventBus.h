#pragma once

// EventBus — synchronous, in-process typed notification hub (Plan §4.2).
//
// Rules:
//   - Delivery is synchronous on the publishing thread. Threading changes
//     require a later architecture decision.
//   - Subscribers receive a const reference to the concrete event.
//   - Subscriptions are owned by the caller via a Subscription token; when the
//     token is destroyed (or reset) the handler is removed. This prevents
//     dangling callbacks from destroyed widgets or services.
//   - Publishing while a handler is running (re-entrancy) is safe: handler
//     lists are copied before iteration.

#include "core/Event.h"

#include <cstdint>
#include <functional>
#include <memory>
#include <unordered_map>
#include <vector>

namespace ardulab::core {

class EventBus;

/// RAII subscription token. Destroying it unsubscribes.
class Subscription final
{
public:
    Subscription() = default;
    ~Subscription();

    Subscription(Subscription&& other) noexcept;
    Subscription& operator=(Subscription&& other) noexcept;

    Subscription(const Subscription&) = delete;
    Subscription& operator=(const Subscription&) = delete;

    [[nodiscard]] bool isActive() const noexcept { return !m_bus.expired() && m_handle != 0; }

    /// Explicitly unsubscribe before destruction.
    void reset();

private:
    friend class EventBus;
    Subscription(std::weak_ptr<EventBus> bus, EventTypeId type, std::uint64_t handle) noexcept;

    std::weak_ptr<EventBus> m_bus;
    EventTypeId m_type = nullptr;
    std::uint64_t m_handle = 0;
};

class EventBus final : public std::enable_shared_from_this<EventBus>
{
public:
    using Handler = std::function<void(const Event&)>;

    /// EventBus must be shared so Subscription tokens can outlive it safely.
    static std::shared_ptr<EventBus> create();

    ~EventBus() = default;
    EventBus(const EventBus&) = delete;
    EventBus& operator=(const EventBus&) = delete;

    /// Subscribe to a concrete event type.
    template <typename TEvent, typename Callable>
    [[nodiscard]] Subscription subscribe(Callable&& callable)
    {
        static_assert(std::is_base_of_v<Event, TEvent>, "TEvent must derive from ardulab::core::Event");
        return subscribeRaw(eventTypeOf<TEvent>(), [fn = std::forward<Callable>(callable)](const Event& e) {
            fn(static_cast<const TEvent&>(e));
        });
    }

    /// Publish synchronously to every current subscriber of the concrete type.
    void publish(const Event& event);

    template <typename TEvent, typename... Args>
    void publishEvent(Args&&... args)
    {
        static_assert(std::is_base_of_v<Event, TEvent>, "TEvent must derive from ardulab::core::Event");
        const TEvent event{std::forward<Args>(args)...};
        publish(event);
    }

    [[nodiscard]] std::size_t subscriberCount(EventTypeId type) const noexcept;

    template <typename TEvent>
    [[nodiscard]] std::size_t subscriberCount() const noexcept
    {
        return subscriberCount(eventTypeOf<TEvent>());
    }

private:
    friend class Subscription;

    EventBus() = default;

    Subscription subscribeRaw(EventTypeId type, Handler handler);
    void unsubscribe(EventTypeId type, std::uint64_t handle) noexcept;

    struct Entry
    {
        std::uint64_t handle;
        Handler handler;
    };

    std::unordered_map<EventTypeId, std::vector<Entry>> m_handlers;
    std::uint64_t m_nextHandle = 1;
};

} // namespace ardulab::core
