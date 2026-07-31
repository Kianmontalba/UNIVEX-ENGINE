//------------------------------------------------------------------------------
// UniVex Engine (UVE) — Proprietary Game Engine
// Copyright (c) 2026 UniVex Studios. All Rights Reserved.
// Unauthorized copying, modification, distribution, or use of this code
// in whole or in part is strictly prohibited without express written
// permission from UniVex Studios.
// Violators will be prosecuted to the fullest extent of the law.
//------------------------------------------------------------------------------

#pragma once

#include <cstddef>
#include <string>
#include <string_view>

#include "uve/memory/i_allocator_uve.h"
#include "uve/memory/i_memory_tracker_uve.h"

namespace UVE::Memory {

class StackAllocatorUVE;

/// Opaque marker into a StackAllocatorUVE's allocation history, returned by
/// StackAllocatorUVE::GetMarkerUVE() and consumed by StackAllocatorUVE::RewindToMarkerUVE() to
/// bulk-free everything allocated since the marker was taken — the idiomatic "free everything
/// this frame/scope" pattern for a stack allocator. Carries the allocator that produced it so
/// RewindToMarkerUVE() can UVE_ASSERT a marker is never used against a different
/// StackAllocatorUVE instance than the one that issued it (e.g. a marker from StackAllocatorA
/// passed into StackAllocatorB's RewindToMarkerUVE()).
/// Thread-safety: value type, safe to copy/move/compare freely.
struct StackMarkerUVE {
    std::size_t offset = 0;
    const StackAllocatorUVE* owner = nullptr;
};

/// StackAllocatorUVE is a fixed-capacity bump-pointer allocator. Each allocation writes a small
/// trailing header recording its own extent, so the allocator always knows each block's extent
/// without external bookkeeping. Two ways to free, both fully implemented — neither is a stub
/// of the other:
///  - DeallocateUVE(pointer) (the IAllocatorUVE-conformance path): strict LIFO — UVE_ASSERTs
///    that `pointer` is exactly the current top of the stack.
///  - GetMarkerUVE()/RewindToMarkerUVE(marker) (the idiomatic bulk scoped-free path): walks
///    every allocation made since the marker in reverse, reporting each one to the tracker
///    individually, then rewinds in one step.
/// Fixed-capacity only — AllocateUVE() UVE_ASSERTs if the stack is exhausted; it never grows.
/// Thread-safety: not thread-safe. Wrap external synchronization around a shared instance if
/// multiple threads allocate from the same stack concurrently.
class StackAllocatorUVE final : public IAllocatorUVE {
public:
    /// Allocates one `capacityBytes`-byte buffer up front (freed in the destructor).
    /// `tracker`, if non-null, must outlive this StackAllocatorUVE.
    explicit StackAllocatorUVE(std::size_t capacityBytes, IMemoryTrackerUVE* tracker = nullptr,
                                std::string_view allocatorTag = "StackAllocatorUVE");
    ~StackAllocatorUVE() override;

    StackAllocatorUVE(const StackAllocatorUVE&) = delete;
    StackAllocatorUVE& operator=(const StackAllocatorUVE&) = delete;

    /// UVE_ASSERTs if the stack does not have room for `sizeBytes` plus alignment padding plus
    /// the per-allocation header (fixed-capacity — never grows).
    [[nodiscard]] void* AllocateUVE(std::size_t sizeBytes, std::size_t alignment,
                                     const char* sourceFile, int sourceLine) override;

    /// Strict-LIFO single-object free: UVE_ASSERTs that `pointer` is exactly the current top of
    /// the stack (the most recently allocated, not-yet-freed block).
    void DeallocateUVE(void* pointer) override;

    /// Total bytes currently committed from the stack's buffer (payloads + alignment padding +
    /// per-allocation headers) — the physical footprint, not the sum of nominal request sizes.
    [[nodiscard]] std::size_t GetAllocatedBytesUVE() const override;

    /// Captures the stack's current position for a later RewindToMarkerUVE() call.
    [[nodiscard]] StackMarkerUVE GetMarkerUVE() const noexcept;

    /// Frees every allocation made since `marker` was captured, in one step, reporting each
    /// freed allocation to the tracker individually along the way. UVE_ASSERTs that
    /// `marker.owner` is this instance and that `marker.offset` does not exceed the stack's
    /// current position — both are usage errors, never silently ignored.
    void RewindToMarkerUVE(const StackMarkerUVE& marker);

    /// Frees every allocation currently on the stack — equivalent to rewinding to the marker
    /// this instance had immediately after construction.
    void Reset();

private:
    [[nodiscard]] std::byte* BufferBytesUVE() const noexcept;
    void RewindToOffsetUVE(std::size_t targetOffset);

    std::size_t m_capacityBytes;
    std::size_t m_currentOffset = 0;
    void* m_buffer;
    IMemoryTrackerUVE* m_tracker;
    std::string m_allocatorTag;
};

} // namespace UVE::Memory
