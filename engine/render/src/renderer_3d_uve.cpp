//------------------------------------------------------------------------------
// UniVex Engine (UVE) — Proprietary Game Engine
// Copyright (c) 2026 UniVex Studios. All Rights Reserved.
// Unauthorized copying, modification, distribution, or use of this code
// in whole or in part is strictly prohibited without express written
// permission from UniVex Studios.
// Violators will be prosecuted to the fullest extent of the law.
//------------------------------------------------------------------------------

#include "uve/render/renderer_3d_uve.h"

#include <array>
#include <cstddef>
#include <cstdint>
#include <optional>
#include <span>
#include <unordered_map>
#include <vector>

#include "uve/asset/asset_reloaded_event_uve.h"
#include "uve/asset/material_asset_uve.h"
#include "uve/asset/mesh_asset_uve.h"
#include "uve/asset/shader_asset_uve.h"
#include "uve/asset/texture_asset_uve.h"
#include "uve/debug/assert_uve.h"
#include "uve/debug/logging_macros_uve.h"
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

/// A material's built pipeline state object plus its three resolved texture handles (Increment
/// 22), cached by MaterialAssetUVE's AssetGuidUVE. Kept separate from MeshGpuResourcesUVE (rather
/// than one map holding both, as an earlier sketch of this design considered) since a mesh's cache
/// entry and a material's cache entry have unrelated shapes — conflating them under one per-GUID
/// record would leave half of every entry meaningless. `albedoTexture`/`normalTexture`/
/// `aoTexture` are never kInvalidTextureHandleUVE once cached — an unset MaterialAssetUVE texture
/// GUID resolves to one of Renderer3DUVE's two fallback textures instead (see
/// ResolveTextureGpuHandleUVE()'s doc comment).
struct MaterialGpuResourcesUVE {
    PipelineHandleUVE pipeline;
    TextureHandleUVE albedoTexture;
    TextureHandleUVE normalTexture;
    TextureHandleUVE aoTexture;
};

/// Fixed texture-unit slots RecordItemsUVE() binds every material's three textures to, and the
/// matching sampler uniform names a material's fragment shader is expected to declare (a
/// sampler2D uniform is just an int uniform holding a texture unit index in GL — SetUniformIntUVE
/// is reused for this, no new RHI needed). A material shader that doesn't declare one of these
/// samplers simply never reads the corresponding bind (SetUniformIntUVE's own documented
/// safe-no-op contract for an unknown/optimized-out uniform name).
constexpr std::uint32_t kAlbedoTextureSlotUVE = 0;
constexpr std::uint32_t kNormalTextureSlotUVE = 1;
constexpr std::uint32_t kAoTextureSlotUVE = 2;

/// 1x1 RGBA8Unorm pixel data for the two fallback textures created once per Renderer3DUVE
/// instance (see ImplUVE::fallbackWhiteTexture/fallbackNormalTexture's own doc comments).
constexpr std::array<std::uint8_t, 4> kWhitePixelUVE{0xFF, 0xFF, 0xFF, 0xFF};
constexpr std::array<std::uint8_t, 4> kFlatNormalPixelUVE{0x80, 0x80, 0xFF, 0xFF};

