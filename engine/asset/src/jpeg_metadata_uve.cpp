// Copyright (c) 2026 UniVex Studios. All Rights Reserved.
#include "uve/asset/jpeg_metadata_uve.h"
#include <cstddef>
#include <cstdio>
#include <cstdlib>
#include <csetjmp>
#include <cstring>
#include <limits>
#include <new>
#include <utility>

#include <jpeglib.h>
namespace UVE::Asset {
namespace {
[[nodiscard]] bool IsSofMarkerUVE(const std::uint8_t marker) noexcept {
    return (marker >= 0xC0U && marker <= 0xC3U) || (marker >= 0xC5U && marker <= 0xC7U) ||
           (marker >= 0xC9U && marker <= 0xCBU) || (marker >= 0xCDU && marker <= 0xCFU);
}
[[nodiscard]] bool IsProgressiveMarkerUVE(const std::uint8_t marker) noexcept {
    return marker == 0xC2U || marker == 0xC6U || marker == 0xCAU || marker == 0xCEU;
}
[[nodiscard]] std::uint16_t ReadU16BEUVE(const std::vector<std::byte>& bytes, const std::size_t offset) noexcept {
    return static_cast<std::uint16_t>((std::to_integer<std::uint16_t>(bytes[offset]) << 8U) |
                                      std::to_integer<std::uint16_t>(bytes[offset + 1U]));
}
struct JpegErrorUVE final {
    jpeg_error_mgr base{};
    std::jmp_buf jump{};
};

void JpegErrorExitUVE(j_common_ptr info) noexcept {
    auto* const error = reinterpret_cast<JpegErrorUVE*>(info->err);
    longjmp(error->jump, 1);
}

struct JpegDecodeStateUVE final {
    jpeg_decompress_struct decoder{};
    bool decoderCreated = false;
    unsigned char* rgbaPixels = nullptr;
    unsigned char* scanline = nullptr;
};

[[nodiscard]] bool DecodeJpegRawRgba8UVE(const std::vector<std::byte>& bytes,
                                          const JpegMetadataUVE& metadata,
                                          std::byte*& outPixels) noexcept {
    outPixels = nullptr;
    auto* const state = new (std::nothrow) JpegDecodeStateUVE{};
    if (state == nullptr) return false;
    JpegErrorUVE error;
    state->decoder.err = jpeg_std_error(&error.base);
    error.base.error_exit = JpegErrorExitUVE;
    if (setjmp(error.jump) != 0) {
        std::free(state->scanline);
        std::free(state->rgbaPixels);
        if (state->decoderCreated) jpeg_destroy_decompress(&state->decoder);
        delete state;
        return false;
    }
    jpeg_create_decompress(&state->decoder);
    state->decoderCreated = true;
    jpeg_mem_src(&state->decoder,
                 const_cast<unsigned char*>(reinterpret_cast<const unsigned char*>(bytes.data())),
                 static_cast<unsigned long>(bytes.size()));
    if (jpeg_read_header(&state->decoder, TRUE) != JPEG_HEADER_OK) {
        jpeg_destroy_decompress(&state->decoder);
        delete state;
        return false;
    }
    state->decoder.out_color_space = JCS_RGB;
    if (!jpeg_start_decompress(&state->decoder) || state->decoder.output_width != metadata.width ||
        state->decoder.output_height != metadata.height || state->decoder.output_components != 3U) {
        jpeg_destroy_decompress(&state->decoder);
        delete state;
        return false;
    }
    const std::size_t outputWidth = static_cast<std::size_t>(state->decoder.output_width);
    const std::size_t outputHeight = static_cast<std::size_t>(state->decoder.output_height);
    const std::size_t rgbaByteCount = outputWidth * outputHeight * 4U;
    state->rgbaPixels = static_cast<unsigned char*>(std::malloc(rgbaByteCount));
    state->scanline = static_cast<unsigned char*>(std::malloc(outputWidth * 3U));
    if (state->rgbaPixels == nullptr || state->scanline == nullptr) {
        std::free(state->scanline);
        std::free(state->rgbaPixels);
        jpeg_destroy_decompress(&state->decoder);
        delete state;
        return false;
    }
    while (state->decoder.output_scanline < state->decoder.output_height) {
        JSAMPROW row = state->scanline;
        if (jpeg_read_scanlines(&state->decoder, &row, 1U) != 1U) {
            std::free(state->scanline);
            std::free(state->rgbaPixels);
            jpeg_destroy_decompress(&state->decoder);
            delete state;
            return false;
        }
        const std::size_t sourceRow = static_cast<std::size_t>(state->decoder.output_scanline - 1U);
        for (std::size_t x = 0U; x < outputWidth; ++x) {
            const std::size_t source = x * 3U;
            const std::size_t target = (sourceRow * outputWidth + x) * 4U;
            state->rgbaPixels[target] = state->scanline[source];
            state->rgbaPixels[target + 1U] = state->scanline[source + 1U];
            state->rgbaPixels[target + 2U] = state->scanline[source + 2U];
            state->rgbaPixels[target + 3U] = 0xFFU;
        }
    }
    if (!jpeg_finish_decompress(&state->decoder)) {
        std::free(state->scanline);
        std::free(state->rgbaPixels);
        jpeg_destroy_decompress(&state->decoder);
        delete state;
        return false;
    }
    std::free(state->scanline);
    state->scanline = nullptr;
    jpeg_destroy_decompress(&state->decoder);
    outPixels = reinterpret_cast<std::byte*>(state->rgbaPixels);
    state->rgbaPixels = nullptr;
    delete state;
    return true;
}

} // namespace

bool ValidateJpegRgba8PixelBudgetUVE(const JpegMetadataUVE& metadata,
                                     const std::uint64_t maximumBytes) noexcept {
    if (metadata.width == 0U || metadata.height == 0U || metadata.precision != 8U ||
        metadata.componentCount == 0U || metadata.componentCount > 4U || maximumBytes == 0U) {
        return false;
    }
    constexpr std::uint64_t kBytesPerRgba8Pixel = 4ULL;
    const std::uint64_t pixelCount = static_cast<std::uint64_t>(metadata.width) *
                                      static_cast<std::uint64_t>(metadata.height);
    if (pixelCount > maximumBytes / kBytesPerRgba8Pixel) {
        return false;
    }
    return pixelCount * kBytesPerRgba8Pixel <= maximumBytes;
}

std::optional<JpegMetadataUVE> ParseJpegMetadataUVE(const std::vector<std::byte>& bytes) {
    if (bytes.size() < 2U || bytes[0] != std::byte{0xFF} || bytes[1] != std::byte{0xD8}) return std::nullopt;
    std::size_t offset = 2U;
    while (offset < bytes.size()) {
        if (bytes[offset++] != std::byte{0xFF}) return std::nullopt;
        while (offset < bytes.size() && bytes[offset] == std::byte{0xFF}) ++offset;
        if (offset >= bytes.size()) return std::nullopt;
        const std::uint8_t marker = std::to_integer<std::uint8_t>(bytes[offset++]);
        if (marker == 0xD9U || marker == 0xDAU) return std::nullopt;
        if (marker >= 0xD0U && marker <= 0xD7U) continue;
        if (marker == 0x01U) continue;
        if (offset + 2U > bytes.size()) return std::nullopt;
        const std::uint16_t segmentLength = ReadU16BEUVE(bytes, offset);
        if (segmentLength < 2U || offset + segmentLength > bytes.size()) return std::nullopt;
        if (IsSofMarkerUVE(marker)) {
            if (segmentLength < 8U) return std::nullopt;
            const std::size_t payload = offset + 2U;
            const std::uint8_t precision = std::to_integer<std::uint8_t>(bytes[payload]);
            const std::uint16_t height = ReadU16BEUVE(bytes, payload + 1U);
            const std::uint16_t width = ReadU16BEUVE(bytes, payload + 3U);
            const std::uint8_t components = std::to_integer<std::uint8_t>(bytes[payload + 5U]);
            if (width == 0U || height == 0U || components == 0U || components > 4U ||
                segmentLength != static_cast<std::uint16_t>(8U + 3U * components)) return std::nullopt;
            return JpegMetadataUVE{width, height, precision, components, IsProgressiveMarkerUVE(marker)};
        }
        offset += segmentLength;
    }
    return std::nullopt;
}

bool DecodeJpegRgba8ImageUVE(const std::vector<std::byte>& bytes, JpegRgba8ImageUVE& outImage) noexcept {
    std::byte* rawPixels = nullptr;
    try {
        const auto metadata = ParseJpegMetadataUVE(bytes);
        if (!metadata.has_value() || !ValidateJpegRgba8PixelBudgetUVE(*metadata) ||
            bytes.size() > static_cast<std::size_t>(std::numeric_limits<unsigned long>::max())) {
            return false;
        }
        if (!DecodeJpegRawRgba8UVE(bytes, *metadata, rawPixels)) return false;
        const std::size_t pixelByteCount = static_cast<std::size_t>(metadata->width) * metadata->height * 4U;
        std::vector<std::byte> pixels(pixelByteCount);
        std::memcpy(pixels.data(), rawPixels, pixelByteCount);
        std::free(rawPixels);
        rawPixels = nullptr;
        JpegRgba8ImageUVE image;
        image.width = metadata->width;
        image.height = metadata->height;
        image.pixels = std::move(pixels);
        outImage = std::move(image);
        return true;
    } catch (const std::bad_alloc&) {
        std::free(rawPixels);
        return false;
    }
}

} // namespace UVE::Asset
