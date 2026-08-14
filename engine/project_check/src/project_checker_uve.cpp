// Copyright (c) 2026 UniVex Studios. All Rights Reserved.

#include "uve/project_check/project_checker_uve.h"

#include <algorithm>
#include <exception>
#include <optional>
#include <string_view>
#include <system_error>

#include "uve/asset/asset_database_uve.h"
#include "uve/asset/uve_file_envelope_uve.h"

namespace UVE::ProjectCheck {
namespace {

[[nodiscard]] const char* SeverityLabelUVE(const ProjectCheckSeverityUVE severity) noexcept {
    return severity == ProjectCheckSeverityUVE::Error ? "error" : "warning";
}

[[nodiscard]] std::string EscapeJsonUVE(const std::string_view value) {
    std::string output;
    output.reserve(value.size() + 8U);
    for (const char character : value) {
        switch (character) {
            case '\\': output += "\\\\"; break;
            case '"': output += "\\\""; break;
            case '\n': output += "\\n"; break;
            case '\r': output += "\\r"; break;
            case '\t': output += "\\t"; break;
            default: output += character; break;
        }
    }
    return output;
}

[[nodiscard]] std::optional<Asset::AssetKindUVE> ExpectedKindUVE(const std::filesystem::path& path) {
    const std::string extension = path.extension().string();
    if (extension == ".uvescene") return Asset::AssetKindUVE::Scene;
    if (extension == ".uveprefab") return Asset::AssetKindUVE::Prefab;
    if (extension == ".uvebundle") return Asset::AssetKindUVE::Bundle;
    if (extension == ".uvemesh") return Asset::AssetKindUVE::Mesh;
    if (extension == ".uvetexture") return Asset::AssetKindUVE::Texture;
    if (extension == ".uveshader") return Asset::AssetKindUVE::Shader;
    if (extension == ".uvematerial") return Asset::AssetKindUVE::Material;
    if (extension == ".uvesave") return Asset::AssetKindUVE::Save;
    return std::nullopt;
}

void AddUVE(ProjectCheckReportUVE& report, const ProjectCheckSeverityUVE severity, std::string code,
            std::filesystem::path path, std::string message, std::string recovery) {
    report.diagnostics.push_back(ProjectCheckDiagnosticUVE{
        severity, std::move(code), std::move(path), std::move(message), std::move(recovery)});
}

void SortUVE(ProjectCheckReportUVE& report) {
    std::sort(report.diagnostics.begin(), report.diagnostics.end(), [](const ProjectCheckDiagnosticUVE& lhs,
                                                                         const ProjectCheckDiagnosticUVE& rhs) {
        if (lhs.path.generic_string() != rhs.path.generic_string()) return lhs.path.generic_string() < rhs.path.generic_string();
        if (lhs.code != rhs.code) return lhs.code < rhs.code;
        return lhs.message < rhs.message;
    });
}

} // namespace

bool ProjectCheckReportUVE::HasErrorsUVE() const noexcept {
    return std::any_of(diagnostics.begin(), diagnostics.end(), [](const ProjectCheckDiagnosticUVE& diagnostic) {
        return diagnostic.severity == ProjectCheckSeverityUVE::Error;
    });
}

ProjectCheckReportUVE ProjectCheckerUVE::RunUVE(const ProjectCheckOptionsUVE& options) const {
    ProjectCheckReportUVE report{};
    report.projectRoot = options.projectRoot.lexically_normal();
    std::error_code error;
    if (report.projectRoot.empty() || !std::filesystem::is_directory(report.projectRoot, error) || error) {
        AddUVE(report, ProjectCheckSeverityUVE::Error, "project.root.invalid", report.projectRoot,
               "Project root is missing, inaccessible, or not a directory.", "Provide an existing readable project root.");
        return report;
    }

    const std::filesystem::path databasePath = options.assetDatabasePath.empty()
        ? report.projectRoot / ".uveassetdb"
        : (options.assetDatabasePath.is_absolute() ? options.assetDatabasePath
                                                   : report.projectRoot / options.assetDatabasePath);
    const std::filesystem::path normalizedDatabasePath = databasePath.lexically_normal();
    Asset::AssetDatabaseUVE database;
    if (std::filesystem::exists(normalizedDatabasePath, error) && !database.LoadUVE(normalizedDatabasePath)) {
        AddUVE(report, ProjectCheckSeverityUVE::Error, "registry.load.failed", normalizedDatabasePath,
               "Asset registry could not be parsed.", "Restore valid .uveassetdb JSON from source control.");
    } else if (!std::filesystem::exists(normalizedDatabasePath, error)) {
        AddUVE(report, ProjectCheckSeverityUVE::Warning, "registry.missing", normalizedDatabasePath,
               "Asset registry is absent; project content remains inspectable.",
               "Create or restore the registry when registered asset identity is required.");
    }

    std::vector<std::filesystem::path> registeredPaths;
    for (const Asset::AssetRecordUVE& record : database.GetRegisteredAssetsUVE()) {
        const std::filesystem::path path = record.path.is_absolute() ? record.path.lexically_normal()
                                                                       : (report.projectRoot / record.path).lexically_normal();
        registeredPaths.push_back(path);
        std::error_code statusError;
        const std::filesystem::file_status status = std::filesystem::symlink_status(path, statusError);
        if (statusError || !std::filesystem::is_regular_file(status)) {
            AddUVE(report, ProjectCheckSeverityUVE::Error, "registry.path.missing", record.path,
                   "Registered asset does not resolve to an ordinary readable file.",
                   "Restore the file or fix the registry record explicitly.");
        }
    }

    std::filesystem::recursive_directory_iterator iterator(
        report.projectRoot, std::filesystem::directory_options::skip_permission_denied, error);
    const std::filesystem::recursive_directory_iterator end;
    if (error) {
        AddUVE(report, ProjectCheckSeverityUVE::Error, "project.enumeration.failed", report.projectRoot,
               "Project content cannot be enumerated.", "Fix root permissions or path accessibility.");
        return report;
    }
    for (; iterator != end; iterator.increment(error)) {
        if (error) {
            AddUVE(report, ProjectCheckSeverityUVE::Error, "project.entry.unreadable", report.projectRoot,
                   "A project directory entry could not be enumerated.", "Fix permissions and rerun the checker.");
            error.clear();
            continue;
        }
        const std::filesystem::directory_entry entry = *iterator;
        std::error_code statusError;
        const std::filesystem::file_status status = entry.symlink_status(statusError);
        if (statusError || std::filesystem::is_symlink(status) || !std::filesystem::is_regular_file(status)) continue;
        const std::filesystem::path path = entry.path().lexically_normal();
        try {
            const std::optional<Asset::AssetKindUVE> expected = ExpectedKindUVE(path);
            if (!expected.has_value()) continue;
            const auto decoded = Asset::ReadUveFileUVE(path);
            if (!decoded.has_value()) {
                AddUVE(report, ProjectCheckSeverityUVE::Error, "envelope.decode.failed", path,
                       "Universal asset envelope is malformed or unsupported.",
                       "Restore or re-export this asset from a trusted source.");
            } else if (decoded->first.assetType != *expected) {
                AddUVE(report, ProjectCheckSeverityUVE::Error, "envelope.kind.mismatch", path,
                       "Envelope asset kind does not match the file extension.",
                       "Use the correct extension or re-export with the correct asset kind.");
            }
            if (std::find(registeredPaths.begin(), registeredPaths.end(), path) == registeredPaths.end()) {
                AddUVE(report, ProjectCheckSeverityUVE::Warning, "registry.file.unregistered", path,
                       "Supported asset file is not represented in the current registry snapshot.",
                       "Register the asset through the project asset workflow if GUID identity is needed.");
            }
        } catch (const std::exception&) {
            AddUVE(report, ProjectCheckSeverityUVE::Error, "validation.exception", path,
                   "An unexpected validation exception was isolated to this file.",
                   "Inspect or restore this file; other project files were still checked.");
        } catch (...) {
            AddUVE(report, ProjectCheckSeverityUVE::Error, "validation.exception", path,
                   "An unknown validation failure was isolated to this file.",
                   "Inspect or restore this file; other project files were still checked.");
        }
    }
    SortUVE(report);
    return report;
}

std::string RenderProjectCheckTextUVE(const ProjectCheckReportUVE& report) {
    std::string output = "Project health: " + report.projectRoot.generic_string() + "\n";
    if (report.diagnostics.empty()) return output + "OK: no diagnostics.\n";
    for (const ProjectCheckDiagnosticUVE& diagnostic : report.diagnostics) {
        output += std::string{SeverityLabelUVE(diagnostic.severity)} + " [" + diagnostic.code + "] " +
                  diagnostic.path.generic_string() + ": " + diagnostic.message + " Recovery: " + diagnostic.recovery + "\n";
    }
    return output;
}

std::string RenderProjectCheckJsonUVE(const ProjectCheckReportUVE& report) {
    std::string output = "{\"projectRoot\":\"" + EscapeJsonUVE(report.projectRoot.generic_string()) +
                         "\",\"hasErrors\":" + (report.HasErrorsUVE() ? "true" : "false") + ",\"diagnostics\":[";
    for (std::size_t index = 0U; index < report.diagnostics.size(); ++index) {
        const ProjectCheckDiagnosticUVE& diagnostic = report.diagnostics[index];
        if (index != 0U) output += ',';
        output += "{\"severity\":\"" + std::string{SeverityLabelUVE(diagnostic.severity)} + "\",\"code\":\"" +
                  EscapeJsonUVE(diagnostic.code) + "\",\"path\":\"" + EscapeJsonUVE(diagnostic.path.generic_string()) +
                  "\",\"message\":\"" + EscapeJsonUVE(diagnostic.message) + "\",\"recovery\":\"" +
                  EscapeJsonUVE(diagnostic.recovery) + "\"}";
    }
    return output + "]}\n";
}

} // namespace UVE::ProjectCheck