/// Translates a loaded TextureAssetUVE's format into the RHI's own TextureFormatUVE (a
/// deliberately separate enum — see Asset::TextureFormatUVE's own doc comment for why). Asset
/// textures never use Depth32Float (that's only ever created directly as a GPU render target), so
/// this mapping is exhaustive over Asset::TextureFormatUVE's two enumerators.
[[nodiscard]] TextureFormatUVE ToRenderTextureFormatUVE(Asset::TextureFormatUVE format) noexcept {
    switch (format) {
        case Asset::TextureFormatUVE::RGBA8Unorm:
            return TextureFormatUVE::RGBA8Unorm;
        case Asset::TextureFormatUVE::RGBA16Float:
            return TextureFormatUVE::RGBA16Float;
    }
    UVE_ASSERT(false && "Unhandled Asset::TextureFormatUVE");
    return TextureFormatUVE::RGBA8Unorm;
}

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

    /// A 1x1 opaque-white texture, used whenever a material leaves albedoTexture/aoTexture unset
    /// (kInvalidAssetGuidUVE) — sampling it always yields {1,1,1,1}, so
    /// `texture(uAlbedoTexture, uv) * uAlbedoColor == uAlbedoColor` and
    /// `texture(uAOTexture, uv).r == 1.0` (no occlusion), with no shader-side branching needed.
    TextureHandleUVE fallbackWhiteTexture;

    /// A 1x1 flat tangent-space "up" normal texture ({0.5,0.5,1.0} encoded as {128,128,255}),
    /// used whenever a material leaves normalTexture unset. Bound/sampled for
    /// forward-compatibility with a future lighting increment; unused in this increment's unlit
    /// color output.
    TextureHandleUVE fallbackNormalTexture;

    std::unordered_map<Asset::AssetGuidUVE, MeshGpuResourcesUVE> meshCache;
    std::unordered_map<Asset::AssetGuidUVE, MaterialGpuResourcesUVE> materialCache;

    /// GPU textures uploaded from a loaded TextureAssetUVE, cached by that texture's own
    /// AssetGuidUVE — independent of materialCache, so two materials sharing an albedo texture
    /// GUID upload it only once.
    std::unordered_map<Asset::AssetGuidUVE, TextureHandleUVE> textureCache;

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

        const auto textureIt = textureCache.find(event.guid);
        if (textureIt != textureCache.end()) {
            renderDevice.DestroyTextureUVE(textureIt->second);
            textureCache.erase(textureIt);

            // MaterialGpuResourcesUVE doesn't track which texture GUIDs it resolved from, so
            // there's no cheap way to know which cached materials referenced this one — clear the
            // whole cache instead. Every material's pipeline and resolved texture handles get
            // recomputed lazily next frame they're drawn (mostly cache hits against textureCache
            // for anything unaffected). Coarse but simple and obviously correct, matching this
            // codebase's existing preference for whole-unit invalidation over fine-grained
            // dependency tracking (compare ShaderManagerUVE's own hot-reload).
            for (const auto& [materialGuid, materialResources] : materialCache) {
                renderDevice.DestroyPipelineUVE(materialResources.pipeline);
            }
            materialCache.clear();
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

    /// Returns the GPU handle for `textureGuid`, or std::nullopt if it's still loading (caller
    /// should abort this frame's resolution without caching anything, and retry next frame — the
    /// same async-non-blocking convention as the shader/mesh/material readiness checks
    /// elsewhere in this file). `kInvalidAssetGuidUVE` (unset in the source MaterialAssetUVE)
    /// returns `fallbackHandle` immediately, no load ever attempted. A texture whose load has
    /// permanently failed (HasFailedUVE()) also resolves to `fallbackHandle` — logged once — since
    /// retrying a failed load forever would never succeed.
    [[nodiscard]] std::optional<TextureHandleUVE> ResolveTextureGpuHandleUVE(Asset::AssetGuidUVE textureGuid,
                                                                              TextureHandleUVE fallbackHandle) {
        if (textureGuid == Asset::kInvalidAssetGuidUVE) {
            return fallbackHandle;
        }
        const auto existingIt = textureCache.find(textureGuid);
        if (existingIt != textureCache.end()) {
            return existingIt->second;
        }

        Asset::AssetHandleUVE<Asset::TextureAssetUVE> textureHandle =
            assetManager.LoadUVE<Asset::TextureAssetUVE>(textureGuid, assetDatabase);
        if (textureHandle.HasFailedUVE()) {
            UVE_WARNING("Renderer3DUVE: texture asset load failed - falling back to the default texture");
            return fallbackHandle;
        }
        if (!textureHandle.IsReadyUVE()) {
            return std::nullopt;
        }

        const Asset::TextureAssetUVE* const textureAsset = textureHandle.TryGetUVE();
        const TextureDescUVE desc{textureAsset->width, textureAsset->height,
                                   ToRenderTextureFormatUVE(textureAsset->format), 1};
        const TextureHandleUVE handle =
            renderDevice.CreateTextureUVE(desc, std::as_bytes(std::span(textureAsset->pixels)));
        textureCache.emplace(textureGuid, handle);
        return handle;
    }

    /// Returns the cached (creating-if-needed) pipeline + resolved textures for `item`'s material,
    /// or nullptr if the material's vertex/fragment ShaderAssetUVE or any of its set texture GUIDs
    /// hasn't finished loading yet this frame (silently skipped, same async-non-blocking
    /// convention MeshRendererUVE uses for mesh/material readiness — it appears once everything is
    /// ready, no special-casing).
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

        const std::optional<TextureHandleUVE> albedoTexture =
            ResolveTextureGpuHandleUVE(material->albedoTexture, fallbackWhiteTexture);
        const std::optional<TextureHandleUVE> normalTexture =
            ResolveTextureGpuHandleUVE(material->normalTexture, fallbackNormalTexture);
        const std::optional<TextureHandleUVE> aoTexture =
            ResolveTextureGpuHandleUVE(material->aoTexture, fallbackWhiteTexture);
        if (!albedoTexture.has_value() || !normalTexture.has_value() || !aoTexture.has_value()) {
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

        const auto insertResult = materialCache.emplace(
            guid, MaterialGpuResourcesUVE{pipeline, *albedoTexture, *normalTexture, *aoTexture});
        return &insertResult.first->second;
    }

    void RecordItemsUVE(const std::vector<RenderItemUVE>& items, const Math::Matrix4x4UVE& viewProjection,
                        ICommandBufferUVE& commandBuffer) {
        for (const RenderItemUVE& item : items) {
            const MaterialGpuResourcesUVE* const materialResources = ResolveMaterialGpuResourcesUVE(item);
            if (materialResources == nullptr) {
                continue;
            }
            const MeshGpuResourcesUVE& meshResources = ResolveMeshGpuResourcesUVE(item);
            const Asset::MaterialAssetUVE* const material = item.materialHandle.TryGetUVE();

            commandBuffer.BindPipelineUVE(materialResources->pipeline);
            commandBuffer.SetUniformMatrix4x4UVE("uModel", item.worldMatrix);
            commandBuffer.SetUniformMatrix4x4UVE("uViewProjection", viewProjection);
            commandBuffer.SetUniformVector3UVE("uAlbedoColor", material->albedoColor);
            commandBuffer.SetUniformFloatUVE("uMetallic", material->metallic);
            commandBuffer.SetUniformFloatUVE("uRoughness", material->roughness);
            commandBuffer.SetUniformVector3UVE("uEmissiveColor", material->emissiveColor);
            commandBuffer.BindTextureUVE(materialResources->albedoTexture, kAlbedoTextureSlotUVE);
            commandBuffer.SetUniformIntUVE("uAlbedoTexture", static_cast<std::int32_t>(kAlbedoTextureSlotUVE));
            commandBuffer.BindTextureUVE(materialResources->normalTexture, kNormalTextureSlotUVE);
            commandBuffer.SetUniformIntUVE("uNormalTexture", static_cast<std::int32_t>(kNormalTextureSlotUVE));
            commandBuffer.BindTextureUVE(materialResources->aoTexture, kAoTextureSlotUVE);
            commandBuffer.SetUniformIntUVE("uAOTexture", static_cast<std::int32_t>(kAoTextureSlotUVE));
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
    m_impl->fallbackWhiteTexture = renderDevice.CreateTextureUVE(
        TextureDescUVE{1, 1, TextureFormatUVE::RGBA8Unorm, 1}, std::as_bytes(std::span(kWhitePixelUVE)));
    m_impl->fallbackNormalTexture = renderDevice.CreateTextureUVE(
        TextureDescUVE{1, 1, TextureFormatUVE::RGBA8Unorm, 1}, std::as_bytes(std::span(kFlatNormalPixelUVE)));

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
    for (const auto& [guid, textureHandle] : m_impl->textureCache) {
        m_impl->renderDevice.DestroyTextureUVE(textureHandle);
    }
    m_impl->renderDevice.DestroyTextureUVE(m_impl->colorTarget);
    m_impl->renderDevice.DestroyTextureUVE(m_impl->depthTarget);
    m_impl->renderDevice.DestroyTextureUVE(m_impl->fallbackWhiteTexture);
    m_impl->renderDevice.DestroyTextureUVE(m_impl->fallbackNormalTexture);
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

    m_impl->RecordItemsUVE(queue.opaqueItems, viewProjection, commandBuffer);
    m_impl->RecordItemsUVE(queue.transparentItems, viewProjection, commandBuffer);

    commandBuffer.EndRenderPassUVE();
    m_impl->renderSystem.EndFrameUVE();
}

} // namespace UVE::Render
