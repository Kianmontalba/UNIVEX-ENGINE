// Copyright (c) 2026 UniVex Studios. All Rights Reserved.
#include "uve/save/save_payload_compression_uve.h"
#include <cstring>
#include <iterator>
#include <utility>
namespace UVE::Save {
namespace {
constexpr std::byte kLegacyMagic[] = {std::byte{'U'}, std::byte{'V'}, std::byte{'S'}, std::byte{'C'}};
constexpr std::byte kChecksummedMagic[] = {std::byte{'U'}, std::byte{'V'}, std::byte{'S'}, std::byte{'2'}};
constexpr std::size_t kLegacyHeaderBytes = sizeof(kLegacyMagic) + sizeof(std::uint64_t);
constexpr std::size_t kChecksummedHeaderBytes = kLegacyHeaderBytes + sizeof(std::uint64_t);
[[nodiscard]] bool HasMagicUVE(const std::vector<std::byte>& payload, const std::byte (&magic)[4]) noexcept {
    return payload.size() >= sizeof(magic) && std::memcmp(payload.data(), magic, sizeof(magic)) == 0;
}
void AppendUint64UVE(std::vector<std::byte>& output, const std::uint64_t value) {
    const auto* bytes = reinterpret_cast<const std::byte*>(&value);
    output.insert(output.end(), bytes, bytes + sizeof(value));
}
[[nodiscard]] bool ReadUint64UVE(const std::vector<std::byte>& input, const std::size_t offset,
                                 std::uint64_t& value) noexcept {
    if (offset > input.size() || input.size() - offset < sizeof(value)) {
        return false;
    }
    std::memcpy(&value, input.data() + offset, sizeof(value));
    return true;
}
[[nodiscard]] std::uint64_t ComputeSavePayloadFingerprintUVE(const std::vector<std::byte>& payload) noexcept {
    constexpr std::uint64_t kOffsetBasis = 1469598103934665603ULL;
    constexpr std::uint64_t kPrime = 1099511628211ULL;
    std::uint64_t fingerprint = kOffsetBasis;
    for (const std::byte value : payload) {
        fingerprint ^= std::to_integer<std::uint8_t>(value);
        fingerprint *= kPrime;
    }
    return fingerprint;
}
} // namespace
std::vector<std::byte> CompressSavePayloadUVE(const std::vector<std::byte>& payload) {
    if (payload.size() > kMaximumCompressedSavePayloadBytesUVE) {
        return {};
    }
    std::vector<std::byte> compressed;
    compressed.reserve(kChecksummedHeaderBytes + payload.size());
    compressed.insert(compressed.end(), std::begin(kChecksummedMagic), std::end(kChecksummedMagic));
    AppendUint64UVE(compressed, payload.size());
    AppendUint64UVE(compressed, ComputeSavePayloadFingerprintUVE(payload));
    for (std::size_t offset = 0U; offset < payload.size();) {
        const std::byte value = payload[offset];
        std::size_t runLength = 1U;
        while (offset + runLength < payload.size() && payload[offset + runLength] == value && runLength < 255U) {
            ++runLength;
        }
        compressed.push_back(static_cast<std::byte>(runLength));
        compressed.push_back(value);
        offset += runLength;
    }
    if (compressed.size() >= payload.size()) {
        return payload;
    }
    return compressed;
}
bool DecompressSavePayloadUVE(const std::vector<std::byte>& payload,
                              std::vector<std::byte>& outPayload) {
    const bool checksummed = HasMagicUVE(payload, kChecksummedMagic);
    const bool legacyCompressed = HasMagicUVE(payload, kLegacyMagic);
    if (!checksummed && !legacyCompressed) {
        if (payload.size() > kMaximumCompressedSavePayloadBytesUVE) {
            return false;
        }
        outPayload = payload;
        return true;
    }
    const std::size_t headerBytes = checksummed ? kChecksummedHeaderBytes : kLegacyHeaderBytes;
    std::uint64_t expectedSize = 0U;
    if (!ReadUint64UVE(payload, sizeof(kChecksummedMagic), expectedSize) ||
        expectedSize > kMaximumCompressedSavePayloadBytesUVE || payload.size() < headerBytes) {
        return false;
    }
    std::uint64_t expectedFingerprint = 0U;
    if (checksummed && !ReadUint64UVE(payload, kLegacyHeaderBytes, expectedFingerprint)) {
        return false;
    }
    std::vector<std::byte> expanded;
    expanded.reserve(static_cast<std::size_t>(expectedSize));
    for (std::size_t offset = headerBytes; offset < payload.size();) {
        if (offset + 2U > payload.size()) {
            return false;
        }
        const std::size_t runLength = std::to_integer<std::uint8_t>(payload[offset]);
        if (runLength == 0U || expanded.size() > static_cast<std::size_t>(expectedSize) - runLength) {
            return false;
        }
        expanded.insert(expanded.end(), runLength, payload[offset + 1U]);
        offset += 2U;
    }
    if (expanded.size() != static_cast<std::size_t>(expectedSize) ||
        (checksummed && ComputeSavePayloadFingerprintUVE(expanded) != expectedFingerprint)) {
        return false;
    }
    outPayload = std::move(expanded);
    return true;
}
} // namespace UVE::Save
