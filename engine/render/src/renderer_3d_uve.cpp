// Copyright (c) 2026 UniVex Studios. All Rights Reserved.


#include "uve/render/renderer_3d_uve.h"

#include <algorithm>
#include <array>
#include <cstddef>
#include <cmath>
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
#include "uve/math/aabb_uve.h"
#include "uve/debug/logging_macros_uve.h"
#include "uve/math/frustum_uve.h"
#include "uve/math/matrix4x4_uve.h"
#include "uve/render/i_light_system_uve.h"
#include "uve/render/render_queue_uve.h"
#include "uve/render/shader/built_in_shaders_uve.h"
#include "uve/render/shader/shader_program_desc_uve.h"
#include "uve/render/shader/shader_program_uve.h"
#include "uve/scene/components/camera_component_uve.h"

namespace UVE::Render {

namespace {

[[nodiscard]] Math::AabbUVE ComputeLightSpaceCameraBoundsUVE(const CameraFrustumCornersUVE& cameraCorners,
                                                              const Math::Matrix4x4UVE& lightView) noexcept {
    Math::Vector3UVE minimum = Math::TransformPointUVE(lightView, cameraCorners[0]);
    Math::Vector3UVE maximum = minimum;
    for (std::size_t cornerIndex = 1; cornerIndex < cameraCorners.size(); ++cornerIndex) {
        const Math::Vector3UVE corner = Math::TransformPointUVE(lightView, cameraCorners[cornerIndex]);
        minimum.x = std::min(minimum.x, corner.x);
        minimum.y = std::min(minimum.y, corner.y);
        minimum.z = std::min(minimum.z, corner.z);
        maximum.x = std::max(maximum.x, corner.x);
        maximum.y = std::max(maximum.y, corner.y);
        maximum.z = std::max(maximum.z, corner.z);
    }
    return Math::AabbUVE{minimum, maximum};
}

/// Expands and snaps the XY center of a light-space orthographic box to the shadow-map texel grid.
/// The half extents are widened by one texel on each side before snapping, so the resulting box
/// remains conservative even when the snapped center moves by up to half a texel.
/// Z remains fitted exactly: directional-shadow resolution is only two-dimensional, while the
/// existing padded near/far bounds already protect depth coverage.
[[nodiscard]] Math::AabbUVE StabilizeLightSpaceShadowBoundsUVE(const Math::AabbUVE& fittedBounds, float padding,
                                                               std::uint32_t shadowMapResolution) noexcept {
    const float clampedPadding = std::max(padding, 0.0F);
    const float minimumX = fittedBounds.min.x - clampedPadding;
    const float maximumX = fittedBounds.max.x + clampedPadding;
    const float minimumY = fittedBounds.min.y - clampedPadding;
    const float maximumY = fittedBounds.max.y + clampedPadding;

    const auto stabilizeAxis = [shadowMapResolution](float minimum, float maximum) noexcept {
        const float center = (minimum + maximum) * 0.5F;
        const float halfExtent = (maximum - minimum) * 0.5F;
        if (shadowMapResolution <= 2U || halfExtent <= 0.0F) {
            return std::array<float, 2>{minimum, maximum};
        }

        const float resolution = static_cast<float>(shadowMapResolution);
        const float stabilizedHalfExtent = halfExtent * resolution / (resolution - 2.0F);
        const float texelExtent = (stabilizedHalfExtent * 2.0F) / resolution;
        const float snappedCenter = std::floor(center / texelExtent) * texelExtent;
        return std::array<float, 2>{snappedCenter - stabilizedHalfExtent, snappedCenter + stabilizedHalfExtent};
    };

    const std::array<float, 2> stabilizedX = stabilizeAxis(minimumX, maximumX);
    const std::array<float, 2> stabilizedY = stabilizeAxis(minimumY, maximumY);
    return Math::AabbUVE{{stabilizedX[0], stabilizedY[0], fittedBounds.min.z - clampedPadding},
                         {stabilizedX[1], stabilizedY[1], fittedBounds.max.z + clampedPadding}};
}

/// A mesh's uploaded GPU buffers, cached by MeshAssetUVE's AssetGuidUVE.
struct MeshGpuResourcesUVE {
    BufferHandleUVE vertexBuffer;
    BufferHandleUVE indexBuffer;
    std::uint32_t indexCount = 0;
};

/// A material's manager-owned linked program plus its resolved texture handles, cached by
/// MaterialAssetUVE's AssetGuidUVE. `program` owns its linked pipeline through ShaderManagerUVE;
/// Renderer3DUVE only retains a shared reference and must never destroy that pipeline directly.
/// The source GUIDs let AssetReloaded events invalidate exactly the materials that reference a
/// changed vertex or fragment shader. `albedoTexture`/`normalTexture`/`aoTexture` are never
/// kInvalidTextureHandleUVE once cached — an unset MaterialAssetUVE texture GUID resolves to one
/// of Renderer3DUVE's two fallback textures (see ResolveTextureGpuHandleUVE()'s doc comment).
struct MaterialGpuResourcesUVE {
    std::shared_ptr<Shader::ShaderProgramUVE> program;
    Asset::AssetGuidUVE vertexShaderGuid;
    Asset::AssetGuidUVE fragmentShaderGuid;
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
constexpr std::size_t kShadowCascadeCountUVE = 3;
constexpr std::uint32_t kShadowCascadeFirstTextureSlotUVE = kShadowMapTextureSlotUVE;

using ShadowCascadeMatricesUVE = std::array<Math::Matrix4x4UVE, kShadowCascadeCountUVE>;
using ShadowCascadeSplitsUVE = std::array<float, kShadowCascadeCountUVE>;

[[nodiscard]] ShadowCascadeSplitsUVE ComputeCascadeSplitsUVE(float nearPlane, float farPlane,
                                                              float splitLambda) noexcept {
    ShadowCascadeSplitsUVE splits{};
    const float clampedLambda = std::clamp(splitLambda, 0.0F, 1.0F);
    for (std::size_t cascadeIndex = 0; cascadeIndex < kShadowCascadeCountUVE; ++cascadeIndex) {
        const float progress = static_cast<float>(cascadeIndex + 1U) / static_cast<float>(kShadowCascadeCountUVE);
        const float uniformSplit = nearPlane + (farPlane - nearPlane) * progress;
        const float logarithmicSplit = nearPlane * std::pow(farPlane / nearPlane, progress);
        splits[cascadeIndex] = uniformSplit * (1.0F - clampedLambda) + logarithmicSplit * clampedLambda;
    }
    return splits;
}

[[nodiscard]] CameraFrustumCornersUVE ComputeCascadeFrustumCornersUVE(
    const CameraFrustumCornersUVE& fullCameraCorners, float nearRatio, float farRatio) noexcept {
    CameraFrustumCornersUVE cascadeCorners{};
    for (std::size_t cornerIndex = 0; cornerIndex < 4; ++cornerIndex) {
        const Math::Vector3UVE nearCorner = fullCameraCorners[cornerIndex];
        const Math::Vector3UVE farCorner = fullCameraCorners[cornerIndex + 4U];
        cascadeCorners[cornerIndex] = nearCorner + (farCorner - nearCorner) * nearRatio;
        cascadeCorners[cornerIndex + 4U] = nearCorner + (farCorner - nearCorner) * farRatio;
    }
    return cascadeCorners;
}

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

/// MeshVertexUVE's binary layout (position, normal, UV, tangent, handedness — see
/// mesh_asset_uve.h), described once here for CreatePipelineUVE(). MeshVertexUVE is a
/// standard-layout aggregate of Math::Vector3UVE (itself standard-layout) and floats, so offsetof()
/// is well-defined.
const std::vector<VertexAttributeUVE>& MeshVertexLayoutUVE() {
    static const std::vector<VertexAttributeUVE> layout = {
        VertexAttributeUVE{"POSITION", VertexAttributeFormatUVE::Float3, offsetof(Asset::MeshVertexUVE, position)},
        VertexAttributeUVE{"NORMAL", VertexAttributeFormatUVE::Float3, offsetof(Asset::MeshVertexUVE, normal)},
        VertexAttributeUVE{"TEXCOORD0", VertexAttributeFormatUVE::Float2, offsetof(Asset::MeshVertexUVE, u)},
        VertexAttributeUVE{"TANGENT", VertexAttributeFormatUVE::Float4, offsetof(Asset::MeshVertexUVE, tangent)},
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

    /// Fixed three-cascade directional-shadow contract. A zero cascadeCount is the no-directional
    /// light sentinel; all maps remain cleared to 1.0 and material shaders naturally evaluate lit.
    ShadowCascadeMatricesUVE lightSpaceMatrices{};
    ShadowCascadeSplitsUVE cascadeSplits{};
    std::int32_t cascadeCount = 0;
    float cascadeBlendRatio = 0.0F;
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
    float shadowFrustumPadding;
    float shadowCascadeSplitLambda;

    /// Fraction of each non-final cascade range that cross-fades into the following cascade.
    /// The constructor keeps it bounded so canonical shader sampling has a predictable cost.
    float shadowCascadeBlendRatio;

    /// Bounded per-fragment PCF radius supplied to the canonical directional-shadow material
    /// shader. Zero keeps a hard comparison; the constructor clamps larger requested values to 2.
    std::int32_t shadowPcfKernelRadius;

    TextureHandleUVE colorTarget;
    TextureHandleUVE depthTarget;

    /// Persistent depth-only render target the shadow depth pre-pass renders into every frame
    /// (Increment 26) — unlike depthTarget above (the main pass's own depth buffer, written and
    /// never sampled), this is later bound as a sampled texture input during the main color pass.
    std::array<TextureHandleUVE, kShadowCascadeCountUVE> shadowMapTargets{};

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
            float shadowMapFarPlaneIn, float shadowFrustumPaddingIn, float shadowCascadeSplitLambdaIn,
            float shadowCascadeBlendRatioIn, std::uint32_t shadowPcfKernelRadiusIn)
        : renderDevice(renderDeviceIn), renderSystem(renderSystemIn), meshRenderer(meshRendererIn),
          cameraSystem(cameraSystemIn), lightSystem(lightSystemIn), shaderManager(shaderManagerIn),
          assetManager(assetManagerIn), assetDatabase(assetDatabaseIn), eventSystem(eventSystemIn),
          targetWidth(targetWidthIn), targetHeight(targetHeightIn), ambientColor(ambientColorIn),
          shadowMapResolution(shadowMapResolutionIn), shadowMapHalfExtent(shadowMapHalfExtentIn),
          shadowMapNearPlane(shadowMapNearPlaneIn), shadowMapFarPlane(shadowMapFarPlaneIn),
          shadowFrustumPadding(std::max(shadowFrustumPaddingIn, 0.0F)),
          shadowCascadeSplitLambda(std::clamp(shadowCascadeSplitLambdaIn, 0.0F, 1.0F)),
          shadowCascadeBlendRatio(std::clamp(shadowCascadeBlendRatioIn, 0.0F, 0.25F)),
          shadowPcfKernelRadius(static_cast<std::int32_t>(std::min(shadowPcfKernelRadiusIn, 2U))) {}

    void OnAssetReloadedUVE(const Asset::AssetReloadedEventUVE& event) {
        const auto meshIt = meshCache.find(event.guid);
        if (meshIt != meshCache.end()) {
            renderDevice.DestroyBufferUVE(meshIt->second.vertexBuffer);
            renderDevice.DestroyBufferUVE(meshIt->second.indexBuffer);
            meshCache.erase(meshIt);
        }
        // A material asset reload, or a reload of either of its separate shader assets, drops the
        // renderer cache entry. The shared managed program then releases naturally; ShaderManagerUVE
        // remains the sole owner of the linked pipeline lifecycle.
        for (auto materialIt = materialCache.begin(); materialIt != materialCache.end();) {
            const MaterialGpuResourcesUVE& resources = materialIt->second;
            if (materialIt->first == event.guid || resources.vertexShaderGuid == event.guid ||
                resources.fragmentShaderGuid == event.guid) {
                materialIt = materialCache.erase(materialIt);
            } else {
                ++materialIt;
            }
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
        // Asset loaders derive tangents for legacy `.uvemodel` payloads, but runtime/custom mesh
        // loaders may construct MeshAssetUVE directly. Regenerate into this one-time GPU-upload copy
        // so every material draw has the canonical TBN input without mutating shared asset data.
        std::vector<Asset::MeshVertexUVE> vertices = mesh->vertices;
        Asset::GenerateMeshTangentsUVE(vertices, mesh->indices);
        const std::span<const Asset::MeshVertexUVE> vertexSpan(vertices);
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

    /// Returns the cached (creating-if-needed) managed program + resolved textures for `item`'s
    /// material, or nullptr if the material's source assets/textures have not finished loading yet.
    /// The cached program may still be preprocessing or linking; RecordItemsUVE checks IsValidUVE()
    /// and skips that frame without blocking, then renders automatically once ShaderManagerUVE
    /// completes the request.

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

        // `.uveshader` assets are envelope files rather than raw GLSL files, so their already
        // decoded source is supplied as the manager fallback and the root virtual path stays empty.
        // Includes inside that source still use the normal virtual include paths and participate in
        // program-level dependency tracking; AssetReloaded events invalidate root shader assets.
        Shader::ShaderProgramStagesDescUVE programDesc;
        programDesc.vertexSource.stage = ShaderStageUVE::Vertex;
        programDesc.vertexSource.embeddedFallbackSourceCode = vertexShaderAsset->sourceCode;
        programDesc.vertexSource.entryPointName = vertexShaderAsset->entryPointName;
        programDesc.vertexSource.debugNameUVE =
            "Material vertex " + assetDatabase.ResolveUVE(material->vertexShader).string();
        programDesc.fragmentSource.stage = ShaderStageUVE::Fragment;
        programDesc.fragmentSource.embeddedFallbackSourceCode = fragmentShaderAsset->sourceCode;
        programDesc.fragmentSource.entryPointName = fragmentShaderAsset->entryPointName;
        programDesc.fragmentSource.debugNameUVE =
            "Material fragment " + assetDatabase.ResolveUVE(material->fragmentShader).string();
        programDesc.vertexLayout = MeshVertexLayoutUVE();
        programDesc.vertexStride = static_cast<std::uint32_t>(sizeof(Asset::MeshVertexUVE));
        programDesc.depthTestEnabled = true;
        programDesc.depthWriteEnabled = !material->isTransparent;
        programDesc.debugNameUVE = "Material " + assetDatabase.ResolveUVE(guid).string();
        const std::shared_ptr<Shader::ShaderProgramUVE> program = shaderManager.CreateProgramFromStagesUVE(programDesc);

        const auto insertResult = materialCache.emplace(
            guid, MaterialGpuResourcesUVE{program, material->vertexShader, material->fragmentShader, *albedoTexture,
                                          *normalTexture, *aoTexture});
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
                              TextureHandleUVE shadowMapTarget, bool hasCaster, ICommandBufferUVE& commandBuffer) {
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
            const std::shared_ptr<Shader::ShaderProgramUVE>& program = materialResources->program;
            if (!program->IsValidUVE()) {
                continue; // Still compiling or invalid: never bind a stale raw material pipeline.
            }

            program->SetMatrix4x4UVE("uModel", item.worldMatrix);
            program->SetMatrix4x4UVE("uViewProjection", frameUniforms.viewProjection);
            program->SetVector3UVE("uAmbientColor", frameUniforms.ambientColor);
            program->SetVector3UVE("uViewPosition", frameUniforms.viewPosition);
            for (std::size_t lightIndex = 0; lightIndex < kMaxLightsUVE; ++lightIndex) {
                const LightDataUVE& light = frameUniforms.lights[lightIndex];
                const std::string prefix = "uLights[" + std::to_string(lightIndex) + "].";
                program->SetIntUVE(prefix + "type", static_cast<std::int32_t>(light.type));
                program->SetVector3UVE(prefix + "position", light.position);
                program->SetVector3UVE(prefix + "direction", light.direction);
                program->SetVector3UVE(prefix + "color", light.color);
                program->SetFloatUVE(prefix + "intensity", light.intensity);
                program->SetFloatUVE(prefix + "range", light.range);
                program->SetFloatUVE(prefix + "spotAngleDegrees", light.spotAngleDegrees);
            }
            // Preserve the Increment 27 single-map names for project-authored legacy shaders;
            // the canonical Increment 30 shader consumes the bounded array uniforms below.
            program->SetMatrix4x4UVE("uLightSpaceMatrix", frameUniforms.lightSpaceMatrices[0]);
            program->SetIntUVE("uShadowMapTexture", static_cast<std::int32_t>(kShadowMapTextureSlotUVE));
            program->SetIntUVE("uShadowCascadeCount", frameUniforms.cascadeCount);
            program->SetFloatUVE("uShadowCascadeBlendRatio", frameUniforms.cascadeBlendRatio);
            for (std::size_t cascadeIndex = 0; cascadeIndex < kShadowCascadeCountUVE; ++cascadeIndex) {
                const std::string index = std::to_string(cascadeIndex);
                const std::uint32_t textureSlot = kShadowCascadeFirstTextureSlotUVE +
                                                  static_cast<std::uint32_t>(cascadeIndex);
                program->SetMatrix4x4UVE("uLightSpaceMatrices[" + index + "]",
                                         frameUniforms.lightSpaceMatrices[cascadeIndex]);
                program->SetFloatUVE("uShadowCascadeSplits[" + index + "]", frameUniforms.cascadeSplits[cascadeIndex]);
                program->SetIntUVE("uShadowMapTextures[" + index + "]", static_cast<std::int32_t>(textureSlot));
            }
            program->SetIntUVE("uShadowPcfKernelRadius", shadowPcfKernelRadius);
            program->SetVector3UVE("uAlbedoColor", material->albedoColor);
            program->SetFloatUVE("uMetallic", material->metallic);
            program->SetFloatUVE("uRoughness", material->roughness);
            program->SetVector3UVE("uEmissiveColor", material->emissiveColor);
            program->SetIntUVE("uAlbedoTexture", static_cast<std::int32_t>(kAlbedoTextureSlotUVE));
            program->SetIntUVE("uNormalTexture", static_cast<std::int32_t>(kNormalTextureSlotUVE));
            program->SetIntUVE("uAOTexture", static_cast<std::int32_t>(kAoTextureSlotUVE));
            program->ApplyToUVE(commandBuffer);
            commandBuffer.BindTextureUVE(shadowMapTargets[0], kShadowMapTextureSlotUVE);
            for (std::size_t cascadeIndex = 0; cascadeIndex < kShadowCascadeCountUVE; ++cascadeIndex) {
                commandBuffer.BindTextureUVE(shadowMapTargets[cascadeIndex],
                                              kShadowCascadeFirstTextureSlotUVE + static_cast<std::uint32_t>(cascadeIndex));
            }
            commandBuffer.BindTextureUVE(materialResources->albedoTexture, kAlbedoTextureSlotUVE);
            commandBuffer.BindTextureUVE(materialResources->normalTexture, kNormalTextureSlotUVE);
            commandBuffer.BindTextureUVE(materialResources->aoTexture, kAoTextureSlotUVE);
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
                              float shadowMapNearPlane, float shadowMapFarPlane, float shadowFrustumPadding,
                              float shadowCascadeSplitLambda, float shadowCascadeBlendRatio,
                              std::uint32_t shadowPcfKernelRadius)
    : m_impl(std::make_unique<ImplUVE>(renderDevice, renderSystem, meshRenderer, cameraSystem, lightSystem,
                                        shaderManager, assetManager, assetDatabase, eventSystem, targetWidth,
                                        targetHeight, ambientColor, shadowMapResolution, shadowMapHalfExtent,
                                        shadowMapNearPlane, shadowMapFarPlane, shadowFrustumPadding,
                                        shadowCascadeSplitLambda, shadowCascadeBlendRatio, shadowPcfKernelRadius)) {
    m_impl->colorTarget = renderDevice.CreateTextureUVE(
        TextureDescUVE{targetWidth, targetHeight, TextureFormatUVE::RGBA8Unorm, 1});
    m_impl->depthTarget = renderDevice.CreateTextureUVE(
        TextureDescUVE{targetWidth, targetHeight, TextureFormatUVE::Depth32Float, 1});
    m_impl->fallbackWhiteTexture = renderDevice.CreateTextureUVE(
        TextureDescUVE{1, 1, TextureFormatUVE::RGBA8Unorm, 1}, std::as_bytes(std::span(kWhitePixelUVE)));
    m_impl->fallbackNormalTexture = renderDevice.CreateTextureUVE(
        TextureDescUVE{1, 1, TextureFormatUVE::RGBA8Unorm, 1}, std::as_bytes(std::span(kFlatNormalPixelUVE)));
    for (TextureHandleUVE& shadowMapTarget : m_impl->shadowMapTargets) {
        shadowMapTarget = renderDevice.CreateTextureUVE(
            TextureDescUVE{shadowMapResolution, shadowMapResolution, TextureFormatUVE::Depth32Float, 1});
    }

    Shader::ShaderProgramDescUVE shadowProgramDesc;
    shadowProgramDesc.virtualFilePath = std::string(Shader::BuiltIn::kShadowDepthVirtualPath);
    shadowProgramDesc.embeddedFallbackSourceCode = std::string(Shader::BuiltIn::kShadowDepthSource);
    shadowProgramDesc.vertexLayout = MeshVertexLayoutUVE();
    shadowProgramDesc.vertexStride = static_cast<std::uint32_t>(sizeof(Asset::MeshVertexUVE));
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
    // MaterialGpuResourcesUVE holds shared ShaderProgramUVE references only. Releasing the cache
    // lets ShaderManagerUVE-owned program deleters retire their pipelines exactly once.
    m_impl->materialCache.clear();
    for (const auto& [guid, textureHandle] : m_impl->textureCache) {
        m_impl->renderDevice.DestroyTextureUVE(textureHandle);
    }
    m_impl->renderDevice.DestroyTextureUVE(m_impl->colorTarget);
    m_impl->renderDevice.DestroyTextureUVE(m_impl->depthTarget);
    m_impl->renderDevice.DestroyTextureUVE(m_impl->fallbackWhiteTexture);
    m_impl->renderDevice.DestroyTextureUVE(m_impl->fallbackNormalTexture);
    for (const TextureHandleUVE shadowMapTarget : m_impl->shadowMapTargets) {
        m_impl->renderDevice.DestroyTextureUVE(shadowMapTarget);
    }
}

void Renderer3DUVE::RenderFrameUVE(Scene::IEntityManagerUVE& entityManager, Scene::EntityUVE cameraEntity) {
    const float aspectRatio = static_cast<float>(m_impl->targetWidth) / static_cast<float>(m_impl->targetHeight);
    const Math::Matrix4x4UVE viewProjection =
        m_impl->cameraSystem.ComputeViewProjectionUVE(entityManager, cameraEntity, aspectRatio);
    const Math::FrustumUVE frustum = m_impl->cameraSystem.ExtractFrustumUVE(viewProjection);
    const Math::Vector3UVE viewPosition = m_impl->cameraSystem.GetWorldPositionUVE(entityManager, cameraEntity);
    const LightListUVE lights = m_impl->lightSystem.ExtractActiveLightsUVE(entityManager);

    const LightDataUVE* const shadowCaster = FindShadowCasterUVE(lights);
    ShadowCascadeMatricesUVE lightSpaceMatrices{};
    ShadowCascadeSplitsUVE cascadeSplits{};
    std::array<RenderQueueUVE, kShadowCascadeCountUVE> shadowQueues{};
    std::int32_t cascadeCount = 0;
    if (shadowCaster != nullptr) {
        const Scene::CameraComponentUVE& camera =
            entityManager.GetComponentUVE<Scene::CameraComponentUVE>(cameraEntity);
        cascadeSplits = ComputeCascadeSplitsUVE(camera.nearPlane, camera.farPlane, m_impl->shadowCascadeSplitLambda);
        const Math::Matrix4x4UVE lightView =
            Math::Matrix4x4UVE::ViewFromPositionAndRotationUVE(shadowCaster->position, shadowCaster->rotation);
        const CameraFrustumCornersUVE cameraCorners =
            m_impl->cameraSystem.ComputeFrustumCornersUVE(entityManager, cameraEntity, aspectRatio);
        float cascadeNearPlane = camera.nearPlane;
        for (std::size_t cascadeIndex = 0; cascadeIndex < kShadowCascadeCountUVE; ++cascadeIndex) {
            const float cascadeFarPlane = cascadeSplits[cascadeIndex];
            const float nearRatio = (cascadeNearPlane - camera.nearPlane) / (camera.farPlane - camera.nearPlane);
            const float farRatio = (cascadeFarPlane - camera.nearPlane) / (camera.farPlane - camera.nearPlane);
            const CameraFrustumCornersUVE cascadeCorners =
                ComputeCascadeFrustumCornersUVE(cameraCorners, nearRatio, farRatio);
            const Math::AabbUVE fittedBounds = ComputeLightSpaceCameraBoundsUVE(cascadeCorners, lightView);
            const Math::AabbUVE stabilizedBounds = StabilizeLightSpaceShadowBoundsUVE(
                fittedBounds, m_impl->shadowFrustumPadding, m_impl->shadowMapResolution);
            const Math::Matrix4x4UVE lightProjection = Math::Matrix4x4UVE::OrthographicUVE(
                stabilizedBounds.min.x, stabilizedBounds.max.x, stabilizedBounds.min.y, stabilizedBounds.max.y,
                -stabilizedBounds.max.z, -stabilizedBounds.min.z);
            lightSpaceMatrices[cascadeIndex] = lightProjection * lightView;
            const Math::FrustumUVE lightFrustum =
                m_impl->cameraSystem.ExtractFrustumUVE(lightSpaceMatrices[cascadeIndex]);
            shadowQueues[cascadeIndex] = m_impl->meshRenderer.ExtractRenderQueueUVE(
                entityManager, m_impl->assetManager, m_impl->assetDatabase, lightFrustum);
            shadowQueues[cascadeIndex].SortUVE();
            cascadeNearPlane = cascadeFarPlane;
        }
        cascadeCount = static_cast<std::int32_t>(kShadowCascadeCountUVE);
    }

    const FrameUniformsUVE frameUniforms{viewProjection, viewPosition, lights, m_impl->ambientColor,
                                          lightSpaceMatrices, cascadeSplits, cascadeCount,
                                          m_impl->shadowCascadeBlendRatio};

    RenderQueueUVE queue =
        m_impl->meshRenderer.ExtractRenderQueueUVE(entityManager, m_impl->assetManager, m_impl->assetDatabase, frustum);
    queue.SortUVE();

    m_impl->renderSystem.BeginFrameUVE();
    ICommandBufferUVE& commandBuffer = m_impl->renderSystem.GetFrameCommandBufferUVE();

    for (std::size_t cascadeIndex = 0; cascadeIndex < kShadowCascadeCountUVE; ++cascadeIndex) {
        m_impl->RecordShadowPassUVE(shadowQueues[cascadeIndex].opaqueItems, lightSpaceMatrices[cascadeIndex],
                                    m_impl->shadowMapTargets[cascadeIndex], shadowCaster != nullptr, commandBuffer);
    }

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
