#include "uve/platform/editor_project_package_uve.h"

#include <atomic>
#include <filesystem>
#include <fstream>
#include <string>

#include <gtest/gtest.h>

namespace UVE::Platform::Tests {
namespace {

class EditorProjectPackageUVETest : public ::testing::Test {
protected:
    void SetUp() override {
        static std::atomic<unsigned int> nextId{0U};
        packagePath = std::filesystem::temp_directory_path() /
                      ("uve_editor_package_test_" + std::to_string(nextId.fetch_add(1U)) + ".uveditor");
        std::error_code error;
        std::filesystem::remove(packagePath, error);
    }

    void TearDown() override {
        std::error_code error;
        std::filesystem::remove(packagePath, error);
    }

    [[nodiscard]] EditorProjectPackageUVE MakePackage() const {
        EditorProjectPackageUVE package;
        package.revision = 4U;
        package.projectId = "univex-demo-01";
        package.displayName = "UniVex Demo";
        package.engineVersion = {0U, 1U, 0U, 42U};
        package.contentRoot = "assets";
        package.assetDatabasePath = ".uveassetdb";
        package.settingsPath = ".uvesettings";
        return package;
    }

    std::filesystem::path packagePath;
};

TEST_F(EditorProjectPackageUVETest, SaveAndLoad_RoundTripsPortableDescriptor) {
    const EditorProjectPackageUVE expected = MakePackage();

    const EditorProjectPackageResultUVE saveResult =
        EditorProjectPackageCodecUVE::SaveUVE(packagePath, expected);

    ASSERT_TRUE(saveResult.IsAcceptedUVE()) << saveResult.message;
    const EditorProjectPackageLoadResultUVE loadResult =
        EditorProjectPackageCodecUVE::LoadUVE(packagePath);

    ASSERT_TRUE(loadResult.IsAcceptedUVE()) << loadResult.result.message;
    EXPECT_EQ(*loadResult.package, expected);
}

TEST_F(EditorProjectPackageUVETest, Validate_RejectsUnboundedIdentityAndTraversalPaths) {
    EditorProjectPackageUVE package = MakePackage();
    package.projectId = "demo/project";
    EXPECT_EQ(EditorProjectPackageCodecUVE::ValidateUVE(package).code,
              EditorProjectPackageCodeUVE::InvalidPackage);

    package = MakePackage();
    package.contentRoot = "../assets";
    EXPECT_EQ(EditorProjectPackageCodecUVE::ValidateUVE(package).code,
              EditorProjectPackageCodeUVE::InvalidPath);

    package = MakePackage();
    package.settingsPath = "/tmp/settings.json";
    EXPECT_EQ(EditorProjectPackageCodecUVE::ValidateUVE(package).code,
              EditorProjectPackageCodeUVE::InvalidPath);
}

TEST_F(EditorProjectPackageUVETest, Save_RejectsWrongExtensionWithoutCreatingFile) {
    EditorProjectPackageUVE package = MakePackage();
    std::filesystem::path wrongPath = packagePath;
    wrongPath.replace_extension(".json");

    const EditorProjectPackageResultUVE result =
        EditorProjectPackageCodecUVE::SaveUVE(wrongPath, package);

    EXPECT_EQ(result.code, EditorProjectPackageCodeUVE::InvalidPath);
    EXPECT_FALSE(std::filesystem::exists(wrongPath));
}

TEST_F(EditorProjectPackageUVETest, Load_MalformedOrWrongFormatFailsClosed) {
    {
        std::ofstream output(packagePath, std::ios::binary | std::ios::trunc);
        output << "{\"format\":\"other\"}\n";
    }
    EXPECT_EQ(EditorProjectPackageCodecUVE::LoadUVE(packagePath).result.code,
              EditorProjectPackageCodeUVE::ParseFailed);

    {
        std::ofstream output(packagePath, std::ios::binary | std::ios::trunc);
        output << "not-json\n";
    }
    EXPECT_EQ(EditorProjectPackageCodecUVE::LoadUVE(packagePath).result.code,
              EditorProjectPackageCodeUVE::ParseFailed);
}

TEST_F(EditorProjectPackageUVETest, ApplyUpdate_RequiresExpectedRevisionAndMatchingIdentity) {
    const EditorProjectPackageUVE initial = MakePackage();
    ASSERT_TRUE(EditorProjectPackageCodecUVE::SaveUVE(packagePath, initial).IsAcceptedUVE());

    EditorProjectPackageUVE replacement = initial;
    replacement.revision = 5U;
    replacement.displayName = "Updated Demo";

    const EditorProjectPackageResultUVE staleResult =
        EditorProjectPackageCodecUVE::ApplyUpdateUVE(packagePath, 3U, replacement);
    EXPECT_EQ(staleResult.code, EditorProjectPackageCodeUVE::RevisionConflict);
    ASSERT_TRUE(EditorProjectPackageCodecUVE::LoadUVE(packagePath).package.has_value());
    EXPECT_EQ(EditorProjectPackageCodecUVE::LoadUVE(packagePath).package->revision, 4U);

    const EditorProjectPackageResultUVE updateResult =
        EditorProjectPackageCodecUVE::ApplyUpdateUVE(packagePath, 4U, replacement);
    ASSERT_TRUE(updateResult.IsAcceptedUVE()) << updateResult.message;
    EXPECT_EQ(EditorProjectPackageCodecUVE::LoadUVE(packagePath).package->displayName, "Updated Demo");

    EditorProjectPackageUVE differentProject = replacement;
    differentProject.revision = 6U;
    differentProject.projectId = "other-project";
    const EditorProjectPackageResultUVE identityResult =
        EditorProjectPackageCodecUVE::ApplyUpdateUVE(packagePath, 5U, differentProject);
    EXPECT_EQ(identityResult.code, EditorProjectPackageCodeUVE::ProjectIdentityConflict);
    EXPECT_EQ(EditorProjectPackageCodecUVE::LoadUVE(packagePath).package->revision, 5U);
}

TEST_F(EditorProjectPackageUVETest, ApplyUpdate_RejectsNonNewerReplacementWithoutMutation) {
    const EditorProjectPackageUVE initial = MakePackage();
    ASSERT_TRUE(EditorProjectPackageCodecUVE::SaveUVE(packagePath, initial).IsAcceptedUVE());

    EditorProjectPackageUVE replacement = initial;
    replacement.displayName = "Stale";
    const EditorProjectPackageResultUVE result =
        EditorProjectPackageCodecUVE::ApplyUpdateUVE(packagePath, initial.revision, replacement);

    EXPECT_EQ(result.code, EditorProjectPackageCodeUVE::RevisionConflict);
    const auto loaded = EditorProjectPackageCodecUVE::LoadUVE(packagePath);
    ASSERT_TRUE(loaded.IsAcceptedUVE());
    EXPECT_EQ(loaded.package->displayName, initial.displayName);
}

} // namespace
} // namespace UVE::Platform::Tests
