// Copyright (c) 2026 UniVex Studios. All Rights Reserved.


#include "uve/render/viewport_manager_uve.h"

#include <cstdint>
#include <memory>
#include <optional>
#include <thread>
#include <vector>

#include <gtest/gtest.h>

#include "uve/asset/asset_bundle_uve.h"
#include "uve/asset/asset_database_uve.h"
#include "uve/asset/asset_manager_uve.h"
#include "uve/asset/file_system_uve.h"
#include "uve/asset/material_asset_uve.h"
#include "uve/asset/mesh_asset_uve.h"
#include "uve/asset/shader_asset_uve.h"
#include "uve/events/event_system_uve.h"
#include "uve/memory/memory_manager_uve.h"
#include "uve/render/camera_system_uve.h"
#include "uve/render/light_system_uve.h"
#include "uve/render/mesh_renderer_uve.h"
#include "uve/render/null_render_device_uve.h"
#include "uve/render/recorded_command_uve.h"
#include "uve/render/render_system_uve.h"
#include "uve/render/renderer_3d_uve.h"
#include "uve/render/shader/shader_manager_uve.h"
#include "uve/scene/components/camera_component_uve.h"
#include "uve/scene/components/transform_component_uve.h"
#include "uve/scene/entity_manager_uve.h"
#include "uve/scene/scene_graph_uve.h"
#include "uve/threading/thread_pool_uve.h"

