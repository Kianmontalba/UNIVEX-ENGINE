// Copyright (c) 2026 UniVex Studios. All Rights Reserved.


#pragma once

#include <cstdint>
#include <typeindex>

namespace UVE::Events {

/// Opaque handle returned by IEventSystemUVE::Subscribe(), used to later
/// call Unsubscribe(). A default-constructed handle is always invalid.
/// Thread-safety: value type, safe to copy/move/compare freely.
struct EventSubscriptionUVE {
    std::uint64_t id = 0;
    std::type_index eventType = std::type_index(typeid(void));

    /// Returns false for a default-constructed or otherwise never-issued
    /// handle. Does not indicate whether the subscription is still active
    /// (it may have already been unsubscribed) — only whether it was ever
    /// a real handle.
    [[nodiscard]] bool IsValidUVE() const noexcept { return id != 0; }
};

} // namespace UVE::Events
