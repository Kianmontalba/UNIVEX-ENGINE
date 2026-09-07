#pragma once

#include <cstddef>
#include <cstdint>
#include <filesystem>
#include <optional>
#include <string>

namespace UVE::Platform {

inline constexpr std::uint32_t kCurrentEditorProjectSchemaVersionUVE = 1U;
inline constexpr std::size_t kMaximumEditorProjectIdBytesUVE = 128U;
inline constexpr std::size_t kMaximumEditorProjectNameBytesUVE = 256U;
inline constexpr std::size_t kMaximumEditorProjectPathBytesUVE = 256U;

struct EditorProjectVersionUVE final {
    std::uint32_t major = 0U;
    std::uint32_t minor = 0U;
    std::uint32_t patch = 0U;
    std::uint32_t build = 0U;

    [[nodiscard]] bool operator==(const EditorProjectVersionUVE&) const = default;
};

struct EditorProjectPackageUVE final {
    std::uint32_t schemaVersion = kCurrentEditorProjectSchemaVersionUVE;
    std::uint64_t revision = 1U;
    std::string projectId;
    std::string displayName;
    EditorProjectVersionUVE engineVersion{};
    std::filesystem::path contentRoot;
    std::filesystem::path assetDatabasePath;
    std::filesystem::path settingsPath;

    [[nodiscard]] bool operator==(const EditorProjectPackageUVE&) const = default;
};

enum class EditorProjectPackageCodeUVE : std::uint8_t {
    Applied = 0,
    InvalidPath,
    InvalidPackage,
    UnsupportedSchema,
    ReadFailed,
    ParseFailed,
    WriteFailed,
    RevisionConflict,
    ProjectIdentityConflict,
};

struct EditorProjectPackageResultUVE final {
    EditorProjectPackageCodeUVE code = EditorProjectPackageCodeUVE::InvalidPackage;
    std::string message;

    [[nodiscard]] bool IsAcceptedUVE() const noexcept {
        return code == EditorProjectPackageCodeUVE::Applied;
    }
};

struct EditorProjectPackageLoadResultUVE final {
    EditorProjectPackageResultUVE result;
    std::optional<EditorProjectPackageUVE> package;

    [[nodiscard]] bool IsAcceptedUVE() const noexcept {
        return result.IsAcceptedUVE() && package.has_value();
    }
};

/// Stateless `.uveditor` project descriptor authority. The format is intentionally a portable
/// project/content manifest: it references existing relative content, asset-database, and settings
/// authorities but does not embed scene/assets, own credentials, or generate platform binaries.
/// Thread-safety: stateless and safe to call concurrently; each operation owns its local file data.
class EditorProjectPackageCodecUVE final {
public:
    [[nodiscard]] static EditorProjectPackageResultUVE ValidateUVE(
        const EditorProjectPackageUVE& package) noexcept;

    [[nodiscard]] static EditorProjectPackageLoadResultUVE LoadUVE(
        const std::filesystem::path& packagePath);

    [[nodiscard]] static EditorProjectPackageResultUVE SaveUVE(
        const std::filesystem::path& packagePath,
        const EditorProjectPackageUVE& package);

    /// Replaces an existing package only when the caller supplies the current revision, the project
    /// identity matches, the replacement validates, and its revision is strictly newer. Publication
    /// uses the same temporary-sibling rename as SaveUVE, so rejected updates never mutate the file.
    [[nodiscard]] static EditorProjectPackageResultUVE ApplyUpdateUVE(
        const std::filesystem::path& packagePath,
        std::uint64_t expectedRevision,
        const EditorProjectPackageUVE& replacement);
};

} // namespace UVE::Platform
