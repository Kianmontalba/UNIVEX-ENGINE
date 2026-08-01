//------------------------------------------------------------------------------
// UniVex Engine (UVE) — Proprietary Game Engine
// Copyright (c) 2026 UniVex Studios. All Rights Reserved.
// Unauthorized copying, modification, distribution, or use of this code
// in whole or in part is strictly prohibited without express written
// permission from UniVex Studios.
// Violators will be prosecuted to the fullest extent of the law.
//------------------------------------------------------------------------------

#include "uve/render/renderer_3d_uve.h"

#include <cstddef>
#include <span>
#include <unordered_map>
#include <vector>

#include "uve/asset/asset_reloaded_event_uve.h"
#include "uve/asset/material_asset_uve.h"
#include "uve/asset/mesh_asset_uve.h"
#include "uve/asset/shader_asset_uve.h"
#include "uve/math/frustum_uve.h"
#include "uve/math/matrix4x4_uve.h"
#include "uve/render/render_queue_uve.h"

namespace UVE::Render {

namespace {

/// A mesh's uploaded GPU buffers, cached by MeshAssetUVE's AssetGuidUVE.
struct MeshGpuResourcesUVE {
    BufferHandleUVE vertexBuffer;
    BufferHandleUVE indexBuffer;
    std::uint32_t indexCount = 0;
};

/// A material's built pipeline state object, cached by MaterialAssetUVE's AssetGuidUVE. Kept
/// separate from MeshGpuResourcesUVE (rather than one map holding both, as an earlier sketch of
/// this design considered) since a mesh's cache entry and a material's cache entry have
/// unrelated shapes — conflating them under one per-GUID record would leave half of every entry
/// meaningless.
struct MaterialGpuResourcesUVE {
    PipelineHandleUVE pipeline;
};

/// MeshVertexUVE's binary layout (position, normal, u, v — see mesh_asset_uve.h), described once
/// here for CreatePipelineUVE(). MeshVertexUVE is a standard-layout aggregate of Math::Vector3UVE
/// (itself standard-layout) and two floats, so offsetof() is well-defined.
const std::vector<VertexAttributeUVE>& MeshVertexLayoutUVE() {
    static const std::vector<VertexAttributeUVE> layout = {
        VertexAttributeUVE{"POSITION", VertexAttributeFormatUVE::Float3, offsetof(Asset::MeshVertexUVE, position)},
        VertexAttributeUVE{"NORMAL", VertexAttributeFormatUVE::Float3, offsetof(Asset::MeshVertexUVE, normal)},
        VertexAttributeUVE{"TEXCOORD0", VertexAttributeFormatUVE::Float2, offsetof(Asset::MeshVertexUVE, u)},
    };
    return layout;
}

} // namespace

struct Renderer3DUVE::ImplUVE {
    IRenderDeviceUVE& renderDevice;
    IRenderSystemUVE& renderSystem;
    IMeshRendererUVE& meshRenderer;
    ICameraSystemUVE& cameraSystem;
    Asset::IAssetManagerUVE& assetManager;
    Asset::IAssetDatabaseUVE& assetDatabase;
    Events::IEventSystemUVE& eventSystem;
    std::uint32_t targetWidth;
    std::uint32_t targetHeight;
    TextureHandleUVE colorTarget;
    TextureHandleUVE depthTarget;
    std::unordered_map<Asset::AssetGuidUVE, MeshGpuResourcesUVE> meshCache;
    std::unordered_map<Asset::AssetGuidUVE, MaterialGpuResourcesUVE> materialCache;
    Events::EventSubscriptionUVE reloadSubscription;

    ImplUVE(IRenderDeviceUVE& renderDeviceIn, IRenderSystemUVE& renderSystemIn, IMeshRendererUVE& meshRendererIn,
            ICameraSystemUVE& cameraSystemIn, Asset::IAssetManagerUVE& assetManagerIn,
            Asset::IAssetDatabaseUVE& assetDatabaseIn, Events::IEventSystemUVE& eventSystemIn,
            std::uint32_t targetWidthIn, std::uint32_t targetHeightIn)
        : renderDevice(renderDeviceIn), renderSystem(renderSystemIn), meshRenderer(meshRendererIn),
          cameraSystem(cameraSystemIn), assetManager(assetManagerIn), assetDatabase(assetDatabaseIn),
          eventSystem(eventSystemIn), targetWidth(targetWidthIn), targetHeight(targetHeightIn) {}

