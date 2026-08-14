// Copyright (c) 2026 UniVex Studios. All Rights Reserved.

#include <chrono>
#include <filesystem>
#include <fstream>
#include <string>
#include <vector>

#include <gtest/gtest.h>

#include "uve/asset/uve_file_envelope_uve.h"
#include "uve/project_check/project_checker_uve.h"

namespace UVE::ProjectCheck::Tests {
namespace {

class ProjectCheckerUVETest : public ::testing::Test {
protected:
    void SetUp() override {
        const auto stamp = std::chrono::steady_clock::now().time_since_epoch().count();
        m_root = std::filesystem::temp_directory_path() / ("uve_project_check_tests_" + std::to_string(stamp));
        ASSERT_TRUE(std::filesystem::create_directories(m_root));
    }

    void TearDown() override { std::filesystem::remove_all(m_root); }

    [[nodiscard]] std::filesystem::path WriteEnvelopeUVE(const std::string& name, const Asset::AssetKindUVE kind) const {
        const std::filesystem::path path = m_root / name;
        EXPECT_TRUE(Asset::WriteUveFileUVE(path, kind, {}));
        return path;
    }

    std::filesystem::path m_root;
};

TEST_F(ProjectCheckerUVETest, RunUVE_IsReadOnlyAndRendersStableTextAndJson) {
    const std::filesystem::path scene = WriteEnvelopeUVE("valid.uvescene", Asset::AssetKindUVE::Scene);
    const std::string before = [] (const std::filesystem::path& path) {
        std::ifstream input(path, std::ios::binary);
        return std::string{std::istreambuf_iterator<char>(input), std::istreambuf_iterator<char>()};
    }(scene);

    const ProjectCheckReportUVE first = ProjectCheckerUVE{}.RunUVE(ProjectCheckOptionsUVE{m_root, {}});
    const ProjectCheckReportUVE second = ProjectCheckerUVE{}.RunUVE(ProjectCheckOptionsUVE{m_root, {}});
    EXPECT_FALSE(first.HasErrorsUVE());
    EXPECT_EQ(RenderProjectCheckTextUVE(first), RenderProjectCheckTextUVE(second));
    const std::string json = RenderProjectCheckJsonUVE(first);
    EXPECT_EQ(json, RenderProjectCheckJsonUVE(second));
    EXPECT_NE(json.find("\"hasErrors\":false,\"diagnostics\":["), std::string::npos);

    std::ifstream input(scene, std::ios::binary);
    const std::string after{std::istreambuf_iterator<char>(input), std::istreambuf_iterator<char>()};
    EXPECT_EQ(before, after);
    EXPECT_FALSE(std::filesystem::exists(m_root / ".uveassetdb"));
}

TEST_F(ProjectCheckerUVETest, RunUVE_ResolvesRelativeAssetDatabasePathFromProjectRoot) {
    const std::filesystem::path databaseDirectory = m_root / "registry";
    ASSERT_TRUE(std::filesystem::create_directories(databaseDirectory));
    const std::filesystem::path databasePath = databaseDirectory / "custom.uveassetdb";
    {
        std::ofstream databaseFile(databasePath);
        databaseFile << "malformed registry";
    }

    const ProjectCheckReportUVE report =
        ProjectCheckerUVE{}.RunUVE(ProjectCheckOptionsUVE{m_root, "registry/custom.uveassetdb"});
    const auto iterator = std::find_if(report.diagnostics.begin(), report.diagnostics.end(),
                                       [&databasePath](const ProjectCheckDiagnosticUVE& diagnostic) {
                                           return diagnostic.code == "registry.load.failed" &&
                                                  diagnostic.path == databasePath.lexically_normal();
                                       });
    EXPECT_NE(iterator, report.diagnostics.end());
}

TEST_F(ProjectCheckerUVETest, RunUVE_IsolatesCorruptFilesAndAggregatesIndependentDiagnostics) {
    static_cast<void>(WriteEnvelopeUVE("good.uvescene", Asset::AssetKindUVE::Scene));
    static_cast<void>(WriteEnvelopeUVE("wrong.uvemesh", Asset::AssetKindUVE::Blob));
    {
        std::ofstream corrupt(m_root / "broken.uvemesh", std::ios::binary);
        corrupt << "not-a-uve-envelope";
    }

    const ProjectCheckReportUVE report = ProjectCheckerUVE{}.RunUVE(ProjectCheckOptionsUVE{m_root, {}});
    ASSERT_TRUE(report.HasErrorsUVE());
    bool foundDecodeFailure = false;
    bool foundKindMismatch = false;
    bool foundWrongUnregistered = false;
    bool foundGoodUnregistered = false;
    for (const ProjectCheckDiagnosticUVE& diagnostic : report.diagnostics) {
        foundDecodeFailure |= diagnostic.code == "envelope.decode.failed" && diagnostic.path.filename() == "broken.uvemesh";
        foundKindMismatch |= diagnostic.code == "envelope.kind.mismatch" && diagnostic.path.filename() == "wrong.uvemesh";
        foundWrongUnregistered |= diagnostic.code == "registry.file.unregistered" && diagnostic.path.filename() == "wrong.uvemesh";
        foundGoodUnregistered |= diagnostic.code == "registry.file.unregistered" && diagnostic.path.filename() == "good.uvescene";
    }
    EXPECT_TRUE(foundDecodeFailure);
    EXPECT_TRUE(foundKindMismatch);
    EXPECT_TRUE(foundWrongUnregistered);
    EXPECT_TRUE(foundGoodUnregistered);
    EXPECT_TRUE(std::is_sorted(report.diagnostics.begin(), report.diagnostics.end(), [](const ProjectCheckDiagnosticUVE& lhs,
                                                                                          const ProjectCheckDiagnosticUVE& rhs) {
        if (lhs.path.generic_string() != rhs.path.generic_string()) return lhs.path.generic_string() < rhs.path.generic_string();
        return lhs.code < rhs.code;
    }));
}

} // namespace
} // namespace UVE::ProjectCheck::Tests
