// Copyright (c) 2026 UniVex Studios. All Rights Reserved.


#include "uve/render/renderer_3d_uve.h"

#include <algorithm>
#include <array>
#include <atomic>
#include <cstddef>
#include <cstdint>
#include <memory>
#include <limits>
#include <string>
#include <thread>
#include <vector>

#include <gtest/gtest.h>

#include "uve/asset/asset_bundle_uve.h"
#include "uve/asset/asset_database_uve.h"
#include "uve/asset/asset_manager_uve.h"
#include "uve/asset/asset_reloaded_event_uve.h"
#include "uve/asset/file_system_uve.h"
#include "uve/asset/material_asset_uve.h"
#include "uve/asset/mesh_asset_uve.h"
#include "uve/asset/shader_asset_uve.h"
#include "uve/asset/texture_asset_uve.h"
#include "uve/events/event_system_uve.h"
#include "uve/memory/memory_manager_uve.h"
#include "uve/render/camera_system_uve.h"
#include "uve/render/light_system_uve.h"
#include "uve/render/mesh_renderer_uve.h"
#include "uve/render/null_render_device_uve.h"
#include "uve/render/render_system_uve.h"
#include "uve/render/shader/built_in_shaders_uve.h"
#include "uve/render/shader/shader_manager_uve.h"
#include "uve/scene/components/camera_component_uve.h"
#include "uve/scene/components/light_component_uve.h"
#include "uve/scene/components/mesh_component_uve.h"
#include "uve/scene/components/primitive_mesh_component_uve.h"
#include "uve/scene/components/transform_component_uve.h"
#include "uve/scene/entity_manager_uve.h"
#include "uve/scene/scene_graph_uve.h"
#include "uve/threading/thread_pool_uve.h"

namespace UVE::Render::Tests {
namespace {

constexpr int kMaxPollIterationsUVE = 200000;
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

class Renderer3DUVETest : public ::testing::Test {
protected:
    Memory::MemoryManagerUVE memoryManager;
    Events::EventSystemUVE eventSystem;
    Scene::EntityManagerUVE entityManager{memoryManager.GetDefaultAllocatorUVE(), eventSystem};
    Scene::SceneGraphUVE sceneGraph;
    Threading::ThreadPoolUVE threadPool{2};
    Asset::AssetDatabaseUVE assetDatabase;
    Asset::AssetManagerUVE assetManager{threadPool, eventSystem};
    NullRenderDeviceUVE renderDevice;

    // Real (not fake) ShaderManagerUVE (Increment 26) — Renderer3DUVE compiles its built-in
    // shadow-depth program through this, matching the exact fixture shape
    // tests/render/shader/shader_manager_uve_tests.cpp already establishes for exercising
    // ShaderManagerUVE against NullRenderDeviceUVE. assetBundle/fileSystem exist only so
    // ShaderManagerUVE has a real IFileSystemUVE to (fail to) find a virtual shader file on —
    // every program in these tests resolves through its embedded fallback source instead.
    Asset::AssetBundleUVE assetBundle;
    Asset::FileSystemUVE fileSystem{assetBundle};
    Shader::ShaderManagerUVE shaderManager{threadPool, eventSystem, renderDevice, fileSystem,
                                             Shader::ShaderManagerConfigUVE{}};

    RenderSystemUVE renderSystem{renderDevice};
    CameraSystemUVE cameraSystem;
    MeshRendererUVE meshRenderer;
    LightSystemUVE lightSystem;

    Asset::AssetGuidUVE vertexShaderGuid;
    Asset::AssetGuidUVE fragmentShaderGuid;
    Scene::EntityUVE mostRecentCamera = Scene::kInvalidEntityUVE;
    std::unique_ptr<Renderer3DUVE> renderer3D;

    Renderer3DUVETest() {
        vertexShaderGuid = assetDatabase.RegisterUVE("renderer3d_tests_vertex.uveshader");
        fragmentShaderGuid = assetDatabase.RegisterUVE("renderer3d_tests_fragment.uveshader");

        assetManager.RegisterLoaderUVE<Asset::ShaderAssetUVE>(
            [](const std::filesystem::path&, Asset::ShaderAssetUVE& shader) {
                shader.sourceCode = "void main() { }";
                return true;
            });
        assetManager.RegisterLoaderUVE<Asset::MeshAssetUVE>(
            [](const std::filesystem::path&, Asset::MeshAssetUVE& mesh) {
                mesh.vertices = {
                    Asset::MeshVertexUVE{Math::Vector3UVE{0.0F, 0.0F, 0.0F}, Math::Vector3UVE{0.0F, 1.0F, 0.0F}, 0.0F, 0.0F},
                    Asset::MeshVertexUVE{Math::Vector3UVE{1.0F, 0.0F, 0.0F}, Math::Vector3UVE{0.0F, 1.0F, 0.0F}, 1.0F, 0.0F},
                    Asset::MeshVertexUVE{Math::Vector3UVE{0.0F, 1.0F, 0.0F}, Math::Vector3UVE{0.0F, 1.0F, 0.0F}, 0.0F, 1.0F},
                };
                mesh.indices = {0, 1, 2};
                mesh.localBounds =
                    Math::AabbUVE::FromCenterExtentsUVE(Math::Vector3UVE{0.0F, 0.0F, 0.0F}, Math::Vector3UVE{0.5F, 0.5F, 0.5F});
                return true;
            });
        // Default material: valid shader GUIDs, distinguishable scalar/color values, and every
        // texture GUID left unset (kInvalidAssetGuidUVE) - exercises Renderer3DUVE's fallback
        // texture path unless a specific test overrides this loader with a real texture GUID.
        assetManager.RegisterLoaderUVE<Asset::MaterialAssetUVE>(
            [vertexGuid = vertexShaderGuid, fragmentGuid = fragmentShaderGuid](const std::filesystem::path&,
                                                                                 Asset::MaterialAssetUVE& material) {
                material.vertexShader = vertexGuid;
                material.fragmentShader = fragmentGuid;
                material.isTransparent = false;
                material.albedoColor = Math::Vector3UVE{0.2F, 0.4F, 0.6F};
                material.metallic = 0.25F;
                material.roughness = 0.75F;
                material.emissiveColor = Math::Vector3UVE{0.1F, 0.0F, 0.0F};
                return true;
            });
        // Default texture loader: a small, always-ready 2x2 RGBA8Unorm texture, reused by any
        // test that assigns a real texture GUID to a material without needing custom pixel data.
        assetManager.RegisterLoaderUVE<Asset::TextureAssetUVE>(
            [](const std::filesystem::path&, Asset::TextureAssetUVE& texture) {
                texture.width = 2;
                texture.height = 2;
                texture.format = Asset::TextureFormatUVE::RGBA8Unorm;
                texture.pixels.assign(2U * 2U * 4U, std::byte{0xAB});
                return true;
            });

        renderer3D = std::make_unique<Renderer3DUVE>(
            renderDevice, renderSystem, meshRenderer, cameraSystem, lightSystem, shaderManager, assetManager,
            assetDatabase, eventSystem, kTargetWidthUVE, kTargetHeightUVE, kTestAmbientColorUVE,
            kTestShadowMapResolutionUVE, kTestShadowMapHalfExtentUVE, kTestShadowMapNearPlaneUVE,
            kTestShadowMapFarPlaneUVE, kTestShadowFrustumPaddingUVE, kTestShadowCascadeSplitLambdaUVE,
            kTestShadowCascadeBlendRatioUVE, kTestShadowPcfKernelRadiusUVE);
    }

    Scene::EntityUVE MakeCameraEntityUVE(Math::Vector3UVE position = Math::Vector3UVE{}) {
        const Scene::EntityUVE entity = entityManager.CreateEntityUVE();
        Scene::TransformComponentUVE local;
        local.localPosition = position;
        sceneGraph.AttachTransformUVE(entityManager, entity, local);
        sceneGraph.UpdateUVE(entityManager);
        entityManager.AddComponentUVE<Scene::CameraComponentUVE>(entity);
        mostRecentCamera = entity;
        return entity;
    }

    Scene::EntityUVE MakeMeshEntityUVE(Math::Vector3UVE worldPosition, Asset::AssetGuidUVE meshGuid,
                                        Asset::AssetGuidUVE materialGuid) {
        const Scene::EntityUVE entity = entityManager.CreateEntityUVE();
        Scene::TransformComponentUVE local;
        local.localPosition = worldPosition;
        sceneGraph.AttachTransformUVE(entityManager, entity, local);
        sceneGraph.UpdateUVE(entityManager);
        entityManager.AddComponentUVE<Scene::MeshComponentUVE>(entity, Scene::MeshComponentUVE{meshGuid, materialGuid});
        return entity;
    }

    Scene::EntityUVE MakePrimitiveEntityUVE(Math::Vector3UVE worldPosition,
                                             Scene::PrimitiveMeshComponentUVE primitive) {
        const Scene::EntityUVE entity = entityManager.CreateEntityUVE();
        Scene::TransformComponentUVE local;
        local.localPosition = worldPosition;
        sceneGraph.AttachTransformUVE(entityManager, entity, local);
        sceneGraph.UpdateUVE(entityManager);
        entityManager.AddComponentUVE<Scene::PrimitiveMeshComponentUVE>(entity, primitive);
        return entity;
    }

    Scene::EntityUVE MakeLightEntityUVE(Scene::LightComponentUVE light) {
        const Scene::EntityUVE entity = entityManager.CreateEntityUVE();
        sceneGraph.AttachTransformUVE(entityManager, entity, Scene::TransformComponentUVE{});
        sceneGraph.UpdateUVE(entityManager);
        entityManager.AddComponentUVE<Scene::LightComponentUVE>(entity, light);
        return entity;
    }

