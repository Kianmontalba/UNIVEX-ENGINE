// Copyright (c) 2026 UniVex Studios. All Rights Reserved.


#pragma once

#include <cstddef>
#include <string>
#include <string_view>

#include "uve/memory/i_allocator_uve.h"
#include "uve/memory/i_memory_tracker_uve.h"

namespace UVE::Memory {

/// PoolAllocatorUVE is a fixed-size-block pool allocator: a fixed number of same-size,
/// same-alignment blocks carved from one contiguous buffer allocated at construction, handed
/// out and reclaimed in O(1) via an intrusive free-list threaded through the free blocks
/// themselves (no separate free-list storage). Fixed-capacity only — AllocateUVE() UVE_ASSERTs
/// if the pool is exhausted; it never grows. Ideal for many same-size, same-lifetime-shape
/// objects (e.g. one pool per ECS component type).
/// Thread-safety: not thread-safe. Wrap external synchronization around a shared instance if
/// multiple threads allocate from the same pool concurrently.
class PoolAllocatorUVE final : public IAllocatorUVE {
public:
    /// Allocates one contiguous buffer sized for `blockCount` blocks of `blockSizeBytes` bytes
    /// each (rounded up internally to at least `sizeof(void*)` so the free-list pointer always
    /// fits, and to a multiple of `blockAlignment` so every block stays aligned), freed in the
    /// destructor. `blockAlignment` must satisfy IsValidAlignmentUVE() (UVE_ASSERTs otherwise).
    /// `tracker`, if non-null, must outlive this PoolAllocatorUVE. Throws `std::bad_alloc` when
    /// alignment rounding or the total block footprint cannot be represented by `std::size_t`.
    PoolAllocatorUVE(std::size_t blockSizeBytes, std::size_t blockAlignment,
                      std::size_t blockCount, IMemoryTrackerUVE* tracker = nullptr,
                      std::string_view allocatorTag = "PoolAllocatorUVE");
    ~PoolAllocatorUVE() override;

    PoolAllocatorUVE(const PoolAllocatorUVE&) = delete;
    PoolAllocatorUVE& operator=(const PoolAllocatorUVE&) = delete;

    /// `sizeBytes` must be <= the pool's configured block size and `alignment` must be <= the
    /// pool's configured block alignment (UVE_ASSERTs otherwise, and throws
    /// `std::invalid_argument` in Release) — every returned block is exactly one pool block
    /// regardless of the exact requested size. UVE_ASSERTs if the pool is exhausted
    /// (fixed-capacity — never grows), and throws `std::bad_alloc` in Release, so AllocateUVE()
    /// never returns without a valid block.
    [[nodiscard]] void* AllocateUVE(std::size_t sizeBytes, std::size_t alignment,
                                     const char* sourceFile, int sourceLine) override;

    /// `pointer` must have been returned by AllocateUVE() on this same instance — UVE_ASSERTs
    /// on an out-of-range or mid-block (foreign) pointer.
    void DeallocateUVE(void* pointer) override;

    /// Physical footprint of every currently-handed-out block (GetUsedBlocksUVE() * the
    /// internal, alignment/pointer-rounded block stride — not the nominal `blockSizeBytes`).
    [[nodiscard]] std::size_t GetAllocatedBytesUVE() const override;

    /// Number of blocks currently handed out. Debugging/query only — O(1), backed by a single
    /// counter maintained alongside AllocateUVE()/DeallocateUVE(); adds no scanning overhead.
    [[nodiscard]] std::size_t GetUsedBlocksUVE() const noexcept;
    /// Number of blocks currently free: `GetCapacityBlocksUVE() - GetUsedBlocksUVE()`.
    [[nodiscard]] std::size_t GetFreeBlocksUVE() const noexcept;
    /// Total number of blocks this pool was constructed with.
    [[nodiscard]] std::size_t GetCapacityBlocksUVE() const noexcept;

private:
    [[nodiscard]] std::byte* BufferBytesUVE() const noexcept;
    [[nodiscard]] bool OwnsPointerUVE(const void* pointer) const noexcept;

    std::size_t m_blockSizeBytes;
    std::size_t m_blockAlignment;
    std::size_t m_blockStride;
    std::size_t m_blockCount;
    std::size_t m_usedBlockCount = 0;
    void* m_buffer;
    void* m_freeListHead;
    IMemoryTrackerUVE* m_tracker;
    std::string m_allocatorTag;
};

} // namespace UVE::Memory
