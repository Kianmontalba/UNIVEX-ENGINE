// Copyright (c) 2026 UniVex Studios. All Rights Reserved.


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
