// Copyright (c) 2026 UniVex Studios. All Rights Reserved.

#pragma once

#include "uve/render/primitive_geometry_uve.h"

namespace UVE::Render {

/// Returns deterministic unit-scale geometry for one translate-gizmo arrow: a cylindrical shaft
/// along local +Z topped by a cone head, base at the local origin, tip at local (0, 0, 1) - +Z,
/// not +Y, so the editor can orient it with UVE::Math::TryMakeLookAtUVE() (the engine's existing
/// direction-to-rotation builder, which points local +Z at its target) instead of a second,
/// hand-derived rotate-Y-to-direction formula. Flat
/// (per-triangle, unindexed) normals - the mesh is small and drawn a handful of times per frame,
/// so the simplicity of a normal that is always exactly cross(edge1, edge2) for its own triangle
/// (impossible to get backwards, unlike a shared/indexed smooth-normal scheme) is worth more here
/// than the vertex-count savings smooth shading would give. Deliberately separate from
/// GetPrimitiveGeometryUVE()/PrimitiveMeshKindUVE: this is editor-gizmo-only geometry, not a
/// buildable scene primitive kind, so it does not extend that ECS-facing enum. The returned cache
/// is immutable, process-local, and never serialized into a scene document.
[[nodiscard]] const PrimitiveGeometryUVE& GetGizmoArrowGeometryUVE() noexcept;

} // namespace UVE::Render