    void OnAssetReloadedUVE(const Asset::AssetReloadedEventUVE& event) {
        const auto meshIt = meshCache.find(event.guid);
        if (meshIt != meshCache.end()) {
            renderDevice.DestroyBufferUVE(meshIt->second.vertexBuffer);
            renderDevice.DestroyBufferUVE(meshIt->second.indexBuffer);
            meshCache.erase(meshIt);
        }
        const auto materialIt = materialCache.find(event.guid);
        if (materialIt != materialCache.end()) {
            renderDevice.DestroyPipelineUVE(materialIt->second.pipeline);
            materialCache.erase(materialIt);
        }
    }

    /// Returns the cached (creating-if-needed) GPU buffers for `item`'s mesh. `item.meshHandle`
    /// is always ready by construction (MeshRendererUVE::ExtractRenderQueueUVE only includes
    /// asset-ready items), so this never fails.
    [[nodiscard]] const MeshGpuResourcesUVE& ResolveMeshGpuResourcesUVE(const RenderItemUVE& item) {
        const Asset::AssetGuidUVE guid = item.meshHandle.GetGuidUVE();
        const auto existingIt = meshCache.find(guid);
        if (existingIt != meshCache.end()) {
            return existingIt->second;
        }

        const Asset::MeshAssetUVE* const mesh = item.meshHandle.TryGetUVE();
        const std::span<const Asset::MeshVertexUVE> vertexSpan(mesh->vertices);
        const std::span<const std::uint32_t> indexSpan(mesh->indices);
        const std::span<const std::byte> vertexBytes = std::as_bytes(vertexSpan);
        const std::span<const std::byte> indexBytes = std::as_bytes(indexSpan);

        const BufferHandleUVE vertexBuffer =
            renderDevice.CreateBufferUVE(BufferDescUVE{vertexBytes.size(), BufferUsageUVE::Vertex}, vertexBytes);
        const BufferHandleUVE indexBuffer =
            renderDevice.CreateBufferUVE(BufferDescUVE{indexBytes.size(), BufferUsageUVE::Index}, indexBytes);

        const auto insertResult = meshCache.emplace(
            guid, MeshGpuResourcesUVE{vertexBuffer, indexBuffer, static_cast<std::uint32_t>(mesh->indices.size())});
        return insertResult.first->second;
    }

    /// Returns the cached (creating-if-needed) pipeline for `item`'s material, or nullptr if the
    /// material's vertex/fragment ShaderAssetUVE hasn't finished loading yet this frame (silently
    /// skipped, same async-non-blocking convention MeshRendererUVE uses for mesh/material
    /// readiness — it appears once the shaders are ready, no special-casing).
    [[nodiscard]] const MaterialGpuResourcesUVE* ResolveMaterialGpuResourcesUVE(const RenderItemUVE& item) {
        const Asset::AssetGuidUVE guid = item.materialHandle.GetGuidUVE();
        const auto existingIt = materialCache.find(guid);
        if (existingIt != materialCache.end()) {
            return &existingIt->second;
        }

        const Asset::MaterialAssetUVE* const material = item.materialHandle.TryGetUVE();
        Asset::AssetHandleUVE<Asset::ShaderAssetUVE> vertexShaderHandle =
            assetManager.LoadUVE<Asset::ShaderAssetUVE>(material->vertexShader, assetDatabase);
        Asset::AssetHandleUVE<Asset::ShaderAssetUVE> fragmentShaderHandle =
            assetManager.LoadUVE<Asset::ShaderAssetUVE>(material->fragmentShader, assetDatabase);
        if (!vertexShaderHandle.IsReadyUVE() || !fragmentShaderHandle.IsReadyUVE()) {
            return nullptr;
        }

        const Asset::ShaderAssetUVE* const vertexShaderAsset = vertexShaderHandle.TryGetUVE();
        const Asset::ShaderAssetUVE* const fragmentShaderAsset = fragmentShaderHandle.TryGetUVE();
        const ShaderHandleUVE vertexShader = renderDevice.CreateShaderUVE(
            ShaderDescUVE{ShaderStageUVE::Vertex, vertexShaderAsset->sourceCode, vertexShaderAsset->entryPointName});
        const ShaderHandleUVE fragmentShader = renderDevice.CreateShaderUVE(ShaderDescUVE{
            ShaderStageUVE::Fragment, fragmentShaderAsset->sourceCode, fragmentShaderAsset->entryPointName});

        PipelineDescUVE pipelineDesc;
        pipelineDesc.vertexShader = vertexShader;
        pipelineDesc.fragmentShader = fragmentShader;
        pipelineDesc.vertexLayout = MeshVertexLayoutUVE();
        pipelineDesc.depthTestEnabled = true;
        pipelineDesc.depthWriteEnabled = !material->isTransparent;

        const PipelineHandleUVE pipeline = renderDevice.CreatePipelineUVE(pipelineDesc);
        if (pipeline == kInvalidPipelineHandleUVE) {
            return nullptr;
        }

        const auto insertResult = materialCache.emplace(guid, MaterialGpuResourcesUVE{pipeline});
        return &insertResult.first->second;
    }

