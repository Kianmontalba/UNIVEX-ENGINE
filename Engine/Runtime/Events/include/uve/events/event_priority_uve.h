// Copyright (c) 2026 UniVex Studios. All Rights Reserved.


#pragma once

#include <cstdint>

namespace UVE::Events {

/// Priority of a queued event, controlling delivery order within
/// IEventSystemUVE::DispatchQueuedUVE(). Higher-priority events are fully
/// delivered (in their own FIFO publish order) before any lower-priority
/// event is delivered in the same dispatch call. Not used by immediate
/// Publish() calls, which are always synchronous regardless of priority.
enum class EventPriorityUVE : std::uint8_t {
    Normal = 0,
    High = 1,
    Critical = 2,
};

} // namespace UVE::Events
