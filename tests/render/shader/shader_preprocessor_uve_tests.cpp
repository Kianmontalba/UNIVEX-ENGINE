// Copyright (c) 2026 UniVex Studios. All Rights Reserved.


#include "shader_preprocessor_uve.h"

#include <algorithm>
#include <optional>
#include <string>
#include <string_view>
#include <unordered_map>
#include <vector>

#include <gtest/gtest.h>

#include "uve/asset/i_file_system_uve.h"

namespace UVE::Render::Shader::Detail::Tests {
namespace {

// A pure in-memory IFileSystemUVE - the preprocessor's own doc comment states it is
// "independently unit-testable against a fake IFileSystemUVE" since it never touches threading or
// GL, so no real mount/directory is needed here.
class FakeFileSystemUVE final : public Asset::IFileSystemUVE {
public:
    Asset::MountHandleUVE MountDirectoryUVE(std::string, std::filesystem::path, int) override { return 1; }
    Asset::MountHandleUVE MountBundleUVE(std::string, std::filesystem::path, int) override { return 1; }
    void UnmountUVE(Asset::MountHandleUVE) override {}

    [[nodiscard]] bool HasFileUVE(std::string_view virtualPath) const override {
        return files.find(std::string(virtualPath)) != files.end();
    }

    [[nodiscard]] std::optional<std::vector<std::byte>> ReadFileUVE(std::string_view virtualPath) const override {
        const auto it = files.find(std::string(virtualPath));
        if (it == files.end()) {
            return std::nullopt;
        }
        std::vector<std::byte> bytes(it->second.size());
        std::transform(it->second.begin(), it->second.end(), bytes.begin(),
                        [](char character) { return static_cast<std::byte>(character); });
        return bytes;
    }

    [[nodiscard]] bool WriteFileUVE(std::string_view, const std::vector<std::byte>&) override { return false; }
    [[nodiscard]] std::filesystem::path ResolveRealPathUVE(std::string_view) const override { return {}; }

    void AddFileUVE(std::string virtualPath, std::string content) {
        files.emplace(std::move(virtualPath), std::move(content));
    }

