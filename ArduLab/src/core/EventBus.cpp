#include "core/EventBus.h"

#include <algorithm>

namespace ardulab::core {

// ---------------------------------------------------------------------------
// Subscription
// ---------------------------------------------------------------------------

Subscription::Subscription(std::weak_ptr<EventBus> bus, EventTypeId type, std::uint64_t handle) noexcept
    : m_bus(std::move(bus))
    , m_type(type)
    , m_handle(handle)
{
}

Subscription::~Subscription()
{
    reset();
}

Subscription::Subscription(Subscription&& other) noexcept
    : m_bus(std::move(other.m_bus))
    , m_type(other.m_type)
    , m_handle(other.m_handle)
{
    other.m_bus.reset();
    other.m_type = nullptr;
    other.m_handle = 0;
}

Subscription& Subscription::operator=(Subscription&& other) noexcept
{
    if (this != &other) {
        reset();
        m_bus = std::move(other.m_bus);
        m_type = other.m_type;
        m_handle = other.m_handle;
        other.m_bus.reset();
        other.m_type = nullptr;
        other.m_handle = 0;
    }
    return *this;
}

void Subscription::reset()
{
    if (auto bus = m_bus.lock(); bus && m_handle != 0) {
        bus->unsubscribe(m_type, m_handle);
    }
    m_bus.reset();
    m_type = nullptr;
    m_handle = 0;
}

// ---------------------------------------------------------------------------
// EventBus
// ---------------------------------------------------------------------------

std::shared_ptr<EventBus> EventBus::create()
{
    // Private constructor: cannot use make_shared directly.
    return std::shared_ptr<EventBus>(new EventBus());
}

Subscription EventBus::subscribeRaw(EventTypeId type, Handler handler)
{
    const std::uint64_t handle = m_nextHandle++;
    m_handlers[type].push_back(Entry{handle, std::move(handler)});
    return Subscription(weak_from_this(), type, handle);
}

void EventBus::unsubscribe(EventTypeId type, std::uint64_t handle) noexcept
{
    const auto it = m_handlers.find(type);
    if (it == m_handlers.end()) {
        return;
    }
    auto& entries = it->second;
    entries.erase(std::remove_if(entries.begin(), entries.end(),
                                 [handle](const Entry& e) { return e.handle == handle; }),
                  entries.end());
    if (entries.empty()) {
        m_handlers.erase(it);
    }
}

void EventBus::publish(const Event& event)
{
    const auto it = m_handlers.find(event.typeId());
    if (it == m_handlers.end()) {
        return;
    }
    // Copy so handlers may subscribe/unsubscribe during delivery.
    const std::vector<Entry> snapshot = it->second;
    for (const Entry& entry : snapshot) {
        entry.handler(event);
    }
}

std::size_t EventBus::subscriberCount(EventTypeId type) const noexcept
{
    const auto it = m_handlers.find(type);
    return it == m_handlers.end() ? 0 : it->second.size();
}

} // namespace ardulab::core
