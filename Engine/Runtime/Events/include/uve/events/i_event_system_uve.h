// Copyright (c) 2026 UniVex Studios. All Rights Reserved.


#pragma once

#include <cstdint>
#include <functional>
#include <memory>
#include <typeindex>
#include <utility>

#include "uve/events/event_priority_uve.h"
#include "uve/events/event_subscription_uve.h"

namespace UVE::Events {

/// IEventSystemUVE is the interface for the engine's type-safe publish/
/// subscribe event bus, keyed entirely by std::type_index (never by
/// string). Subscribe/Publish/QueueEvent are non-virtual templates so call
/// sites keep full compile-time type safety; they route internally through
/// type-erased virtual primitives (below), which is what lets a concrete
/// implementation be swapped out (for tests, or a future variant) without
/// changing any template call site.
/// Thread-safety: implementations must support Subscribe()/Unsubscribe()/
/// QueueEvent() being called concurrently from any thread; Publish() and
/// DispatchQueuedUVE() are documented per-implementation (see
/// EventSystemUVE) as dispatch-thread-only for this increment.
class IEventSystemUVE {
public:
    virtual ~IEventSystemUVE() = default;

    /// Subscribes `handler` to every future Publish<TEvent>() call and
    /// every future DispatchQueuedUVE() delivery of a QueueEvent<TEvent>().
    /// Returns a handle that can later be passed to Unsubscribe(). Safe to
    /// call from any thread.
    template <typename TEvent>
    EventSubscriptionUVE Subscribe(std::function<void(const TEvent&)> handler) {
        const std::type_index eventType(typeid(TEvent));
        const std::uint64_t id = SubscribeErased(
            eventType, [handler = std::move(handler)](const void* eventPtr) {
                handler(*static_cast<const TEvent*>(eventPtr));
            });
        return EventSubscriptionUVE{id, eventType};
    }

    /// Convenience overload: subscribes a member function, bound to
    /// `instance`, as the handler. Equivalent to Subscribe<TEvent>() with a
    /// lambda that calls `(instance->*method)(event)`.
    template <typename TEvent, typename TClass>
    EventSubscriptionUVE Subscribe(TClass* instance, void (TClass::*method)(const TEvent&)) {
        return Subscribe<TEvent>(
            [instance, method](const TEvent& event) { (instance->*method)(event); });
    }

    /// Unsubscribes a handler previously returned by Subscribe(). A
    /// default-constructed or already-unsubscribed handle is a safe no-op.
    /// Safe to call from any thread.
    virtual void Unsubscribe(const EventSubscriptionUVE& subscription) = 0;

    /// Synchronously and immediately invokes every current subscriber of
    /// TEvent with `event`, in subscription order. Must be called only from
    /// the thread the concrete implementation documents as its dispatch
    /// thread (see EventSystemUVE).
    template <typename TEvent>
    void Publish(const TEvent& event) {
        PublishErased(std::type_index(typeid(TEvent)), &event);
    }

    /// Queues `event` for delivery on a future DispatchQueuedUVE() call, at
    /// the given priority. The event is copied/moved into shared ownership
    /// so it stays alive until dispatched, regardless of the caller's
    /// lifetime. Safe to call from any thread.
    template <typename TEvent>
    void QueueEvent(TEvent event, EventPriorityUVE priority = EventPriorityUVE::Normal) {
        auto ownedEvent = std::make_shared<TEvent>(std::move(event));
        const std::type_index eventType(typeid(TEvent));
        QueueErased(eventType, priority,
                    [this, eventType, ownedEvent]() { PublishErased(eventType, ownedEvent.get()); });
    }

    /// Drains and dispatches every event queued via QueueEvent(): all
    /// Critical-priority events first (in FIFO publish order), then all
    /// High-priority, then all Normal-priority. Fixed and deterministic for
    /// this increment, not configurable. An event queued by a handler
    /// during this call is delivered on a future DispatchQueuedUVE() call,
    /// never within the same call that queued it. Must be called only from
    /// the dispatch thread.
    virtual void DispatchQueuedUVE() = 0;

    /// Removes every subscription and every queued (not yet dispatched)
    /// event.
    virtual void Clear() = 0;

protected:
    /// Type-erased subscribe primitive. The `handler` closure takes a
    /// `const void*` that is guaranteed (by the Subscribe<TEvent> template
    /// above, and by PublishErased only ever being invoked with the
    /// matching eventType) to actually point at a `const TEvent&`.
    virtual std::uint64_t SubscribeErased(std::type_index eventType,
                                           std::function<void(const void*)> handler) = 0;

    /// Type-erased publish primitive: invokes every handler currently
    /// registered for `eventType` with `eventPtr`.
    virtual void PublishErased(std::type_index eventType, const void* eventPtr) = 0;

    /// Type-erased queue primitive: stores `dispatchClosure` (which already
    /// captures the concrete event by shared ownership) under `priority`,
    /// to be invoked later by DispatchQueuedUVE(). `eventType` is passed
    /// through for implementations that want it for diagnostics/metrics; a
    /// conforming implementation is not required to use it for storage.
    virtual void QueueErased(std::type_index eventType, EventPriorityUVE priority,
                              std::function<void()> dispatchClosure) = 0;
};

} // namespace UVE::Events