    void WaitUntilAssetsReadyUVE(Asset::AssetGuidUVE meshGuid, Asset::AssetGuidUVE materialGuid,
                                 bool primeDefaultRenderer = true) {
        Asset::AssetHandleUVE<Asset::MeshAssetUVE> meshHandle =
            assetManager.LoadUVE<Asset::MeshAssetUVE>(meshGuid, assetDatabase);
        Asset::AssetHandleUVE<Asset::MaterialAssetUVE> materialHandle =
            assetManager.LoadUVE<Asset::MaterialAssetUVE>(materialGuid, assetDatabase);
        Asset::AssetHandleUVE<Asset::ShaderAssetUVE> vertexHandle =
            assetManager.LoadUVE<Asset::ShaderAssetUVE>(vertexShaderGuid, assetDatabase);
        Asset::AssetHandleUVE<Asset::ShaderAssetUVE> fragmentHandle =
            assetManager.LoadUVE<Asset::ShaderAssetUVE>(fragmentShaderGuid, assetDatabase);
        for (int iteration = 0; iteration < kMaxPollIterationsUVE; ++iteration) {
            if (meshHandle.IsReadyUVE() && materialHandle.IsReadyUVE() && vertexHandle.IsReadyUVE() &&
                fragmentHandle.IsReadyUVE()) {
                break;
            }
            std::this_thread::yield();
        }
        ASSERT_TRUE(meshHandle.IsReadyUVE());
        ASSERT_TRUE(materialHandle.IsReadyUVE());
        ASSERT_TRUE(vertexHandle.IsReadyUVE());
        ASSERT_TRUE(fragmentHandle.IsReadyUVE());
        if (primeDefaultRenderer && mostRecentCamera != Scene::kInvalidEntityUVE) {
            PrimeMaterialProgramUVE(*renderer3D, mostRecentCamera);
        }
    }

    /// Starts one material-program request through `renderer`, then drains the ShaderManagerUVE
    /// jobs. The test's subsequent explicit RenderFrameUVE call observes the ready managed program
    /// without obscuring the first-frame asynchronous skip from tests that exercise it directly.
    void PrimeMaterialProgramUVE(Renderer3DUVE& renderer, Scene::EntityUVE cameraEntity) {
        renderer.RenderFrameUVE(entityManager, cameraEntity);
        for (int iteration = 0; iteration < kMaxPollIterationsUVE; ++iteration) {
            shaderManager.UpdateUVE(0.0);
            if (shaderManager.GetPendingJobCountUVE() == 0U) {
                return;
            }
            std::this_thread::yield();
        }
        ADD_FAILURE() << "Timed out waiting for managed material program compilation";
    }

    void WaitUntilTextureReadyUVE(Asset::AssetGuidUVE textureGuid) {
        Asset::AssetHandleUVE<Asset::TextureAssetUVE> textureHandle =
            assetManager.LoadUVE<Asset::TextureAssetUVE>(textureGuid, assetDatabase);
        for (int iteration = 0; iteration < kMaxPollIterationsUVE; ++iteration) {
            if (textureHandle.IsReadyUVE()) {
                break;
            }
            std::this_thread::yield();
        }
        ASSERT_TRUE(textureHandle.IsReadyUVE());
    }

    /// Overrides the material loader so its albedoTexture points at `textureGuid`, keeping every
    /// other field identical to the default loader registered in the constructor. Safe to call
    /// any number of times per test (RegisterLoaderUVE<T>() simply replaces the stored loader).
    void UseAlbedoTextureInMaterialUVE(Asset::AssetGuidUVE textureGuid) {
        assetManager.RegisterLoaderUVE<Asset::MaterialAssetUVE>(
            [vertexGuid = vertexShaderGuid, fragmentGuid = fragmentShaderGuid, textureGuid](
                const std::filesystem::path&, Asset::MaterialAssetUVE& material) {
                material.vertexShader = vertexGuid;
                material.fragmentShader = fragmentGuid;
                material.albedoTexture = textureGuid;
                return true;
            });
    }

