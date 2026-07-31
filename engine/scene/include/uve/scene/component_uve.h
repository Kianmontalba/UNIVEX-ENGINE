//------------------------------------------------------------------------------
// UniVex Engine (UVE) — Proprietary Game Engine
// Copyright (c) 2026 UniVex Studios. All Rights Reserved.
// Unauthorized copying, modification, distribution, or use of this code
// in whole or in part is strictly prohibited without express written
// permission from UniVex Studios.
// Violators will be prosecuted to the fullest extent of the law.
//------------------------------------------------------------------------------

#pragma once

#include <type_traits>

namespace UVE::Scene {

/// ComponentUVE is a concept — not a base class — documenting "this type is usable as an ECS
/// component": a plain, non-polymorphic object type. Concrete components (TransformComponentUVE
/// and friends) satisfy it structurally; none derive from it. An earlier draft had every
/// component type derive from an empty ComponentUVE marker base, but that broke portable
/// placement-construction: an empty base class forces stricter brace-elision rules in
/// aggregate/list-initialization that GCC and Clang disagree on for a single, non-brace-enclosed
/// initializer (e.g. IEntityManagerUVE::AddComponentUVE<T>(entity, singleArg) copy-constructing
/// a component from an existing value) — confirmed by direct testing across both compilers this
/// project targets. A concept documents the same intent with zero object-layout or
/// initialization-syntax impact on the types satisfying it.
template <typename T>
concept ComponentUVE = std::is_object_v<T> && !std::is_pointer_v<T>;

} // namespace UVE::Scene
