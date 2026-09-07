// Copyright (c) 2026 UniVex Studios. All Rights Reserved.


#pragma once

#include <array>
#include <cstdint>
#include <deque>
#include <mutex>
#include <typeindex>
#include <unordered_map>
#include <vector>

#include "uve/events/i_event_system_uve.h"

namespace UVE::Events {

/// EventSystemUVE is the concrete, engine-standard implementation of
/// IEventSystemUVE.
/// Thread-safety: Subscribe()/Unsubscribe()/QueueEvent() are thread-safe
/// (guarded by an internal mutex) and may be called from any thread.
/// Publish() and DispatchQueuedUVE() are NOT safe to call concurrently with
/// each other — they are intended to run only on the engine's single
/// dispatch thread (EngineCoreUVE's main loop), which is the conventional
/// threading model for an engine event bus at this stage. Handler
/// invocation always happens outside the internal mutex (subscriber lists
/// and queued-event lists are snapshotted/drained under lock, then invoked
/// unlocked), so a handler is free to Subscribe/Unsubscribe/QueueEvent
/// without deadlocking.
class EventSystemUVE final : public IEventSystemUVE {
public:
    void Unsubscribe(const EventSubscriptionUVE& subscription) override;
    void DispatchQueuedUVE() override;
    void Clear() override;

protected:
    std::uint64_t SubscribeErased(std::type_index eventType,
                                   std::function<void(const void*)> handler) override;
    void PublishErased(std::type_index eventType, const void* eventPtr) override;
    void QueueErased(std::type_index eventType, EventPriorityUVE priority,
                      std::function<void()> dispatchClosure) override;

private:
    struct HandlerEntryUVE {
        std::uint64_t id = 0;
        std::function<void(const void*)> callback;
    };

    static constexpr std::size_t kPriorityCount = 3;

    mutable std::mutex m_mutex;
    std::unordered_map<std::type_index, std::vector<HandlerEntryUVE>> m_handlers;
    std::array<std::deque<std::function<void()>>, kPriorityCount> m_queuesByPriority;
    std::uint64_t m_nextId = 1;
};

} // namespace UVE::Events
