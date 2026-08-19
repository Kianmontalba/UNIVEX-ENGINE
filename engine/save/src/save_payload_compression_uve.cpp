// Copyright (c) 2026 UniVex Studios. All Rights Reserved.
#include "uve/save/save_payload_compression_uve.h"
#include <cstring>
#include <iterator>
#include <utility>
namespace UVE::Save {
namespace {
constexpr std::byte kMagic[] = {std::byte{'U'}, std::byte{'V'}, std::byte{'S'}, std::byte{'C'}};
constexpr std::size_t kHeaderBytes = sizeof(kMagic) + sizeof(std::uint64_t);
[[nodiscard]] bool HasMagicUVE(const std::vector<std::byte>& payload) noexcept {
    return payload.size() >= sizeof(kMagic) && std::memcmp(payload.data(), kMagic, sizeof(kMagic)) == 0;
}
void AppendUint64UVE(std::vector<std::byte>& output, const std::uint64_t value) {
    const auto* bytes = reinterpret_cast<const std::byte*>(&value);
    output.insert(output.end(), bytes, bytes + sizeof(value));
}
[[nodiscard]] bool ReadUint64UVE(const std::vector<std::byte>& input, std::uint64_t& value) noexcept {
    if (input.size() < kHeaderBytes) {
        return false;
    }
    std::memcpy(&value, input.data() + sizeof(kMagic), sizeof(value));
    return true;
}
} // namespace
std::vector<std::byte> CompressSavePayloadUVE(const std::vector<std::byte>& payload) {
    if (payload.size() > kMaximumCompressedSavePayloadBytesUVE) {
        return {};
    }
    std::vector<std::byte> compressed;
    compressed.reserve(kHeaderBytes + payload.size());
    compressed.insert(compressed.end(), std::begin(kMagic), std::end(kMagic));
    AppendUint64UVE(compressed, payload.size());
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
    if (!HasMagicUVE(payload)) {
        if (payload.size() > kMaximumCompressedSavePayloadBytesUVE) {
            return false;
        }
        outPayload = payload;
        return true;
    }
    std::uint64_t expectedSize = 0U;
    if (!ReadUint64UVE(payload, expectedSize) || expectedSize > kMaximumCompressedSavePayloadBytesUVE) {
        return false;
    }
    std::vector<std::byte> expanded;
    expanded.reserve(static_cast<std::size_t>(expectedSize));
    for (std::size_t offset = kHeaderBytes; offset < payload.size();) {
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
    if (expanded.size() != static_cast<std::size_t>(expectedSize)) {
        return false;
    }
    outPayload = std::move(expanded);
    return true;
}
} // namespace UVE::Save