    /// Bounded busy-poll (never a fixed sleep) driving `shaderManager` until Renderer3DUVE's
    /// internal built-in shadow-depth program has finished compiling (Increment 26). The test
    /// can't reach that program directly (it lives inside Renderer3DUVE's PIMPL) - instead this
    /// creates a second program request against the identical built-in descriptor and polls that
    /// one to readiness. ShaderManagerUVE::UpdateUVE() drains every completed job and applies every
    /// pending program link in one call (not just one at a time, see DrainCompletedSourceJobsUVE/
    /// ApplyPendingProgramLinksUVE), and Renderer3DUVE's own request was submitted first (at
    /// fixture construction, before any test body runs) - so by the time this probe program is
    /// ready, Renderer3DUVE's internal one is guaranteed to be ready too.
    void WaitUntilShadowProgramReadyUVE() {
        Shader::ShaderProgramDescUVE probeDesc;
        probeDesc.virtualFilePath = std::string(Shader::BuiltIn::kShadowDepthVirtualPath);
        probeDesc.embeddedFallbackSourceCode = std::string(Shader::BuiltIn::kShadowDepthSource);
        probeDesc.vertexLayout = {VertexAttributeUVE{"POSITION", VertexAttributeFormatUVE::Float3, 0}};
        const std::shared_ptr<Shader::ShaderProgramUVE> probe = shaderManager.CreateProgramUVE(probeDesc);
        for (int iteration = 0; iteration < kMaxPollIterationsUVE; ++iteration) {
            shaderManager.UpdateUVE(0.0);
            if (probe->IsReadyUVE()) {
                break;
            }
            std::this_thread::yield();
        }
        ASSERT_TRUE(probe->IsReadyUVE());
        ASSERT_TRUE(probe->IsValidUVE());
    }
};

TEST_F(Renderer3DUVETest, RenderFrameUVE_EmptyScene_MainPassBeginsAndEndsWithNoDraws) {
    const Scene::EntityUVE cameraEntity = MakeCameraEntityUVE();

    renderer3D->RenderFrameUVE(entityManager, cameraEntity);

    // No directional caster exists, so the optimized frame contains only the empty main color
    // pass. Tone mapping has not linked yet in this first frame.
    const std::vector<RecordedCommandUVE>& commands = renderDevice.GetLastSubmittedCommandsUVE();
    ASSERT_EQ(commands.size(), 2U);
    ASSERT_TRUE(std::holds_alternative<BeginRenderPassCommandUVE>(commands[0U]));
    EXPECT_NE(std::get<BeginRenderPassCommandUVE>(commands[0U]).desc.colorAttachment, kInvalidTextureHandleUVE);
    EXPECT_TRUE(std::holds_alternative<EndRenderPassCommandUVE>(commands[1U]));
}

TEST_F(Renderer3DUVETest, RenderFrameUVE_VisiblePrimitive_RecordsCanonicalGeometryAndAuthoredColor) {
    const Scene::EntityUVE cameraEntity = MakeCameraEntityUVE();
    const Math::Vector3UVE baseColor{0.15F, 0.45F, 0.85F};
    MakePrimitiveEntityUVE(Math::Vector3UVE{0.0F, 0.0F, -10.0F},
                           Scene::PrimitiveMeshComponentUVE{Scene::PrimitiveMeshKindUVE::Cube, baseColor});

    // The renderer-owned built-in program compiles asynchronously. The first frame triggers regular
    // extraction; draining ShaderManagerUVE then makes the next frame’s primitive draw observable.
    renderer3D->RenderFrameUVE(entityManager, cameraEntity);
    for (int iteration = 0; iteration < kMaxPollIterationsUVE; ++iteration) {
        shaderManager.UpdateUVE(0.0);
        if (shaderManager.GetPendingJobCountUVE() == 0U) {
            break;
        }
        std::this_thread::yield();
    }
    ASSERT_EQ(shaderManager.GetPendingJobCountUVE(), 0U);

    renderer3D->RenderFrameUVE(entityManager, cameraEntity);
    const std::vector<RecordedCommandUVE>& commands = renderDevice.GetLastSubmittedCommandsUVE();
    EXPECT_TRUE(std::any_of(commands.cbegin(), commands.cend(), [](const RecordedCommandUVE& command) {
        return std::holds_alternative<DrawIndexedCommandUVE>(command) &&
               std::get<DrawIndexedCommandUVE>(command).indexCount == 36U;
    }));
    const auto primitiveColor = std::find_if(commands.cbegin(), commands.cend(), [](const RecordedCommandUVE& command) {
        return std::holds_alternative<SetUniformVector3CommandUVE>(command) &&
               std::get<SetUniformVector3CommandUVE>(command).name == "uColor";
    });
    ASSERT_NE(primitiveColor, commands.cend());
    EXPECT_EQ(std::get<SetUniformVector3CommandUVE>(*primitiveColor).value, baseColor);
}

TEST_F(Renderer3DUVETest, RenderFrameUVE_FiniteExtremePrimitiveTransformIsRejectedBeforeQueuePublication) {
    const Scene::EntityUVE cameraEntity = MakeCameraEntityUVE();
    const Scene::EntityUVE primitiveEntity = entityManager.CreateEntityUVE();
    Scene::TransformComponentUVE transform;
    transform.localPosition = Math::Vector3UVE{std::numeric_limits<float>::max(), 0.0F, -10.0F};
    transform.localScale = Math::Vector3UVE{std::numeric_limits<float>::max(), 1.0F, 1.0F};
    sceneGraph.AttachTransformUVE(entityManager, primitiveEntity, transform);
    sceneGraph.UpdateUVE(entityManager);
    entityManager.AddComponentUVE<Scene::PrimitiveMeshComponentUVE>(primitiveEntity);

    renderer3D->RenderFrameUVE(entityManager, cameraEntity);
    const Renderer3DFrameDiagnosticsUVE diagnostics = renderer3D->GetLastFrameDiagnosticsUVE();
    EXPECT_EQ(diagnostics.primitiveCandidates, 1U);
    EXPECT_EQ(diagnostics.primitiveItemsExtracted, 0U);
    EXPECT_EQ(diagnostics.primitiveDrawCallsRecorded, 0U);
}

TEST_F(Renderer3DUVETest, RenderFrameUVE_VisiblePrimitive_ReportsEvidenceSpecificDiagnostics) {
    const Scene::EntityUVE cameraEntity = MakeCameraEntityUVE();
    MakePrimitiveEntityUVE(Math::Vector3UVE{0.0F, 0.0F, -10.0F},
                           Scene::PrimitiveMeshComponentUVE{Scene::PrimitiveMeshKindUVE::Cube,
                                                            Math::Vector3UVE{0.75F, 0.2F, 0.1F}});

    renderer3D->RenderFrameUVE(entityManager, cameraEntity);
    const Renderer3DFrameDiagnosticsUVE beforeProgramReady = renderer3D->GetLastFrameDiagnosticsUVE();
    EXPECT_EQ(beforeProgramReady.primitiveCandidates, 1U);
    EXPECT_EQ(beforeProgramReady.primitiveItemsExtracted, 1U);
    EXPECT_EQ(beforeProgramReady.primitiveDrawCallsRecorded, 0U);
    EXPECT_FALSE(beforeProgramReady.primitiveProgramReady);
    EXPECT_TRUE(beforeProgramReady.mainPassRecorded);
    EXPECT_FALSE(beforeProgramReady.toneMappingPassRecorded);
    EXPECT_EQ(beforeProgramReady.glDrawCallsIssued, 0U);

    for (int iteration = 0; iteration < kMaxPollIterationsUVE; ++iteration) {
        shaderManager.UpdateUVE(0.0);
        if (shaderManager.GetPendingJobCountUVE() == 0U) {
            break;
        }
        std::this_thread::yield();
    }
    ASSERT_EQ(shaderManager.GetPendingJobCountUVE(), 0U);

    EditorViewportVisualStateUVE editorVisualState;
    editorVisualState.enabled = true;
    editorVisualState.viewportMinX = 0.10F;
    editorVisualState.viewportMinY = 0.10F;
    editorVisualState.viewportMaxX = 0.90F;
    editorVisualState.viewportMaxY = 0.90F;
    editorVisualState.activeSelectionVisible = true;
    editorVisualState.selectionMinX = 0.25F;
    editorVisualState.selectionMinY = 0.30F;
    editorVisualState.selectionMaxX = 0.65F;
    editorVisualState.selectionMaxY = 0.70F;
    editorVisualState.activeGizmoAxis = 3;
    editorVisualState.cameraForward = Math::Vector3UVE{0.0F, 0.0F, -1.0F};
    renderer3D->SetEditorViewportVisualStateUVE(editorVisualState);
    renderer3D->RenderFrameUVE(entityManager, cameraEntity);
    const Renderer3DFrameDiagnosticsUVE afterProgramReady = renderer3D->GetLastFrameDiagnosticsUVE();
    EXPECT_EQ(afterProgramReady.primitiveCandidates, 1U);
    EXPECT_EQ(afterProgramReady.primitiveItemsExtracted, 1U);
    EXPECT_EQ(afterProgramReady.primitiveDrawCallsRecorded, 1U);
    EXPECT_TRUE(afterProgramReady.primitiveProgramReady);
    EXPECT_TRUE(afterProgramReady.mainPassRecorded);
    EXPECT_TRUE(afterProgramReady.toneMappingProgramReady);
    EXPECT_TRUE(afterProgramReady.toneMappingPassRecorded);
    EXPECT_TRUE(afterProgramReady.editorVisualProgramReady);
    EXPECT_TRUE(afterProgramReady.editorVisualPassRecorded);
    EXPECT_EQ(afterProgramReady.glDrawCallsIssued, 0U);

    bool selectionUniformPublished = false;
    bool gizmoAxisUniformPublished = false;
    bool cameraForwardUniformPublished = false;
    for (const RecordedCommandUVE& command : renderDevice.GetLastSubmittedCommandsUVE()) {
        if (const auto* integerUniform = std::get_if<SetUniformIntCommandUVE>(&command)) {
            if (integerUniform->name == "uSelectionVisible") {
                selectionUniformPublished = integerUniform->value == 1;
            } else if (integerUniform->name == "uActiveGizmoAxis") {
                gizmoAxisUniformPublished = integerUniform->value == 3;
            }
        } else if (const auto* vectorUniform = std::get_if<SetUniformVector3CommandUVE>(&command)) {
            if (vectorUniform->name == "uCameraForward") {
                cameraForwardUniformPublished = vectorUniform->value.x == 0.0F &&
                                               vectorUniform->value.y == 0.0F &&
                                               vectorUniform->value.z == -1.0F;
            }
        }
    }
    EXPECT_TRUE(selectionUniformPublished);
    EXPECT_TRUE(gizmoAxisUniformPublished);
    EXPECT_TRUE(cameraForwardUniformPublished);
}

TEST_F(Renderer3DUVETest, RenderFrameUVE_VisibleMesh_RecordsExpectedCommandSequence) {
    const Scene::EntityUVE cameraEntity = MakeCameraEntityUVE();
    const Asset::AssetGuidUVE meshGuid = assetDatabase.RegisterUVE("renderer3d_tests_mesh.uvemodel");
    const Asset::AssetGuidUVE materialGuid = assetDatabase.RegisterUVE("renderer3d_tests_material.uvemat");
    MakeMeshEntityUVE(Math::Vector3UVE{0.0F, 0.0F, -10.0F}, meshGuid, materialGuid);
    WaitUntilAssetsReadyUVE(meshGuid, materialGuid);

    renderer3D->RenderFrameUVE(entityManager, cameraEntity);

    const std::vector<RecordedCommandUVE>& commands = renderDevice.GetLastSubmittedCommandsUVE();
    // ShaderProgramUVE flushes its pending uniforms from an unordered cache, so insertion order is
    // intentionally not a renderer contract. Assert named effects and pass structure instead.
    ASSERT_GE(commands.size(), 2U);
    ASSERT_TRUE(std::holds_alternative<BeginRenderPassCommandUVE>(commands[0U]));
    EXPECT_NE(std::get<BeginRenderPassCommandUVE>(commands[0U]).desc.colorAttachment, kInvalidTextureHandleUVE);
    EXPECT_TRUE(std::any_of(commands.cbegin(), commands.cend(), [](const RecordedCommandUVE& command) {
        return std::holds_alternative<BindPipelineCommandUVE>(command);
    }));
    const auto findMatrix = [&commands](const std::string& name) {
        return std::find_if(commands.cbegin(), commands.cend(), [&name](const RecordedCommandUVE& command) {
            return std::holds_alternative<SetUniformMatrix4x4CommandUVE>(command) &&
                   std::get<SetUniformMatrix4x4CommandUVE>(command).name == name;
        });
    };
    const auto findInt = [&commands](const std::string& name) {
        return std::find_if(commands.cbegin(), commands.cend(), [&name](const RecordedCommandUVE& command) {
            return std::holds_alternative<SetUniformIntCommandUVE>(command) &&
                   std::get<SetUniformIntCommandUVE>(command).name == name;
        });
    };
    const auto findVector = [&commands](const std::string& name) {
        return std::find_if(commands.cbegin(), commands.cend(), [&name](const RecordedCommandUVE& command) {
            return std::holds_alternative<SetUniformVector3CommandUVE>(command) &&
                   std::get<SetUniformVector3CommandUVE>(command).name == name;
        });
    };
    const auto model = findMatrix("uModel");
    const auto viewProjection = findMatrix("uViewProjection");
    const auto cascadeCount = findInt("uShadowCascadeCount");
    const auto pcfRadius = findInt("uShadowPcfKernelRadius");
    const auto ambient = findVector("uAmbientColor");
    const auto viewPosition = findVector("uViewPosition");
    ASSERT_NE(model, commands.cend());
    ASSERT_NE(viewProjection, commands.cend());
    ASSERT_NE(cascadeCount, commands.cend());
    ASSERT_NE(pcfRadius, commands.cend());
    ASSERT_NE(ambient, commands.cend());
    ASSERT_NE(viewPosition, commands.cend());
    EXPECT_EQ(std::get<SetUniformIntCommandUVE>(*cascadeCount).value, 0);
    EXPECT_EQ(std::get<SetUniformIntCommandUVE>(*pcfRadius).value, static_cast<std::int32_t>(kTestShadowPcfKernelRadiusUVE));
    EXPECT_EQ(std::get<SetUniformVector3CommandUVE>(*ambient).value, kTestAmbientColorUVE);
    EXPECT_EQ(std::get<SetUniformVector3CommandUVE>(*viewPosition).value, (Math::Vector3UVE{0.0F, 0.0F, 0.0F}));
    EXPECT_TRUE(std::any_of(commands.cbegin(), commands.cend(), [](const RecordedCommandUVE& command) {
        return std::holds_alternative<DrawIndexedCommandUVE>(command) &&
               std::get<DrawIndexedCommandUVE>(command).indexCount == 3U;
    }));
    EXPECT_TRUE(std::holds_alternative<EndRenderPassCommandUVE>(commands.back()));
}

TEST_F(Renderer3DUVETest, RenderFrameUVE_ManagedMaterialProgram_SkipsUntilReadyThenDraws) {
    const Scene::EntityUVE cameraEntity = MakeCameraEntityUVE();
    const Asset::AssetGuidUVE meshGuid = assetDatabase.RegisterUVE("renderer3d_managed_program_mesh.uvemodel");
    const Asset::AssetGuidUVE materialGuid = assetDatabase.RegisterUVE("renderer3d_managed_program_material.uvemat");
    MakeMeshEntityUVE(Math::Vector3UVE{0.0F, 0.0F, -10.0F}, meshGuid, materialGuid);
    WaitUntilAssetsReadyUVE(meshGuid, materialGuid, false);

    renderer3D->RenderFrameUVE(entityManager, cameraEntity);
    const std::vector<RecordedCommandUVE>& commandsWhileLinking = renderDevice.GetLastSubmittedCommandsUVE();
    EXPECT_FALSE(std::any_of(commandsWhileLinking.cbegin(), commandsWhileLinking.cend(),
                             [](const RecordedCommandUVE& command) {
                                 return std::holds_alternative<DrawIndexedCommandUVE>(command);
                             }));

    for (int iteration = 0; iteration < kMaxPollIterationsUVE; ++iteration) {
        shaderManager.UpdateUVE(0.0);
        if (shaderManager.GetPendingJobCountUVE() == 0U) {
            break;
        }
        std::this_thread::yield();
    }
    ASSERT_EQ(shaderManager.GetPendingJobCountUVE(), 0U);

    renderer3D->RenderFrameUVE(entityManager, cameraEntity);
    const std::vector<RecordedCommandUVE>& commandsWhenReady = renderDevice.GetLastSubmittedCommandsUVE();
    EXPECT_TRUE(std::any_of(commandsWhenReady.cbegin(), commandsWhenReady.cend(), [](const RecordedCommandUVE& command) {
        return std::holds_alternative<DrawIndexedCommandUVE>(command);
    }));
}

TEST_F(Renderer3DUVETest, RenderFrameUVE_ShadowPcfKernelRadiusAboveTwo_ClampsToTwo) {
    const Scene::EntityUVE cameraEntity = MakeCameraEntityUVE();
    const Asset::AssetGuidUVE meshGuid = assetDatabase.RegisterUVE("renderer3d_pcf_clamp_mesh.uvemodel");
    const Asset::AssetGuidUVE materialGuid = assetDatabase.RegisterUVE("renderer3d_pcf_clamp_material.uvemat");
    MakeMeshEntityUVE(Math::Vector3UVE{0.0F, 0.0F, -10.0F}, meshGuid, materialGuid);
    WaitUntilAssetsReadyUVE(meshGuid, materialGuid, false);

    Renderer3DUVE clampedRenderer{renderDevice, renderSystem, meshRenderer, cameraSystem, lightSystem,
                                  shaderManager, assetManager, assetDatabase, eventSystem, kTargetWidthUVE,
                                  kTargetHeightUVE, kTestAmbientColorUVE, kTestShadowMapResolutionUVE,
                                  kTestShadowMapHalfExtentUVE, kTestShadowMapNearPlaneUVE,
                                  kTestShadowMapFarPlaneUVE, kTestShadowFrustumPaddingUVE,
                                  kTestShadowCascadeSplitLambdaUVE, kTestShadowCascadeBlendRatioUVE, 3U};
    PrimeMaterialProgramUVE(clampedRenderer, cameraEntity);
    clampedRenderer.RenderFrameUVE(entityManager, cameraEntity);

    const std::vector<RecordedCommandUVE>& commands = renderDevice.GetLastSubmittedCommandsUVE();
    const auto radiusCommand = std::find_if(commands.cbegin(), commands.cend(), [](const RecordedCommandUVE& command) {
        if (!std::holds_alternative<SetUniformIntCommandUVE>(command)) {
            return false;
        }
        return std::get<SetUniformIntCommandUVE>(command).name == "uShadowPcfKernelRadius";
    });
    ASSERT_NE(radiusCommand, commands.cend());
    EXPECT_EQ(std::get<SetUniformIntCommandUVE>(*radiusCommand).value, 2);
}

TEST_F(Renderer3DUVETest, RenderFrameUVE_ShadowCascadeBlendRatioAboveQuarter_ClampsToQuarter) {
    const Scene::EntityUVE cameraEntity = MakeCameraEntityUVE();
    const Asset::AssetGuidUVE meshGuid = assetDatabase.RegisterUVE("renderer3d_blend_clamp_mesh.uvemodel");
    const Asset::AssetGuidUVE materialGuid = assetDatabase.RegisterUVE("renderer3d_blend_clamp_material.uvemat");
    MakeMeshEntityUVE(Math::Vector3UVE{0.0F, 0.0F, -10.0F}, meshGuid, materialGuid);
    WaitUntilAssetsReadyUVE(meshGuid, materialGuid, false);

    Renderer3DUVE clampedRenderer{renderDevice, renderSystem, meshRenderer, cameraSystem, lightSystem,
                                  shaderManager, assetManager, assetDatabase, eventSystem, kTargetWidthUVE,
                                  kTargetHeightUVE, kTestAmbientColorUVE, kTestShadowMapResolutionUVE,
                                  kTestShadowMapHalfExtentUVE, kTestShadowMapNearPlaneUVE,
                                  kTestShadowMapFarPlaneUVE, kTestShadowFrustumPaddingUVE,
                                  kTestShadowCascadeSplitLambdaUVE, 0.5F, kTestShadowPcfKernelRadiusUVE};
    PrimeMaterialProgramUVE(clampedRenderer, cameraEntity);
    clampedRenderer.RenderFrameUVE(entityManager, cameraEntity);

    const std::vector<RecordedCommandUVE>& commands = renderDevice.GetLastSubmittedCommandsUVE();
    const auto blendCommand = std::find_if(commands.cbegin(), commands.cend(), [](const RecordedCommandUVE& command) {
        if (!std::holds_alternative<SetUniformFloatCommandUVE>(command)) {
            return false;
        }
        return std::get<SetUniformFloatCommandUVE>(command).name == "uShadowCascadeBlendRatio";
    });
    ASSERT_NE(blendCommand, commands.cend());
    EXPECT_FLOAT_EQ(std::get<SetUniformFloatCommandUVE>(*blendCommand).value, 0.25F);
}

TEST_F(Renderer3DUVETest, RenderFrameUVE_ActiveLightEntity_PushesComputedLightUniformsInSlotZero) {
    const Scene::EntityUVE cameraEntity = MakeCameraEntityUVE();
    const Asset::AssetGuidUVE meshGuid = assetDatabase.RegisterUVE("renderer3d_tests_lit_mesh.uvemodel");
    const Asset::AssetGuidUVE materialGuid = assetDatabase.RegisterUVE("renderer3d_tests_lit_material.uvemat");
    MakeMeshEntityUVE(Math::Vector3UVE{0.0F, 0.0F, -10.0F}, meshGuid, materialGuid);
    // Identity rotation, so LightSystemUVE derives direction {0,0,-1} (see
    // LightSystemUVETest's own RotateVectorUVE-based coverage for the rotated case). A Point
    // light never casts a shadow (Increment 26 scopes that to Directional only), so this test's
    // shadow pass stays empty regardless.
    Scene::LightComponentUVE light{Math::Vector3UVE{0.9F, 0.8F, 0.7F}, 4.5F};
    light.type = Scene::LightTypeUVE::Point;
    light.range = 22.0F;
    MakeLightEntityUVE(light);
    WaitUntilAssetsReadyUVE(meshGuid, materialGuid);

    renderer3D->RenderFrameUVE(entityManager, cameraEntity);

    const std::vector<RecordedCommandUVE>& commands = renderDevice.GetLastSubmittedCommandsUVE();
    const auto typeUniform = std::find_if(commands.cbegin(), commands.cend(), [](const RecordedCommandUVE& command) {
        return std::holds_alternative<SetUniformIntCommandUVE>(command) &&
               std::get<SetUniformIntCommandUVE>(command).name == "uLights[0].type";
    });
    ASSERT_NE(typeUniform, commands.cend());
    EXPECT_EQ(std::get<SetUniformIntCommandUVE>(*typeUniform).value, 1); // Point
}

TEST_F(Renderer3DUVETest, RenderFrameUVE_TwoLightsOfDifferentTypes_PopulateSlotsZeroAndOneOthersStaySentinel) {
    const Scene::EntityUVE cameraEntity = MakeCameraEntityUVE();
    const Asset::AssetGuidUVE meshGuid = assetDatabase.RegisterUVE("renderer3d_tests_multilight_mesh.uvemodel");
    const Asset::AssetGuidUVE materialGuid = assetDatabase.RegisterUVE("renderer3d_tests_multilight_material.uvemat");
    MakeMeshEntityUVE(Math::Vector3UVE{0.0F, 0.0F, -10.0F}, meshGuid, materialGuid);
    Scene::LightComponentUVE directionalLight{Math::Vector3UVE{1.0F, 0.0F, 0.0F}, 2.0F};
    Scene::LightComponentUVE spotLight{Math::Vector3UVE{0.0F, 1.0F, 0.0F}, 3.0F};
    spotLight.type = Scene::LightTypeUVE::Spot;
    spotLight.spotAngleDegrees = 15.0F;
    MakeLightEntityUVE(directionalLight);
    MakeLightEntityUVE(spotLight);
    WaitUntilAssetsReadyUVE(meshGuid, materialGuid);

    renderer3D->RenderFrameUVE(entityManager, cameraEntity);

    const std::vector<RecordedCommandUVE>& commands = renderDevice.GetLastSubmittedCommandsUVE();
    // Slot 0: directional light.
    const auto directionalType = std::find_if(commands.cbegin(), commands.cend(), [](const RecordedCommandUVE& command) {
        return std::holds_alternative<SetUniformIntCommandUVE>(command) &&
               std::get<SetUniformIntCommandUVE>(command).name == "uLights[0].type";
    });
    ASSERT_NE(directionalType, commands.cend());
    EXPECT_EQ(std::get<SetUniformIntCommandUVE>(*directionalType).value, 0); // Directional
    const auto cascadeCount = std::find_if(commands.cbegin(), commands.cend(), [](const RecordedCommandUVE& command) {
        return std::holds_alternative<SetUniformIntCommandUVE>(command) &&
               std::get<SetUniformIntCommandUVE>(command).name == "uShadowCascadeCount";
    });
    ASSERT_NE(cascadeCount, commands.cend());
    EXPECT_EQ(std::get<SetUniformIntCommandUVE>(*cascadeCount).value, 3);
}

TEST_F(Renderer3DUVETest, RenderFrameUVE_AmbientColorFromConstructor_AlwaysPushedRegardlessOfLight) {
    const Scene::EntityUVE cameraEntity = MakeCameraEntityUVE();
    const Asset::AssetGuidUVE meshGuid = assetDatabase.RegisterUVE("renderer3d_tests_ambient_mesh.uvemodel");
    const Asset::AssetGuidUVE materialGuid = assetDatabase.RegisterUVE("renderer3d_tests_ambient_material.uvemat");
    MakeMeshEntityUVE(Math::Vector3UVE{0.0F, 0.0F, -10.0F}, meshGuid, materialGuid);
    WaitUntilAssetsReadyUVE(meshGuid, materialGuid);

    const auto assertAmbientColor = [](const std::vector<RecordedCommandUVE>& commands) {
        const auto ambient = std::find_if(commands.cbegin(), commands.cend(), [](const RecordedCommandUVE& command) {
            return std::holds_alternative<SetUniformVector3CommandUVE>(command) &&
                   std::get<SetUniformVector3CommandUVE>(command).name == "uAmbientColor";
        });
        ASSERT_NE(ambient, commands.cend());
        EXPECT_EQ(std::get<SetUniformVector3CommandUVE>(*ambient).value, kTestAmbientColorUVE);
    };

    renderer3D->RenderFrameUVE(entityManager, cameraEntity);
    assertAmbientColor(renderDevice.GetLastSubmittedCommandsUVE());

    MakeLightEntityUVE(Scene::LightComponentUVE{Math::Vector3UVE{1.0F, 1.0F, 1.0F}, 1.0F});
    renderer3D->RenderFrameUVE(entityManager, cameraEntity);
    assertAmbientColor(renderDevice.GetLastSubmittedCommandsUVE());
}

TEST_F(Renderer3DUVETest, RenderFrameUVE_CameraAtKnownPosition_PushesMatchingViewPositionUniform) {
    // Stays on the same viewing axis as the mesh below (default identity rotation looks down -Z)
    // so the mesh remains inside the frustum — an off-axis camera position would cull it, leaving
    // no recorded item to assert uniforms on.
    const Math::Vector3UVE cameraPosition{0.0F, 0.0F, 5.0F};
    const Scene::EntityUVE cameraEntity = MakeCameraEntityUVE(cameraPosition);
    const Asset::AssetGuidUVE meshGuid = assetDatabase.RegisterUVE("renderer3d_tests_viewpos_mesh.uvemodel");
    const Asset::AssetGuidUVE materialGuid = assetDatabase.RegisterUVE("renderer3d_tests_viewpos_material.uvemat");
    MakeMeshEntityUVE(Math::Vector3UVE{0.0F, 0.0F, -10.0F}, meshGuid, materialGuid);
    WaitUntilAssetsReadyUVE(meshGuid, materialGuid);

    renderer3D->RenderFrameUVE(entityManager, cameraEntity);

    const std::vector<RecordedCommandUVE>& commands = renderDevice.GetLastSubmittedCommandsUVE();
    const auto viewPosition = std::find_if(commands.cbegin(), commands.cend(), [](const RecordedCommandUVE& command) {
        return std::holds_alternative<SetUniformVector3CommandUVE>(command) &&
               std::get<SetUniformVector3CommandUVE>(command).name == "uViewPosition";
    });
    ASSERT_NE(viewPosition, commands.cend());
    EXPECT_EQ(std::get<SetUniformVector3CommandUVE>(*viewPosition).value, cameraPosition);
}

TEST_F(Renderer3DUVETest, RenderFrameUVE_MaterialWithoutTextures_UsesFallbackTexturesForAllThreeSlots) {
    const Scene::EntityUVE cameraEntity = MakeCameraEntityUVE();
    const Asset::AssetGuidUVE meshGuid = assetDatabase.RegisterUVE("renderer3d_tests_fallback_mesh.uvemodel");
    const Asset::AssetGuidUVE materialAGuid = assetDatabase.RegisterUVE("renderer3d_tests_fallback_material_a.uvemat");
    const Asset::AssetGuidUVE materialBGuid = assetDatabase.RegisterUVE("renderer3d_tests_fallback_material_b.uvemat");
    // Both materials resolve through the same default (no-texture) loader from the fixture, but
    // are registered under two distinct AssetGuidUVEs, so each gets its own materialCache entry
    // and independently resolves its fallback texture handles.
    MakeMeshEntityUVE(Math::Vector3UVE{-1.0F, 0.0F, -10.0F}, meshGuid, materialAGuid);
    MakeMeshEntityUVE(Math::Vector3UVE{1.0F, 0.0F, -10.0F}, meshGuid, materialBGuid);
    WaitUntilAssetsReadyUVE(meshGuid, materialAGuid);
    WaitUntilAssetsReadyUVE(meshGuid, materialBGuid);

    renderer3D->RenderFrameUVE(entityManager, cameraEntity);

    const std::vector<RecordedCommandUVE>& commands = renderDevice.GetLastSubmittedCommandsUVE();
    // Slot 3 (the shadow map, bound once per item in the main pass regardless of material) is
    // excluded here. The final slot-0 bind is the HDR scene color consumed by the fullscreen
    // tone-mapping pass; the first six remain the two items' material fallback bindings.
    std::vector<BindTextureCommandUVE> textureBinds;
    for (const RecordedCommandUVE& command : commands) {
        if (const auto* const bindTexture = std::get_if<BindTextureCommandUVE>(&command)) {
            if (bindTexture->slot < 3U) {
                textureBinds.push_back(*bindTexture);
            }
        }
    }
    ASSERT_EQ(textureBinds.size(), 7U); // 2 items x 3 material texture slots, then tone-map source

    // Group by slot: item1's slot-N handle must equal item2's slot-N handle (fallback reuse
    // across two independently-resolved materials), and the albedo/AO slots (both default to the
    // white fallback) must share a handle distinct from the normal slot's flat-normal fallback.
    EXPECT_EQ(textureBinds[0].texture, textureBinds[3].texture); // albedo, item1 vs item2
    EXPECT_EQ(textureBinds[1].texture, textureBinds[4].texture); // normal, item1 vs item2
    EXPECT_EQ(textureBinds[2].texture, textureBinds[5].texture); // ao, item1 vs item2
    EXPECT_EQ(textureBinds[0].texture, textureBinds[2].texture); // albedo == ao (both white fallback)
    EXPECT_NE(textureBinds[0].texture, textureBinds[1].texture); // albedo != normal (different fallback)
    EXPECT_NE(textureBinds[0].texture, kInvalidTextureHandleUVE);
    EXPECT_NE(textureBinds[1].texture, kInvalidTextureHandleUVE);
}

TEST_F(Renderer3DUVETest, RenderFrameUVE_MaterialWithAlbedoTexture_UploadsAndBindsRealTexture) {
    const std::size_t baselineLiveResources = renderDevice.GetLiveResourceCountUVE();

    const Scene::EntityUVE cameraEntity = MakeCameraEntityUVE();
    const Asset::AssetGuidUVE meshGuid = assetDatabase.RegisterUVE("renderer3d_tests_textured_mesh.uvemodel");
    const Asset::AssetGuidUVE materialGuid = assetDatabase.RegisterUVE("renderer3d_tests_textured_material.uvemat");
    const Asset::AssetGuidUVE textureGuid = assetDatabase.RegisterUVE("renderer3d_tests_albedo.uvetex");
    UseAlbedoTextureInMaterialUVE(textureGuid);

    MakeMeshEntityUVE(Math::Vector3UVE{0.0F, 0.0F, -10.0F}, meshGuid, materialGuid);
    WaitUntilAssetsReadyUVE(meshGuid, materialGuid);
    WaitUntilTextureReadyUVE(textureGuid);
    PrimeMaterialProgramUVE(*renderer3D, cameraEntity);

    renderer3D->RenderFrameUVE(entityManager, cameraEntity);
    const std::size_t liveResourcesAfterFirstFrame = renderDevice.GetLiveResourceCountUVE();
    // The manager releases its temporary stage objects after linking; persistent renderer work is
    // mesh buffers, the managed pipeline, and the uploaded material texture.
    EXPECT_GT(liveResourcesAfterFirstFrame, baselineLiveResources);

    renderer3D->RenderFrameUVE(entityManager, cameraEntity);
    EXPECT_EQ(renderDevice.GetLiveResourceCountUVE(), liveResourcesAfterFirstFrame);
}

TEST_F(Renderer3DUVETest, RenderFrameUVE_Rgba16FloatAlbedoTexture_UploadsSuccessfully) {
    const std::size_t baselineLiveResources = renderDevice.GetLiveResourceCountUVE();

    const Scene::EntityUVE cameraEntity = MakeCameraEntityUVE();
    const Asset::AssetGuidUVE meshGuid = assetDatabase.RegisterUVE("renderer3d_tests_f16_mesh.uvemodel");
    const Asset::AssetGuidUVE materialGuid = assetDatabase.RegisterUVE("renderer3d_tests_f16_material.uvemat");
    const Asset::AssetGuidUVE textureGuid = assetDatabase.RegisterUVE("renderer3d_tests_f16_albedo.uvetex");
    UseAlbedoTextureInMaterialUVE(textureGuid);
    assetManager.RegisterLoaderUVE<Asset::TextureAssetUVE>(
        [](const std::filesystem::path&, Asset::TextureAssetUVE& texture) {
            texture.width = 2;
            texture.height = 2;
            texture.format = Asset::TextureFormatUVE::RGBA16Float;
            texture.pixels.assign(2U * 2U * 8U, std::byte{0});
            return true;
        });

    MakeMeshEntityUVE(Math::Vector3UVE{0.0F, 0.0F, -10.0F}, meshGuid, materialGuid);
    WaitUntilAssetsReadyUVE(meshGuid, materialGuid);
    WaitUntilTextureReadyUVE(textureGuid);
    PrimeMaterialProgramUVE(*renderer3D, cameraEntity);

    renderer3D->RenderFrameUVE(entityManager, cameraEntity);
    EXPECT_GT(renderDevice.GetLiveResourceCountUVE(), baselineLiveResources);

    const std::vector<RecordedCommandUVE>& commands = renderDevice.GetLastSubmittedCommandsUVE();
    EXPECT_TRUE(std::any_of(commands.cbegin(), commands.cend(), [](const RecordedCommandUVE& command) {
        return std::holds_alternative<DrawIndexedCommandUVE>(command);
    }));
}

TEST_F(Renderer3DUVETest, RenderFrameUVE_FailedTextureUsesFallbackAndReportsDiagnostic) {
    const Scene::EntityUVE cameraEntity = MakeCameraEntityUVE();
    const Asset::AssetGuidUVE meshGuid = assetDatabase.RegisterUVE("renderer3d_tests_failed_texture_mesh.uvemodel");
    const Asset::AssetGuidUVE materialGuid = assetDatabase.RegisterUVE("renderer3d_tests_failed_texture_material.uvemat");
    const Asset::AssetGuidUVE textureGuid = assetDatabase.RegisterUVE("renderer3d_tests_failed_texture.uvetex");
    UseAlbedoTextureInMaterialUVE(textureGuid);
    assetManager.RegisterLoaderUVE<Asset::TextureAssetUVE>(
        [](const std::filesystem::path&, Asset::TextureAssetUVE&) { return false; });

    MakeMeshEntityUVE(Math::Vector3UVE{0.0F, 0.0F, -10.0F}, meshGuid, materialGuid);
    WaitUntilAssetsReadyUVE(meshGuid, materialGuid);
    Asset::AssetHandleUVE<Asset::TextureAssetUVE> textureHandle =
        assetManager.LoadUVE<Asset::TextureAssetUVE>(textureGuid, assetDatabase);
    for (int iteration = 0; iteration < kMaxPollIterationsUVE && !textureHandle.HasFailedUVE(); ++iteration) {
        std::this_thread::yield();
    }
    ASSERT_TRUE(textureHandle.HasFailedUVE());

    renderer3D->RenderFrameUVE(entityManager, cameraEntity);
    const Renderer3DFrameDiagnosticsUVE diagnostics = renderer3D->GetLastFrameDiagnosticsUVE();
    EXPECT_EQ(diagnostics.failedAssetLoads, 0U);
    EXPECT_EQ(diagnostics.textureFallbacks, 1U);

    // Rebuilding the material cache must not retry or repeatedly report the same permanent
    // first-load failure; an explicit texture reload is the only event that clears the memo.
    eventSystem.Publish(Asset::AssetReloadedEventUVE{materialGuid});
    renderer3D->RenderFrameUVE(entityManager, cameraEntity);
    const Renderer3DFrameDiagnosticsUVE afterMaterialEviction = renderer3D->GetLastFrameDiagnosticsUVE();
    EXPECT_EQ(afterMaterialEviction.failedAssetLoads, 0U);
    EXPECT_EQ(afterMaterialEviction.textureFallbacks, 1U);
}

TEST_F(Renderer3DUVETest, RenderFrameUVE_TextureAssetNotYetReady_SkipsItemUntilLoaded) {
    const Scene::EntityUVE cameraEntity = MakeCameraEntityUVE();
    const Asset::AssetGuidUVE meshGuid = assetDatabase.RegisterUVE("renderer3d_tests_pending_mesh.uvemodel");
    const Asset::AssetGuidUVE materialGuid = assetDatabase.RegisterUVE("renderer3d_tests_pending_material.uvemat");
    const Asset::AssetGuidUVE textureGuid = assetDatabase.RegisterUVE("renderer3d_tests_pending_albedo.uvetex");
    UseAlbedoTextureInMaterialUVE(textureGuid);

    std::atomic<bool> textureLoadGateUVE{false};
    assetManager.RegisterLoaderUVE<Asset::TextureAssetUVE>(
        [&textureLoadGateUVE](const std::filesystem::path&, Asset::TextureAssetUVE& texture) {
            while (!textureLoadGateUVE.load()) {
                std::this_thread::yield();
            }
            texture.width = 2;
            texture.height = 2;
            texture.format = Asset::TextureFormatUVE::RGBA8Unorm;
            texture.pixels.assign(2U * 2U * 4U, std::byte{0xCD});
            return true;
        });

    MakeMeshEntityUVE(Math::Vector3UVE{0.0F, 0.0F, -10.0F}, meshGuid, materialGuid);
    WaitUntilAssetsReadyUVE(meshGuid, materialGuid);

    // The texture load kicked off inside this call is blocked on textureLoadGateUVE, so the item
    // must be skipped this frame. With no directional light, the optimized renderer records no
    // shadow passes: the empty main pass is followed by the independent tone-mapping output pass.
    renderer3D->RenderFrameUVE(entityManager, cameraEntity);
    const std::vector<RecordedCommandUVE> commandsBeforeReady = renderDevice.GetLastSubmittedCommandsUVE();
    // Release the loader before assertions so any failed expectation cannot strand its worker thread.
    textureLoadGateUVE = true;
    ASSERT_EQ(commandsBeforeReady.size(), 8U);
    EXPECT_TRUE(std::holds_alternative<BeginRenderPassCommandUVE>(commandsBeforeReady[0U]));
    EXPECT_TRUE(std::holds_alternative<EndRenderPassCommandUVE>(commandsBeforeReady[1U]));
    EXPECT_TRUE(std::holds_alternative<BeginRenderPassCommandUVE>(commandsBeforeReady[2U]));
    EXPECT_TRUE(std::holds_alternative<DrawCommandUVE>(commandsBeforeReady[6U]));
    EXPECT_TRUE(std::holds_alternative<EndRenderPassCommandUVE>(commandsBeforeReady[7U]));


    WaitUntilTextureReadyUVE(textureGuid);
    PrimeMaterialProgramUVE(*renderer3D, cameraEntity);

    renderer3D->RenderFrameUVE(entityManager, cameraEntity);
    const std::vector<RecordedCommandUVE>& commandsAfterReady = renderDevice.GetLastSubmittedCommandsUVE();
    EXPECT_TRUE(std::any_of(commandsAfterReady.cbegin(), commandsAfterReady.cend(), [](const RecordedCommandUVE& command) {
        return std::holds_alternative<DrawIndexedCommandUVE>(command);
    }));
}

TEST_F(Renderer3DUVETest, RenderFrameUVE_CalledTwiceWithSameScene_ReusesGpuResourceCache) {
    const Scene::EntityUVE cameraEntity = MakeCameraEntityUVE();
    const Asset::AssetGuidUVE meshGuid = assetDatabase.RegisterUVE("renderer3d_tests_reuse_mesh.uvemodel");
    const Asset::AssetGuidUVE materialGuid = assetDatabase.RegisterUVE("renderer3d_tests_reuse_material.uvemat");
    MakeMeshEntityUVE(Math::Vector3UVE{0.0F, 0.0F, -10.0F}, meshGuid, materialGuid);
    WaitUntilAssetsReadyUVE(meshGuid, materialGuid);

    renderer3D->RenderFrameUVE(entityManager, cameraEntity);
    const std::size_t liveResourcesAfterFirstFrame = renderDevice.GetLiveResourceCountUVE();

    renderer3D->RenderFrameUVE(entityManager, cameraEntity);
    const std::size_t liveResourcesAfterSecondFrame = renderDevice.GetLiveResourceCountUVE();

    EXPECT_EQ(liveResourcesAfterSecondFrame, liveResourcesAfterFirstFrame);
}

TEST_F(Renderer3DUVETest, AssetReloadedEventUVE_ForCachedMesh_EvictsAndRecreatesGpuResources) {
    const Scene::EntityUVE cameraEntity = MakeCameraEntityUVE();
    const Asset::AssetGuidUVE meshGuid = assetDatabase.RegisterUVE("renderer3d_tests_reload_mesh.uvemodel");
    const Asset::AssetGuidUVE materialGuid = assetDatabase.RegisterUVE("renderer3d_tests_reload_material.uvemat");
    MakeMeshEntityUVE(Math::Vector3UVE{0.0F, 0.0F, -10.0F}, meshGuid, materialGuid);
    WaitUntilAssetsReadyUVE(meshGuid, materialGuid);

    renderer3D->RenderFrameUVE(entityManager, cameraEntity);
    const std::size_t liveResourcesBeforeReload = renderDevice.GetLiveResourceCountUVE();

    eventSystem.Publish(Asset::AssetReloadedEventUVE{meshGuid});
    const std::size_t liveResourcesAfterReload = renderDevice.GetLiveResourceCountUVE();
    // The mesh's vertex + index buffer are destroyed on eviction; the material's pipeline (a
    // different GUID) is untouched.
    EXPECT_EQ(liveResourcesAfterReload, liveResourcesBeforeReload - 2U);

    renderer3D->RenderFrameUVE(entityManager, cameraEntity);
    const std::size_t liveResourcesAfterRerender = renderDevice.GetLiveResourceCountUVE();
    EXPECT_EQ(liveResourcesAfterRerender, liveResourcesBeforeReload);
}

TEST_F(Renderer3DUVETest, AssetReloadedEventUVE_ForCachedTexture_EvictsTextureAndInvalidatesMaterialCache) {
    const Scene::EntityUVE cameraEntity = MakeCameraEntityUVE();
    const Asset::AssetGuidUVE meshGuid = assetDatabase.RegisterUVE("renderer3d_tests_texreload_mesh.uvemodel");
    const Asset::AssetGuidUVE materialGuid = assetDatabase.RegisterUVE("renderer3d_tests_texreload_material.uvemat");
    const Asset::AssetGuidUVE textureGuid = assetDatabase.RegisterUVE("renderer3d_tests_texreload_albedo.uvetex");
    UseAlbedoTextureInMaterialUVE(textureGuid);

    MakeMeshEntityUVE(Math::Vector3UVE{0.0F, 0.0F, -10.0F}, meshGuid, materialGuid);
    WaitUntilAssetsReadyUVE(meshGuid, materialGuid);
    WaitUntilTextureReadyUVE(textureGuid);

    renderer3D->RenderFrameUVE(entityManager, cameraEntity);
    const std::size_t liveResourcesBeforeReload = renderDevice.GetLiveResourceCountUVE();

    eventSystem.Publish(Asset::AssetReloadedEventUVE{textureGuid});
    const std::size_t liveResourcesAfterReload = renderDevice.GetLiveResourceCountUVE();
    // Texture eviction is synchronous. The program is manager-owned, so renderer cache eviction
    // only releases its shared reference; the exact retirement point is intentionally internal to
    // ShaderManagerUVE and must not be asserted through renderer resource counts.
    EXPECT_LT(liveResourcesAfterReload, liveResourcesBeforeReload);

    PrimeMaterialProgramUVE(*renderer3D, cameraEntity);
    renderer3D->RenderFrameUVE(entityManager, cameraEntity);
    const std::size_t liveResourcesAfterRerender = renderDevice.GetLiveResourceCountUVE();
    EXPECT_GT(liveResourcesAfterRerender, liveResourcesAfterReload);
}

TEST_F(Renderer3DUVETest, RenderFrameUVE_ParticleRuntimeInput_ExtractsCopiedItemsAndDoesNotLeakAcrossFrames) {
    const Scene::EntityUVE cameraEntity = MakeCameraEntityUVE();
    Scene::ParticleRuntimeUVE particleRuntime;
    const Scene::EntityUVE particleEntity{31U, 1U};
    ASSERT_TRUE(particleRuntime.AttachUVE(particleEntity, Scene::ParticleEmitterComponentUVE{4U}));
    ASSERT_TRUE(particleRuntime.EmitDetailedUVE(
                       particleEntity,
                       Scene::ParticleEmissionUVE{2U, Math::Vector3UVE{0.0F, 0.0F, -4.0F}, {}, 2.0F})
                    .IsAcceptedUVE());

    for (int iteration = 0; iteration < kMaxPollIterationsUVE; ++iteration) {
        EXPECT_NO_FATAL_FAILURE(
            renderer3D->RenderFrameWithParticleRuntimeUVE(entityManager, cameraEntity, particleRuntime));
        if (renderer3D->GetLastFrameDiagnosticsUVE().particleProgramReady) {
            break;
        }
        shaderManager.UpdateUVE(0.0);
    }
    const Renderer3DFrameDiagnosticsUVE particleDiagnostics = renderer3D->GetLastFrameDiagnosticsUVE();
    ASSERT_TRUE(particleDiagnostics.particleProgramReady);
    EXPECT_EQ(particleDiagnostics.particleItemsExtracted, 2U);
    EXPECT_EQ(particleDiagnostics.particleDrawCommandsRecorded, 2U);
    EXPECT_EQ(particleDiagnostics.particleDrawCommandsSubmitted, 2U);
    EXPECT_EQ(particleDiagnostics.particleDrawCallsRecorded, 1U);
    EXPECT_FALSE(particleDiagnostics.particleItemsTruncated);
    EXPECT_FALSE(particleDiagnostics.particleDrawCommandsSubmissionTruncated);

    const std::vector<RecordedCommandUVE>& particleCommands = renderDevice.GetLastSubmittedCommandsUVE();
    const auto particleDraw = std::find_if(particleCommands.cbegin(), particleCommands.cend(), [](const RecordedCommandUVE& command) {
        return std::holds_alternative<DrawCommandUVE>(command) &&
               std::get<DrawCommandUVE>(command).vertexCount == 12U;
    });
    ASSERT_NE(particleDraw, particleCommands.cend());

    EXPECT_NO_FATAL_FAILURE(renderer3D->RenderFrameUVE(entityManager, cameraEntity));
    const Renderer3DFrameDiagnosticsUVE legacyDiagnostics = renderer3D->GetLastFrameDiagnosticsUVE();
    EXPECT_EQ(legacyDiagnostics.particleItemsExtracted, 0U);
    EXPECT_EQ(legacyDiagnostics.particleDrawCommandsRecorded, 0U);
    EXPECT_EQ(legacyDiagnostics.particleDrawCommandsSubmitted, 0U);
    EXPECT_EQ(legacyDiagnostics.particleDrawCallsRecorded, 0U);
    EXPECT_FALSE(legacyDiagnostics.particleItemsTruncated);
}

TEST_F(Renderer3DUVETest, RenderFrameUVE_ActiveCameraPath_MatchesEngineCoreIntegration) {
    // Exercises the exact call shape EngineCoreUVE::Render() uses once SetActiveCameraUVE() has
    // been called: RenderFrameUVE(*entityManager, activeCamera) against a scene with both a
    // camera and a visible mesh, driven end-to-end through real (not fake) collaborators.
    const Scene::EntityUVE cameraEntity = MakeCameraEntityUVE();
    const Asset::AssetGuidUVE meshGuid = assetDatabase.RegisterUVE("renderer3d_tests_active_camera_mesh.uvemodel");
    const Asset::AssetGuidUVE materialGuid = assetDatabase.RegisterUVE("renderer3d_tests_active_camera_material.uvemat");
    MakeMeshEntityUVE(Math::Vector3UVE{0.0F, 0.0F, -10.0F}, meshGuid, materialGuid);
    WaitUntilAssetsReadyUVE(meshGuid, materialGuid, false);

    EXPECT_NO_FATAL_FAILURE(renderer3D->RenderFrameUVE(entityManager, cameraEntity));

    EXPECT_EQ(renderSystem.GetFrameIndexUVE(), 1U);
    const std::vector<RecordedCommandUVE>& commands = renderDevice.GetLastSubmittedCommandsUVE();
    EXPECT_FALSE(commands.empty());
}

TEST_F(Renderer3DUVETest, RenderFrameUVE_DirectionalLight_PushesThreeOrderedCascadeSplits) {
    const Scene::EntityUVE cameraEntity = MakeCameraEntityUVE();
    const Asset::AssetGuidUVE meshGuid = assetDatabase.RegisterUVE("renderer3d_cascade_mesh.uvemodel");
    const Asset::AssetGuidUVE materialGuid = assetDatabase.RegisterUVE("renderer3d_cascade_material.uvemat");
    MakeMeshEntityUVE(Math::Vector3UVE{0.0F, 0.0F, -10.0F}, meshGuid, materialGuid);
    MakeLightEntityUVE(Scene::LightComponentUVE{Math::Vector3UVE{1.0F, 1.0F, 1.0F}, 2.0F});
    WaitUntilAssetsReadyUVE(meshGuid, materialGuid);

    renderer3D->RenderFrameUVE(entityManager, cameraEntity);

    std::array<float, 3> splits{};
    std::array<bool, 3> foundSplits{};
    std::array<bool, 3> foundMatrices{};
    float cascadeBlendRatio = 0.0F;
    bool foundCascadeBlendRatio = false;
    for (const RecordedCommandUVE& command : renderDevice.GetLastSubmittedCommandsUVE()) {
        if (const auto* const uniform = std::get_if<SetUniformFloatCommandUVE>(&command)) {
            if (uniform->name == "uShadowCascadeBlendRatio") {
                cascadeBlendRatio = uniform->value;
                foundCascadeBlendRatio = true;
            }
            for (std::size_t cascadeIndex = 0; cascadeIndex < 3; ++cascadeIndex) {
                if (uniform->name == "uShadowCascadeSplits[" + std::to_string(cascadeIndex) + "]") {
                    splits[cascadeIndex] = uniform->value;
                    foundSplits[cascadeIndex] = true;
                }
            }
        }
        if (const auto* const uniform = std::get_if<SetUniformMatrix4x4CommandUVE>(&command)) {
            for (std::size_t cascadeIndex = 0; cascadeIndex < 3; ++cascadeIndex) {
                if (uniform->name == "uLightSpaceMatrices[" + std::to_string(cascadeIndex) + "]") {
                    foundMatrices[cascadeIndex] = uniform->value != Math::Matrix4x4UVE::IdentityUVE();
                }
            }
        }
    }

    EXPECT_TRUE(foundSplits[0]);
    EXPECT_TRUE(foundSplits[1]);
    EXPECT_TRUE(foundSplits[2]);
    EXPECT_TRUE(foundCascadeBlendRatio);
    EXPECT_FLOAT_EQ(cascadeBlendRatio, kTestShadowCascadeBlendRatioUVE);
    EXPECT_GT(splits[0], 0.0F);
    EXPECT_LT(splits[0], splits[1]);
    EXPECT_LT(splits[1], splits[2]);
    EXPECT_TRUE(foundMatrices[0]);
    EXPECT_TRUE(foundMatrices[1]);
    EXPECT_TRUE(foundMatrices[2]);
}

TEST_F(Renderer3DUVETest, RenderFrameUVE_DirectionalCascadeMatrices_SnapToShadowTexelGrid) {
    const Asset::AssetGuidUVE meshGuid = assetDatabase.RegisterUVE("renderer3d_stabilization_mesh.uvemodel");
    const Asset::AssetGuidUVE materialGuid = assetDatabase.RegisterUVE("renderer3d_stabilization_material.uvemat");
    MakeMeshEntityUVE(Math::Vector3UVE{0.0F, 0.0F, -10.0F}, meshGuid, materialGuid);
    MakeLightEntityUVE(Scene::LightComponentUVE{Math::Vector3UVE{1.0F, 1.0F, 1.0F}, 2.0F});
    WaitUntilAssetsReadyUVE(meshGuid, materialGuid);

    const auto captureCascadeMatrices = [this]() {
        std::array<Math::Matrix4x4UVE, 3> matrices{};
        std::array<bool, 3> foundMatrices{};
        for (const RecordedCommandUVE& command : renderDevice.GetLastSubmittedCommandsUVE()) {
            const auto* const uniform = std::get_if<SetUniformMatrix4x4CommandUVE>(&command);
            if (uniform == nullptr) {
                continue;
            }
            for (std::size_t cascadeIndex = 0; cascadeIndex < matrices.size(); ++cascadeIndex) {
                if (uniform->name == "uLightSpaceMatrices[" + std::to_string(cascadeIndex) + "]") {
                    matrices[cascadeIndex] = uniform->value;
                    foundMatrices[cascadeIndex] = true;
                }
            }
        }
        for (const bool foundMatrix : foundMatrices) {
            EXPECT_TRUE(foundMatrix);
        }
        return matrices;
    };

    const Scene::EntityUVE initialCamera = MakeCameraEntityUVE();
    PrimeMaterialProgramUVE(*renderer3D, initialCamera);
    renderer3D->RenderFrameUVE(entityManager, initialCamera);
    const std::array<Math::Matrix4x4UVE, 3> initialMatrices = captureCascadeMatrices();
    const float cascadeZeroTexelWidth =
        2.0F / (static_cast<float>(kTestShadowMapResolutionUVE) * initialMatrices[0].m[0][0]);
    ASSERT_GT(cascadeZeroTexelWidth, 0.0F);

    const Scene::EntityUVE subTexelCamera =
        MakeCameraEntityUVE(Math::Vector3UVE{cascadeZeroTexelWidth * 0.25F, 0.0F, 0.0F});
    renderer3D->RenderFrameUVE(entityManager, subTexelCamera);
    const std::array<Math::Matrix4x4UVE, 3> subTexelMatrices = captureCascadeMatrices();
    EXPECT_EQ(subTexelMatrices, initialMatrices);

    const Scene::EntityUVE nextTexelCamera =
        MakeCameraEntityUVE(Math::Vector3UVE{cascadeZeroTexelWidth * 1.25F, 0.0F, 0.0F});
    renderer3D->RenderFrameUVE(entityManager, nextTexelCamera);
    const std::array<Math::Matrix4x4UVE, 3> nextTexelMatrices = captureCascadeMatrices();
    EXPECT_NE(nextTexelMatrices[0], initialMatrices[0]);
    EXPECT_FLOAT_EQ(nextTexelMatrices[0].m[0][0], initialMatrices[0].m[0][0]);
    EXPECT_FLOAT_EQ(nextTexelMatrices[0].m[0][3] - initialMatrices[0].m[0][3],
                    -2.0F / static_cast<float>(kTestShadowMapResolutionUVE));
}

TEST_F(Renderer3DUVETest, RenderFrameUVE_NoDirectionalLight_SkipsShadowPassEvenWithVisibleMesh) {
    // A visible, asset-ready mesh exists, and the shadow program has even had the chance to
    // become valid (polled to readiness below) - but with no Directional light entity at all,
    // FindShadowCasterUVE() finds no caster, so no shadow pass is recorded.
    const Scene::EntityUVE cameraEntity = MakeCameraEntityUVE();
    const Asset::AssetGuidUVE meshGuid = assetDatabase.RegisterUVE("renderer3d_tests_noshadow_mesh.uvemodel");
    const Asset::AssetGuidUVE materialGuid = assetDatabase.RegisterUVE("renderer3d_tests_noshadow_material.uvemat");
    MakeMeshEntityUVE(Math::Vector3UVE{0.0F, 0.0F, -10.0F}, meshGuid, materialGuid);
    WaitUntilAssetsReadyUVE(meshGuid, materialGuid);
    WaitUntilShadowProgramReadyUVE();

    renderer3D->RenderFrameUVE(entityManager, cameraEntity);

    const std::vector<RecordedCommandUVE>& commands = renderDevice.GetLastSubmittedCommandsUVE();
    ASSERT_FALSE(commands.empty());
    ASSERT_TRUE(std::holds_alternative<BeginRenderPassCommandUVE>(commands[0U]));
    EXPECT_NE(std::get<BeginRenderPassCommandUVE>(commands[0U]).desc.colorAttachment, kInvalidTextureHandleUVE);
    const auto mainPassEnd = std::find_if(commands.cbegin(), commands.cend(), [](const RecordedCommandUVE& command) {
        return std::holds_alternative<EndRenderPassCommandUVE>(command);
    });
    ASSERT_NE(mainPassEnd, commands.cend());
}

TEST_F(Renderer3DUVETest, RenderFrameUVE_InvalidShadowTargetsSkipShadowPassSafely) {
    const Scene::EntityUVE cameraEntity = MakeCameraEntityUVE();
    MakeLightEntityUVE(Scene::LightComponentUVE{Math::Vector3UVE{1.0F, 1.0F, 1.0F}, 2.0F});

    Renderer3DUVE invalidShadowRenderer(
        renderDevice, renderSystem, meshRenderer, cameraSystem, lightSystem, shaderManager, assetManager,
        assetDatabase, eventSystem, kTargetWidthUVE, kTargetHeightUVE, kTestAmbientColorUVE, 0U,
        kTestShadowMapHalfExtentUVE, kTestShadowMapNearPlaneUVE, kTestShadowMapFarPlaneUVE,
        kTestShadowFrustumPaddingUVE, kTestShadowCascadeSplitLambdaUVE, kTestShadowCascadeBlendRatioUVE,
        kTestShadowPcfKernelRadiusUVE);
    for (int iteration = 0; iteration < kMaxPollIterationsUVE; ++iteration) {
        shaderManager.UpdateUVE(0.0);
        if (shaderManager.GetPendingJobCountUVE() == 0U) {
            break;
        }
        std::this_thread::yield();
    }
    ASSERT_EQ(shaderManager.GetPendingJobCountUVE(), 0U);

    invalidShadowRenderer.RenderFrameUVE(entityManager, cameraEntity);

    const std::vector<RecordedCommandUVE>& commands = renderDevice.GetLastSubmittedCommandsUVE();
    ASSERT_FALSE(commands.empty());
    ASSERT_TRUE(std::holds_alternative<BeginRenderPassCommandUVE>(commands.front()));
    EXPECT_NE(std::get<BeginRenderPassCommandUVE>(commands.front()).desc.colorAttachment,
              kInvalidTextureHandleUVE);
}

TEST_F(Renderer3DUVETest, RenderFrameUVE_InvalidMainTargetsSkipFrameSafely) {
    const Scene::EntityUVE cameraEntity = MakeCameraEntityUVE();
    Renderer3DUVE invalidMainRenderer(
        renderDevice, renderSystem, meshRenderer, cameraSystem, lightSystem, shaderManager, assetManager,
        assetDatabase, eventSystem, 0U, kTargetHeightUVE, kTestAmbientColorUVE, kTestShadowMapResolutionUVE,
        kTestShadowMapHalfExtentUVE, kTestShadowMapNearPlaneUVE, kTestShadowMapFarPlaneUVE,
        kTestShadowFrustumPaddingUVE, kTestShadowCascadeSplitLambdaUVE, kTestShadowCascadeBlendRatioUVE,
        kTestShadowPcfKernelRadiusUVE);

    invalidMainRenderer.RenderFrameUVE(entityManager, cameraEntity);

    const Renderer3DFrameDiagnosticsUVE diagnostics = invalidMainRenderer.GetLastFrameDiagnosticsUVE();
    EXPECT_EQ(diagnostics.meshItemsExtracted, 0U);
    EXPECT_EQ(diagnostics.primitiveItemsExtracted, 0U);
    EXPECT_FALSE(diagnostics.mainPassRecorded);
    EXPECT_FALSE(diagnostics.toneMappingPassRecorded);
}

TEST_F(Renderer3DUVETest, RenderFrameUVE_FittedLightFrustum_CastsOffCameraOccluderWithoutMainPassDraw) {
    const Scene::EntityUVE cameraEntity = MakeCameraEntityUVE();
    const Asset::AssetGuidUVE visibleMeshGuid = assetDatabase.RegisterUVE("renderer3d_fitted_visible.uvemodel");
    const Asset::AssetGuidUVE visibleMaterialGuid = assetDatabase.RegisterUVE("renderer3d_fitted_visible.uvemat");
    const Asset::AssetGuidUVE offCameraMeshGuid = assetDatabase.RegisterUVE("renderer3d_fitted_offcamera.uvemodel");
    const Asset::AssetGuidUVE offCameraMaterialGuid = assetDatabase.RegisterUVE("renderer3d_fitted_offcamera.uvemat");
    MakeMeshEntityUVE(Math::Vector3UVE{0.0F, 0.0F, -10.0F}, visibleMeshGuid, visibleMaterialGuid);
    // At z=-10 the default camera's 60-degree view only reaches roughly +/-5.8 on X, while the
    // fitted directional-light frustum includes this potential caster across its far-range width.
    MakeMeshEntityUVE(Math::Vector3UVE{50.0F, 0.0F, -10.0F}, offCameraMeshGuid, offCameraMaterialGuid);
    MakeLightEntityUVE(Scene::LightComponentUVE{Math::Vector3UVE{1.0F, 1.0F, 1.0F}, 3.0F});
    WaitUntilAssetsReadyUVE(visibleMeshGuid, visibleMaterialGuid);
    WaitUntilAssetsReadyUVE(offCameraMeshGuid, offCameraMaterialGuid);
    WaitUntilShadowProgramReadyUVE();

    renderer3D->RenderFrameUVE(entityManager, cameraEntity);

    const std::vector<RecordedCommandUVE>& commands = renderDevice.GetLastSubmittedCommandsUVE();
    const auto shadowPassEnd = std::find_if(commands.cbegin(), commands.cend(), [](const RecordedCommandUVE& command) {
        return std::holds_alternative<EndRenderPassCommandUVE>(command);
    });
    ASSERT_NE(shadowPassEnd, commands.cend());
    const std::size_t shadowDrawCount = static_cast<std::size_t>(std::count_if(
        commands.cbegin(), shadowPassEnd, [](const RecordedCommandUVE& command) {
            return std::holds_alternative<DrawIndexedCommandUVE>(command);
        }));
    const std::size_t mainDrawCount = static_cast<std::size_t>(std::count_if(
        std::next(shadowPassEnd), commands.cend(), [](const RecordedCommandUVE& command) {
            return std::holds_alternative<DrawIndexedCommandUVE>(command);
        }));

    EXPECT_EQ(shadowDrawCount, 2U);
    EXPECT_EQ(mainDrawCount, 1U);
}

TEST_F(Renderer3DUVETest, RenderFrameUVE_DirectionalLightAndReadyShadowProgram_ShadowPassDrawsOpaqueItem) {
    const Scene::EntityUVE cameraEntity = MakeCameraEntityUVE();
    const Asset::AssetGuidUVE meshGuid = assetDatabase.RegisterUVE("renderer3d_tests_shadowdraw_mesh.uvemodel");
    const Asset::AssetGuidUVE materialGuid = assetDatabase.RegisterUVE("renderer3d_tests_shadowdraw_material.uvemat");
    MakeMeshEntityUVE(Math::Vector3UVE{0.0F, 0.0F, -10.0F}, meshGuid, materialGuid);
    MakeLightEntityUVE(Scene::LightComponentUVE{Math::Vector3UVE{1.0F, 1.0F, 1.0F}, 3.0F});
    WaitUntilAssetsReadyUVE(meshGuid, materialGuid);
    WaitUntilShadowProgramReadyUVE();

    renderer3D->RenderFrameUVE(entityManager, cameraEntity);

    const std::vector<RecordedCommandUVE>& commands = renderDevice.GetLastSubmittedCommandsUVE();
    // ShaderProgramUVE::ApplyToUVE() flushes its pending uniforms from an unordered_map, so
    // uModel/uLightSpaceMatrix can appear in either order - this searches a small window instead
    // of asserting a fixed position for either one individually.
    ASSERT_TRUE(std::holds_alternative<BeginRenderPassCommandUVE>(commands[0]));
    ASSERT_TRUE(std::holds_alternative<BindPipelineCommandUVE>(commands[1]));

    bool foundModelUniform = false;
    bool foundLightSpaceUniform = false;
    for (std::size_t index = 2; index < 4; ++index) {
        ASSERT_TRUE(std::holds_alternative<SetUniformMatrix4x4CommandUVE>(commands[index]));
        const std::string& name = std::get<SetUniformMatrix4x4CommandUVE>(commands[index]).name;
        if (name == "uModel") {
            foundModelUniform = true;
        } else if (name == "uLightSpaceMatrix") {
            foundLightSpaceUniform = true;
            EXPECT_NE(std::get<SetUniformMatrix4x4CommandUVE>(commands[index]).value, Math::Matrix4x4UVE::IdentityUVE());
        }
    }
    EXPECT_TRUE(foundModelUniform);
    EXPECT_TRUE(foundLightSpaceUniform);

    EXPECT_TRUE(std::holds_alternative<BindVertexBufferCommandUVE>(commands[4]));
    EXPECT_TRUE(std::holds_alternative<BindIndexBufferCommandUVE>(commands[5]));
    ASSERT_TRUE(std::holds_alternative<DrawIndexedCommandUVE>(commands[6]));
    EXPECT_EQ(std::get<DrawIndexedCommandUVE>(commands[6]).indexCount, 3U);
    EXPECT_TRUE(std::holds_alternative<EndRenderPassCommandUVE>(commands[7]));

    // The main pass follows immediately after the shadow pass ends.
    EXPECT_TRUE(std::holds_alternative<BeginRenderPassCommandUVE>(commands[8]));
}

} // namespace
} // namespace UVE::Render::Tests
