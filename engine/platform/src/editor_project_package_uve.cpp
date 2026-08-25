#include "uve/platform/editor_project_package_uve.h"

#include <cctype>
#include <exception>
#include <fstream>
#include <system_error>
#include <utility>

#include <nlohmann/json.hpp>

namespace UVE::Platform {
namespace {

using JsonUVE = nlohmann::json;

[[nodiscard]] EditorProjectPackageResultUVE MakeResultUVE(
    const EditorProjectPackageCodeUVE code, std::string message) {
    return {code, std::move(message)};
}

[[nodiscard]] bool IsBoundedTextUVE(const std::string& value, const std::size_t maximumBytes,
                                    const bool requireNonEmpty) noexcept {
    return (!requireNonEmpty || !value.empty()) && value.size() <= maximumBytes &&
           value.find('\0') == std::string::npos;
}

[[nodiscard]] bool IsProjectIdUVE(const std::string& value) noexcept {
    if (!IsBoundedTextUVE(value, kMaximumEditorProjectIdBytesUVE, true)) {
        return false;
    }
    for (const char rawCharacter : value) {
        const unsigned char character = static_cast<unsigned char>(rawCharacter);
        if (std::isalnum(character) == 0 && character != '-' && character != '_' && character != '.') {
            return false;
        }
    }
    return true;
}

[[nodiscard]] bool IsRelativePathUVE(const std::filesystem::path& path) noexcept {
    const std::string value = path.generic_string();
    if (value.empty() || value.size() > kMaximumEditorProjectPathBytesUVE ||
        value.find('\0') != std::string::npos || value.find('\\') != std::string::npos ||
        path.is_absolute() || path.has_root_name() || path.has_root_directory()) {
        return false;
    }
    for (const auto& component : path) {
        if (component == "." || component == ".." || component.empty()) {
            return false;
        }
    }
    return true;
}

[[nodiscard]] bool IsPackagePathUVE(const std::filesystem::path& path) noexcept {
    return path.extension() == ".uveditor" && !path.filename().empty();
}

[[nodiscard]] std::optional<EditorProjectPackageUVE> DecodePackageUVE(const JsonUVE& json) {
    if (!json.is_object() || json.value("format", "") != "uveditor") {
        return std::nullopt;
    }

    EditorProjectPackageUVE package;
    package.schemaVersion = json.at("schemaVersion").get<std::uint32_t>();
    package.revision = json.at("revision").get<std::uint64_t>();
    package.projectId = json.at("projectId").get<std::string>();
    package.displayName = json.at("displayName").get<std::string>();
    const JsonUVE& engineVersion = json.at("engineVersion");
    package.engineVersion.major = engineVersion.at("major").get<std::uint32_t>();
    package.engineVersion.minor = engineVersion.at("minor").get<std::uint32_t>();
    package.engineVersion.patch = engineVersion.at("patch").get<std::uint32_t>();
    package.engineVersion.build = engineVersion.at("build").get<std::uint32_t>();
    package.contentRoot = json.at("contentRoot").get<std::string>();
    package.assetDatabasePath = json.at("assetDatabasePath").get<std::string>();
    package.settingsPath = json.at("settingsPath").get<std::string>();
    return package;
}

[[nodiscard]] JsonUVE EncodePackageUVE(const EditorProjectPackageUVE& package) {
    return JsonUVE{{"format", "uveditor"},
                   {"schemaVersion", package.schemaVersion},
                   {"revision", package.revision},
                   {"projectId", package.projectId},
                   {"displayName", package.displayName},
                   {"engineVersion", {{"major", package.engineVersion.major},
                                       {"minor", package.engineVersion.minor},
                                       {"patch", package.engineVersion.patch},
                                       {"build", package.engineVersion.build}}},
                   {"contentRoot", package.contentRoot.generic_string()},
                   {"assetDatabasePath", package.assetDatabasePath.generic_string()},
                   {"settingsPath", package.settingsPath.generic_string()}};
}

[[nodiscard]] EditorProjectPackageResultUVE WriteJsonAtomicallyUVE(
    const std::filesystem::path& packagePath, const JsonUVE& json) {
    std::error_code error;
    const std::filesystem::path parent = packagePath.parent_path();
    if (!parent.empty()) {
        std::filesystem::create_directories(parent, error);
        if (error) {
            return MakeResultUVE(EditorProjectPackageCodeUVE::WriteFailed,
                                 "Unable to create the .uveditor parent directory.");
        }
    }

    const std::filesystem::path temporaryPath = packagePath.string() + ".tmp";
    {
        std::ofstream output(temporaryPath, std::ios::binary | std::ios::trunc);
        if (!output.is_open()) {
            return MakeResultUVE(EditorProjectPackageCodeUVE::WriteFailed,
                                 "Unable to open the temporary .uveditor file.");
        }
        output << json.dump(2) << '\n';
        output.flush();
        if (!output.good()) {
            output.close();
            std::filesystem::remove(temporaryPath, error);
            return MakeResultUVE(EditorProjectPackageCodeUVE::WriteFailed,
                                 "Unable to write the complete temporary .uveditor file.");
        }
    }

    error.clear();
    std::filesystem::rename(temporaryPath, packagePath, error);
    if (error) {
        std::filesystem::remove(temporaryPath, error);
        return MakeResultUVE(EditorProjectPackageCodeUVE::WriteFailed,
                             "Unable to atomically publish the .uveditor file.");
    }
    return MakeResultUVE(EditorProjectPackageCodeUVE::Applied, "The .uveditor package was published.");
}

} // namespace

EditorProjectPackageResultUVE EditorProjectPackageCodecUVE::ValidateUVE(
    const EditorProjectPackageUVE& package) noexcept {
    if (package.schemaVersion != kCurrentEditorProjectSchemaVersionUVE) {
        return MakeResultUVE(EditorProjectPackageCodeUVE::UnsupportedSchema,
                             "The .uveditor schema version is unsupported.");
    }
    if (package.revision == 0U) {
        return MakeResultUVE(EditorProjectPackageCodeUVE::InvalidPackage,
                             "The .uveditor revision must be nonzero.");
    }
    if (!IsProjectIdUVE(package.projectId)) {
        return MakeResultUVE(EditorProjectPackageCodeUVE::InvalidPackage,
                             "The .uveditor project ID is empty or contains unsupported characters.");
    }
    if (!IsBoundedTextUVE(package.displayName, kMaximumEditorProjectNameBytesUVE, true)) {
        return MakeResultUVE(EditorProjectPackageCodeUVE::InvalidPackage,
                             "The .uveditor display name is empty or exceeds its bound.");
    }
    if (!IsRelativePathUVE(package.contentRoot) || !IsRelativePathUVE(package.assetDatabasePath) ||
        !IsRelativePathUVE(package.settingsPath)) {
        return MakeResultUVE(EditorProjectPackageCodeUVE::InvalidPath,
                             "The .uveditor paths must be bounded, relative, normalized, and traversal-free.");
    }
    return MakeResultUVE(EditorProjectPackageCodeUVE::Applied, "The .uveditor package is valid.");
}

EditorProjectPackageLoadResultUVE EditorProjectPackageCodecUVE::LoadUVE(
    const std::filesystem::path& packagePath) {
    if (!IsPackagePathUVE(packagePath)) {
        return {MakeResultUVE(EditorProjectPackageCodeUVE::InvalidPath,
                              "The project package path must use the .uveditor extension."), std::nullopt};
    }

    std::ifstream input(packagePath, std::ios::binary);
    if (!input.is_open()) {
        return {MakeResultUVE(EditorProjectPackageCodeUVE::ReadFailed,
                              "Unable to open the .uveditor package."), std::nullopt};
    }

    try {
        const JsonUVE json = JsonUVE::parse(input);
        const std::optional<EditorProjectPackageUVE> package = DecodePackageUVE(json);
        if (!package.has_value()) {
            return {MakeResultUVE(EditorProjectPackageCodeUVE::ParseFailed,
                                  "The .uveditor package has an invalid format marker."), std::nullopt};
        }
        const EditorProjectPackageResultUVE validation = ValidateUVE(*package);
        if (!validation.IsAcceptedUVE()) {
            return {validation, std::nullopt};
        }
        return {validation, package};
    } catch (const std::exception&) {
        return {MakeResultUVE(EditorProjectPackageCodeUVE::ParseFailed,
                              "The .uveditor package is malformed or missing required fields."), std::nullopt};
    }
}

EditorProjectPackageResultUVE EditorProjectPackageCodecUVE::SaveUVE(
    const std::filesystem::path& packagePath, const EditorProjectPackageUVE& package) {
    if (!IsPackagePathUVE(packagePath)) {
        return MakeResultUVE(EditorProjectPackageCodeUVE::InvalidPath,
                             "The project package path must use the .uveditor extension.");
    }
    const EditorProjectPackageResultUVE validation = ValidateUVE(package);
    if (!validation.IsAcceptedUVE()) {
        return validation;
    }
    return WriteJsonAtomicallyUVE(packagePath, EncodePackageUVE(package));
}

EditorProjectPackageResultUVE EditorProjectPackageCodecUVE::ApplyUpdateUVE(
    const std::filesystem::path& packagePath, const std::uint64_t expectedRevision,
    const EditorProjectPackageUVE& replacement) {
    const EditorProjectPackageLoadResultUVE current = LoadUVE(packagePath);
    if (!current.IsAcceptedUVE()) {
        return current.result;
    }
    if (current.package->revision != expectedRevision) {
        return MakeResultUVE(EditorProjectPackageCodeUVE::RevisionConflict,
                             "The .uveditor update expected a different current revision.");
    }
    if (current.package->projectId != replacement.projectId) {
        return MakeResultUVE(EditorProjectPackageCodeUVE::ProjectIdentityConflict,
                             "The .uveditor update belongs to a different project.");
    }
    if (replacement.revision <= current.package->revision) {
        return MakeResultUVE(EditorProjectPackageCodeUVE::RevisionConflict,
                             "The .uveditor replacement revision must be strictly newer.");
    }
    const EditorProjectPackageResultUVE validation = ValidateUVE(replacement);
    if (!validation.IsAcceptedUVE()) {
        return validation;
    }
    return WriteJsonAtomicallyUVE(packagePath, EncodePackageUVE(replacement));
}

} // namespace UVE::Platform