    void RecordItemsUVE(const std::vector<RenderItemUVE>& items, ICommandBufferUVE& commandBuffer) {
        for (const RenderItemUVE& item : items) {
            const MaterialGpuResourcesUVE* const materialResources = ResolveMaterialGpuResourcesUVE(item);
            if (materialResources == nullptr) {
                continue;
            }
            const MeshGpuResourcesUVE& meshResources = ResolveMeshGpuResourcesUVE(item);

            commandBuffer.BindPipelineUVE(materialResources->pipeline);
            commandBuffer.BindVertexBufferUVE(meshResources.vertexBuffer);
            commandBuffer.BindIndexBufferUVE(meshResources.indexBuffer);
            commandBuffer.DrawIndexedUVE(meshResources.indexCount);
        }
    }
};

Renderer3DUVE::Renderer3DUVE(IRenderDeviceUVE& renderDevice, IRenderSystemUVE& renderSystem,
                              IMeshRendererUVE& meshRenderer, ICameraSystemUVE& cameraSystem,
                              Asset::IAssetManagerUVE& assetManager, Asset::IAssetDatabaseUVE& assetDatabase,
                              Events::IEventSystemUVE& eventSystem, std::uint32_t targetWidth,
                              std::uint32_t targetHeight)
    : m_impl(std::make_unique<ImplUVE>(renderDevice, renderSystem, meshRenderer, cameraSystem, assetManager,
                                        assetDatabase, eventSystem, targetWidth, targetHeight)) {
    m_impl->colorTarget = renderDevice.CreateTextureUVE(
        TextureDescUVE{targetWidth, targetHeight, TextureFormatUVE::RGBA8Unorm, 1});
    m_impl->depthTarget = renderDevice.CreateTextureUVE(
        TextureDescUVE{targetWidth, targetHeight, TextureFormatUVE::Depth32Float, 1});

    ImplUVE* const implPtr = m_impl.get();
    m_impl->reloadSubscription = eventSystem.Subscribe<Asset::AssetReloadedEventUVE>(
        [implPtr](const Asset::AssetReloadedEventUVE& event) { implPtr->OnAssetReloadedUVE(event); });
}

Renderer3DUVE::~Renderer3DUVE() {
    m_impl->eventSystem.Unsubscribe(m_impl->reloadSubscription);
    for (const auto& [guid, meshResources] : m_impl->meshCache) {
        m_impl->renderDevice.DestroyBufferUVE(meshResources.vertexBuffer);
        m_impl->renderDevice.DestroyBufferUVE(meshResources.indexBuffer);
    }
    for (const auto& [guid, materialResources] : m_impl->materialCache) {
        m_impl->renderDevice.DestroyPipelineUVE(materialResources.pipeline);
    }
    m_impl->renderDevice.DestroyTextureUVE(m_impl->colorTarget);
    m_impl->renderDevice.DestroyTextureUVE(m_impl->depthTarget);
}

void Renderer3DUVE::RenderFrameUVE(Scene::IEntityManagerUVE& entityManager, Scene::EntityUVE cameraEntity) {
    const float aspectRatio = static_cast<float>(m_impl->targetWidth) / static_cast<float>(m_impl->targetHeight);
    const Math::Matrix4x4UVE viewProjection =
        m_impl->cameraSystem.ComputeViewProjectionUVE(entityManager, cameraEntity, aspectRatio);
    const Math::FrustumUVE frustum = m_impl->cameraSystem.ExtractFrustumUVE(viewProjection);

    RenderQueueUVE queue =
        m_impl->meshRenderer.ExtractRenderQueueUVE(entityManager, m_impl->assetManager, m_impl->assetDatabase, frustum);
    queue.SortUVE();

    m_impl->renderSystem.BeginFrameUVE();
    ICommandBufferUVE& commandBuffer = m_impl->renderSystem.GetFrameCommandBufferUVE();

    RenderPassDescUVE passDesc;
    passDesc.colorAttachment = m_impl->colorTarget;
    passDesc.depthAttachment = m_impl->depthTarget;
    commandBuffer.BeginRenderPassUVE(passDesc);

    m_impl->RecordItemsUVE(queue.opaqueItems, commandBuffer);
    m_impl->RecordItemsUVE(queue.transparentItems, commandBuffer);

    commandBuffer.EndRenderPassUVE();
    m_impl->renderSystem.EndFrameUVE();
}

} // namespace UVE::Render
