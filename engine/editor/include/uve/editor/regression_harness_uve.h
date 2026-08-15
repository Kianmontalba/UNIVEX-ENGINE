// Copyright (c) 2026 UniVex Studios. All Rights Reserved.

#pragma once

#include "uve/core/animation_clip_uve.h"
#include "uve/editor/editor_bridge_uve.h"
#include "uve/scripting/script_graph_canvas_uve.h"

#include <cstddef>
#include <cstdint>
#include <string>
#include <vector>

namespace UVE::Editor {

enum class EditorRegressionOperationKindUVE : std::uint8_t {
    AddNode = 0,
    MoveNode,
    SetSelection,
    SetView,
    Undo,
    Redo,
};

struct EditorRegressionOperationUVE final {
    EditorRegressionOperationKindUVE kind = EditorRegressionOperationKindUVE::AddNode;
    std::uint32_t nodeId = 0U;
    std::string nodeTypeId;
    Scripting::ScriptGraphCanvasPointUVE position;
    std::vector<std::uint32_t> selection;
    Scripting::ScriptGraphCanvasViewUVE view;
};

struct EditorAnimationRegressionCaseUVE final {
    static constexpr std::size_t kMaximumOperationsUVE = 128U;
    static constexpr std::size_t kMaximumSampleTimesUVE = 128U;

    Scripting::ScriptGraphSchemaUVE graphSchema;
    Core::AnimationClipUVE animationClip;
    std::vector<EditorRegressionOperationUVE> operations;
    std::vector<double> animationSampleTimes;
};

enum class EditorRegressionCodeUVE : std::uint8_t {
    Passed = 0,
    InvalidCase,
    GraphEditRejected,
    PersistenceFailed,
    CompileFailed,
    AnimationFailed,
    CapacityExceeded,
    NonDeterministic,
};

struct EditorRegressionRunResultUVE final {
    EditorRegressionCodeUVE code = EditorRegressionCodeUVE::InvalidCase;
    std::string message;
    std::string deterministicDigest;
    std::size_t appliedOperationCount = 0U;
    std::size_t sampledAnimationCount = 0U;
    EditorBridgeVisualScriptingSnapshotUVE visualScriptDto;

    [[nodiscard]] bool IsPassedUVE() const noexcept {
        return code == EditorRegressionCodeUVE::Passed;
    }
};

struct EditorRegressionDeterminismResultUVE final {
    EditorRegressionCodeUVE code = EditorRegressionCodeUVE::InvalidCase;
    std::string message;
    std::string deterministicDigest;
    std::size_t requestedIterations = 0U;
    std::size_t completedIterations = 0U;

    [[nodiscard]] bool IsPassedUVE() const noexcept {
        return code == EditorRegressionCodeUVE::Passed;
    }
};

[[nodiscard]] EditorRegressionRunResultUVE RunEditorAnimationRegressionUVE(
    const EditorAnimationRegressionCaseUVE& regressionCase,
    Scripting::ScriptNodeRegistryUVE& registry);

[[nodiscard]] EditorRegressionDeterminismResultUVE RunEditorAnimationDeterminismUVE(
    const EditorAnimationRegressionCaseUVE& regressionCase,
    Scripting::ScriptNodeRegistryUVE& registry, std::size_t iterations = 2U);

} // namespace UVE::Editor
