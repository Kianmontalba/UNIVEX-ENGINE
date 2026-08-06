// Copyright (c) 2026 UniVex Studios. All Rights Reserved.


#pragma once

#include <cstdint>
#include <filesystem>
#include <string>

namespace UVE::Asset {

/// Which programmable stage a ShaderAssetUVE belongs to. Deliberately a separate type from
/// `Render::ShaderStageUVE` (engine/render, Increment 10) — see `Asset::TextureFormatUVE`'s doc
/// comment for why: avoiding a dependency cycle, since engine/render already depends on
/// engine/asset. `Compute` is reserved for the future `ComputeSystemUVE` (Part 7.2) — unused by
/// anything built so far.
enum class ShaderStageKindUVE : std::uint8_t { Vertex, Fragment, Compute };

/// The CPU-side, engine-native representation of a `.uveshader` asset (Part 2's file-format
/// table): shader source text, stored and validated as-is. `ShaderManagerUVE`'s real "compile
/// GLSL/HLSL/Metal at runtime" step (Part 7.2) is blocked in this environment — no glslang/
/// shaderc/DXC is available — so this asset only carries source text; compilation is future work
/// once such a library exists.
struct ShaderAssetUVE {
    ShaderStageKindUVE stage = ShaderStageKindUVE::Vertex;
    std::string sourceCode;
    std::string entryPointName = "main";
};

/// Loads `path` as a `.uve*` envelope with `AssetKindUVE::Shader`, filling `outShader`. Returns
/// false (logging the reason) if the file is missing/malformed, isn't actually a Shader asset, or
/// its `sourceCode` is empty (a shader asset with no source is meaningless).
[[nodiscard]] bool LoadShaderAssetUVE(const std::filesystem::path& path, ShaderAssetUVE& outShader);

/// Writes `shader` to `path` as a `.uve*` envelope with `AssetKindUVE::Shader`. Returns false
/// (logging the reason) if the file can't be written.
[[nodiscard]] bool SaveShaderAssetUVE(const ShaderAssetUVE& shader, const std::filesystem::path& path);

} // namespace UVE::Asset
