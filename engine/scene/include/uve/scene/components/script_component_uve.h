//------------------------------------------------------------------------------
// UniVex Engine (UVE) — Proprietary Game Engine
// Copyright (c) 2026 UniVex Studios. All Rights Reserved.
// Unauthorized copying, modification, distribution, or use of this code
// in whole or in part is strictly prohibited without express written
// permission from UniVex Studios.
// Violators will be prosecuted to the fullest extent of the law.
//------------------------------------------------------------------------------

#pragma once

#include <string>

namespace UVE::Scene {

/// One of the master spec's named built-in components (Part 7.3). Deliberately minimal
/// placeholder data: a path-based reference to the attached script asset. Part 8's C# bridge
/// (ScriptAPIUVE) decides the real entity-identity/marshaling mechanism later; this component
/// only records which script is attached.
struct ScriptComponentUVE final {
    std::string scriptAssetPath;
};

} // namespace UVE::Scene
