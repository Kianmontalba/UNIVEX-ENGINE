// Copyright (c) 2026 UniVex Studios. All Rights Reserved.

#include <algorithm>
#include <iostream>
#include <string>
#include <string_view>

#include "uve/project_check/project_checker_uve.h"

namespace {

constexpr int kSuccessExitCodeUVE = 0;
constexpr int kHealthErrorExitCodeUVE = 1;
constexpr int kUsageExitCodeUVE = 2;
constexpr int kOperationalFailureExitCodeUVE = 3;

void PrintUsageUVE() {
    std::cerr << "Usage: uve_project_check --project-root <path> [--asset-db <path>] [--format text|json]\n";
}

} // namespace

int main(int argc, char** argv) {
    UVE::ProjectCheck::ProjectCheckOptionsUVE options{};
    std::string format = "text";
    for (int index = 1; index < argc; ++index) {
        const std::string_view argument = argv[index];
        if ((argument == "--project-root" || argument == "--asset-db" || argument == "--format") && index + 1 >= argc) {
            PrintUsageUVE();
            return kUsageExitCodeUVE;
        }
        if (argument == "--project-root") {
            options.projectRoot = argv[++index];
        } else if (argument == "--asset-db") {
            options.assetDatabasePath = argv[++index];
        } else if (argument == "--format") {
            format = argv[++index];
        } else {
            PrintUsageUVE();
            return kUsageExitCodeUVE;
        }
    }
    if (options.projectRoot.empty() || (format != "text" && format != "json")) {
        PrintUsageUVE();
        return kUsageExitCodeUVE;
    }
    try {
        const UVE::ProjectCheck::ProjectCheckReportUVE report = UVE::ProjectCheck::ProjectCheckerUVE{}.RunUVE(options);
        if (format == "json") {
            std::cout << UVE::ProjectCheck::RenderProjectCheckJsonUVE(report);
        } else {
            std::cout << UVE::ProjectCheck::RenderProjectCheckTextUVE(report);
        }
        const bool invalidRoot = std::any_of(report.diagnostics.begin(), report.diagnostics.end(),
                                             [](const UVE::ProjectCheck::ProjectCheckDiagnosticUVE& diagnostic) {
                                                 return diagnostic.code == "project.root.invalid";
                                             });
        if (invalidRoot) {
            return kOperationalFailureExitCodeUVE;
        }
        return report.HasErrorsUVE() ? kHealthErrorExitCodeUVE : kSuccessExitCodeUVE;
    } catch (const std::exception& exception) {
        std::cerr << "uve_project_check operational failure: " << exception.what() << '\n';
    } catch (...) {
        std::cerr << "uve_project_check operational failure: unknown exception\n";
    }
    return kOperationalFailureExitCodeUVE;
}
