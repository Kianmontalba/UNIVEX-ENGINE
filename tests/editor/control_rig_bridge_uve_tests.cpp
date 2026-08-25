// Copyright (c) 2026 UniVex Studios. All Rights Reserved.

#include <gtest/gtest.h>

#include <algorithm>

#include "uve/core/engine_core_uve.h"
#include "uve/editor/editor_bridge_uve.h"
#include "uve/editor/editor_uve.h"
#include "uve/plugins/control_rig_editor_uve.h"

namespace UVE::Editor::Tests {
namespace {

[[nodiscard]] Core::EngineConfigUVE MakeConfigUVE() {
    Core::EngineConfigUVE config{};
    config.headlessUVE = true;
    config.logFilePath = "uve_control_rig_bridge_tests.log";
    config.settingsFilePath = "uve_control_rig_bridge_tests_settings.json";
    config.assetDatabaseFilePath = "uve_control_rig_bridge_tests_assets.json";
    config.saveDirectoryPath = "uve_control_rig_bridge_tests_saves";
    config.shaderCachePath = "uve_control_rig_bridge_tests_shader_cache";
    config.shaderSourceRealDirectoryUVE = "engine/render/shader/built_in";
    config.shaderSourceMountPrefixUVE = "shaders";
    return config;
}

[[nodiscard]] Core::ControlRigAutoRigRequestUVE MakeRequestUVE() {
    Core::ControlRigAutoRigRequestUVE request;
    request.rigId = "bridge_autorig";
    request.skeleton = Core::SkeletonDefinitionUVE{
        "humanoid",
        {{"root", ""}, {"spine", "root"}, {"upper_arm.L", "spine"},
         {"forearm.L", "upper_arm.L"}, {"hand.L", "forearm.L"},
         {"upper_arm.R", "spine"}, {"forearm.R", "upper_arm.R"}, {"hand.R", "forearm.R"},
         {"thigh.L", "root"}, {"shin.L", "thigh.L"}, {"foot.L", "shin.L"},
         {"thigh.R", "root"}, {"shin.R", "thigh.R"}, {"foot.R", "shin.R"}, {"head", "spine"}}};
    request.referencePose.skeletonId = request.skeleton.skeletonId;
    for (std::size_t index = 0U; index < request.skeleton.joints.size(); ++index) {
        request.referencePose.localJoints.push_back(
            Core::TransformPoseUVE{{0.0F, static_cast<float>(index), 0.0F}, {}, {1.0F, 1.0F, 1.0F}});
    }
    return request;
}

} // namespace

TEST(EditorBridgeControlRigUVETest, ReadControlRigCopiesAuthoringSnapshotAndAdvertisesCapability) {
    Core::EngineCoreUVE engine(MakeConfigUVE());
    engine.Init();
    ASSERT_TRUE(engine.Load());
    {
        EditorUVE editor(engine.GetServicesUVE(), "uve_control_rig_bridge.uvescene");
        editor.InitUVE();
        Core::ControlRigEditorAuthoringSessionUVE session;
        ASSERT_TRUE(session.InitializeUVE(MakeRequestUVE()));
        EditorBridgeUVE bridge(editor, nullptr, nullptr, nullptr, &session);

        const EditorBridgeSnapshotUVE initial = bridge.GetSnapshotUVE();
        EXPECT_EQ(initial.controlRig.rigId, "bridge_autorig");
        EXPECT_FALSE(initial.controlRig.viewportControls.empty());
        EXPECT_TRUE(std::find(initial.capabilities.cbegin(), initial.capabilities.cend(),
                              EditorBridgeCapabilityUVE::ReadControlRig) != initial.capabilities.cend());

        EditorBridgeRequestUVE read{};
        read.protocolVersion = kEditorBridgeProtocolVersionUVE;
        read.requestId = 1U;
        read.expectedRevision = initial.revision;
        read.kind = EditorBridgeRequestKindUVE::ReadControlRig;
        const EditorBridgeResponseUVE response = bridge.DispatchUVE(read);
        ASSERT_TRUE(response.applied);
        EXPECT_EQ(response.code, "bridge.control_rig.snapshot.read");
        EXPECT_EQ(response.snapshot.controlRig.rigId, "bridge_autorig");

        ASSERT_TRUE(session.SelectControlUVE("ctrl_hand_ik_l"));
        const EditorBridgeSnapshotUVE changed = bridge.GetSnapshotUVE();
        EXPECT_GT(changed.revision, initial.revision);
        EXPECT_EQ(changed.controlRig.selectedControlId, "ctrl_hand_ik_l");

        editor.ShutdownUVE();
    }
    engine.Shutdown();
}

} // namespace UVE::Editor::Tests
