//------------------------------------------------------------------------------
// UniVex Engine (UVE) — Proprietary Game Engine
// Copyright (c) 2026 UniVex Studios. All Rights Reserved.
// Unauthorized copying, modification, distribution, or use of this code
// in whole or in part is strictly prohibited without express written
// permission from UniVex Studios.
// Violators will be prosecuted to the fullest extent of the law.
//------------------------------------------------------------------------------

#include "uve/memory/stack_allocator_uve.h"

#include <cstdint>
#include <cstring>
#include <new>

#include "uve/debug/assert_uve.h"
#include "uve/memory/alignment_utils_uve.h"

namespace UVE::Memory {

namespace {

constexpr std::uintptr_t AlignUpUVE(std::uintptr_t address, std::size_t alignment) {
    const std::uintptr_t mask = static_cast<std::uintptr_t>(alignment) - 1;
    return (address + mask) & ~mask;
}

// Trailing per-allocation header: written immediately after each block's payload, storing
// enough information to reconstruct the block's user pointer and rewind past it in one step.
// Read/written via memcpy (not a reinterpret_cast'd pointer), since its address (the end of a
// variably-aligned payload) has no guaranteed alignment of its own — same rationale as
// PoolAllocatorUVE's intrusive free-list pointer.
struct StackBlockHeaderUVE {
    std::size_t blockStartOffset; // offset (from the buffer start) this block's padding began at
    std::size_t sizeBytes;        // the payload's requested size
};

void WriteHeaderUVE(void* address, const StackBlockHeaderUVE& header) noexcept {
    std::memcpy(address, &header, sizeof(StackBlockHeaderUVE));
}

StackBlockHeaderUVE ReadHeaderUVE(void* address) noexcept {
    StackBlockHeaderUVE header{};
    std::memcpy(&header, address, sizeof(StackBlockHeaderUVE));
    return header;
}

} // namespace

StackAllocatorUVE::StackAllocatorUVE(std::size_t capacityBytes, IMemoryTrackerUVE* tracker,
                                      std::string_view allocatorTag)
    : m_capacityBytes(capacityBytes), m_buffer(nullptr), m_tracker(tracker), m_allocatorTag(allocatorTag) {
    UVE_ASSERT(capacityBytes > 0);
    m_buffer = ::operator new(capacityBytes, std::align_val_t{alignof(std::max_align_t)});
}

StackAllocatorUVE::~StackAllocatorUVE() {
    ::operator delete(m_buffer, std::align_val_t{alignof(std::max_align_t)});
}

void* StackAllocatorUVE::AllocateUVE(std::size_t sizeBytes, std::size_t alignment,
                                      const char* sourceFile, int sourceLine) {
    UVE_ASSERT(IsValidAlignmentUVE(alignment));

    const std::size_t blockStartOffset = m_currentOffset;
    const std::uintptr_t rawAddress =
        reinterpret_cast<std::uintptr_t>(BufferBytesUVE() + blockStartOffset);
    const std::uintptr_t alignedAddress = AlignUpUVE(rawAddress, alignment);
    const std::size_t userPointerOffset =
        static_cast<std::size_t>(alignedAddress - reinterpret_cast<std::uintptr_t>(m_buffer));

    const std::size_t headerOffset = userPointerOffset + sizeBytes;
    const std::size_t newOffset = headerOffset + sizeof(StackBlockHeaderUVE);
    UVE_ASSERT(newOffset <= m_capacityBytes);

    void* const userPointer = BufferBytesUVE() + userPointerOffset;
    WriteHeaderUVE(BufferBytesUVE() + headerOffset, StackBlockHeaderUVE{blockStartOffset, sizeBytes});
    m_currentOffset = newOffset;

    if (m_tracker != nullptr) {
        m_tracker->RecordAllocationUVE(userPointer, sizeBytes, alignment, m_allocatorTag, sourceFile,
                                        sourceLine);
    }
    return userPointer;
}

void StackAllocatorUVE::DeallocateUVE(void* pointer) {
    if (pointer == nullptr) {
        return;
    }
    UVE_ASSERT(m_currentOffset >= sizeof(StackBlockHeaderUVE));

    const std::size_t headerOffset = m_currentOffset - sizeof(StackBlockHeaderUVE);
    const StackBlockHeaderUVE header = ReadHeaderUVE(BufferBytesUVE() + headerOffset);
    const std::size_t userPointerOffset = headerOffset - header.sizeBytes;
    void* const topUserPointer = BufferBytesUVE() + userPointerOffset;

    UVE_ASSERT(pointer == topUserPointer);

    m_currentOffset = header.blockStartOffset;

    if (m_tracker != nullptr) {
        m_tracker->RecordDeallocationUVE(pointer);
    }
}

std::size_t StackAllocatorUVE::GetAllocatedBytesUVE() const {
    return m_currentOffset;
}

StackMarkerUVE StackAllocatorUVE::GetMarkerUVE() const noexcept {
    return StackMarkerUVE{m_currentOffset, this};
}

void StackAllocatorUVE::RewindToMarkerUVE(const StackMarkerUVE& marker) {
    UVE_ASSERT(marker.owner == this);
    UVE_ASSERT(marker.offset <= m_currentOffset);
    RewindToOffsetUVE(marker.offset);
}

void StackAllocatorUVE::Reset() {
    RewindToOffsetUVE(0);
}

void StackAllocatorUVE::RewindToOffsetUVE(std::size_t targetOffset) {
    while (m_currentOffset > targetOffset) {
        UVE_ASSERT(m_currentOffset >= sizeof(StackBlockHeaderUVE));
        const std::size_t headerOffset = m_currentOffset - sizeof(StackBlockHeaderUVE);
        const StackBlockHeaderUVE header = ReadHeaderUVE(BufferBytesUVE() + headerOffset);
        const std::size_t userPointerOffset = headerOffset - header.sizeBytes;
        void* const userPointer = BufferBytesUVE() + userPointerOffset;

        m_currentOffset = header.blockStartOffset;

        if (m_tracker != nullptr) {
            m_tracker->RecordDeallocationUVE(userPointer);
        }
    }
}

std::byte* StackAllocatorUVE::BufferBytesUVE() const noexcept {
    return static_cast<std::byte*>(m_buffer);
}

} // namespace UVE::Memory
