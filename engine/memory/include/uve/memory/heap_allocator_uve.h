// Copyright (c) 2026 UniVex Studios. All Rights Reserved.


#pragma once

#include <cstddef>
#include <string>
#include <string_view>
#include <unordered_map>

#include "uve/memory/i_allocator_uve.h"
#include "uve/memory/i_memory_tracker_uve.h"

namespace UVE::Memory {

/// HeapAllocatorUVE is a tracked, general-purpose allocator wrapping the C++17 aligned
/// `::operator new`/`::operator delete` pair. It has no fixed capacity — this is the engine's
/// "default" allocator (see MemoryManagerUVE::GetDefaultAllocatorUVE()), not a hot-path
/// allocator; PoolAllocatorUVE/StackAllocatorUVE exist for hot-path use. Optionally reports
/// every allocation/deallocation to an IMemoryTrackerUVE (`nullptr` = untracked), so it stays
/// independently usable/testable without a MemoryManagerUVE.
/// Thread-safety: not thread-safe. `m_allocatedBytes` and the internal outstanding-allocation
/// bookkeeping are plain (non-atomic/non-mutex-guarded) state — wrap external synchronization
/// around a shared instance if multiple threads allocate through the same HeapAllocatorUVE
/// concurrently.
class HeapAllocatorUVE final : public IAllocatorUVE {
public:
    /// `tracker`, if non-null, must outlive this HeapAllocatorUVE. `allocatorTag` is the tag
    /// reported to `tracker` for every allocation made through this instance — defaults to
    /// "HeapAllocatorUVE"; pass a caller-specific name to distinguish multiple instances in
    /// leak reports (e.g. "AnimationPool" was the master-spec's own example tag).
    explicit HeapAllocatorUVE(IMemoryTrackerUVE* tracker = nullptr,
                               std::string_view allocatorTag = "HeapAllocatorUVE");

    [[nodiscard]] void* AllocateUVE(std::size_t sizeBytes, std::size_t alignment,
                                     const char* sourceFile, int sourceLine) override;
    /// `pointer` must have been returned by AllocateUVE() on this same instance and not yet
    /// freed — UVE_ASSERTs on a pointer this allocator never allocated (double-free or
    /// wrong-allocator bug), and logs an error and safely no-ops in Release rather than
    /// dereferencing bookkeeping this allocator never recorded.
    void DeallocateUVE(void* pointer) override;
    [[nodiscard]] std::size_t GetAllocatedBytesUVE() const override;

private:
    struct OutstandingAllocationUVE {
        std::size_t sizeBytes;
        std::size_t alignment;
    };

    IMemoryTrackerUVE* m_tracker;
    std::string m_allocatorTag;
    std::size_t m_allocatedBytes = 0;
    std::unordered_map<void*, OutstandingAllocationUVE> m_outstandingAllocations;
};

} // namespace UVE::Memory
