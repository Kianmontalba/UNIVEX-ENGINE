// Copyright (c) 2026 UniVex Studios. All Rights Reserved.


#pragma once

#include <cstdint>
#include <filesystem>
#include <vector>

namespace UVE::Asset {

inline constexpr std::size_t kMaximumAudioAssetSamplesUVE = 1U << 20U;

/// Typed CPU audio envelope for bounded interleaved normalized PCM16-derived samples. Runtime
/// devices, streaming cursors, mixer routing, compression, and platform codec ownership remain
/// outside this asset contract.
struct AudioAssetUVE final {
    std::uint16_t channels = 0U;
    std::uint32_t sampleRate = 0U;
    std::vector<float> samples;
};

/// Loads a `.uveaudio` envelope as a validated AudioAssetUVE, publishing output only after all
/// metadata, sample-count, finite-value, and payload checks succeed.
[[nodiscard]] bool LoadAudioAssetUVE(const std::filesystem::path& path, AudioAssetUVE& outAudio);

/// Saves a validated AudioAssetUVE as an AssetKindUVE::Audio envelope. Invalid metadata, non-finite
/// samples, mismatched interleaving, and oversized sample arrays are rejected before writing.
[[nodiscard]] bool SaveAudioAssetUVE(const AudioAssetUVE& audio, const std::filesystem::path& path);

} // namespace UVE::Asset