namespace UVE::Render::Tests {
namespace {

constexpr std::uint32_t kTargetWidthUVE = 64;
constexpr std::uint32_t kTargetHeightUVE = 64;
constexpr Math::Vector3UVE kTestAmbientColorUVE{0.1F, 0.2F, 0.3F};
constexpr std::uint32_t kTestShadowMapResolutionUVE = 64;
constexpr float kTestShadowMapHalfExtentUVE = 20.0F;
constexpr float kTestShadowMapNearPlaneUVE = 0.1F;
constexpr float kTestShadowMapFarPlaneUVE = 100.0F;
constexpr float kTestShadowFrustumPaddingUVE = 1.0F;
constexpr float kTestShadowCascadeSplitLambdaUVE = 0.5F;
constexpr float kTestShadowCascadeBlendRatioUVE = 0.1F;
constexpr std::uint32_t kTestShadowPcfKernelRadiusUVE = 1;
constexpr int kMaxPollIterationsUVE = 200000;

// Pure pane-management/hit-testing tests: no renderer/ECS involvement needed.
class ViewportManagerPaneUVETest : public ::testing::Test {
protected:
    ViewportManagerUVE manager;
};

TEST_F(ViewportManagerPaneUVETest, StartsEmpty) {
    EXPECT_EQ(manager.GetPaneCountUVE(), 0U);
    EXPECT_EQ(manager.GetPaneUVE(0), nullptr);
}

TEST_F(ViewportManagerPaneUVETest, AddPaneUVE_AppendsAndIsRetrievable) {
    ViewportPaneUVE pane;
    pane.originX01 = 0.25F;
    pane.originY01 = 0.5F;
    pane.sizeX01 = 0.5F;
    pane.sizeY01 = 0.5F;
    manager.AddPaneUVE(pane);

    ASSERT_EQ(manager.GetPaneCountUVE(), 1U);
    const ViewportPaneUVE* stored = manager.GetPaneUVE(0);
    ASSERT_NE(stored, nullptr);
    EXPECT_FLOAT_EQ(stored->originX01, 0.25F);
    EXPECT_FLOAT_EQ(stored->originY01, 0.5F);
    EXPECT_FLOAT_EQ(stored->sizeX01, 0.5F);
    EXPECT_FLOAT_EQ(stored->sizeY01, 0.5F);
}

TEST_F(ViewportManagerPaneUVETest, GetPaneUVE_OutOfRangeReturnsNullptr) {
    manager.AddPaneUVE(ViewportPaneUVE{});
    EXPECT_EQ(manager.GetPaneUVE(1), nullptr);
    EXPECT_EQ(manager.GetPaneUVE(1000), nullptr);
}

TEST_F(ViewportManagerPaneUVETest, RemovePaneUVE_RemovesTheCorrectPane) {
    ViewportPaneUVE first;
    first.originX01 = 0.0F;
    ViewportPaneUVE second;
    second.originX01 = 0.5F;
    manager.AddPaneUVE(first);
    manager.AddPaneUVE(second);

    manager.RemovePaneUVE(0);

    ASSERT_EQ(manager.GetPaneCountUVE(), 1U);
    EXPECT_FLOAT_EQ(manager.GetPaneUVE(0)->originX01, 0.5F);
}

TEST_F(ViewportManagerPaneUVETest, RemovePaneUVE_OutOfRangeIsANoOp) {
    manager.AddPaneUVE(ViewportPaneUVE{});
    manager.RemovePaneUVE(5);
    EXPECT_EQ(manager.GetPaneCountUVE(), 1U);
}

TEST_F(ViewportManagerPaneUVETest, FindPaneAtNormalizedPositionUVE_NoPanesReturnsNullopt) {
    EXPECT_EQ(manager.FindPaneAtNormalizedPositionUVE(0.5F, 0.5F), std::nullopt);
}

TEST_F(ViewportManagerPaneUVETest, FindPaneAtNormalizedPositionUVE_HitsContainingPane) {
    ViewportPaneUVE left;
    left.originX01 = 0.0F;
    left.sizeX01 = 0.5F;
    left.sizeY01 = 1.0F;
    ViewportPaneUVE right;
    right.originX01 = 0.5F;
    right.sizeX01 = 0.5F;
    right.sizeY01 = 1.0F;
    manager.AddPaneUVE(left);
    manager.AddPaneUVE(right);

    EXPECT_EQ(manager.FindPaneAtNormalizedPositionUVE(0.1F, 0.5F), 0U);
    EXPECT_EQ(manager.FindPaneAtNormalizedPositionUVE(0.9F, 0.5F), 1U);
}

TEST_F(ViewportManagerPaneUVETest, FindPaneAtNormalizedPositionUVE_MissReturnsNullopt) {
    ViewportPaneUVE pane;
    pane.originX01 = 0.0F;
    pane.sizeX01 = 0.25F;
    pane.sizeY01 = 0.25F;
    manager.AddPaneUVE(pane);

    EXPECT_EQ(manager.FindPaneAtNormalizedPositionUVE(0.9F, 0.9F), std::nullopt);
}

TEST_F(ViewportManagerPaneUVETest, FindPaneAtNormalizedPositionUVE_OverlapPrefersMostRecentlyAdded) {
    ViewportPaneUVE bottom;
    bottom.originX01 = 0.0F;
    bottom.originY01 = 0.0F;
    bottom.sizeX01 = 1.0F;
    bottom.sizeY01 = 1.0F;
    ViewportPaneUVE top;
    top.originX01 = 0.25F;
    top.originY01 = 0.25F;
    top.sizeX01 = 0.5F;
    top.sizeY01 = 0.5F;
    manager.AddPaneUVE(bottom);
    manager.AddPaneUVE(top);

    EXPECT_EQ(manager.FindPaneAtNormalizedPositionUVE(0.5F, 0.5F), 1U);
}

TEST_F(ViewportManagerPaneUVETest, FindPaneAtNormalizedPositionUVE_NonFiniteInputReturnsNullopt) {
    manager.AddPaneUVE(ViewportPaneUVE{});
    EXPECT_EQ(manager.FindPaneAtNormalizedPositionUVE(std::numeric_limits<float>::quiet_NaN(), 0.5F), std::nullopt);
}

// Integration tests: RenderAllPanesUVE composing with a real Renderer3DUVE + NullRenderDeviceUVE,
// following the exact fixture shape tests/render/renderer_3d_uve_tests.cpp already establishes.
class ViewportManagerRenderUVETest : public ::testing::Test {
protected:
    Memory::MemoryManagerUVE memoryManager;
    Events::EventSystemUVE eventSystem;
    Scene::EntityManagerUVE entityManager{memoryManager.GetDefaultAllocatorUVE(), eventSystem};
    Scene::SceneGraphUVE sceneGraph;
    Threading::ThreadPoolUVE threadPool{2};
    Asset::AssetDatabaseUVE assetDatabase;
    Asset::AssetManagerUVE assetManager{threadPool, eventSystem};
    NullRenderDeviceUVE renderDevice;

    Asset::AssetBundleUVE assetBundle;
    Asset::FileSystemUVE fileSystem{assetBundle};
    Shader::ShaderManagerUVE shaderManager{threadPool, eventSystem, renderDevice, fileSystem,
                                             Shader::ShaderManagerConfigUVE{}};

    RenderSystemUVE renderSystem{renderDevice};
    CameraSystemUVE cameraSystem;
    MeshRendererUVE meshRenderer;
    LightSystemUVE lightSystem;

    std::unique_ptr<Renderer3DUVE> renderer3D;
    ViewportManagerUVE viewportManager;

