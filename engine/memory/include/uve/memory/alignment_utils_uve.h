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

namespace UVE::Memory {

/// Returns true iff `alignment` is a valid allocation alignment: non-zero and a power of two.
/// Every allocator's AllocateUVE() (HeapAllocatorUVE/PoolAllocatorUVE/StackAllocatorUVE) opens
/// with `UVE_ASSERT(IsValidAlignmentUVE(alignment));`; PoolAllocatorUVE's constructor applies
/// the same check to its fixed `blockAlignment` up front.
[[nodiscard]] constexpr bool IsValidAlignmentUVE(std::size_t alignment) noexcept {
    return alignment != 0 && (alignment & (alignment - 1)) == 0;
}

} // namespace UVE::Memory
