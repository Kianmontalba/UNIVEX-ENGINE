

#include "uve/asset/uve_file_envelope_uve.h"

#include <array>
#include <cstring>
#include <fstream>
#include <limits>
#include <string>
#include <type_traits>

#include "uve/debug/logging_macros_uve.h"

namespace UVE::Asset {

namespace {

constexpr std::array<char, 4> kUveMagicUVE{'U', 'V', 'E', '\0'};
constexpr std::uint32_t kEnvelopeVersionUVE = 1;
constexpr std::uint32_t kCompressionMethodNoneUVE = 0;
constexpr std::uint64_t kMaximumPayloadBytesUVE = kMaximumUveFilePayloadBytesUVE;
constexpr std::size_t kEnvelopeHeaderBytesUVE =
    kUveMagicUVE.size() + sizeof(std::uint32_t) + sizeof(std::uint32_t) + sizeof(std::uint32_t) + sizeof(std::uint64_t);

[[nodiscard]] bool IsAssetKindValidUVE(const std::uint32_t assetType) noexcept {
    return assetType >= static_cast<std::uint32_t>(AssetKindUVE::Scene) &&
           assetType <= static_cast<std::uint32_t>(AssetKindUVE::Animation);
}

template <typename T>
void AppendValueUVE(std::vector<std::byte>& bytes, const T value) {
    static_assert(std::is_trivially_copyable_v<T>);
    const auto* const valueBytes = reinterpret_cast<const std::byte*>(&value);
    bytes.insert(bytes.end(), valueBytes, valueBytes + sizeof(T));
}

template <typename T>
[[nodiscard]] bool ReadValueUVE(const std::vector<std::byte>& bytes, std::size_t& offset, T& outValue) {
    static_assert(std::is_trivially_copyable_v<T>);
    if (offset > bytes.size() || bytes.size() - offset < sizeof(T)) {
        return false;
    }
    std::memcpy(&outValue, bytes.data() + offset, sizeof(T));
    offset += sizeof(T);
    return true;
}

[[nodiscard]] std::string SourceDescriptionUVE(const std::string_view sourceDescription) {
    return sourceDescription.empty() ? std::string{"<memory>"} : std::string{sourceDescription};
}

} // namespace

bool IsUveFilePayloadSizeValidUVE(const std::size_t payloadBytes) noexcept {
    return payloadBytes <= kMaximumUveFilePayloadBytesUVE;
}

std::vector<std::byte> EncodeUveFileEnvelopeUVE(const AssetKindUVE assetType,
                                                const std::vector<std::byte>& payload) {
    if (!IsUveFilePayloadSizeValidUVE(payload.size())) {
        return {};
    }
    std::vector<std::byte> envelope;
    envelope.reserve(kEnvelopeHeaderBytesUVE + payload.size());
    const auto* const magicBytes = reinterpret_cast<const std::byte*>(kUveMagicUVE.data());
    envelope.insert(envelope.end(), magicBytes, magicBytes + kUveMagicUVE.size());
    AppendValueUVE(envelope, kEnvelopeVersionUVE);
    AppendValueUVE(envelope, static_cast<std::uint32_t>(assetType));
    AppendValueUVE(envelope, kCompressionMethodNoneUVE);
    AppendValueUVE(envelope, static_cast<std::uint64_t>(payload.size()));
    envelope.insert(envelope.end(), payload.begin(), payload.end());
    return envelope;
}

std::optional<std::pair<UveFileHeaderUVE, std::vector<std::byte>>>
DecodeUveFileEnvelopeUVE(const std::vector<std::byte>& envelope, const std::string_view sourceDescription) {
    const std::string source = SourceDescriptionUVE(sourceDescription);
    if (envelope.size() < kUveMagicUVE.size() ||
        std::memcmp(envelope.data(), kUveMagicUVE.data(), kUveMagicUVE.size()) != 0) {
        UVE_ERROR("UveFileEnvelopeUVE: \"{}\" is not a valid .uve file (bad magic)", source);
        return std::nullopt;
    }

    std::size_t offset = kUveMagicUVE.size();
    UveFileHeaderUVE header;
    std::uint32_t assetType = 0;
    std::uint64_t payloadLength = 0;
    if (!ReadValueUVE(envelope, offset, header.version) || !ReadValueUVE(envelope, offset, assetType) ||
        !ReadValueUVE(envelope, offset, header.compressionMethod) || !ReadValueUVE(envelope, offset, payloadLength)) {
        UVE_ERROR("UveFileEnvelopeUVE: \"{}\" has a truncated header", source);
        return std::nullopt;
    }
    header.assetType = static_cast<AssetKindUVE>(assetType);

    if (header.version != kEnvelopeVersionUVE) {
        UVE_ERROR("UveFileEnvelopeUVE: \"{}\" has unsupported envelope version {}", source, header.version);
        return std::nullopt;
    }
    if (!IsAssetKindValidUVE(assetType)) {
        UVE_ERROR("UveFileEnvelopeUVE: \"{}\" has an invalid asset kind {}", source, assetType);
        return std::nullopt;
    }
    if (header.compressionMethod != kCompressionMethodNoneUVE) {
        UVE_ERROR("UveFileEnvelopeUVE: \"{}\" uses unsupported compression method {}", source,
                  header.compressionMethod);
        return std::nullopt;
    }
    if (payloadLength > kMaximumPayloadBytesUVE || payloadLength > std::numeric_limits<std::size_t>::max() ||
        payloadLength > envelope.size() - offset) {
        UVE_ERROR("UveFileEnvelopeUVE: \"{}\" has an invalid or oversized payload length", source);
        return std::nullopt;
    }

    const std::size_t payloadSize = static_cast<std::size_t>(payloadLength);
    return std::make_pair(header,
                          std::vector<std::byte>{envelope.begin() + static_cast<std::ptrdiff_t>(offset),
                                                 envelope.begin() + static_cast<std::ptrdiff_t>(offset + payloadSize)});
}

bool WriteUveFileUVE(const std::filesystem::path& path, const AssetKindUVE assetType,
                     const std::vector<std::byte>& payload) {
    if (!IsUveFilePayloadSizeValidUVE(payload.size())) {
        UVE_ERROR("UveFileEnvelopeUVE: refused oversized payload for {}", path.string());
        return false;
    }
    if (!IsAssetKindValidUVE(static_cast<std::uint32_t>(assetType))) {
        UVE_ERROR("UveFileEnvelopeUVE: refused to write \"{}\" with invalid asset kind {}", path.string(),
                  static_cast<std::uint32_t>(assetType));
        return false;
    }
    std::ofstream file(path, std::ios::binary);
    if (!file.is_open()) {
        UVE_ERROR("UveFileEnvelopeUVE: failed to open \"{}\" for writing", path.string());
        return false;
    }

    const std::vector<std::byte> envelope = EncodeUveFileEnvelopeUVE(assetType, payload);
    file.write(reinterpret_cast<const char*>(envelope.data()), static_cast<std::streamsize>(envelope.size()));
    if (!file.good()) {
        UVE_ERROR("UveFileEnvelopeUVE: failed to write \"{}\": stream error after write", path.string());
        return false;
    }
    return true;
}

std::optional<std::pair<UveFileHeaderUVE, std::vector<std::byte>>>
ReadUveFileUVE(const std::filesystem::path& path) {
    std::ifstream file(path, std::ios::binary | std::ios::ate);
    if (!file.is_open()) {
        UVE_ERROR("UveFileEnvelopeUVE: failed to open \"{}\" for reading", path.string());
        return std::nullopt;
    }

    const std::streamoff fileSize = file.tellg();
    if (fileSize < 0 || static_cast<std::uint64_t>(fileSize) >
                            kEnvelopeHeaderBytesUVE + kMaximumPayloadBytesUVE) {
        UVE_ERROR("UveFileEnvelopeUVE: \"{}\" has an invalid or oversized payload length", path.string());
        return std::nullopt;
    }

    std::vector<std::byte> envelope(static_cast<std::size_t>(fileSize));
    file.seekg(0, std::ios::beg);
    if (!envelope.empty()) {
        file.read(reinterpret_cast<char*>(envelope.data()), static_cast<std::streamsize>(envelope.size()));
        if (!file) {
            UVE_ERROR("UveFileEnvelopeUVE: \"{}\" could not be read completely", path.string());
            return std::nullopt;
        }
    }

    return DecodeUveFileEnvelopeUVE(envelope, path.string());
}

} // namespace UVE::Asset
