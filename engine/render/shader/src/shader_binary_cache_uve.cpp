//------------------------------------------------------------------------------
// UniVex Engine (UVE) — Proprietary Game Engine
// Copyright (c) 2026 UniVex Studios. All Rights Reserved.
// Unauthorized copying, modification, distribution, or use of this code
// in whole or in part is strictly prohibited without express written
// permission from UniVex Studios.
// Violators will be prosecuted to the fullest extent of the law.
//------------------------------------------------------------------------------

#include "shader_binary_cache_uve.h"

#include <array>
#include <cstdio>
#include <fstream>
#include <system_error>

#include "uve/debug/logging_macros_uve.h"

namespace UVE::Render::Shader::Detail {

namespace {

constexpr std::array<char, 8> kMagicUVE = {'U', 'V', 'E', 'P', 'B', 'C', '0', '1'};
constexpr std::uint32_t kCacheFormatVersionUVE = 1;

#if defined(_WIN32)
constexpr const char* kPlatformDirectoryUVE = "windows";
#elif defined(__APPLE__)
constexpr const char* kPlatformDirectoryUVE = "macos";
#else
constexpr const char* kPlatformDirectoryUVE = "linux";
#endif

template <typename TValue>
void WriteRawUVE(std::ofstream& stream, const TValue& value) {
    stream.write(reinterpret_cast<const char*>(&value), sizeof(TValue));
}

template <typename TValue>
[[nodiscard]] bool ReadRawUVE(std::ifstream& stream, TValue& outValue) {
    stream.read(reinterpret_cast<char*>(&outValue), sizeof(TValue));
    return stream.good();
}

} // namespace

std::filesystem::path GetCacheFilePathUVE(const std::filesystem::path& cachePath, std::uint64_t contentHash) {
    std::array<char, 17> hexDigits{};
    std::snprintf(hexDigits.data(), hexDigits.size(), "%016llx", static_cast<unsigned long long>(contentHash));
    return cachePath / kPlatformDirectoryUVE / (std::string(hexDigits.data()) + ".uveshadercache");
}

std::optional<CacheEntryUVE> ReadCacheEntryUVE(const std::filesystem::path& filePath) {
    std::ifstream stream(filePath, std::ios::binary);
    if (!stream.is_open()) {
        return std::nullopt;
    }

    std::array<char, 8> magic{};
    stream.read(magic.data(), static_cast<std::streamsize>(magic.size()));
    if (!stream.good() || magic != kMagicUVE) {
        return std::nullopt;
    }

    std::uint32_t formatVersion = 0;
    if (!ReadRawUVE(stream, formatVersion) || formatVersion != kCacheFormatVersionUVE) {
        return std::nullopt;
    }

    CacheEntryUVE entry;
    if (!ReadRawUVE(stream, entry.glBinaryFormat)) {
        return std::nullopt;
    }

    std::uint64_t payloadLength = 0;
    if (!ReadRawUVE(stream, payloadLength)) {
        return std::nullopt;
    }

    entry.payload.resize(static_cast<std::size_t>(payloadLength));
    if (payloadLength > 0) {
        stream.read(reinterpret_cast<char*>(entry.payload.data()), static_cast<std::streamsize>(payloadLength));
        if (!stream.good()) {
            return std::nullopt;
        }
    }

    return entry;
}

bool WriteCacheEntryUVE(const std::filesystem::path& filePath, std::uint32_t glBinaryFormat,
                        std::span<const std::byte> payload) {
    std::error_code errorCode;
    std::filesystem::create_directories(filePath.parent_path(), errorCode);
    if (errorCode) {
        UVE_WARNING("ShaderManagerUVE: failed to create shader cache directory \"{}\": {}",
                     filePath.parent_path().string(), errorCode.message());
        return false;
    }

    std::ofstream stream(filePath, std::ios::binary | std::ios::trunc);
    if (!stream.is_open()) {
        UVE_WARNING("ShaderManagerUVE: failed to open shader cache file \"{}\" for writing", filePath.string());
        return false;
    }

    stream.write(kMagicUVE.data(), static_cast<std::streamsize>(kMagicUVE.size()));
    WriteRawUVE(stream, kCacheFormatVersionUVE);
    WriteRawUVE(stream, glBinaryFormat);
    WriteRawUVE(stream, static_cast<std::uint64_t>(payload.size()));
    if (!payload.empty()) {
        stream.write(reinterpret_cast<const char*>(payload.data()), static_cast<std::streamsize>(payload.size()));
    }

    if (!stream.good()) {
        UVE_WARNING("ShaderManagerUVE: failed writing shader cache file \"{}\"", filePath.string());
        return false;
    }
    return true;
}

} // namespace UVE::Render::Shader::Detail
