// Copyright (c) 2026 UniVex Studios. All Rights Reserved.


#include "uve/memory/memory_manager_uve.h"

#include "uve/debug/assert_uve.h"
#include "uve/debug/logging_macros_uve.h"

namespace UVE::Memory {

MemoryManagerUVE::MemoryManagerUVE() : m_defaultAllocator(this, "MemoryManagerUVE::DefaultAllocator") {}

void MemoryManagerUVE::RecordAllocationUVE(void* pointer, std::size_t sizeBytes,
                                            std::size_t alignment, std::string_view allocatorTag,
                                            const char* sourceFile, int sourceLine) {
    AllocationRecordUVE record;
    record.pointer = pointer;
    record.sizeBytes = sizeBytes;
    record.alignment = alignment;
    record.allocatorTag = std::string(allocatorTag);
    record.sourceFile = sourceFile;
    record.sourceLine = sourceLine;
    record.allocationId = m_nextAllocationId.fetch_add(1, std::memory_order_relaxed);

    const std::lock_guard<std::mutex> lock(m_mutex);
    m_activeAllocations.emplace(pointer, record);
    m_activeBytes += sizeBytes;
    ++m_totalAllocationsEver;
    if (m_activeBytes > m_peakBytes) {
        m_peakBytes = m_activeBytes;
    }
}

void MemoryManagerUVE::RecordDeallocationUVE(void* pointer) {
    const std::lock_guard<std::mutex> lock(m_mutex);
    const auto activeIt = m_activeAllocations.find(pointer);
    UVE_ASSERT(activeIt != m_activeAllocations.end());
    m_activeBytes -= activeIt->second.sizeBytes;
    m_activeAllocations.erase(activeIt);
}

IAllocatorUVE& MemoryManagerUVE::GetDefaultAllocatorUVE() {
    return m_defaultAllocator;
}

std::size_t MemoryManagerUVE::GetActiveAllocationCountUVE() const {
    const std::lock_guard<std::mutex> lock(m_mutex);
    return m_activeAllocations.size();
}

std::size_t MemoryManagerUVE::GetActiveBytesUVE() const {
    const std::lock_guard<std::mutex> lock(m_mutex);
    return m_activeBytes;
}

std::size_t MemoryManagerUVE::GetPeakBytesUVE() const {
    const std::lock_guard<std::mutex> lock(m_mutex);
    return m_peakBytes;
}

std::uint64_t MemoryManagerUVE::GetTotalAllocationsEverUVE() const {
    const std::lock_guard<std::mutex> lock(m_mutex);
    return m_totalAllocationsEver;
}

bool MemoryManagerUVE::HasLeaksUVE() const {
    const std::lock_guard<std::mutex> lock(m_mutex);
    return !m_activeAllocations.empty();
}

std::vector<AllocationRecordUVE> MemoryManagerUVE::GetLeakedAllocationsUVE() const {
    const std::lock_guard<std::mutex> lock(m_mutex);
    std::vector<AllocationRecordUVE> leaks;
    leaks.reserve(m_activeAllocations.size());
    for (const auto& activeEntry : m_activeAllocations) {
        leaks.push_back(activeEntry.second);
    }
    return leaks;
}

void MemoryManagerUVE::LogLeakReportUVE() {
    const std::vector<AllocationRecordUVE> leaks = GetLeakedAllocationsUVE();
    for (const AllocationRecordUVE& leak : leaks) {
        UVE_ERROR("Memory leak: allocation #{} ({} bytes, allocator=\"{}\", source={}:{})",
                  leak.allocationId, leak.sizeBytes, leak.allocatorTag,
                  leak.sourceFile != nullptr ? leak.sourceFile : "<unknown>", leak.sourceLine);
    }
    if (!leaks.empty()) {
        UVE_ERROR("Memory leak summary: {} leaked allocation(s), {} byte(s) total", leaks.size(),
                  GetActiveBytesUVE());
    }
}

} // namespace UVE::Memory
