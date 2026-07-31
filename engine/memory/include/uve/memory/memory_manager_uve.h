//------------------------------------------------------------------------------
// UniVex Engine (UVE) — Proprietary Game Engine
// Copyright (c) 2026 UniVex Studios. All Rights Reserved.
// Unauthorized copying, modification, distribution, or use of this code
// in whole or in part is strictly prohibited without express written
// permission from UniVex Studios.
// Violators will be prosecuted to the fullest extent of the law.
//------------------------------------------------------------------------------

#pragma once

#include <atomic>
#include <cstddef>
#include <cstdint>
#include <mutex>
#include <string_view>
#include <unordered_map>
#include <vector>

#include "uve/memory/heap_allocator_uve.h"
#include "uve/memory/i_memory_manager_uve.h"

namespace UVE::Memory {

/// MemoryManagerUVE is the concrete, engine-standard implementation of IMemoryManagerUVE. Owns
/// its own tracked HeapAllocatorUVE (returned by GetDefaultAllocatorUVE()) and maintains a
/// mutex-guarded map of every currently-active allocation reported to it by any allocator
/// (including its own default one), plus running stats (active bytes/count, peak bytes,
/// total-allocations-ever). Every accepted allocation is assigned a unique, monotonically
/// increasing allocationId via an atomic counter at the moment RecordAllocationUVE() accepts
/// it.
/// Thread-safety: thread-safe. RecordAllocationUVE()/RecordDeallocationUVE() and every stats
/// accessor are guarded by an internal mutex (the id counter uses an atomic instead, so it can
/// be incremented without taking the lock) — safe to call concurrently from any thread.
class MemoryManagerUVE final : public IMemoryManagerUVE {
public:
    MemoryManagerUVE();

    void RecordAllocationUVE(void* pointer, std::size_t sizeBytes, std::size_t alignment,
                              std::string_view allocatorTag, const char* sourceFile,
                              int sourceLine) override;
    void RecordDeallocationUVE(void* pointer) override;

    [[nodiscard]] IAllocatorUVE& GetDefaultAllocatorUVE() override;
    [[nodiscard]] std::size_t GetActiveAllocationCountUVE() const override;
    [[nodiscard]] std::size_t GetActiveBytesUVE() const override;
    [[nodiscard]] std::size_t GetPeakBytesUVE() const override;
    [[nodiscard]] std::uint64_t GetTotalAllocationsEverUVE() const override;
    [[nodiscard]] bool HasLeaksUVE() const override;
    [[nodiscard]] std::vector<AllocationRecordUVE> GetLeakedAllocationsUVE() const override;
    void LogLeakReportUVE() override;

private:
    mutable std::mutex m_mutex;
    std::unordered_map<void*, AllocationRecordUVE> m_activeAllocations;
    std::atomic<std::uint64_t> m_nextAllocationId{1};
    std::size_t m_activeBytes = 0;
    std::size_t m_peakBytes = 0;
    std::uint64_t m_totalAllocationsEver = 0;
    HeapAllocatorUVE m_defaultAllocator;
};

} // namespace UVE::Memory
