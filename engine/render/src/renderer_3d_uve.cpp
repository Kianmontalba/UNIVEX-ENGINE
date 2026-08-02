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
#include <string>
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
#include "uve/render/i_light_system_uve.h"
#include "uve/render/render_queue_uve.h"
#include "uve/render/shader/built_in_shaders_uve.h"
#include "uve/render/shader/shader_program_desc_uve.h"
#include "uve/render/shader/shader_program_uve.h"

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

/// Slot the directional-light shadow map is bound to for the main color pass (Increment 26) —
/// the next slot after the three material texture slots above, following the same fixed-constant
/// convention.
constexpr std::uint32_t kShadowMapTextureSlotUVE = 3;

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

/// Finds the first active Directional light in `lights` for the shadow depth pre-pass (Increment
/// 26) — Point/Spot shadows are out of scope this increment (see docs/CODING_STANDARDS.md). A
/// simple linear scan, no sorting: the same first-N-encountered spirit as
/// ILightSystemUVE::ExtractActiveLightsUVE() itself, not a distance- or importance-based
/// selection. Returns nullptr if no active (intensity > 0) Directional light exists this frame.
[[nodiscard]] const LightDataUVE* FindShadowCasterUVE(const LightListUVE& lights) noexcept {
    for (const LightDataUVE& light : lights) {
        if (light.type == Scene::LightTypeUVE::Directional && light.intensity > 0.0F) {
            return &light;
        }
    }
    return nullptr;
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

/// Frame-constant uniform data threaded into RecordItemsUVE() for every item this frame: the
/// view-projection matrix, the rendering camera's world position (Increment 24 — the view vector
/// a specular term needs), up to kMaxLightsUVE active lights (Increment 25 — Point/Spot +
/// multi-light; trailing unused slots hold the "no light" LightDataUVE{} sentinel from
/// ILightSystemUVE::ExtractActiveLightsUVE()), and the global ambient term. Bundled into one
/// struct — mirroring PipelineDescUVE's/RenderPassDescUVE's own precedent for grouping related
/// descriptor data — rather than growing RecordItemsUVE's parameter list to six positional
/// parameters. Module-private: never crosses the RHI boundary, unlike PipelineDescUVE.
struct FrameUniformsUVE {
    Math::Matrix4x4UVE viewProjection;
    Math::Vector3UVE viewPosition;
    LightListUVE lights;
    Math::Vector3UVE ambientColor;

    /// The shadow-casting light's projection*view matrix (Increment 26) — Matrix4x4UVE::
    /// IdentityUVE() when no active Directional light exists this frame, which is a safe
    /// no-op sentinel: the shadow depth pre-pass never draws anything into the shadow map in that
    /// case either (see RecordShadowPassUVE), so the map stays cleared to 1.0 and every shadow
    /// comparison in the main pass naturally evaluates "not in shadow" regardless of what this
    /// matrix is.
    Math::Matrix4x4UVE lightSpaceMatrix;
};

} // namespace

struct Renderer3DUVE::ImplUVE {
    IRenderDeviceUVE& renderDevice;
    IRenderSystemUVE& renderSystem;
    IMeshRendererUVE& meshRenderer;
    ICameraSystemUVE& cameraSystem;
    ILightSystemUVE& lightSystem;
    Shader::IShaderManagerUVE& shaderManager;
    Asset::IAssetManagerUVE& assetManager;
    Asset::IAssetDatabaseUVE& assetDatabase;
    Events::IEventSystemUVE& eventSystem;
    std::uint32_t targetWidth;
    std::uint32_t targetHeight;

    /// Flat ambient term added to every rendered item every frame, regardless of whether an
    /// active light exists this frame (see EngineConfigUVE::ambientColor, Increment 23).
    Math::Vector3UVE ambientColor;

    /// Shadow depth pre-pass tuning (see EngineConfigUVE::shadowMapResolution/shadowMapHalfExtent/
    /// shadowMapNearPlane/shadowMapFarPlane, Increment 26).
    std::uint32_t shadowMapResolution;
    float shadowMapHalfExtent;
    float shadowMapNearPlane;
    float shadowMapFarPlane;