    ViewportManagerRenderUVETest() {
        renderer3D = std::make_unique<Renderer3DUVE>(
            renderDevice, renderSystem, meshRenderer, cameraSystem, lightSystem, shaderManager, assetManager,
            assetDatabase, eventSystem, kTargetWidthUVE, kTargetHeightUVE, kTestAmbientColorUVE,
            kTestShadowMapResolutionUVE, kTestShadowMapHalfExtentUVE, kTestShadowMapNearPlaneUVE,
            kTestShadowMapFarPlaneUVE, kTestShadowFrustumPaddingUVE, kTestShadowCascadeSplitLambdaUVE,
            kTestShadowCascadeBlendRatioUVE, kTestShadowPcfKernelRadiusUVE);
    }

    Scene::EntityUVE MakeCameraEntityUVE() {
        const Scene::EntityUVE entity = entityManager.CreateEntityUVE();
        Scene::TransformComponentUVE local;
        sceneGraph.AttachTransformUVE(entityManager, entity, local);
        sceneGraph.UpdateUVE(entityManager);
        entityManager.AddComponentUVE<Scene::CameraComponentUVE>(entity);
        return entity;
    }

    // The built-in ToneMapping program compiles asynchronously (ShaderManagerUVE) - the
    // constructor only requests it. Matching renderer_3d_uve_tests.cpp's own established pattern:
    // a first RenderFrameUVE call kicks off the request, then draining ShaderManagerUVE's job
    // queue makes it ready for every subsequent frame (including the ones RenderAllPanesUVE
    // drives), so the ToneMapping pass actually gets recorded instead of silently short-circuiting
    // on !toneMappingProgram->IsValidUVE().
    void PrimeToneMappingProgramUVE() {
        renderer3D->RenderFrameUVE(entityManager, MakeCameraEntityUVE());
        for (int iteration = 0; iteration < kMaxPollIterationsUVE; ++iteration) {
            shaderManager.UpdateUVE(0.0);
            if (shaderManager.GetPendingJobCountUVE() == 0U) {
                return;
            }
            std::this_thread::yield();
        }
        ADD_FAILURE() << "Timed out waiting for tone mapping shader compilation";
    }

