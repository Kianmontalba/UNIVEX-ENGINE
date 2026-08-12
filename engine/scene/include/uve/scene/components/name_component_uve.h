// UniVex Engine (UVE) — Proprietary Game Engine
// Copyright (c) 2026 UniVex Studios. All Rights Reserved.
// Unauthorized copying, modification, distribution, or use of this code
// in whole or in part is strictly prohibited without express written
// permission from UniVex Studios.
// Violators will be prosecuted to the fullest extent of the law.

#pragma once

#include <string>

namespace UVE::Scene {

/// Persistent, human-readable authored metadata for a scene entity. Names are intentionally not
/// required to be globally unique: the editor gives newly created entities deterministic defaults,
/// while callers remain free to assign duplicate names when that better describes a scene. The
/// component is optional so legacy documents and runtime-created entities remain valid without it.
/// Thread-safety: value type; safe to copy and move freely with no shared state.
struct NameComponentUVE final {
    std::string name;
};

} // namespace UVE::Scene
