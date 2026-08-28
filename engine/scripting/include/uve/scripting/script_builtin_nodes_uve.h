// Copyright (c) 2026 UniVex Studios. All Rights Reserved.
#pragma once

#include "uve/scripting/script_graph_uve.h"

namespace UVE::Scripting {

/// Registers the engine-owned node descriptors exposed to the editor palette.
/// This catalog defines graph/pin contracts and identifies action nodes that require execution power;
/// pure value nodes remain data-driven and execution-independent.
[[nodiscard]] bool RegisterBuiltInScriptNodesUVE(ScriptNodeRegistryUVE& registry);

} // namespace UVE::Scripting