    TextureHandleUVE colorTarget;
    TextureHandleUVE depthTarget;

    /// Persistent depth-only render target the shadow depth pre-pass renders into every frame
    /// (Increment 26) — unlike depthTarget above (the main pass's own depth buffer, written and
    /// never sampled), this is later bound as a sampled texture input during the main color pass.
    TextureHandleUVE shadowMapTarget;

    /// The built-in shadow-depth vertex+fragment program (engine/render/shader/built_in/
    /// shadow_depth.glsl), compiled once via shaderManager at construction — not tied to any
    /// MaterialAssetUVE, matching EngineCoreUVE's demo-triangle precedent for a built-in
    /// (non-material) shader. May still be compiling (IsReadyUVE() == false) or have failed
    /// (IsValidUVE() == false) on any given frame; RecordShadowPassUVE() checks IsValidUVE()
    /// before every use, exactly like RenderDemoTriangleUVE() does.
    std::shared_ptr<Shader::ShaderProgramUVE> shadowProgram;

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
            ICameraSystemUVE& cameraSystemIn, ILightSystemUVE& lightSystemIn,
            Shader::IShaderManagerUVE& shaderManagerIn, Asset::IAssetManagerUVE& assetManagerIn,
            Asset::IAssetDatabaseUVE& assetDatabaseIn, Events::IEventSystemUVE& eventSystemIn,
            std::uint32_t targetWidthIn, std::uint32_t targetHeightIn, Math::Vector3UVE ambientColorIn,
            std::uint32_t shadowMapResolutionIn, float shadowMapHalfExtentIn, float shadowMapNearPlaneIn,
            float shadowMapFarPlaneIn)
        : renderDevice(renderDeviceIn), renderSystem(renderSystemIn), meshRenderer(meshRendererIn),
          cameraSystem(cameraSystemIn), lightSystem(lightSystemIn), shaderManager(shaderManagerIn),
          assetManager(assetManagerIn), assetDatabase(assetDatabaseIn), eventSystem(eventSystemIn),
          targetWidth(targetWidthIn), targetHeight(targetHeightIn), ambientColor(ambientColorIn),
          shadowMapResolution(shadowMapResolutionIn), shadowMapHalfExtent(shadowMapHalfExtentIn),
          shadowMapNearPlane(shadowMapNearPlaneIn), shadowMapFarPlane(shadowMapFarPlaneIn) {}

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

    /// Renders the directional-light shadow depth pre-pass (Increment 26) — always exactly one
    /// BeginRenderPassUVE/EndRenderPassUVE pair every frame, regardless of whether a shadow caster
    /// exists. When `hasCaster` is false, or shadowProgram hasn't finished compiling yet
    /// (!IsValidUVE()), the pass still runs but draws nothing, leaving shadowMapTarget cleared to
    /// 1.0 (far plane) — the same "sentinel via clear-value default" reasoning documented on
    /// FrameUniformsUVE::lightSpaceMatrix. Draws every opaque item unconditionally (no light-frustum
    /// culling this increment — a documented known limitation, reusing the camera-frustum-culled
    /// list the main pass already computed).
    void RecordShadowPassUVE(const std::vector<RenderItemUVE>& items, const Math::Matrix4x4UVE& lightSpaceMatrix,
                              bool hasCaster, ICommandBufferUVE& commandBuffer) {
        RenderPassDescUVE passDesc;
        passDesc.colorAttachment = kInvalidTextureHandleUVE;
        passDesc.depthAttachment = shadowMapTarget;
        passDesc.depthLoadOp = LoadOpUVE::Clear;
        passDesc.clearDepth = 1.0F;
        commandBuffer.BeginRenderPassUVE(passDesc);

        if (hasCaster && shadowProgram->IsValidUVE()) {
            for (const RenderItemUVE& item : items) {
                const MeshGpuResourcesUVE& meshResources = ResolveMeshGpuResourcesUVE(item);
                shadowProgram->SetMatrix4x4UVE("uModel", item.worldMatrix);
                shadowProgram->SetMatrix4x4UVE("uLightSpaceMatrix", lightSpaceMatrix);
                shadowProgram->ApplyToUVE(commandBuffer);
                commandBuffer.BindVertexBufferUVE(meshResources.vertexBuffer);
                commandBuffer.BindIndexBufferUVE(meshResources.indexBuffer);
                commandBuffer.DrawIndexedUVE(meshResources.indexCount);
            }
        }

        commandBuffer.EndRenderPassUVE();
    }

    void RecordItemsUVE(const std::vector<RenderItemUVE>& items, const FrameUniformsUVE& frameUniforms,
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
            commandBuffer.SetUniformMatrix4x4UVE("uViewProjection", frameUniforms.viewProjection);
            commandBuffer.SetUniformVector3UVE("uAmbientColor", frameUniforms.ambientColor);
            commandBuffer.SetUniformVector3UVE("uViewPosition", frameUniforms.viewPosition);
            for (std::size_t lightIndex = 0; lightIndex < kMaxLightsUVE; ++lightIndex) {
                const LightDataUVE& light = frameUniforms.lights[lightIndex];
                const std::string prefix = "uLights[" + std::to_string(lightIndex) + "].";
                commandBuffer.SetUniformIntUVE(prefix + "type", static_cast<std::int32_t>(light.type));
                commandBuffer.SetUniformVector3UVE(prefix + "position", light.position);
                commandBuffer.SetUniformVector3UVE(prefix + "direction", light.direction);
                commandBuffer.SetUniformVector3UVE(prefix + "color", light.color);
                commandBuffer.SetUniformFloatUVE(prefix + "intensity", light.intensity);
                commandBuffer.SetUniformFloatUVE(prefix + "range", light.range);
                commandBuffer.SetUniformFloatUVE(prefix + "spotAngleDegrees", light.spotAngleDegrees);
            }
            commandBuffer.SetUniformMatrix4x4UVE("uLightSpaceMatrix", frameUniforms.lightSpaceMatrix);
            commandBuffer.BindTextureUVE(shadowMapTarget, kShadowMapTextureSlotUVE);
            commandBuffer.SetUniformIntUVE("uShadowMapTexture", static_cast<std::int32_t>(kShadowMapTextureSlotUVE));
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
                              ILightSystemUVE& lightSystem, Shader::IShaderManagerUVE& shaderManager,
                              Asset::IAssetManagerUVE& assetManager, Asset::IAssetDatabaseUVE& assetDatabase,
                              Events::IEventSystemUVE& eventSystem, std::uint32_t targetWidth,
                              std::uint32_t targetHeight, Math::Vector3UVE ambientColor,
                              std::uint32_t shadowMapResolution, float shadowMapHalfExtent,
                              float shadowMapNearPlane, float shadowMapFarPlane)
    : m_impl(std::make_unique<ImplUVE>(renderDevice, renderSystem, meshRenderer, cameraSystem, lightSystem,
                                        shaderManager, assetManager, assetDatabase, eventSystem, targetWidth,
                                        targetHeight, ambientColor, shadowMapResolution, shadowMapHalfExtent,
                                        shadowMapNearPlane, shadowMapFarPlane)) {
    m_impl->colorTarget = renderDevice.CreateTextureUVE(
        TextureDescUVE{targetWidth, targetHeight, TextureFormatUVE::RGBA8Unorm, 1});
    m_impl->depthTarget = renderDevice.CreateTextureUVE(
        TextureDescUVE{targetWidth, targetHeight, TextureFormatUVE::Depth32Float, 1});
    m_impl->fallbackWhiteTexture = renderDevice.CreateTextureUVE(
        TextureDescUVE{1, 1, TextureFormatUVE::RGBA8Unorm, 1}, std::as_bytes(std::span(kWhitePixelUVE)));
    m_impl->fallbackNormalTexture = renderDevice.CreateTextureUVE(
        TextureDescUVE{1, 1, TextureFormatUVE::RGBA8Unorm, 1}, std::as_bytes(std::span(kFlatNormalPixelUVE)));
    m_impl->shadowMapTarget = renderDevice.CreateTextureUVE(
        TextureDescUVE{shadowMapResolution, shadowMapResolution, TextureFormatUVE::Depth32Float, 1});

    Shader::ShaderProgramDescUVE shadowProgramDesc;
    shadowProgramDesc.virtualFilePath = std::string(Shader::BuiltIn::kShadowDepthVirtualPath);
    shadowProgramDesc.embeddedFallbackSourceCode = std::string(Shader::BuiltIn::kShadowDepthSource);
    shadowProgramDesc.vertexLayout = MeshVertexLayoutUVE();
    shadowProgramDesc.depthTestEnabled = true;
    shadowProgramDesc.depthWriteEnabled = true;
    shadowProgramDesc.debugNameUVE = "ShadowDepth";
    m_impl->shadowProgram = shaderManager.CreateProgramUVE(shadowProgramDesc);

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
    m_impl->renderDevice.DestroyTextureUVE(m_impl->shadowMapTarget);
}

