#pragma once

#include <algorithm>
#include <cstdint>
#include <functional>
#include <typeindex>
#include <unordered_map>
#include <utility>
#include <vector>

namespace event {

using SubscriptionId = std::uint64_t;

// Synchronous, type-safe publish/subscribe channel between components.
// Handlers run on the publisher's call, in subscription order.
class EventBus {
public:
    template <typename E>
    SubscriptionId subscribe(std::function<void(const E&)> handler) {
        const SubscriptionId id = next_id_++;
        handlers_[std::type_index(typeid(E))].push_back(
            Entry{id, [handler = std::move(handler)](const void* event) {
                      handler(*static_cast<const E*>(event));
                  }});
        return id;
    }

    void unsubscribe(SubscriptionId id) {
        for (auto& [type, entries] : handlers_) {
            std::erase_if(entries, [id](const Entry& entry) { return entry.id == id; });
        }
    }

    template <typename E>
    void publish(const E& event) {
        const auto it = handlers_.find(std::type_index(typeid(E)));
        if (it == handlers_.end()) return;

        // Iterate a copy so handlers may subscribe/unsubscribe while dispatching.
        const std::vector<Entry> snapshot = it->second;
        for (const Entry& entry : snapshot) entry.callback(&event);
    }

private:
    struct Entry {
        SubscriptionId id;
        std::function<void(const void*)> callback;
    };

    std::unordered_map<std::type_index, std::vector<Entry>> handlers_;
    SubscriptionId next_id_ = 1;
};

// Unsubscribes when destroyed, so a subscriber never outlives its own lifetime.
class ScopedSubscription {
public:
    ScopedSubscription() = default;
    ScopedSubscription(EventBus& bus, SubscriptionId id) : bus_(&bus), id_(id) {}
    ScopedSubscription(const ScopedSubscription&) = delete;
    ScopedSubscription& operator=(const ScopedSubscription&) = delete;
    ScopedSubscription(ScopedSubscription&& other) noexcept
        : bus_(std::exchange(other.bus_, nullptr)), id_(other.id_) {}
    ScopedSubscription& operator=(ScopedSubscription&& other) noexcept {
        if (this == &other) return *this;
        reset();
        bus_ = std::exchange(other.bus_, nullptr);
        id_ = other.id_;
        return *this;
    }
    ~ScopedSubscription() { reset(); }

    void reset() {
        if (!bus_) {
            return;
        }
        bus_->unsubscribe(id_);
        bus_ = nullptr;
    }

private:
    EventBus* bus_ = nullptr; // non-owning
    SubscriptionId id_ = 0;
};

template <typename E>
ScopedSubscription subscribe_scoped(EventBus& bus, std::function<void(const E&)> handler) {
    return ScopedSubscription(bus, bus.subscribe<E>(std::move(handler)));
}

} // namespace event
