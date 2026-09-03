// Copyright (c) 2026 UniVex Studios. All Rights Reserved.


#include "uve/memory/pool_allocator_uve.h"

#include <algorithm>
#include <cstring>
#include <limits>
#include <new>
#include <stdexcept>

#include "uve/debug/assert_uve.h"
#include "uve/memory/alignment_utils_uve.h"

namespace UVE::Memory {

namespace {

constexpr std::size_t RoundUpUVE(std::size_t value, std::size_t alignment) {
    if (alignment == 0U || value > std::numeric_limits<std::size_t>::max() - (alignment - 1U)) {
        throw std::bad_alloc{};
    }
    return ((value + alignment - 1U) / alignment) * alignment;
}

// The pool's free list is intrusive: a free block's own bytes store the address of the next
// free block. Reading/writing through std::memcpy (rather than reinterpret_cast'ing the byte
// pointer to void**) avoids both a -Wcast-align violation (std::byte* has alignof 1; void**
// does not) and the strict-aliasing UB of treating raw storage as a different type in place.
void WriteNextPointerUVE(void* block, void* nextPointer) noexcept {
    std::memcpy(block, &nextPointer, sizeof(void*));
}

void* ReadNextPointerUVE(void* block) noexcept {
    void* nextPointer = nullptr;
    std::memcpy(&nextPointer, block, sizeof(void*));
    return nextPointer;
}

} // namespace

PoolAllocatorUVE::PoolAllocatorUVE(std::size_t blockSizeBytes, std::size_t blockAlignment,
                                    std::size_t blockCount, IMemoryTrackerUVE* tracker,
                                    std::string_view allocatorTag)
    : m_blockSizeBytes(blockSizeBytes),
      m_blockAlignment(blockAlignment),
      m_blockStride(RoundUpUVE(std::max(blockSizeBytes, sizeof(void*)), blockAlignment)),
      m_blockCount(blockCount),
      m_buffer(nullptr),
      m_freeListHead(nullptr),
      m_tracker(tracker),
      m_allocatorTag(allocatorTag) {
    UVE_ASSERT(IsValidAlignmentUVE(blockAlignment));
    UVE_ASSERT(blockSizeBytes > 0);
    UVE_ASSERT(blockCount > 0);
    if (m_blockCount == 0U || m_blockStride > std::numeric_limits<std::size_t>::max() / m_blockCount) {
        throw std::bad_alloc{};
    }

    m_buffer = ::operator new(m_blockStride * m_blockCount, std::align_val_t{blockAlignment});

    // Thread the intrusive free list through every block, in ascending address order.
    for (std::size_t blockIndex = 0; blockIndex < m_blockCount; ++blockIndex) {
        void* const block = BufferBytesUVE() + (blockIndex * m_blockStride);
        void* const nextBlock =
            (blockIndex + 1 < m_blockCount) ? (BufferBytesUVE() + ((blockIndex + 1) * m_blockStride))
                                             : nullptr;
        WriteNextPointerUVE(block, nextBlock);
    }
    m_freeListHead = m_buffer;
}

PoolAllocatorUVE::~PoolAllocatorUVE() {
    ::operator delete(m_buffer, std::align_val_t{m_blockAlignment});
}

void* PoolAllocatorUVE::AllocateUVE(std::size_t sizeBytes, std::size_t alignment,
                                     const char* sourceFile, int sourceLine) {
    UVE_ASSERT(IsValidAlignmentUVE(alignment));
    UVE_ASSERT(sizeBytes <= m_blockSizeBytes);
    UVE_ASSERT(alignment <= m_blockAlignment);
    UVE_ASSERT(m_freeListHead != nullptr);
    if (sizeBytes > m_blockSizeBytes || alignment > m_blockAlignment) {
        UVE_ERROR("PoolAllocatorUVE: request (size={}, align={}) exceeds this pool's block contract (size={}, align={})",
                   sizeBytes, alignment, m_blockSizeBytes, m_blockAlignment);
        throw std::invalid_argument("PoolAllocatorUVE::AllocateUVE: request exceeds block contract");
    }
    if (m_freeListHead == nullptr) {
        UVE_ERROR("PoolAllocatorUVE: pool exhausted ({} of {} blocks in use)", m_usedBlockCount, m_blockCount);
        throw std::bad_alloc{};
    }

    void* const block = m_freeListHead;
    m_freeListHead = ReadNextPointerUVE(block);
    ++m_usedBlockCount;

    if (m_tracker != nullptr) {
        m_tracker->RecordAllocationUVE(block, sizeBytes, alignment, m_allocatorTag, sourceFile,
                                        sourceLine);
    }
    return block;
}

void PoolAllocatorUVE::DeallocateUVE(void* pointer) {
    if (pointer == nullptr) {
        return;
    }
    UVE_ASSERT(OwnsPointerUVE(pointer));

    WriteNextPointerUVE(pointer, m_freeListHead);
    m_freeListHead = pointer;
    --m_usedBlockCount;

    if (m_tracker != nullptr) {
        m_tracker->RecordDeallocationUVE(pointer);
    }
}

std::size_t PoolAllocatorUVE::GetAllocatedBytesUVE() const {
    return m_usedBlockCount * m_blockStride;
}

std::size_t PoolAllocatorUVE::GetUsedBlocksUVE() const noexcept {
    return m_usedBlockCount;
}

std::size_t PoolAllocatorUVE::GetFreeBlocksUVE() const noexcept {
    return m_blockCount - m_usedBlockCount;
}

std::size_t PoolAllocatorUVE::GetCapacityBlocksUVE() const noexcept {
    return m_blockCount;
}

std::byte* PoolAllocatorUVE::BufferBytesUVE() const noexcept {
    return static_cast<std::byte*>(m_buffer);
}

bool PoolAllocatorUVE::OwnsPointerUVE(const void* pointer) const noexcept {
    const auto* const bufferStart = static_cast<const std::byte*>(m_buffer);
    const auto* const candidate = static_cast<const std::byte*>(pointer);
    if (candidate < bufferStart) {
        return false;
    }
    const std::size_t offset = static_cast<std::size_t>(candidate - bufferStart);
    const std::size_t totalSize = m_blockStride * m_blockCount;
    if (offset >= totalSize) {
        return false;
    }
    return (offset % m_blockStride) == 0;
}

} // namespace UVE::Memory