void Renderer3DUVE::RenderFrameUVE(Scene::IEntityManagerUVE& entityManager, Scene::EntityUVE cameraEntity) {
    const float aspectRatio = static_cast<float>(m_impl->targetWidth) / static_cast<float>(m_impl->targetHeight);
    const Math::Matrix4x4UVE viewProjection =
        m_impl->cameraSystem.ComputeViewProjectionUVE(entityManager, cameraEntity, aspectRatio);
    const Math::FrustumUVE frustum = m_impl->cameraSystem.ExtractFrustumUVE(viewProjection);
    const Math::Vector3UVE viewPosition = m_impl->cameraSystem.GetWorldPositionUVE(entityManager, cameraEntity);
    const LightListUVE lights = m_impl->lightSystem.ExtractActiveLightsUVE(entityManager);

    const LightDataUVE* const shadowCaster = FindShadowCasterUVE(lights);
    Math::Matrix4x4UVE lightSpaceMatrix = Math::Matrix4x4UVE::IdentityUVE();
    if (shadowCaster != nullptr) {
        const Math::Matrix4x4UVE lightProjection =
            Math::Matrix4x4UVE::OrthographicUVE(-m_impl->shadowMapHalfExtent, m_impl->shadowMapHalfExtent,
                                                 -m_impl->shadowMapHalfExtent, m_impl->shadowMapHalfExtent,
                                                 m_impl->shadowMapNearPlane, m_impl->shadowMapFarPlane);
        const Math::Matrix4x4UVE lightView =
            Math::Matrix4x4UVE::ViewFromPositionAndRotationUVE(shadowCaster->position, shadowCaster->rotation);
        lightSpaceMatrix = lightProjection * lightView;
    }

    const FrameUniformsUVE frameUniforms{viewProjection, viewPosition, lights, m_impl->ambientColor,
                                          lightSpaceMatrix};

    RenderQueueUVE queue =
        m_impl->meshRenderer.ExtractRenderQueueUVE(entityManager, m_impl->assetManager, m_impl->assetDatabase, frustum);
    queue.SortUVE();

    m_impl->renderSystem.BeginFrameUVE();
    ICommandBufferUVE& commandBuffer = m_impl->renderSystem.GetFrameCommandBufferUVE();

    m_impl->RecordShadowPassUVE(queue.opaqueItems, lightSpaceMatrix, shadowCaster != nullptr, commandBuffer);

    RenderPassDescUVE passDesc;
    passDesc.colorAttachment = m_impl->colorTarget;
    passDesc.depthAttachment = m_impl->depthTarget;
    commandBuffer.BeginRenderPassUVE(passDesc);

    m_impl->RecordItemsUVE(queue.opaqueItems, frameUniforms, commandBuffer);
    m_impl->RecordItemsUVE(queue.transparentItems, frameUniforms, commandBuffer);

    commandBuffer.EndRenderPassUVE();
    m_impl->renderSystem.EndFrameUVE();
}

} // namespace UVE::Render
