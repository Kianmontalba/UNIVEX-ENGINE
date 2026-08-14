// Copyright (c) 2026 UniVex Studios. All Rights Reserved.

#pragma once

#include <filesystem>
#include <string>
#include <vector>

namespace UVE::ProjectCheck {

enum class ProjectCheckSeverityUVE {
    Warning,
    Error,
};

struct ProjectCheckDiagnosticUVE final {
    ProjectCheckSeverityUVE severity = ProjectCheckSeverityUVE::Error;
    std::string code;
    std::filesystem::path path;
    std::string message;
    std::string recovery;
};

struct ProjectCheckReportUVE final {
    std::filesystem::path projectRoot;
    std::vector<ProjectCheckDiagnosticUVE> diagnostics;

    [[nodiscard]] bool HasErrorsUVE() const noexcept;
};

struct ProjectCheckOptionsUVE final {
    std::filesystem::path projectRoot;
    std::filesystem::path assetDatabasePath;
};

/// Read-only, deterministic project health validation. The checker never writes,
/// imports, registers assets, loads an editor scene, follows symlinks, or starts a window.
class ProjectCheckerUVE final {
public:
    [[nodiscard]] ProjectCheckReportUVE RunUVE(const ProjectCheckOptionsUVE& options) const;
};

[[nodiscard]] std::string RenderProjectCheckTextUVE(const ProjectCheckReportUVE& report);
[[nodiscard]] std::string RenderProjectCheckJsonUVE(const ProjectCheckReportUVE& report);

} // namespace UVE::ProjectCheck
