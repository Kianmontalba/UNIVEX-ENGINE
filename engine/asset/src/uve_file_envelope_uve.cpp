// Copyright (c) 2026 UniVex Studios. All Rights Reserved.


#include "uve/asset/uve_file_envelope_uve.h"

#include <array>
#include <fstream>

#include "uve/debug/logging_macros_uve.h"

namespace UVE::Asset {

namespace {

constexpr std::array<char, 4> kUveMagicUVE{'U', 'V', 'E', '\0'};
constexpr std::uint32_t kEnvelopeVersionUVE = 1;
constexpr std::uint32_t kCompressionMethodNoneUVE = 0;

void WriteUint32UVE(std::ofstream& file, std::uint32_t value) {
    file.write(reinterpret_cast<const char*>(&value), sizeof(value));
}

void WriteUint64UVE(std::ofstream& file, std::uint64_t value) {
    file.write(reinterpret_cast<const char*>(&value), sizeof(value));
}

[[nodiscard]] bool ReadUint32UVE(std::ifstream& file, std::uint32_t& outValue) {
    file.read(reinterpret_cast<char*>(&outValue), sizeof(outValue));
    return static_cast<bool>(file);
}

[[nodiscard]] bool ReadUint64UVE(std::ifstream& file, std::uint64_t& outValue) {
    file.read(reinterpret_cast<char*>(&outValue), sizeof(outValue));
    return static_cast<bool>(file);
}

} // namespace

bool WriteUveFileUVE(const std::filesystem::path& path, AssetKindUVE assetType,
                      const std::vector<std::byte>& payload) {
    std::ofstream file(path, std::ios::binary);
    if (!file.is_open()) {
        UVE_ERROR("UveFileEnvelopeUVE: failed to open \"{}\" for writing", path.string());
        return false;
    }

    file.write(kUveMagicUVE.data(), static_cast<std::streamsize>(kUveMagicUVE.size()));
    WriteUint32UVE(file, kEnvelopeVersionUVE);
    WriteUint32UVE(file, static_cast<std::uint32_t>(assetType));
    WriteUint32UVE(file, kCompressionMethodNoneUVE);
    WriteUint64UVE(file, payload.size());
    if (!payload.empty()) {
        file.write(reinterpret_cast<const char*>(payload.data()),
                   static_cast<std::streamsize>(payload.size()));
    }

    if (!file.good()) {
        UVE_ERROR("UveFileEnvelopeUVE: failed to write \"{}\": stream error after write", path.string());
        return false;
    }
    return true;
}

std::optional<std::pair<UveFileHeaderUVE, std::vector<std::byte>>>
ReadUveFileUVE(const std::filesystem::path& path) {
    std::ifstream file(path, std::ios::binary);
    if (!file.is_open()) {
        UVE_ERROR("UveFileEnvelopeUVE: failed to open \"{}\" for reading", path.string());
        return std::nullopt;
    }

    std::array<char, 4> magic{};
    file.read(magic.data(), static_cast<std::streamsize>(magic.size()));
    if (!file || magic != kUveMagicUVE) {
        UVE_ERROR("UveFileEnvelopeUVE: \"{}\" is not a valid .uve file (bad magic)", path.string());
        return std::nullopt;
    }

    UveFileHeaderUVE header;
    std::uint32_t assetType = 0;
    std::uint64_t payloadLength = 0;
    if (!ReadUint32UVE(file, header.version) || !ReadUint32UVE(file, assetType) ||
        !ReadUint32UVE(file, header.compressionMethod) || !ReadUint64UVE(file, payloadLength)) {
        UVE_ERROR("UveFileEnvelopeUVE: \"{}\" has a truncated header", path.string());
        return std::nullopt;
    }
    header.assetType = static_cast<AssetKindUVE>(assetType);

    if (header.version != kEnvelopeVersionUVE) {
        UVE_ERROR("UveFileEnvelopeUVE: \"{}\" has unsupported envelope version {}", path.string(),
                   header.version);
        return std::nullopt;
    }
    if (header.compressionMethod != kCompressionMethodNoneUVE) {
        UVE_ERROR("UveFileEnvelopeUVE: \"{}\" uses unsupported compression method {}", path.string(),
                   header.compressionMethod);
        return std::nullopt;
    }

    std::vector<std::byte> payload(payloadLength);
    if (payloadLength > 0) {
        file.read(reinterpret_cast<char*>(payload.data()), static_cast<std::streamsize>(payloadLength));
        if (!file) {
            UVE_ERROR("UveFileEnvelopeUVE: \"{}\" has a truncated payload", path.string());
            return std::nullopt;
        }
    }

    return std::make_pair(header, std::move(payload));
}

} // namespace UVE::Asset
