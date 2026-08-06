// Copyright (c) 2026 UniVex Studios. All Rights Reserved.


#pragma once

#include <cstdint>
#include <string>
#include <string_view>
#include <utility>
#include <vector>

#include "uve/asset/i_file_system_uve.h"

namespace UVE::Render::Shader::Detail {

/// The result of one PreprocessShaderSourceUVE() call — pure text/dependency-graph resolution,
/// no GL/threading involved, so this whole module is independently unit-testable against a fake
/// IFileSystemUVE.
struct PreprocessResultUVE {
    bool success = false;
    std::string errorMessage;

    /// The fully #include-expanded, #ifdef/#define-resolved GLSL text, ready to hand to
    /// IRenderDeviceUVE::CreateShaderUVE() — contains no UVE-preprocessor directives anymore
    /// (only #version/#extension/#line, which are native GLSL/GL preprocessor syntax, pass
    /// through untouched).
    std::string resolvedSource;

    /// Every virtual path actually read while resolving this source (the root file, plus every
    /// transitively #include'd file) — what ShaderManagerUVE's hot-reload polling watches for
    /// on-disk changes. Empty if the root was resolved from an embedded fallback string (no file
    /// was ever read).
    std::vector<std::string> dependencyClosure;

    /// index -> virtual path, in the order each file was first encountered (root is always index
    /// 0). Matches the `source-string-number` GL's own #line directive (emitted into
    /// `resolvedSource`) uses, so Detail::ParseGlInfoLogUVE() can resolve a driver diagnostic's
    /// line back to the file that actually contains it.
    std::vector<std::string> fileIndexTable;
};

/// Resolves `virtualFilePath` (or, if it doesn't exist per `fileSystem.HasFileUVE()`,
/// `embeddedFallbackSourceCode`) into final GLSL text: recursively expands `#include "path"`
/// directives (absolute virtual paths only, resolved via `fileSystem` — see this function's .cpp
/// doc comment for the full algorithm), and resolves `#define`/`#undef`/`#ifdef`/`#ifndef`/
/// `#else`/`#endif` conditional compilation using `defines` as the initial macro table (object-
/// like macros only — no function-like macros, no line continuation). Never throws; a malformed
/// input (unmatched #endif, a missing #include target, an #include cycle) produces
/// `success = false` with a human-readable `errorMessage`, never a crash or an infinite loop.
[[nodiscard]] PreprocessResultUVE PreprocessShaderSourceUVE(
    Asset::IFileSystemUVE& fileSystem, const std::string& virtualFilePath,
    const std::string& embeddedFallbackSourceCode, const std::vector<std::pair<std::string, std::string>>& defines);

/// A plain FNV-1a 64-bit hash over `data` — the primitive ShaderManagerUVE composes its
/// program-binary cache keys from (resolved source + stage + entry point + backend identity +
/// cache format version, concatenated). Deterministic, non-cryptographic, zero new dependency —
/// matches this codebase's "Foundations"-quality precedent for non-adversarial hashing needs.
[[nodiscard]] std::uint64_t ComputeFnv1aHashUVE(std::string_view data) noexcept;

} // namespace UVE::Render::Shader::Detail
