// Copyright (c) 2026 UniVex Studios. All Rights Reserved.


#include "uve/asset/gltf_importer_uve.h"

#include <algorithm>
#include <cctype>
#include <cstddef>
#include <cstdint>
#include <filesystem>
#include <fstream>
#include <limits>
#include <new>
#include <optional>
#include <string>
#include <string_view>
#include <system_error>
#include <utility>
#include <vector>

#include <nlohmann/json.hpp>

#include "uve/asset/gltf_mesh_converter_uve.h"
#include "uve/asset/gltf_metadata_uve.h"
#include "uve/asset/mesh_asset_uve.h"
#include "uve/debug/logging_macros_uve.h"

namespace UVE::Asset {
namespace {

constexpr std::size_t kGlbHeaderBytesUVE = 12U;
constexpr std::size_t kGlbChunkHeaderBytesUVE = 8U;
constexpr std::size_t kMaximumGltfSourceBytesUVE = kMaximumGltfDataUriDecodedBytesUVE;

struct AccessorDefinitionUVE final {
    std::uint64_t bufferView = 0U;
    std::uint64_t byteOffset = 0U;
    std::uint64_t count = 0U;
    std::uint32_t componentType = 0U;
    std::string type;
};

struct BufferViewDefinitionUVE final {
    std::uint64_t buffer = 0U;
    std::uint64_t byteOffset = 0U;
    std::uint64_t byteLength = 0U;
    std::uint64_t byteStride = 0U;
};

[[nodiscard]] std::optional<std::uint64_t> ReadJsonUintUVE(const nlohmann::json& object,
                                                           const char* key,
                                                           const bool required) {
    if (!object.contains(key)) {
        return required ? std::nullopt : std::optional<std::uint64_t>{0U};
    }
    const auto& value = object.at(key);
    if (value.is_number_unsigned()) {
        return value.get<std::uint64_t>();
    }
    if (value.is_number_integer() && value.get<std::int64_t>() >= 0) {
        return static_cast<std::uint64_t>(value.get<std::int64_t>());
    }
    return std::nullopt;
}

[[nodiscard]] std::optional<std::uint32_t> ReadJsonU32UVE(const nlohmann::json& object, const char* key) {
    const auto value = ReadJsonUintUVE(object, key, true);
    if (!value.has_value() || *value > std::numeric_limits<std::uint32_t>::max()) {
        return std::nullopt;
    }
    return static_cast<std::uint32_t>(*value);
}

[[nodiscard]] bool ReadBoundedFileBytesUVE(const std::filesystem::path& path,
                                           std::vector<std::byte>& outBytes) {
    std::ifstream input(path, std::ios::binary | std::ios::ate);
    if (!input.is_open()) {
        UVE_ERROR("GltfImporterUVE: failed to open source \"{}\"", path.string());
        return false;
    }
    const std::streamoff fileSize = input.tellg();
    if (fileSize < 0 || static_cast<std::uint64_t>(fileSize) > kMaximumGltfSourceBytesUVE ||
        static_cast<std::uint64_t>(fileSize) > std::numeric_limits<std::size_t>::max()) {
        UVE_ERROR("GltfImporterUVE: source \"{}\" exceeds the bounded source-size limit", path.string());
        return false;
    }
    std::vector<std::byte> bytes(static_cast<std::size_t>(fileSize));
    input.seekg(0, std::ios::beg);
    if (!bytes.empty()) {
        input.read(reinterpret_cast<char*>(bytes.data()), static_cast<std::streamsize>(bytes.size()));
        if (!input) {
            UVE_ERROR("GltfImporterUVE: source \"{}\" could not be read completely", path.string());
            return false;
        }
    }
    outBytes = std::move(bytes);
    return true;
}

[[nodiscard]] std::uint32_t ReadU32LittleEndianUVE(const std::vector<std::byte>& bytes,
                                                   const std::size_t offset) noexcept {
    return static_cast<std::uint32_t>(std::to_integer<std::uint8_t>(bytes[offset])) |
           (static_cast<std::uint32_t>(std::to_integer<std::uint8_t>(bytes[offset + 1U])) << 8U) |
           (static_cast<std::uint32_t>(std::to_integer<std::uint8_t>(bytes[offset + 2U])) << 16U) |
           (static_cast<std::uint32_t>(std::to_integer<std::uint8_t>(bytes[offset + 3U])) << 24U);
}

[[nodiscard]] bool ParseJsonDocumentUVE(const std::string_view jsonSource, nlohmann::json& outDocument) {
    try {
        const nlohmann::json document = nlohmann::json::parse(jsonSource);
        if (!document.is_object() || !document.contains("asset") || !document.at("asset").is_object() ||
            !document.at("asset").contains("version") || !document.at("asset").at("version").is_string() ||
            document.at("asset").at("version").get<std::string>() != "2.0") {
            return false;
        }
        outDocument = document;
        return true;
    } catch (const nlohmann::json::exception&) {
        return false;
    }
}

[[nodiscard]] bool ExtractGlbDocumentAndBufferUVE(const std::vector<std::byte>& bytes,
                                                  nlohmann::json& outDocument,
                                                  std::vector<std::byte>& outBuffer) {
    if (bytes.size() < kGlbHeaderBytesUVE + kGlbChunkHeaderBytesUVE ||
        ReadU32LittleEndianUVE(bytes, 0U) != 0x46546C67U || ReadU32LittleEndianUVE(bytes, 4U) != 2U ||
        ReadU32LittleEndianUVE(bytes, 8U) != bytes.size()) {
        return false;
    }
    const std::size_t jsonLength = ReadU32LittleEndianUVE(bytes, 12U);
    if (ReadU32LittleEndianUVE(bytes, 16U) != 0x4E4F534AU || jsonLength > bytes.size() - 20U) {
        return false;
    }
    const auto* const jsonBegin = reinterpret_cast<const char*>(bytes.data() + 20U);
    if (!ParseJsonDocumentUVE(std::string_view{jsonBegin, jsonLength}, outDocument)) {
        return false;
    }

    bool foundBinaryChunk = false;
    std::size_t offset = 20U + jsonLength;
    while (offset < bytes.size()) {
        if (bytes.size() - offset < kGlbChunkHeaderBytesUVE) {
            return false;
        }
        const std::size_t chunkLength = ReadU32LittleEndianUVE(bytes, offset);
        const std::uint32_t chunkType = ReadU32LittleEndianUVE(bytes, offset + 4U);
        offset += kGlbChunkHeaderBytesUVE;
        if (chunkLength > bytes.size() - offset) {
            return false;
        }
        if (chunkType == 0x004E4942U) {
            if (foundBinaryChunk) {
                return false;
            }
            outBuffer.assign(bytes.begin() + static_cast<std::ptrdiff_t>(offset),
                             bytes.begin() + static_cast<std::ptrdiff_t>(offset + chunkLength));
            foundBinaryChunk = true;
        }
        offset += chunkLength;
    }
    return foundBinaryChunk;
}

[[nodiscard]] bool LoadGltfDocumentAndBufferUVE(const std::filesystem::path& sourcePath,
                                                const std::vector<std::byte>& sourceBytes,
                                                nlohmann::json& outDocument,
                                                std::vector<std::byte>& outBuffer) {
    std::string extension = sourcePath.extension().string();
    std::transform(extension.begin(), extension.end(), extension.begin(), [](const unsigned char value) {
        return static_cast<char>(std::tolower(value));
    });
    if (extension == ".glb") {
        return ExtractGlbDocumentAndBufferUVE(sourceBytes, outDocument, outBuffer);
    }
    const auto* const jsonBegin = reinterpret_cast<const char*>(sourceBytes.data());
    if (!ParseJsonDocumentUVE(std::string_view{jsonBegin, sourceBytes.size()}, outDocument)) {
        return false;
    }

    if (!outDocument.contains("buffers") || !outDocument.at("buffers").is_array() ||
        outDocument.at("buffers").size() != 1U || !outDocument.at("buffers").at(0).is_object()) {
        return false;
    }
    const auto& buffer = outDocument.at("buffers").at(0);
    const auto byteLength = ReadJsonUintUVE(buffer, "byteLength", true);
    if (!byteLength.has_value() || *byteLength > kMaximumGltfDataUriDecodedBytesUVE) {
        return false;
    }
    if (!buffer.contains("uri") || !buffer.at("uri").is_string()) {
        return false;
    }
    const std::string uri = buffer.at("uri").get<std::string>();
    const auto uriKind = ClassifyGltfResourceUriUVE(uri);
    if (uriKind == GltfResourceUriKindUVE::DataUri) {
        if (!DecodeGltfDataUriUVE(uri, outBuffer, kMaximumGltfDataUriDecodedBytesUVE)) {
            return false;
        }
    } else if (uriKind == GltfResourceUriKindUVE::RelativePath) {
        std::string normalizedUri = uri;
        std::replace(normalizedUri.begin(), normalizedUri.end(), '\\', '/');
        if (!ReadBoundedFileBytesUVE(sourcePath.parent_path() / normalizedUri, outBuffer)) {
            return false;
        }
    } else {
        return false;
    }
    return outBuffer.size() >= *byteLength;
}

[[nodiscard]] std::optional<BufferViewDefinitionUVE> ReadBufferViewUVE(const nlohmann::json& document,
                                                                       const std::uint64_t index) {
    if (!document.contains("bufferViews") || !document.at("bufferViews").is_array() ||
        index >= document.at("bufferViews").size() || !document.at("bufferViews").at(index).is_object()) {
        return std::nullopt;
    }
    const auto& value = document.at("bufferViews").at(index);
    const auto buffer = ReadJsonUintUVE(value, "buffer", true);
    const auto byteOffset = ReadJsonUintUVE(value, "byteOffset", false);
    const auto byteLength = ReadJsonUintUVE(value, "byteLength", true);
    const auto byteStride = ReadJsonUintUVE(value, "byteStride", false);
    if (!buffer || !byteOffset || !byteLength || !byteStride) {
        return std::nullopt;
    }
    return BufferViewDefinitionUVE{*buffer, *byteOffset, *byteLength, *byteStride};
}

[[nodiscard]] std::optional<AccessorDefinitionUVE> ReadAccessorUVE(const nlohmann::json& document,
                                                                   const std::uint64_t index) {
    if (!document.contains("accessors") || !document.at("accessors").is_array() ||
        index >= document.at("accessors").size() || !document.at("accessors").at(index).is_object()) {
        return std::nullopt;
    }
    const auto& value = document.at("accessors").at(index);
    const auto bufferView = ReadJsonUintUVE(value, "bufferView", true);
    const auto byteOffset = ReadJsonUintUVE(value, "byteOffset", false);
    const auto count = ReadJsonUintUVE(value, "count", true);
    const auto componentType = ReadJsonU32UVE(value, "componentType");
    if (!bufferView || !byteOffset || !count || !componentType || !value.contains("type") ||
        !value.at("type").is_string() || *count > kMaximumGltfAccessorElementsUVE) {
        return std::nullopt;
    }
    return AccessorDefinitionUVE{*bufferView, *byteOffset, *count, *componentType, value.at("type").get<std::string>()};
}

[[nodiscard]] std::optional<GltfComponentTypeUVE> ToComponentTypeUVE(const std::uint32_t value) noexcept {
    switch (value) {
    case 5121U:
        return GltfComponentTypeUVE::UnsignedByte;
    case 5123U:
        return GltfComponentTypeUVE::UnsignedShort;
    case 5125U:
        return GltfComponentTypeUVE::UnsignedInt;
    case 5126U:
        return GltfComponentTypeUVE::Float;
    default:
        return std::nullopt;
    }
}

[[nodiscard]] bool AddU64UVE(const std::uint64_t left, const std::uint64_t right,
                             std::uint64_t& outValue) noexcept {
    if (right > std::numeric_limits<std::uint64_t>::max() - left) {
        return false;
    }
    outValue = left + right;
    return true;
}

[[nodiscard]] std::optional<GltfAccessorViewUVE> BuildAccessorViewUVE(
    const nlohmann::json& document, const std::vector<std::byte>& buffer, const std::uint64_t accessorIndex,
    const std::string_view expectedType, const bool allowIndices) {
    const auto accessor = ReadAccessorUVE(document, accessorIndex);
    if (!accessor || accessor->type != expectedType) {
        return std::nullopt;
    }
    const auto componentType = ToComponentTypeUVE(accessor->componentType);
    if (!componentType || (!allowIndices && *componentType != GltfComponentTypeUVE::Float) ||
        (allowIndices && *componentType == GltfComponentTypeUVE::Float)) {
        return std::nullopt;
    }
    const auto view = ReadBufferViewUVE(document, accessor->bufferView);
    if (!view || view->buffer != 0U) {
        return std::nullopt;
    }
    std::uint64_t totalOffset = 0U;
    std::uint64_t viewEnd = 0U;
    if (!AddU64UVE(view->byteOffset, view->byteLength, viewEnd) || viewEnd > buffer.size() ||
        !AddU64UVE(view->byteOffset, accessor->byteOffset, totalOffset) ||
        accessor->byteOffset > view->byteLength) {
        return std::nullopt;
    }
    const std::uint64_t componentSize = *componentType == GltfComponentTypeUVE::UnsignedByte
                                             ? 1U
                                             : *componentType == GltfComponentTypeUVE::UnsignedShort ? 2U : 4U;
    const std::uint64_t elementSize = expectedType == "VEC3" ? componentSize * 3U
                                      : expectedType == "VEC2" ? componentSize * 2U
                                                               : componentSize;
    const std::uint64_t stride = view->byteStride == 0U ? elementSize : view->byteStride;
    if (!ValidateGltfAccessorSpanUVE(view->byteLength, accessor->byteOffset, accessor->count, stride, elementSize) ||
        !ValidateGltfAccessorSpanUVE(buffer.size(), totalOffset, accessor->count, stride, elementSize)) {
        return std::nullopt;
    }
    return GltfAccessorViewUVE{std::span<const std::byte>{buffer.data(), buffer.size()}, totalOffset,
                               accessor->count, view->byteStride, *componentType};
}

[[nodiscard]] bool ConvertDocumentUVE(const nlohmann::json& document, const std::vector<std::byte>& buffer,
                                      MeshAssetUVE& outMesh) {
    if (!document.contains("buffers") || !document.at("buffers").is_array() ||
        document.at("buffers").size() != 1U || !document.at("buffers").at(0).is_object()) {
        return false;
    }
    const auto declaredBufferLength = ReadJsonUintUVE(document.at("buffers").at(0), "byteLength", true);
    if (!declaredBufferLength.has_value() || *declaredBufferLength > kMaximumGltfDataUriDecodedBytesUVE ||
        buffer.size() < *declaredBufferLength) {
        return false;
    }
    if (!document.contains("meshes") || !document.at("meshes").is_array() || document.at("meshes").size() != 1U ||
        !document.at("meshes").at(0).is_object() || !document.at("meshes").at(0).contains("primitives") ||
        !document.at("meshes").at(0).at("primitives").is_array() ||
        document.at("meshes").at(0).at("primitives").size() != 1U) {
        return false;
    }
    const auto& primitive = document.at("meshes").at(0).at("primitives").at(0);
    if (!primitive.is_object() || !primitive.contains("attributes") || !primitive.at("attributes").is_object()) {
        return false;
    }
    const auto& attributes = primitive.at("attributes");
    const auto positionIndex = ReadJsonUintUVE(attributes, "POSITION", true);
    if (!positionIndex) {
        return false;
    }
    GltfPrimitiveSourceUVE source;
    source.mode = primitive.contains("mode") ? ReadJsonU32UVE(primitive, "mode").value_or(0U) : 4U;
    const auto positions = BuildAccessorViewUVE(document, buffer, *positionIndex, "VEC3", false);
    if (!positions) {
        return false;
    }
    source.positions = *positions;
    if (attributes.contains("NORMAL")) {
        const auto normalIndex = ReadJsonUintUVE(attributes, "NORMAL", true);
        if (!normalIndex) return false;
        const auto normals = BuildAccessorViewUVE(document, buffer, *normalIndex, "VEC3", false);
        if (!normals) return false;
        source.normals = *normals;
    }
    if (attributes.contains("TEXCOORD_0")) {
        const auto texcoordIndex = ReadJsonUintUVE(attributes, "TEXCOORD_0", true);
        if (!texcoordIndex) return false;
        const auto texcoords = BuildAccessorViewUVE(document, buffer, *texcoordIndex, "VEC2", false);
        if (!texcoords) return false;
        source.texcoords0 = *texcoords;
    }
    if (primitive.contains("indices")) {
        const auto indexAccessor = ReadJsonUintUVE(primitive, "indices", true);
        if (!indexAccessor) return false;
        const auto indices = BuildAccessorViewUVE(document, buffer, *indexAccessor, "SCALAR", true);
        if (!indices) return false;
        source.indices = *indices;
    }
    return ConvertGltfPrimitiveUVE(source, outMesh);
}

[[nodiscard]] bool SaveMeshAssetAtomicallyUVE(const MeshAssetUVE& mesh,
                                              const std::filesystem::path& destinationPath) {
    std::error_code errorCode;
    if (const std::filesystem::path parent = destinationPath.parent_path(); !parent.empty()) {
        std::filesystem::create_directories(parent, errorCode);
        if (errorCode) {
            UVE_ERROR("GltfImporterUVE: failed to create destination directory \"{}\": {}", parent.string(),
                      errorCode.message());
            return false;
        }
    }
    const std::filesystem::path temporaryPath = destinationPath.string() + ".uve_gltf_tmp";
    std::filesystem::remove(temporaryPath, errorCode);
    if (!SaveMeshAssetUVE(mesh, temporaryPath)) {
        std::filesystem::remove(temporaryPath, errorCode);
        return false;
    }
    std::filesystem::rename(temporaryPath, destinationPath, errorCode);
    if (errorCode) {
        UVE_ERROR("GltfImporterUVE: failed to publish destination \"{}\": {}", destinationPath.string(),
                  errorCode.message());
        std::filesystem::remove(temporaryPath, errorCode);
        return false;
    }
    return true;
}

[[nodiscard]] bool ImportGltfSourceUVE(const std::filesystem::path& sourcePath,
                                       const std::filesystem::path& destinationPath,
                                       const AssetImportSettingsUVE& /*settings*/) {
    try {
        if (destinationPath.extension() != ".uvemodel") {
            UVE_ERROR("GltfImporterUVE: destination \"{}\" must use the .uvemodel extension",
                      destinationPath.string());
            return false;
        }
        std::vector<std::byte> sourceBytes;
        if (!ReadBoundedFileBytesUVE(sourcePath, sourceBytes)) return false;
        nlohmann::json document;
        std::vector<std::byte> buffer;
        if (!LoadGltfDocumentAndBufferUVE(sourcePath, sourceBytes, document, buffer)) {
            UVE_ERROR("GltfImporterUVE: source \"{}\" failed bounded glTF/GLB structure or buffer loading",
                      sourcePath.string());
            return false;
        }
        MeshAssetUVE mesh;
        if (!ConvertDocumentUVE(document, buffer, mesh)) {
            UVE_ERROR("GltfImporterUVE: source \"{}\" failed bounded one-primitive mesh conversion",
                      sourcePath.string());
            return false;
        }
        return SaveMeshAssetAtomicallyUVE(mesh, destinationPath);
    } catch (const std::bad_alloc&) {
        return false;
    }
}

} // namespace

void RegisterGltfImporterUVE(IAssetImporterUVE& importer) {
    importer.RegisterImporterUVE("gltf", &ImportGltfSourceUVE);
    importer.RegisterImporterUVE("glb", &ImportGltfSourceUVE);
}

} // namespace UVE::Asset
