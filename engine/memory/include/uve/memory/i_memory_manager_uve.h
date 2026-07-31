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
#include <vector>

#include "uve/memory/i_allocator_uve.h"
#include "uve/memory/i_memory_tracker_uve.h"

namespace UVE::Memory {

/// IMemoryManagerUVE is the engine's memory-tracking/leak-detection hub interface. Extends
/// IMemoryTrackerUVE (so every allocator can report to it through that minimal surface) and
/// adds the fuller stats/leak-report/default-allocator surface EngineServicesUVE exposes.
/// Thread-safety: every method must be safe to call concurrently from any thread, anticipating
/// future worker-thread allocation.
class IMemoryManagerUVE : public IMemoryTrackerUVE {
public:
    /// Returns this manager's own tracked, general-purpose default allocator (a
    /// HeapAllocatorUVE wired to report to this manager). Systems without a more specialized
    /// allocator should use this rather than constructing their own untracked HeapAllocatorUVE.
    [[nodiscard]] virtual IAllocatorUVE& GetDefaultAllocatorUVE() = 0;

    /// Number of allocations currently active (recorded but not yet deallocated).
    [[nodiscard]] virtual std::size_t GetActiveAllocationCountUVE() const = 0;
    /// Sum of `sizeBytes` across every currently-active allocation.
    [[nodiscard]] virtual std::size_t GetActiveBytesUVE() const = 0;
    /// The highest GetActiveBytesUVE() has ever reached (high-water mark).
    [[nodiscard]] virtual std::size_t GetPeakBytesUVE() const = 0;
    /// Total number of allocations ever recorded, active or not (monotonically increasing).
    [[nodiscard]] virtual std::uint64_t GetTotalAllocationsEverUVE() const = 0;

    /// True iff any allocation recorded via RecordAllocationUVE() has not been matched by a
    /// RecordDeallocationUVE() call.
    [[nodiscard]] virtual bool HasLeaksUVE() const = 0;

    /// Returns a snapshot copy of every currently-active (never-deallocated) allocation record,
    /// for tests/tooling to assert against directly rather than parsing log output.
    [[nodiscard]] virtual std::vector<AllocationRecordUVE> GetLeakedAllocationsUVE() const = 0;

    /// Logs one UVE_ERROR per leaked allocation (including its allocationId, size, allocator
    /// tag, and source location) plus a one-line summary, via the active logger. Does not
    /// abort — leak *detection* is this increment's job, not a forced crash. Typically called
    /// once, from EngineCoreUVE::Shutdown(), before this manager is torn down.
    virtual void LogLeakReportUVE() = 0;
};

} // namespace UVE::Memory
