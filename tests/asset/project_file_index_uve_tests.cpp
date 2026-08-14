// Copyright (c) 2026 UniVex Studios. All Rights Reserved.

#include "uve/asset/project_file_index_uve.h"

#include <algorithm>
#include <filesystem>
#include <fstream>
#include <string>
#include <system_error>
#include <vector>

#include <gtest/gtest.h>

#include "uve/asset/asset_database_uve.h"

namespace UVE::Asset::Tests {
namespace {

void WriteFixtureFileUVE(const std::filesystem::path& path, const std::string_view contents) {
    if (!path.parent_path().empty()) {
        std::filesystem::create_directories(path.parent_path());
    }
    std::ofstream file(path, std::ios::binary);
    ASSERT_TRUE(file.is_open());
    file << contents;
    ASSERT_TRUE(file.good());
}

[[nodiscard]] std::vector<std::string> SnapshotPathsUVE(const ProjectFileSnapshotUVE& snapshot) {
    std::vector<std::string> paths;
    paths.reserve(snapshot.entries.size());
    for (const ProjectFileEntryUVE& entry : snapshot.entries) {
        paths.push_back(entry.relativePath.generic_string());
    }
    return paths;
}

TEST(ProjectFileIndexUVETest, RefreshUVE_MissingRootPublishesValidEmptySnapshot) {
    const std::filesystem::path root = "uve_project_file_index_tests_missing";
    std::filesystem::remove_all(root);

    AssetDatabaseUVE assetDatabase;
    ProjectFileIndexUVE index(root);

    ASSERT_TRUE(index.RefreshUVE(assetDatabase));
    const ProjectFileSnapshotUVE snapshot = index.GetSnapshotUVE();
    EXPECT_EQ(snapshot.contentRoot, root.lexically_normal());
    EXPECT_FALSE(snapshot.contentRootExists);
    EXPECT_TRUE(snapshot.entries.empty());
    EXPECT_EQ(snapshot.refreshGeneration, 1U);
}

TEST(ProjectFileIndexUVETest, GetSnapshotUVE_UsesCachedTreeUntilExplicitRefresh) {
    const std::filesystem::path root = "uve_project_file_index_tests_cached";
    std::filesystem::remove_all(root);
    WriteFixtureFileUVE(root / "first.txt", "first");

    AssetDatabaseUVE assetDatabase;
    ProjectFileIndexUVE index(root);
    ASSERT_TRUE(index.RefreshUVE(assetDatabase));
    const ProjectFileSnapshotUVE firstSnapshot = index.GetSnapshotUVE();
    ASSERT_EQ(SnapshotPathsUVE(firstSnapshot), std::vector<std::string>({"first.txt"}));

    WriteFixtureFileUVE(root / "second.txt", "second");
    const ProjectFileSnapshotUVE cachedSnapshot = index.GetSnapshotUVE();
    EXPECT_EQ(cachedSnapshot.refreshGeneration, firstSnapshot.refreshGeneration);
    EXPECT_EQ(SnapshotPathsUVE(cachedSnapshot), SnapshotPathsUVE(firstSnapshot));

    ASSERT_TRUE(index.RefreshUVE(assetDatabase));
    const ProjectFileSnapshotUVE refreshedSnapshot = index.GetSnapshotUVE();
    EXPECT_EQ(refreshedSnapshot.refreshGeneration, firstSnapshot.refreshGeneration + 1U);
    EXPECT_EQ(SnapshotPathsUVE(refreshedSnapshot), std::vector<std::string>({"first.txt", "second.txt"}));

    std::filesystem::remove_all(root);
}

TEST(ProjectFileIndexUVETest, RefreshUVE_NormalizesRelativeTrailingSeparatorBeforeRootBoundaryComparison) {
    const std::filesystem::path root = "uve_project_file_index_tests_trailing_separator";
    const std::filesystem::path configuredRoot = root.generic_string() + "/";
    std::filesystem::remove_all(root);
    WriteFixtureFileUVE(root / "nested" / "asset.txt", "asset");

    AssetDatabaseUVE assetDatabase;
    ProjectFileIndexUVE index(configuredRoot);

    ASSERT_TRUE(index.RefreshUVE(assetDatabase));
    const ProjectFileSnapshotUVE snapshot = index.GetSnapshotUVE();
    EXPECT_EQ(snapshot.contentRoot, root);
    EXPECT_TRUE(snapshot.contentRootExists);
    EXPECT_EQ(SnapshotPathsUVE(snapshot), std::vector<std::string>({"nested", "nested/asset.txt"}));

    std::filesystem::remove_all(root);
}

TEST(ProjectFileIndexUVETest, RefreshUVE_BuildsDeterministicFoldersFirstTreeAndCorrelatesInRootRecords) {
    const std::filesystem::path root = "uve_project_file_index_tests_tree";
    const std::filesystem::path outside = "uve_project_file_index_tests_outside.txt";
    std::filesystem::remove_all(root);
    std::filesystem::remove(outside);
    WriteFixtureFileUVE(root / "BFolder" / "other.txt", "other");
    WriteFixtureFileUVE(root / "aFolder" / "registered.txt", "registered");
    WriteFixtureFileUVE(root / "root.txt", "root");
    WriteFixtureFileUVE(outside, "outside");

    AssetDatabaseUVE assetDatabase;
    const AssetGuidUVE registeredGuid = assetDatabase.RegisterUVE(root / "aFolder" / "registered.txt");
    const AssetGuidUVE outsideGuid = assetDatabase.RegisterUVE(outside);
    ASSERT_NE(registeredGuid, kInvalidAssetGuidUVE);
    ASSERT_NE(outsideGuid, kInvalidAssetGuidUVE);

    ProjectFileIndexUVE index(root);
    ASSERT_TRUE(index.RefreshUVE(assetDatabase));
    const ProjectFileSnapshotUVE snapshot = index.GetSnapshotUVE();
    EXPECT_TRUE(snapshot.contentRootExists);
    EXPECT_EQ(SnapshotPathsUVE(snapshot),
              std::vector<std::string>({"BFolder", "aFolder", "BFolder/other.txt", "aFolder/registered.txt", "root.txt"}));

    const auto registeredIt = std::find_if(snapshot.entries.begin(), snapshot.entries.end(), [](const ProjectFileEntryUVE& entry) {
        return entry.relativePath.generic_string() == "aFolder/registered.txt";
    });
    ASSERT_NE(registeredIt, snapshot.entries.end());
    ASSERT_TRUE(registeredIt->registeredAssetGuid.has_value());
    EXPECT_EQ(*registeredIt->registeredAssetGuid, registeredGuid);
    EXPECT_TRUE(std::none_of(snapshot.entries.begin(), snapshot.entries.end(), [outsideGuid](const ProjectFileEntryUVE& entry) {
        return entry.registeredAssetGuid == outsideGuid;
    }));

    std::filesystem::remove_all(root);
    std::filesystem::remove(outside);
}

TEST(ProjectFileIndexUVETest, RefreshUVE_SkipsAllSymlinksAndRetainsLastGoodSnapshotWhenRootBecomesSymlink) {
    const std::filesystem::path root = "uve_project_file_index_tests_symlink_root";
    const std::filesystem::path outside = "uve_project_file_index_tests_symlink_outside";
    std::filesystem::remove_all(root);
    std::filesystem::remove_all(outside);
    WriteFixtureFileUVE(root / "safe.txt", "safe");
    WriteFixtureFileUVE(outside / "outside.txt", "outside");

    std::error_code errorCode;
    std::filesystem::create_directory_symlink(outside, root / "outside_link", errorCode);
    if (errorCode) {
        GTEST_SKIP() << "Host does not permit test symlink creation: " << errorCode.message();
    }
    errorCode.clear();
    std::filesystem::create_directory_symlink(root, root / "cycle_link", errorCode);
    if (errorCode) {
        GTEST_SKIP() << "Host does not permit cycle symlink creation: " << errorCode.message();
    }

    AssetDatabaseUVE assetDatabase;
    ProjectFileIndexUVE index(root);
    ASSERT_TRUE(index.RefreshUVE(assetDatabase));
    const ProjectFileSnapshotUVE goodSnapshot = index.GetSnapshotUVE();
    EXPECT_EQ(SnapshotPathsUVE(goodSnapshot), std::vector<std::string>({"safe.txt"}));

    std::filesystem::remove_all(root);
    errorCode.clear();
    std::filesystem::create_directory_symlink(outside, root, errorCode);
    if (errorCode) {
        GTEST_SKIP() << "Host does not permit root symlink creation: " << errorCode.message();
    }

    EXPECT_FALSE(index.RefreshUVE(assetDatabase));
    const ProjectFileSnapshotUVE retainedSnapshot = index.GetSnapshotUVE();
    EXPECT_EQ(retainedSnapshot.refreshGeneration, goodSnapshot.refreshGeneration);
    EXPECT_EQ(SnapshotPathsUVE(retainedSnapshot), SnapshotPathsUVE(goodSnapshot));

    std::filesystem::remove(root, errorCode);
    std::filesystem::remove_all(outside);
}

} // namespace
} // namespace UVE::Asset::Tests