    std::unordered_map<std::string, std::string> files;
};

TEST(ShaderPreprocessorUVETest, NoFileOnDisk_UsesEmbeddedFallback) {
    FakeFileSystemUVE fileSystem;
    const PreprocessResultUVE result =
        PreprocessShaderSourceUVE(fileSystem, "shaders/missing.glsl", "void main() {}\n", {});

    ASSERT_TRUE(result.success);
    EXPECT_NE(result.resolvedSource.find("void main() {}"), std::string::npos);
    EXPECT_TRUE(result.dependencyClosure.empty());
}

TEST(ShaderPreprocessorUVETest, FileOnDisk_PrefersFileOverEmbeddedFallback) {
    FakeFileSystemUVE fileSystem;
    fileSystem.AddFileUVE("shaders/real.glsl", "void realMain() {}\n");
    const PreprocessResultUVE result =
        PreprocessShaderSourceUVE(fileSystem, "shaders/real.glsl", "void embeddedMain() {}\n", {});

    ASSERT_TRUE(result.success);
    EXPECT_NE(result.resolvedSource.find("realMain"), std::string::npos);
    EXPECT_EQ(result.resolvedSource.find("embeddedMain"), std::string::npos);
    ASSERT_EQ(result.dependencyClosure.size(), 1U);
    EXPECT_EQ(result.dependencyClosure[0], "shaders/real.glsl");
}

TEST(ShaderPreprocessorUVETest, IncludeDirective_ExpandsRecursivelyAndTracksDependencyClosure) {
    FakeFileSystemUVE fileSystem;
    fileSystem.AddFileUVE("shaders/inc/common.glsl", "vec3 commonHelper() { return vec3(1.0); }\n");
    fileSystem.AddFileUVE("shaders/main.glsl", "#include \"shaders/inc/common.glsl\"\nvoid main() {}\n");

    const PreprocessResultUVE result = PreprocessShaderSourceUVE(fileSystem, "shaders/main.glsl", "", {});

    ASSERT_TRUE(result.success);
    EXPECT_NE(result.resolvedSource.find("commonHelper"), std::string::npos);
    EXPECT_EQ(result.dependencyClosure.size(), 2U);
    EXPECT_NE(std::find(result.dependencyClosure.begin(), result.dependencyClosure.end(), "shaders/main.glsl"),
              result.dependencyClosure.end());
    EXPECT_NE(
        std::find(result.dependencyClosure.begin(), result.dependencyClosure.end(), "shaders/inc/common.glsl"),
        result.dependencyClosure.end());
}

TEST(ShaderPreprocessorUVETest, IncludeCycle_FailsCleanlyRatherThanInfiniteLooping) {
    FakeFileSystemUVE fileSystem;
    fileSystem.AddFileUVE("shaders/a.glsl", "#include \"shaders/b.glsl\"\n");
    fileSystem.AddFileUVE("shaders/b.glsl", "#include \"shaders/a.glsl\"\n");

    const PreprocessResultUVE result = PreprocessShaderSourceUVE(fileSystem, "shaders/a.glsl", "", {});

    EXPECT_FALSE(result.success);
    EXPECT_FALSE(result.errorMessage.empty());
}

TEST(ShaderPreprocessorUVETest, MissingIncludeTarget_FailsWithErrorMessage) {
    FakeFileSystemUVE fileSystem;
    fileSystem.AddFileUVE("shaders/main.glsl", "#include \"shaders/does_not_exist.glsl\"\n");

    const PreprocessResultUVE result = PreprocessShaderSourceUVE(fileSystem, "shaders/main.glsl", "", {});

    EXPECT_FALSE(result.success);
    EXPECT_FALSE(result.errorMessage.empty());
}

TEST(ShaderPreprocessorUVETest, IncludedOnceGuard_SameFileIncludedTwice_ExpandsOnlyOnce) {
    FakeFileSystemUVE fileSystem;
    fileSystem.AddFileUVE("shaders/inc/common.glsl", "vec3 commonHelper() { return vec3(1.0); }\n");
    fileSystem.AddFileUVE("shaders/main.glsl",
                           "#include \"shaders/inc/common.glsl\"\n#include \"shaders/inc/common.glsl\"\n");

    const PreprocessResultUVE result = PreprocessShaderSourceUVE(fileSystem, "shaders/main.glsl", "", {});

    ASSERT_TRUE(result.success);
    const std::size_t firstOccurrence = result.resolvedSource.find("commonHelper");
    ASSERT_NE(firstOccurrence, std::string::npos);
    const std::size_t secondOccurrence = result.resolvedSource.find("commonHelper", firstOccurrence + 1);
    EXPECT_EQ(secondOccurrence, std::string::npos);
}

TEST(ShaderPreprocessorUVETest, IfdefDefined_KeepsTrueBranchDropsElseBranch) {
    FakeFileSystemUVE fileSystem;
    constexpr std::string_view source = "#ifdef UVE_TEST_FLAG\nkeep_this();\n#else\ndrop_this();\n#endif\n";

    const PreprocessResultUVE result =
        PreprocessShaderSourceUVE(fileSystem, "shaders/x.glsl", std::string(source), {{"UVE_TEST_FLAG", ""}});

    ASSERT_TRUE(result.success);
    EXPECT_NE(result.resolvedSource.find("keep_this"), std::string::npos);
    EXPECT_EQ(result.resolvedSource.find("drop_this"), std::string::npos);
}

TEST(ShaderPreprocessorUVETest, IfndefUndefined_KeepsTrueBranch) {
    FakeFileSystemUVE fileSystem;
    constexpr std::string_view source = "#ifndef UVE_TEST_UNDEFINED_FLAG\nkeep_this();\n#else\ndrop_this();\n#endif\n";

    const PreprocessResultUVE result = PreprocessShaderSourceUVE(fileSystem, "shaders/x.glsl", std::string(source), {});

    ASSERT_TRUE(result.success);
    EXPECT_NE(result.resolvedSource.find("keep_this"), std::string::npos);
    EXPECT_EQ(result.resolvedSource.find("drop_this"), std::string::npos);
}

TEST(ShaderPreprocessorUVETest, UnmatchedEndif_FailsCleanly) {
    FakeFileSystemUVE fileSystem;
    const PreprocessResultUVE result =
        PreprocessShaderSourceUVE(fileSystem, "shaders/x.glsl", "#endif\nvoid main() {}\n", {});

    EXPECT_FALSE(result.success);
    EXPECT_FALSE(result.errorMessage.empty());
}

TEST(ShaderPreprocessorUVETest, UnmatchedIfdefAtEndOfFile_DoesNotCrash) {
    // Only unmatched #else/#endif are documented as hard failures (PreprocessShaderSourceUVE()'s
    // own doc comment); an #ifdef left open at end-of-file has no matching #else/#endif to ever
    // trip that check, so this is a deliberately-not-hardened edge case, not a promised failure -
    // the only contract this asserts is "never crashes."
    FakeFileSystemUVE fileSystem;
    const PreprocessResultUVE result =
        PreprocessShaderSourceUVE(fileSystem, "shaders/x.glsl", "#ifdef UVE_SOMETHING\nvoid main() {}\n", {});
    static_cast<void>(result);
    SUCCEED();
}

TEST(ShaderPreprocessorUVETest, MacroSubstitution_IsTokenBoundaryAware) {
    FakeFileSystemUVE fileSystem;
    // UVE_VAL must substitute inside "x = UVE_VAL;" but the definition must NOT bleed into the
    // longer identifier "UVE_VALUE" elsewhere in the source.
    constexpr std::string_view source = "int x = UVE_VAL;\nint UVE_VALUE = 2;\n";

    const PreprocessResultUVE result =
        PreprocessShaderSourceUVE(fileSystem, "shaders/x.glsl", std::string(source), {{"UVE_VAL", "42"}});

    ASSERT_TRUE(result.success);
    EXPECT_NE(result.resolvedSource.find("int x = 42;"), std::string::npos);
    EXPECT_NE(result.resolvedSource.find("int UVE_VALUE = 2;"), std::string::npos);
}

TEST(ShaderPreprocessorUVETest, DefineThenUndef_MacroNoLongerSubstitutedAfterUndef) {
    FakeFileSystemUVE fileSystem;
    constexpr std::string_view source = "#define UVE_LOCAL 1\nfirst = UVE_LOCAL;\n#undef UVE_LOCAL\nsecond = UVE_LOCAL;\n";

    const PreprocessResultUVE result = PreprocessShaderSourceUVE(fileSystem, "shaders/x.glsl", std::string(source), {});

    ASSERT_TRUE(result.success);
    EXPECT_NE(result.resolvedSource.find("first = 1;"), std::string::npos);
    EXPECT_NE(result.resolvedSource.find("second = UVE_LOCAL;"), std::string::npos);
}

TEST(ShaderPreprocessorUVETest, EmbeddedFallback_FileIndexTableHasSingleEntry) {
    FakeFileSystemUVE fileSystem;
    const PreprocessResultUVE result =
        PreprocessShaderSourceUVE(fileSystem, "shaders/missing.glsl", "void main() {}\n", {});

    ASSERT_TRUE(result.success);
    ASSERT_FALSE(result.fileIndexTable.empty());
    EXPECT_EQ(result.fileIndexTable[0], "shaders/missing.glsl (embedded fallback)");
}

TEST(ShaderPreprocessorUVETest, ComputeFnv1aHashUVE_SameInput_ProducesSameHash) {
    EXPECT_EQ(ComputeFnv1aHashUVE("hello world"), ComputeFnv1aHashUVE("hello world"));
}

TEST(ShaderPreprocessorUVETest, ComputeFnv1aHashUVE_DifferentInput_ProducesDifferentHash) {
    EXPECT_NE(ComputeFnv1aHashUVE("hello world"), ComputeFnv1aHashUVE("hello there"));
}

TEST(ShaderPreprocessorUVETest, ComputeFnv1aHashUVE_EmptyInput_DoesNotCrashAndIsDeterministic) {
    EXPECT_EQ(ComputeFnv1aHashUVE(""), ComputeFnv1aHashUVE(""));
}

} // namespace
} // namespace UVE::Render::Shader::Detail::Tests
