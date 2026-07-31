//------------------------------------------------------------------------------
// UniVex Engine (UVE) — Proprietary Game Engine
// Copyright (c) 2026 UniVex Studios. All Rights Reserved.
// Unauthorized copying, modification, distribution, or use of this code
// in whole or in part is strictly prohibited without express written
// permission from UniVex Studios.
// Violators will be prosecuted to the fullest extent of the law.
//------------------------------------------------------------------------------

#include "uve/events/event_system_uve.h"

#include <string>
#include <vector>

#include <gtest/gtest.h>

namespace UVE::Events::Tests {
namespace {

struct TestEventUVE {
    int value = 0;
};

struct OtherTestEventUVE {
    std::string text;
};

class HandlerHostUVE {
public:
    void OnTestEvent(const TestEventUVE& event) { receivedValue = event.value; }
    int receivedValue = -1;
};

TEST(EventSystemUVETest, Publish_InvokesSubscribedHandler) {
    EventSystemUVE eventSystem;
    int received = -1;
    eventSystem.Subscribe<TestEventUVE>([&received](const TestEventUVE& event) { received = event.value; });

    eventSystem.Publish(TestEventUVE{42});

    EXPECT_EQ(received, 42);
}

TEST(EventSystemUVETest, Publish_MultipleSubscribers_AllInvoked) {
    EventSystemUVE eventSystem;
    int callCount = 0;
    for (int i = 0; i < 3; ++i) {
        eventSystem.Subscribe<TestEventUVE>([&callCount](const TestEventUVE&) { ++callCount; });
    }

    eventSystem.Publish(TestEventUVE{1});

    EXPECT_EQ(callCount, 3);
}

TEST(EventSystemUVETest, Publish_DifferentEventTypes_DoNotCrossFire) {
    EventSystemUVE eventSystem;
    bool testEventFired = false;
    bool otherEventFired = false;
    eventSystem.Subscribe<TestEventUVE>([&testEventFired](const TestEventUVE&) { testEventFired = true; });
    eventSystem.Subscribe<OtherTestEventUVE>(
        [&otherEventFired](const OtherTestEventUVE&) { otherEventFired = true; });

    eventSystem.Publish(TestEventUVE{1});

    EXPECT_TRUE(testEventFired);
    EXPECT_FALSE(otherEventFired);
}

TEST(EventSystemUVETest, Unsubscribe_StopsFutureDelivery) {
    EventSystemUVE eventSystem;
    int callCount = 0;
    const EventSubscriptionUVE subscription =
        eventSystem.Subscribe<TestEventUVE>([&callCount](const TestEventUVE&) { ++callCount; });

    eventSystem.Unsubscribe(subscription);
    eventSystem.Publish(TestEventUVE{1});

    EXPECT_EQ(callCount, 0);
}

TEST(EventSystemUVETest, Unsubscribe_InvalidHandle_IsNoOp) {
    EventSystemUVE eventSystem;
    int callCount = 0;
    eventSystem.Subscribe<TestEventUVE>([&callCount](const TestEventUVE&) { ++callCount; });

    eventSystem.Unsubscribe(EventSubscriptionUVE{});
    eventSystem.Publish(TestEventUVE{1});

    EXPECT_EQ(callCount, 1);
}

TEST(EventSystemUVETest, MemberFunctionSubscribe_Invoked) {
    EventSystemUVE eventSystem;
    HandlerHostUVE host;
    eventSystem.Subscribe<TestEventUVE>(&host, &HandlerHostUVE::OnTestEvent);

    eventSystem.Publish(TestEventUVE{7});

    EXPECT_EQ(host.receivedValue, 7);
}

TEST(EventSystemUVETest, QueueEvent_NotDeliveredUntilDispatchQueued) {
    EventSystemUVE eventSystem;
    int callCount = 0;
    eventSystem.Subscribe<TestEventUVE>([&callCount](const TestEventUVE&) { ++callCount; });

    eventSystem.QueueEvent(TestEventUVE{1});
    EXPECT_EQ(callCount, 0);

    eventSystem.DispatchQueuedUVE();
    EXPECT_EQ(callCount, 1);
}

TEST(EventSystemUVETest, DispatchQueued_PreservesSamePriorityOrder) {
    EventSystemUVE eventSystem;
    std::vector<int> order;
    eventSystem.Subscribe<TestEventUVE>([&order](const TestEventUVE& event) { order.push_back(event.value); });

    eventSystem.QueueEvent(TestEventUVE{1});
    eventSystem.QueueEvent(TestEventUVE{2});
    eventSystem.QueueEvent(TestEventUVE{3});
    eventSystem.DispatchQueuedUVE();

    EXPECT_EQ(order, (std::vector<int>{1, 2, 3}));
}

TEST(EventSystemUVETest, DispatchQueued_CriticalBeforeHighBeforeNormal) {
    EventSystemUVE eventSystem;
    std::vector<int> order;
    eventSystem.Subscribe<TestEventUVE>([&order](const TestEventUVE& event) { order.push_back(event.value); });

    eventSystem.QueueEvent(TestEventUVE{1}, EventPriorityUVE::Normal);
    eventSystem.QueueEvent(TestEventUVE{2}, EventPriorityUVE::Critical);
    eventSystem.QueueEvent(TestEventUVE{3}, EventPriorityUVE::High);
    eventSystem.QueueEvent(TestEventUVE{4}, EventPriorityUVE::Critical);
    eventSystem.DispatchQueuedUVE();

    EXPECT_EQ(order, (std::vector<int>{2, 4, 3, 1}));
}

TEST(EventSystemUVETest, DispatchQueued_ClearsQueueAfterDispatch) {
    EventSystemUVE eventSystem;
    int callCount = 0;
    eventSystem.Subscribe<TestEventUVE>([&callCount](const TestEventUVE&) { ++callCount; });

    eventSystem.QueueEvent(TestEventUVE{1});
    eventSystem.DispatchQueuedUVE();
    eventSystem.DispatchQueuedUVE();

    EXPECT_EQ(callCount, 1);
}

TEST(EventSystemUVETest, Clear_RemovesSubscriptionsAndQueue) {
    EventSystemUVE eventSystem;
    int callCount = 0;
    eventSystem.Subscribe<TestEventUVE>([&callCount](const TestEventUVE&) { ++callCount; });
    eventSystem.QueueEvent(TestEventUVE{1});

    eventSystem.Clear();
    eventSystem.DispatchQueuedUVE();
    eventSystem.Publish(TestEventUVE{1});

    EXPECT_EQ(callCount, 0);
}

} // namespace
} // namespace UVE::Events::Tests
