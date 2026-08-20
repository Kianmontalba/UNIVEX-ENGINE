// Copyright (c) 2026 UniVex Studios. All Rights Reserved.


#include "uve/audio/wav_importer_uve.h"

#include <cstdint>
#include <filesystem>
#include <fstream>
#include <limits>
#include <new>
#include <system_error>
#include <utility>
#include <vector>

#include "uve/asset/audio_asset_uve.h"
#include "uve/audio/wav_metadata_uve.h"
#include "uve/audio/wav_pcm16_decoder_uve.h"
#include "uve/debug/logging_macros_uve.h"

namespace UVE::Audio {
namespace {

constexpr std::uint64_t kMaximumWavImporterSourceBytesUVE = 64ULL * 1024ULL * 1024ULL;

[[nodiscard]] bool ReadWavSourceBytesUVE(const std::filesystem::path& sourcePath,
                                         std::vector<std::byte>& outBytes) {
    std::ifstream input(sourcePath, std::ios::binary | std::ios::ate);
    if (!input.is_open()) {
        UVE_ERROR("WavImporterUVE: failed to open source \"{}\"", sourcePath.string());
        return false;
    }
    const std::streamoff fileSize = input.tellg();
    if (fileSize < 0 || static_cast<std::uint64_t>(fileSize) > kMaximumWavImporterSourceBytesUVE ||
        static_cast<std::uint64_t>(fileSize) > std::numeric_limits<std::size_t>::max()) {
        UVE_ERROR("WavImporterUVE: source \"{}\" exceeds the bounded source-size limit", sourcePath.string());
        return false;
    }
    std::vector<std::byte> bytes(static_cast<std::size_t>(fileSize));
    input.seekg(0, std::ios::beg);
    if (!bytes.empty()) {
        input.read(reinterpret_cast<char*>(bytes.data()), static_cast<std::streamsize>(bytes.size()));
        if (!input) {
            UVE_ERROR("WavImporterUVE: source \"{}\" could not be read completely", sourcePath.string());
            return false;
        }
    }
    outBytes = std::move(bytes);
    return true;
}

[[nodiscard]] bool SaveAudioAssetAtomicallyUVE(const Asset::AudioAssetUVE& audio,
                                               const std::filesystem::path& destinationPath) {
    std::error_code errorCode;
    if (const std::filesystem::path parent = destinationPath.parent_path(); !parent.empty()) {
        std::filesystem::create_directories(parent, errorCode);
        if (errorCode) {
            UVE_ERROR("WavImporterUVE: failed to create destination directory \"{}\": {}", parent.string(),
                      errorCode.message());
            return false;
        }
    }
    const std::filesystem::path temporaryPath = destinationPath.string() + ".uve_wav_tmp";
    std::filesystem::remove(temporaryPath, errorCode);
    if (!Asset::SaveAudioAssetUVE(audio, temporaryPath)) {
        std::filesystem::remove(temporaryPath, errorCode);
        return false;
    }
    std::filesystem::rename(temporaryPath, destinationPath, errorCode);
    if (errorCode) {
        UVE_ERROR("WavImporterUVE: failed to publish destination \"{}\": {}", destinationPath.string(),
                  errorCode.message());
        std::filesystem::remove(temporaryPath, errorCode);
        return false;
    }
    return true;
}

[[nodiscard]] bool ImportWavSourceUVE(const std::filesystem::path& sourcePath,
                                      const std::filesystem::path& destinationPath,
                                      const Asset::AssetImportSettingsUVE& /*settings*/) {
    try {
        if (destinationPath.extension() != ".uveaudio") {
            UVE_ERROR("WavImporterUVE: destination \"{}\" must use the .uveaudio extension",
                      destinationPath.string());
            return false;
        }
        std::vector<std::byte> sourceBytes;
        if (!ReadWavSourceBytesUVE(sourcePath, sourceBytes)) return false;
        const auto metadata = ParseWavMetadataUVE(sourceBytes);
        if (!metadata.has_value()) {
            UVE_ERROR("WavImporterUVE: source \"{}\" failed bounded PCM WAV metadata parsing", sourcePath.string());
            return false;
        }
        std::vector<float> samples;
        if (!DecodeWavPcm16SamplesUVE(sourceBytes, samples)) {
            UVE_ERROR("WavImporterUVE: source \"{}\" failed bounded PCM16 sample decoding", sourcePath.string());
            return false;
        }
        if (metadata->channels == 0U || metadata->dataBytes / sizeof(std::int16_t) != samples.size()) {
            UVE_ERROR("WavImporterUVE: source \"{}\" published inconsistent PCM16 sample facts",
                      sourcePath.string());
            return false;
        }
        Asset::AudioAssetUVE audio;
        audio.channels = metadata->channels;
        audio.sampleRate = metadata->sampleRate;
        audio.samples = std::move(samples);
        return SaveAudioAssetAtomicallyUVE(audio, destinationPath);
    } catch (const std::bad_alloc&) {
        return false;
    }
}

} // namespace

void RegisterWavImporterUVE(Asset::IAssetImporterUVE& importer) {
    importer.RegisterImporterUVE("wav", &ImportWavSourceUVE);
}

} // namespace UVE::Audio
