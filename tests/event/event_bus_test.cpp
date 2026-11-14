#include <gtest/gtest.h>

#include <string>
#include <vector>

#include "event/event_bus.hpp"

namespace {

struct ItemProduced {
    int amount;
};

struct PowerLost {};

} // namespace

TEST(EventBus, SubscriberReceivesPublishedEvent) {
    // Given
    event::EventBus bus;
    int received = 0;
    bus.subscribe<ItemProduced>([&](const ItemProduced& e) { received += e.amount; });

    // When
    bus.publish(ItemProduced{5});

    // Then
    EXPECT_EQ(received, 5);
}

TEST(EventBus, SubscriberIgnoresOtherEventTypes) {
    // Given
    event::EventBus bus;
    int calls = 0;
    bus.subscribe<ItemProduced>([&](const ItemProduced&) { ++calls; });

    // When
    bus.publish(PowerLost{});

    // Then
    EXPECT_EQ(calls, 0);
}

TEST(EventBus, SubscribersAreCalledInSubscriptionOrder) {
    // Given
    event::EventBus bus;
    std::string order;
    bus.subscribe<PowerLost>([&](const PowerLost&) { order += 'a'; });
    bus.subscribe<PowerLost>([&](const PowerLost&) { order += 'b'; });

    // When
    bus.publish(PowerLost{});

    // Then
    EXPECT_EQ(order, "ab");
}

TEST(EventBus, UnsubscribedHandlerIsNoLongerCalled) {
    // Given
    event::EventBus bus;
    int calls = 0;
    auto id = bus.subscribe<PowerLost>([&](const PowerLost&) { ++calls; });
    bus.publish(PowerLost{});

    // When
    bus.unsubscribe(id);
    bus.publish(PowerLost{});

    // Then
    EXPECT_EQ(calls, 1);
}

TEST(EventBus, HandlerSubscribedDuringPublishRunsOnlyFromNextEvent) {
    // Given
    event::EventBus bus;
    int late_calls = 0;
    bool subscribed = false;
    bus.subscribe<PowerLost>([&](const PowerLost&) {
        if (subscribed) return;
        subscribed = true;
        bus.subscribe<PowerLost>([&](const PowerLost&) { ++late_calls; });
    });

    // When
    bus.publish(PowerLost{});
    int after_first = late_calls;
    bus.publish(PowerLost{});

    // Then
    EXPECT_EQ(after_first, 0);
    EXPECT_EQ(late_calls, 1);
}

TEST(EventBus, ScopedSubscriptionStopsReceivingWhenDestroyed) {
    // Given
    struct Ping {};
    event::EventBus bus;
    int received = 0;
    {
        auto subscription = event::subscribe_scoped<Ping>(bus, [&](const Ping&) { ++received; });

        // When
        bus.publish(Ping{});
    }
    bus.publish(Ping{});

    // Then
    EXPECT_EQ(received, 1);
}
