// Copyright (c) 2026 UniVex Studios. All Rights Reserved.

#include <gtest/gtest.h>

#include <utility>

#include "uve/core/engine_core_uve.h"
#include "uve/editor/editor_bridge_uve.h"
#include "uve/editor/editor_uve.h"

namespace UVE::Editor::Tests {
namespace {

[[nodiscard]] Core::EngineConfigUVE MakeConfigUVE() {
    Core::EngineConfigUVE config{};
    config.headlessUVE = true;
    config.logFilePath = "uve_motion_query_trajectory_bridge_tests.log";
    config.settingsFilePath = "uve_motion_query_trajectory_bridge_tests_settings.json";
    config.assetDatabaseFilePath = "uve_motion_query_trajectory_bridge_tests_assets.json";
    config.saveDirectoryPath = "uve_motion_query_trajectory_bridge_tests_saves";
    config.shaderCachePath = "uve_motion_query_trajectory_bridge_tests_shader_cache";
    config.shaderSourceRealDirectoryUVE = "engine/render/shader/built_in";
    config.shaderSourceMountPrefixUVE = "shaders";
    return config;
}

Core::TimeSampledTrajectoryUVE MakeTrajectoryUVE() {
    Core::TimeSampledTrajectoryUVE trajectory;
    trajectory.context = Core::AnimationMotionContextUVE::HeavyLanding;
    trajectory.samples = {
        {0.0, {0.0F, 0.0F, 0.0F}, {}, {0.0F, 0.0F, 1.0F}, 0.4F, 0.9F},
        {0.25, {0.0F, -0.2F, 0.0F}, {}, {0.0F, 0.0F, 1.0F}, 0.42F, 0.95F},
    };
    return trajectory;
}

} // namespace

TEST(EditorBridgeMotionQueryTrajectoryUVETest, SharedTrajectoryPreviewCopiesContextSamplesAndShapes) {
    Core::EngineCoreUVE engine(MakeConfigUVE());
    engine.Init();
    ASSERT_TRUE(engine.Load());
    {
        EditorUVE editor(engine.GetServicesUVE(), "uve_motion_query_trajectory_bridge.uvescene");
        editor.InitUVE();
        EditorBridgeUVE bridge(editor);
        const EditorBridgeSnapshotUVE before = bridge.GetSnapshotUVE();

        bridge.SetMotionQueryTrajectoryPreviewUVE(MakeTrajectoryUVE());
        const EditorBridgeSnapshotUVE snapshot = bridge.GetSnapshotUVE();
        ASSERT_GT(snapshot.revision, before.revision);
        ASSERT_TRUE(snapshot.motionQuery.trajectoryPreview.available);
        EXPECT_EQ(snapshot.motionQuery.trajectoryPreview.trajectory.context,
                  Core::AnimationMotionContextUVE::HeavyLanding);
        ASSERT_EQ(snapshot.motionQuery.trajectoryPreview.trajectory.samples.size(), 2U);
        EXPECT_FLOAT_EQ(snapshot.motionQuery.trajectoryPreview.trajectory.samples[1].capsuleRadius, 0.42F);
        EXPECT_FALSE(snapshot.motionQuery.trajectoryPreview.collisionPrediction.has_value());

        Core::TimeSampledTrajectoryUVE invalid = MakeTrajectoryUVE();
        invalid.schemaVersion = 99U;
        bridge.SetMotionQueryTrajectoryPreviewUVE(std::move(invalid));
        const EditorBridgeSnapshotUVE rejected = bridge.GetSnapshotUVE();
        EXPECT_FALSE(rejected.motionQuery.trajectoryPreview.available);
        EXPECT_NE(rejected.motionQuery.trajectoryPreview.diagnostic.find("schema"), std::string::npos);

        editor.ShutdownUVE();
    }
    engine.Shutdown();
}

} // namespace UVE::Editor::Tests