    // Finds the ToneMapping pass among the last submitted commands: it is the only
    // BeginRenderPassCommandUVE whose desc targets the default framebuffer (both attachments
    // invalid), matching the signature the production code uses at
    // engine/render/src/renderer_3d_uve.cpp's ToneMapping pass lambda.
    [[nodiscard]] static std::optional<RenderPassDescUVE>
    FindToneMappingPassDescUVE(const std::vector<RecordedCommandUVE>& commands) {
        for (const RecordedCommandUVE& command : commands) {
            if (const auto* beginPass = std::get_if<BeginRenderPassCommandUVE>(&command)) {
                if (beginPass->desc.colorAttachment == kInvalidTextureHandleUVE &&
                    beginPass->desc.depthAttachment == kInvalidTextureHandleUVE) {
                    return beginPass->desc;
                }
            }
        }
        return std::nullopt;
    }
};

TEST_F(ViewportManagerRenderUVETest, RenderAllPanesUVE_ZeroWindowSizeIsANoOp) {
    ViewportPaneUVE pane;
    pane.cameraEntity = MakeCameraEntityUVE();
    viewportManager.AddPaneUVE(pane);

    viewportManager.RenderAllPanesUVE(*renderer3D, entityManager, 0U, 128U);
    EXPECT_TRUE(renderDevice.GetLastSubmittedCommandsUVE().empty());
}

TEST_F(ViewportManagerRenderUVETest, RenderAllPanesUVE_SkipsUiOnlyPane) {
    ViewportPaneUVE pane;
    pane.cameraEntity = MakeCameraEntityUVE();
    pane.uiOnly = true;
    viewportManager.AddPaneUVE(pane);

    viewportManager.RenderAllPanesUVE(*renderer3D, entityManager, 128U, 128U);
    EXPECT_TRUE(renderDevice.GetLastSubmittedCommandsUVE().empty());
}

TEST_F(ViewportManagerRenderUVETest, RenderAllPanesUVE_SkipsPaneWithInvalidRect) {
    ViewportPaneUVE pane;
    pane.cameraEntity = MakeCameraEntityUVE();
    pane.originX01 = 0.9F;
    pane.sizeX01 = 0.5F; // origin + size > 1.0 -> invalid.
    viewportManager.AddPaneUVE(pane);

    viewportManager.RenderAllPanesUVE(*renderer3D, entityManager, 128U, 128U);
    EXPECT_TRUE(renderDevice.GetLastSubmittedCommandsUVE().empty());
}

TEST_F(ViewportManagerRenderUVETest, RenderAllPanesUVE_SkipsPaneWithInvalidCameraEntity) {
    ViewportPaneUVE pane;
    pane.cameraEntity = Scene::kInvalidEntityUVE;
    viewportManager.AddPaneUVE(pane);

    viewportManager.RenderAllPanesUVE(*renderer3D, entityManager, 128U, 128U);
    EXPECT_TRUE(renderDevice.GetLastSubmittedCommandsUVE().empty());
}

TEST_F(ViewportManagerRenderUVETest, RenderAllPanesUVE_SkipsPaneWithNonCameraEntity) {
    ViewportPaneUVE pane;
    pane.cameraEntity = entityManager.CreateEntityUVE(); // No CameraComponentUVE attached.
    viewportManager.AddPaneUVE(pane);

    viewportManager.RenderAllPanesUVE(*renderer3D, entityManager, 128U, 128U);
    EXPECT_TRUE(renderDevice.GetLastSubmittedCommandsUVE().empty());
}

TEST_F(ViewportManagerRenderUVETest, RenderAllPanesUVE_ValidPaneRendersWithCorrectPixelRegion) {
    PrimeToneMappingProgramUVE();

    ViewportPaneUVE pane;
    pane.cameraEntity = MakeCameraEntityUVE();
    pane.originX01 = 0.5F;
    pane.originY01 = 0.0F;
    pane.sizeX01 = 0.5F;
    pane.sizeY01 = 0.5F;
    viewportManager.AddPaneUVE(pane);

    viewportManager.RenderAllPanesUVE(*renderer3D, entityManager, 200U, 100U);

    const std::optional<RenderPassDescUVE> toneMappingDesc =
        FindToneMappingPassDescUVE(renderDevice.GetLastSubmittedCommandsUVE());
    ASSERT_TRUE(toneMappingDesc.has_value());
    ASSERT_TRUE(toneMappingDesc->viewportOverride.has_value());
    // originX01=0.5, sizeX01=0.5 over a 200-wide window -> pixelX=100, width=100.
    EXPECT_EQ(toneMappingDesc->viewportOverride->x, 100U);
    EXPECT_EQ(toneMappingDesc->viewportOverride->width, 100U);
    // originY01=0.0, sizeY01=0.5 over a 100-tall window, top-left origin, flipped to GL's
    // bottom-left convention: pixelYFromTop=0, height=50 -> pixelYFromBottom=100-0-50=50.
    EXPECT_EQ(toneMappingDesc->viewportOverride->y, 50U);
    EXPECT_EQ(toneMappingDesc->viewportOverride->height, 50U);
}

// NullRenderDeviceUVE::GetLastSubmittedCommandsUVE() reflects only the most recently submitted
// frame (each RenderFrameToRegionUVE call is its own Begin/EndFrameUVE), so a multi-pane call
// can't be asserted by counting passes across the whole call. Instead this confirms
// RenderAllPanesUVE actually reaches and renders the second (last) pane with its own distinct
// region, rather than stopping after the first - the single-pane test above already covers the
// per-pane pixel-rect math itself.
TEST_F(ViewportManagerRenderUVETest, RenderAllPanesUVE_ProcessesEveryValidPaneInOrder) {
    PrimeToneMappingProgramUVE();

    ViewportPaneUVE first;
    first.cameraEntity = MakeCameraEntityUVE();
    first.sizeX01 = 0.5F;
    ViewportPaneUVE second;
    second.cameraEntity = MakeCameraEntityUVE();
    second.originX01 = 0.5F;
    second.sizeX01 = 0.5F;
    viewportManager.AddPaneUVE(first);
    viewportManager.AddPaneUVE(second);

    viewportManager.RenderAllPanesUVE(*renderer3D, entityManager, 128U, 128U);

    const std::optional<RenderPassDescUVE> toneMappingDesc =
        FindToneMappingPassDescUVE(renderDevice.GetLastSubmittedCommandsUVE());
    ASSERT_TRUE(toneMappingDesc.has_value());
    ASSERT_TRUE(toneMappingDesc->viewportOverride.has_value());
    EXPECT_EQ(toneMappingDesc->viewportOverride->x, 64U);
    EXPECT_EQ(toneMappingDesc->viewportOverride->width, 64U);
}

} // namespace
} // namespace UVE::Render::Tests
