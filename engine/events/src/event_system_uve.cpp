//------------------------------------------------------------------------------
// UniVex Engine (UVE) — Proprietary Game Engine
// Copyright (c) 2026 UniVex Studios. All Rights Reserved.
// Unauthorized copying, modification, distribution, or use of this code
// in whole or in part is strictly prohibited without express written
// permission from UniVex Studios.
// Violators will be prosecuted to the fullest extent of the law.
//------------------------------------------------------------------------------

#include "uve/events/event_system_uve.h"

#include <algorithm>
#include <utility>

namespace UVE::Events {

namespace {
constexpr std::size_t PriorityIndexUVE(EventPriorityUVE priority) {
    return static_cast<std::size_t>(priority);
}
} // namespace

std::uint64_t EventSystemUVE::SubscribeErased(std::type_index eventType,
                                                std::function<void(const void*)> handler) {
    const std::lock_guard<std::mutex> lock(m_mutex);
    const std::uint64_t id = m_nextId++;
    m_handlers[eventType].push_back(HandlerEntryUVE{id, std::move(handler)});
    return id;
}

void EventSystemUVE::Unsubscribe(const EventSubscriptionUVE& subscription) {
    if (!subscription.IsValidUVE()) {
        return;
    }
    const std::lock_guard<std::mutex> lock(m_mutex);
    const auto handlerListIt = m_handlers.find(subscription.eventType);
    if (handlerListIt == m_handlers.end()) {
        return;
    }
    std::vector<HandlerEntryUVE>& entries = handlerListIt->second;
    entries.erase(std::remove_if(entries.begin(), entries.end(),
                                  [&subscription](const HandlerEntryUVE& entry) {
                                      return entry.id == subscription.id;
                                  }),
                  entries.end());
}

void EventSystemUVE::PublishErased(std::type_index eventType, const void* eventPtr) {
    // Snapshot the handler list under lock, then invoke outside the lock.
    // This makes Unsubscribe-during-dispatch safe: a handler unsubscribing
    // itself or another handler mid-Publish never corrupts internal state.
    // Documented trade-off: a handler that unsubscribes after the snapshot
    // was already taken still fires once more in this same Publish call.
    std::vector<HandlerEntryUVE> snapshot;
    {
        const std::lock_guard<std::mutex> lock(m_mutex);
        const auto handlerListIt = m_handlers.find(eventType);
        if (handlerListIt == m_handlers.end()) {
            return;
        }
        snapshot = handlerListIt->second;
    }
    for (const HandlerEntryUVE& entry : snapshot) {
        entry.callback(eventPtr);
    }
}

void EventSystemUVE::QueueErased(std::type_index /*eventType*/, EventPriorityUVE priority,
                                  std::function<void()> dispatchClosure) {
    const std::lock_guard<std::mutex> lock(m_mutex);
    m_queuesByPriority[PriorityIndexUVE(priority)].push_back(std::move(dispatchClosure));
}

void EventSystemUVE::DispatchQueuedUVE() {
    static constexpr EventPriorityUVE kDispatchOrder[] = {
        EventPriorityUVE::Critical, EventPriorityUVE::High, EventPriorityUVE::Normal};

    for (const EventPriorityUVE priority : kDispatchOrder) {
        std::deque<std::function<void()>> drained;
        {
            const std::lock_guard<std::mutex> lock(m_mutex);
            drained.swap(m_queuesByPriority[PriorityIndexUVE(priority)]);
        }
        for (const std::function<void()>& dispatchClosure : drained) {
            dispatchClosure();
        }
    }
}

void EventSystemUVE::Clear() {
    const std::lock_guard<std::mutex> lock(m_mutex);
    m_handlers.clear();
    for (std::deque<std::function<void()>>& queue : m_queuesByPriority) {
        queue.clear();
    }
}

} // namespace UVE::Events
