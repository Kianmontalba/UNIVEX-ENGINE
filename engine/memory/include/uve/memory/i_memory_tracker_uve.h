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
#include <cstdint>
#include <string>
#include <string_view>

namespace UVE::Memory {

/// A single tracked allocation, as recorded by an IMemoryTrackerUVE. `sourceFile` points at a
/// string literal (always the __FILE__ expansion at the allocating call site) and is therefore
/// valid for the lifetime of the program without needing to be copied — the same convention
/// used by UVE::Debug::LogMessageUVE.
/// `allocatorTag` identifies which allocator/purpose made the allocation (e.g.
/// "HeapAllocatorUVE", or a caller-supplied name like "AnimationPool"). It is deliberately a
/// std::string for this increment; the field is named and positioned so a future MemoryTagUVE
/// enum (General/Renderer/Animation/Physics/ECS/Audio) could be introduced alongside or in
/// place of it later without reshaping this struct — no such enum exists yet.
/// `allocationId` is a unique, monotonically increasing identifier assigned by the recording
/// IMemoryTrackerUVE (never supplied by the caller) — it survives pointer reuse after a free,
/// so leak reports and future crash-report/profiler tooling can key off it instead of a raw
/// pointer.
/// Thread-safety: value type, safe to copy/move freely; holds no shared state.
struct AllocationRecordUVE {
    void* pointer = nullptr;
    std::size_t sizeBytes = 0;
    std::size_t alignment = 0;
    std::string allocatorTag;
    const char* sourceFile = nullptr;
    int sourceLine = 0;
    std::uint64_t allocationId = 0;
};

/// IMemoryTrackerUVE is the minimal interface every allocator (HeapAllocatorUVE,
/// PoolAllocatorUVE, StackAllocatorUVE) depends on to optionally report its activity —
/// deliberately smaller than IMemoryManagerUVE, since allocators don't need to know about
/// stats/leak-report methods, only how to record individual events.
/// Thread-safety: implementations must support both methods being called concurrently from any
/// thread, since allocators may run on worker threads.
class IMemoryTrackerUVE {
public:
    virtual ~IMemoryTrackerUVE() = default;

    /// Records a new allocation. `allocatorTag` identifies the allocator/purpose; `sourceFile`
    /// may be nullptr if the caller has no source-location information. Implementations assign
    /// and store the new record's allocationId internally — it is not supplied by the caller.
    virtual void RecordAllocationUVE(void* pointer, std::size_t sizeBytes, std::size_t alignment,
                                      std::string_view allocatorTag, const char* sourceFile,
                                      int sourceLine) = 0;

    /// Records that a previously-recorded allocation at `pointer` has been freed.
    virtual void RecordDeallocationUVE(void* pointer) = 0;
};

} // namespace UVE::Memory
