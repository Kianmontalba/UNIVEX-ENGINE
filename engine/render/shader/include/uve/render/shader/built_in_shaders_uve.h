//------------------------------------------------------------------------------
// UniVex Engine (UVE) — Proprietary Game Engine
// Copyright (c) 2026 UniVex Studios. All Rights Reserved.
// Unauthorized copying, modification, distribution, or use of this code
// in whole or in part is strictly prohibited without express written
// permission from UniVex Studios.
// Violators will be prosecuted to the fullest extent of the law.
//------------------------------------------------------------------------------

#pragma once

#include <string_view>

namespace UVE::Render::Shader::BuiltIn {

/// The built-in shaders (4 from Increment 21, plus shadow_depth from Increment 26), each a single
/// physical `.glsl` file under
/// engine/render/shader/built_in/ containing both stages, split via `#ifdef VERTEX_SHADER` /
/// `#ifdef FRAGMENT_SHADER` (ShaderManagerUVE::CreateProgramUVE() compiles the same resolved
/// source twice, injecting the matching macro each time). Every constant here is the embedded
/// fallback ShaderManagerUVE transparently uses when the corresponding virtual path isn't
/// reachable (see ShaderProgramDescUVE's doc comment) — kept byte-identical to its `.glsl` file
/// by convention, enforced by tests/render/shader/built_in_shaders_parity_uve_tests.cpp.

inline constexpr std::string_view kBasic2DVirtualPath = "shaders/basic_2d.glsl";
extern const std::string_view kBasic2DSource;

inline constexpr std::string_view kBasic3DVirtualPath = "shaders/basic_3d.glsl";
extern const std::string_view kBasic3DSource;

inline constexpr std::string_view kBasic3DTexturedVirtualPath = "shaders/basic_3d_textured.glsl";
extern const std::string_view kBasic3DTexturedSource;

inline constexpr std::string_view kFullscreenQuadVirtualPath = "shaders/fullscreen_quad.glsl";
extern const std::string_view kFullscreenQuadSource;

inline constexpr std::string_view kShadowDepthVirtualPath = "shaders/shadow_depth.glsl";
extern const std::string_view kShadowDepthSource;

} // namespace UVE::Render::Shader::BuiltIn
