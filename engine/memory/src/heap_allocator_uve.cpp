//------------------------------------------------------------------------------
// UniVex Engine (UVE) — Proprietary Game Engine
// Copyright (c) 2026 UniVex Studios. All Rights Reserved.
// Unauthorized copying, modification, distribution, or use of this code
// in whole or in part is strictly prohibited without express written
// permission from UniVex Studios.
// Violators will be prosecuted to the fullest extent of the law.
//------------------------------------------------------------------------------

#include "uve/memory/heap_allocator_uve.h"

#include <new>

#include "uve/debug/assert_uve.h"
#include "uve/memory/alignment_utils_uve.h"

namespace UVE::Memory {

HeapAllocatorUVE::HeapAllocatorUVE(IMemoryTrackerUVE* tracker, std::string_view allocatorTag)
    : m_tracker(tracker), m_allocatorTag(allocatorTag) {}

void* HeapAllocatorUVE::AllocateUVE(std::size_t sizeBytes, std::size_t alignment,
                                     const char* sourceFile, int sourceLine) {
    UVE_ASSERT(IsValidAlignmentUVE(alignment));

    void* const pointer = ::operator new(sizeBytes, std::align_val_t{alignment});
    m_outstandingAllocations.emplace(pointer, OutstandingAllocationUVE{sizeBytes, alignment});
    m_allocatedBytes += sizeBytes;

    if (m_tracker != nullptr) {
        m_tracker->RecordAllocationUVE(pointer, sizeBytes, alignment, m_allocatorTag, sourceFile,
                                        sourceLine);
    }
    return pointer;
}

void HeapAllocatorUVE::DeallocateUVE(void* pointer) {
    if (pointer == nullptr) {
        return;
    }

    const auto outstandingIt = m_outstandingAllocations.find(pointer);
    UVE_ASSERT(outstandingIt != m_outstandingAllocations.end());

    const OutstandingAllocationUVE metadata = outstandingIt->second;
    m_outstandingAllocations.erase(outstandingIt);
    m_allocatedBytes -= metadata.sizeBytes;

    // Report the deallocation to the tracker before actually freeing the memory: passing
    // `pointer` to RecordDeallocationUVE() after ::operator delete() has run trips
    // -Wuse-after-free under optimization, even though only the address value (never the
    // pointee) is used past that point.
    if (m_tracker != nullptr) {
        m_tracker->RecordDeallocationUVE(pointer);
    }

    ::operator delete(pointer, std::align_val_t{metadata.alignment});
}

std::size_t HeapAllocatorUVE::GetAllocatedBytesUVE() const {
    return m_allocatedBytes;
}

} // namespace UVE::Memory
