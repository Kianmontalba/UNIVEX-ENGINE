// Copyright (c) 2026 UniVex Studios. All Rights Reserved.


#include "uve/render/shader/shader_manager_uve.h"

#include <chrono>
#include <cstdint>
#include <filesystem>
#include <fstream>
#include <memory>
#include <optional>
#include <string>
#include <string_view>
#include <thread>
#include <utility>

#include <gtest/gtest.h>

#include "uve/asset/asset_bundle_uve.h"
#include "uve/asset/file_system_uve.h"
#include "uve/events/event_system_uve.h"
#include "uve/render/null_render_device_uve.h"
#include "uve/render/shader/built_in_shaders_uve.h"
#include "uve/threading/thread_pool_uve.h"

namespace UVE::Render::Shader::Tests {
namespace {

// Bounded busy-poll (never a fixed sleep), matching this codebase's established
// WaitForTerminalStateUVE pattern (see tests/asset/asset_manager_uve_tests.cpp). Readiness only
// advances when UpdateUVE() is called (the background job just does file I/O/preprocessing; the
// real "compile" and the ready-flag flip both happen inside UpdateUVE(), on the calling thread).
template <typename T>
[[nodiscard]] bool WaitUntilReadyUVE(ShaderManagerUVE& shaderManager, const T& obj, int maxIterations = 200000) {
    for (int iteration = 0; iteration < maxIterations; ++iteration) {
        shaderManager.UpdateUVE(0.0);
        if (obj.IsReadyUVE()) {
            return true;
        }
        std::this_thread::yield();
    }
    return false;
}

class ShaderManagerUVETest : public ::testing::Test {
protected:
    void SetUp() override {
        assetBundle = std::make_unique<Asset::AssetBundleUVE>();
        fileSystem = std::make_unique<Asset::FileSystemUVE>(*assetBundle);
        threadPool = std::make_unique<Threading::ThreadPoolUVE>(1);
        eventSystem = std::make_unique<Events::EventSystemUVE>();
        renderDevice = std::make_unique<NullRenderDeviceUVE>();
    }

    void TearDown() override {
        std::filesystem::remove_all(tempDirectory);
        std::filesystem::remove_all(tempCacheDirectory);
    }

    [[nodiscard]] std::unique_ptr<ShaderManagerUVE> MakeManagerUVE(ShaderManagerConfigUVE config = {}) {
        return std::make_unique<ShaderManagerUVE>(*threadPool, *eventSystem, *renderDevice, *fileSystem, config);
    }

    std::unique_ptr<Asset::IAssetBundleUVE> assetBundle;
    std::unique_ptr<Asset::IFileSystemUVE> fileSystem;
    std::unique_ptr<Threading::IThreadPoolUVE> threadPool;
    std::unique_ptr<Events::IEventSystemUVE> eventSystem;
    std::unique_ptr<IRenderDeviceUVE> renderDevice;

    const std::filesystem::path tempDirectory = "uve_shader_manager_tests_shaders";
    const std::filesystem::path tempCacheDirectory = "uve_shader_manager_tests_cache";
};

TEST_F(ShaderManagerUVETest, CreateSourceUVE_EmbeddedFallback_BecomesReadyAndValid) {
    const std::unique_ptr<ShaderManagerUVE> shaderManager = MakeManagerUVE();

    ShaderSourceCompileDescUVE desc;
    desc.stage = ShaderStageUVE::Vertex;
    desc.virtualFilePath = "shaders/missing_on_disk.glsl";
    desc.embeddedFallbackSourceCode = "#version 330 core\nvoid main() { gl_Position = vec4(0.0); }\n";

    const std::shared_ptr<ShaderSourceUVE> source = shaderManager->CreateSourceUVE(desc);
    ASSERT_TRUE(WaitUntilReadyUVE(*shaderManager, *source));
    EXPECT_TRUE(source->IsValidUVE());
    EXPECT_NE(source->GetHandleUVE(), kInvalidShaderHandleUVE);
}

TEST_F(ShaderManagerUVETest, CreateProgramUVE_BuiltInBasic3D_BecomesReadyAndValid) {
    const std::unique_ptr<ShaderManagerUVE> shaderManager = MakeManagerUVE();

    ShaderProgramDescUVE desc;
    desc.virtualFilePath = std::string(BuiltIn::kBasic3DVirtualPath);
    desc.embeddedFallbackSourceCode = std::string(BuiltIn::kBasic3DSource);
    desc.vertexLayout = {VertexAttributeUVE{"POSITION", VertexAttributeFormatUVE::Float3, 0}};
    desc.vertexStride = 3U * static_cast<std::uint32_t>(sizeof(float));

    const std::shared_ptr<ShaderProgramUVE> program = shaderManager->CreateProgramUVE(desc);
    ASSERT_TRUE(WaitUntilReadyUVE(*shaderManager, *program));
    EXPECT_TRUE(program->IsValidUVE());
    EXPECT_NE(program->GetPipelineHandleUVE(), kInvalidPipelineHandleUVE);
}

TEST_F(ShaderManagerUVETest, CreateProgramFromStagesUVE_SeparateEmbeddedStages_BecomesReadyAndValid) {
    const std::unique_ptr<ShaderManagerUVE> shaderManager = MakeManagerUVE();

    ShaderProgramStagesDescUVE desc;
    desc.vertexSource.stage = ShaderStageUVE::Vertex;
    desc.vertexSource.embeddedFallbackSourceCode =
        "#version 330 core\nlayout(location = 0) in vec3 aPosition;\nvoid main() { gl_Position = vec4(aPosition, 1.0); }\n";
    desc.fragmentSource.stage = ShaderStageUVE::Fragment;
    desc.fragmentSource.embeddedFallbackSourceCode =
        "#version 330 core\nout vec4 FragColor;\nvoid main() { FragColor = vec4(1.0); }\n";
    desc.vertexLayout = {VertexAttributeUVE{"POSITION", VertexAttributeFormatUVE::Float3, 0}};
    desc.vertexStride = 3U * static_cast<std::uint32_t>(sizeof(float));
    desc.debugNameUVE = "SeparateEmbeddedStages";

    const std::shared_ptr<ShaderProgramUVE> program = shaderManager->CreateProgramFromStagesUVE(desc);
    ASSERT_TRUE(WaitUntilReadyUVE(*shaderManager, *program));
    EXPECT_TRUE(program->IsValidUVE());
    EXPECT_NE(program->GetPipelineHandleUVE(), kInvalidPipelineHandleUVE);
}

TEST_F(ShaderManagerUVETest, HotReload_SeparateFragmentStage_RebuildsSameProgram) {
    std::filesystem::remove_all(tempDirectory);
    std::filesystem::create_directories(tempDirectory);
    const std::filesystem::path vertexPath = tempDirectory / "material.vert";
    const std::filesystem::path fragmentPath = tempDirectory / "material.frag";
    {
        std::ofstream vertexFile(vertexPath);
        vertexFile << "#version 330 core\nlayout(location = 0) in vec3 aPosition;\n"
                      "void main() { gl_Position = vec4(aPosition, 1.0); }\n";
    }
    {
        std::ofstream fragmentFile(fragmentPath);
        fragmentFile << "#version 330 core\nout vec4 FragColor;\n"
                        "void main() { FragColor = vec4(1.0); }\n// marker-v1\n";
    }
    fileSystem->MountDirectoryUVE("material", tempDirectory, 0);

    ShaderManagerConfigUVE config;
    config.hotReloadEnabledUVE = true;
    config.hotReloadPollIntervalSecondsUVE = 0.0;
    const std::unique_ptr<ShaderManagerUVE> shaderManager = MakeManagerUVE(config);

    ShaderProgramStagesDescUVE desc;
    desc.vertexSource.virtualFilePath = "material/material.vert";
    desc.vertexSource.embeddedFallbackSourceCode = "#version 330 core\nvoid main() {}\n";
    desc.fragmentSource.virtualFilePath = "material/material.frag";
    desc.fragmentSource.embeddedFallbackSourceCode = "#version 330 core\nout vec4 FragColor;\nvoid main() {}\n";
    desc.vertexLayout = {VertexAttributeUVE{"POSITION", VertexAttributeFormatUVE::Float3, 0}};
    desc.vertexStride = 3U * static_cast<std::uint32_t>(sizeof(float));

    const std::shared_ptr<ShaderProgramUVE> program = shaderManager->CreateProgramFromStagesUVE(desc);
    ASSERT_TRUE(WaitUntilReadyUVE(*shaderManager, *program));
    ASSERT_TRUE(program->IsValidUVE());
    const std::uint64_t initialHash = program->GetContentHashUVE();

    std::this_thread::sleep_for(std::chrono::milliseconds(20));
    {
        std::ofstream fragmentFile(fragmentPath, std::ios::trunc);
        fragmentFile << "#version 330 core\nout vec4 FragColor;\n"
                        "void main() { FragColor = vec4(0.5); }\n// marker-v2\n";
    }

    bool reloaded = false;
    for (int iteration = 0; iteration < 200000 && !reloaded; ++iteration) {
        shaderManager->UpdateUVE(1.0);
        reloaded = program->IsValidUVE() && program->GetContentHashUVE() != initialHash;
        if (!reloaded) {
            std::this_thread::yield();
        }
    }
    EXPECT_TRUE(reloaded);
}

TEST_F(ShaderManagerUVETest, CreateSourceUVE_IncludeDirectiveOnRealDisk_ResolvesThroughVfs) {
    std::filesystem::remove_all(tempDirectory);
    std::filesystem::create_directories(tempDirectory / "inc");
    {
        std::ofstream includeFile(tempDirectory / "inc" / "common.glsl");
        includeFile << "vec3 uveTestHelper() { return vec3(1.0); }\n";
    }
    {
        std::ofstream mainFile(tempDirectory / "main.glsl");
        mainFile << "#include \"shaders/inc/common.glsl\"\nvoid main() {}\n";
    }
    fileSystem->MountDirectoryUVE("shaders", tempDirectory, 0);

    const std::unique_ptr<ShaderManagerUVE> shaderManager = MakeManagerUVE();
    ShaderSourceCompileDescUVE desc;
    desc.stage = ShaderStageUVE::Vertex;
    desc.virtualFilePath = "shaders/main.glsl";
    desc.embeddedFallbackSourceCode = "#version 330 core\nvoid main() {}\n";

    const std::shared_ptr<ShaderSourceUVE> source = shaderManager->CreateSourceUVE(desc);
    ASSERT_TRUE(WaitUntilReadyUVE(*shaderManager, *source));
    EXPECT_NE(source->GetResolvedSourceUVE().find("uveTestHelper"), std::string::npos);
}

TEST_F(ShaderManagerUVETest, HotReload_FileChangeOnDisk_TriggersRecompileWithNewResolvedSource) {
    std::filesystem::remove_all(tempDirectory);
    std::filesystem::create_directories(tempDirectory);
    const std::filesystem::path shaderFile = tempDirectory / "reloadable.glsl";
    {
        std::ofstream file(shaderFile);
        file << "#version 330 core\nvoid main() { gl_Position = vec4(0.0); }\n// marker-v1\n";
    }
    fileSystem->MountDirectoryUVE("shaders", tempDirectory, 0);

    ShaderManagerConfigUVE config;
    config.hotReloadEnabledUVE = true;
    config.hotReloadPollIntervalSecondsUVE = 0.0;
    const std::unique_ptr<ShaderManagerUVE> shaderManager = MakeManagerUVE(config);

    ShaderSourceCompileDescUVE desc;
    desc.stage = ShaderStageUVE::Vertex;
    desc.virtualFilePath = "shaders/reloadable.glsl";
    desc.embeddedFallbackSourceCode = "#version 330 core\nvoid main() {}\n";
    desc.hotReloadEnabledUVE = true;

    const std::shared_ptr<ShaderSourceUVE> source = shaderManager->CreateSourceUVE(desc);
    ASSERT_TRUE(WaitUntilReadyUVE(*shaderManager, *source));
    ASSERT_NE(source->GetResolvedSourceUVE().find("marker-v1"), std::string::npos);

    // Sleep briefly so the modified file's mtime is guaranteed to differ from the original write
    // (some filesystems have coarse mtime resolution) before rewriting it with new content.
    std::this_thread::sleep_for(std::chrono::milliseconds(20));
    {
        std::ofstream file(shaderFile, std::ios::trunc);
        file << "#version 330 core\nvoid main() { gl_Position = vec4(0.0); }\n// marker-v2\n";
    }

    bool sawUpdatedMarker = false;
    for (int iteration = 0; iteration < 200000 && !sawUpdatedMarker; ++iteration) {
        shaderManager->UpdateUVE(1.0);
        if (source->GetResolvedSourceUVE().find("marker-v2") != std::string::npos) {
            sawUpdatedMarker = true;
        }
        std::this_thread::yield();
    }
    EXPECT_TRUE(sawUpdatedMarker);
}

TEST_F(ShaderManagerUVETest, GetPendingJobCountUVE_DropsToZeroOnceCreationSettles) {
    const std::unique_ptr<ShaderManagerUVE> shaderManager = MakeManagerUVE();
    ShaderSourceCompileDescUVE desc;
    desc.stage = ShaderStageUVE::Fragment;
    desc.embeddedFallbackSourceCode = "#version 330 core\nout vec4 c;\nvoid main() { c = vec4(1.0); }\n";

    const std::shared_ptr<ShaderSourceUVE> source = shaderManager->CreateSourceUVE(desc);
    ASSERT_TRUE(WaitUntilReadyUVE(*shaderManager, *source));
    EXPECT_EQ(shaderManager->GetPendingJobCountUVE(), 0U);
}

TEST_F(ShaderManagerUVETest, NullRenderDeviceBackend_NeverReportsBinaryCacheHit) {
    // NullRenderDeviceUVE::GetPipelineBinaryUVE()/CreatePipelineFromBinaryUVE() are documented
    // no-op/false stand-ins - ShaderManagerUVE must therefore always take the from-source path
    // against this backend, never claim a cache hit that never happened.
    ShaderManagerConfigUVE config;
    config.cachePath = tempCacheDirectory;
    const std::unique_ptr<ShaderManagerUVE> shaderManager = MakeManagerUVE(config);

    ShaderProgramDescUVE desc;
    desc.virtualFilePath = std::string(BuiltIn::kBasic3DVirtualPath);
    desc.embeddedFallbackSourceCode = std::string(BuiltIn::kBasic3DSource);
    desc.vertexLayout = {VertexAttributeUVE{"POSITION", VertexAttributeFormatUVE::Float3, 0}};
    desc.vertexStride = 3U * static_cast<std::uint32_t>(sizeof(float));

    const std::shared_ptr<ShaderProgramUVE> program = shaderManager->CreateProgramUVE(desc);
    ASSERT_TRUE(WaitUntilReadyUVE(*shaderManager, *program));
    EXPECT_FALSE(shaderManager->GetLastCompileUsedCacheUVE());
}

// Guards against the built-in .glsl files under engine/render/shader/built_in/ ever drifting from
// their embedded C++ string fallback counterparts (see built_in_shaders_uve.cpp's generation
// note) - both must be used by the engine at different times (real file when reachable, embedded
// string otherwise) and are expected to be byte-identical. Assumes the process's current working
// directory is the repository root, matching every other relative default path in this codebase
// (EngineConfigUVE::shaderSourceRealDirectoryUVE, assetDatabaseFilePath, logFilePath, ...).
class BuiltInShaderParityUVETest : public ::testing::TestWithParam<std::pair<std::string_view, std::string_view>> {};

[[nodiscard]] std::optional<std::string> ReadFileToStringUVE(const std::filesystem::path& path) {
    std::ifstream file(path, std::ios::binary);
    if (!file) {
        return std::nullopt;
    }
    return std::string(std::istreambuf_iterator<char>(file), std::istreambuf_iterator<char>());
}

TEST_P(BuiltInShaderParityUVETest, EmbeddedSource_IsByteIdenticalToPhysicalFile) {
    const auto& [relativePath, embeddedSource] = GetParam();
    const std::filesystem::path fullPath = std::filesystem::path("engine/render/shader/built_in") / relativePath;
    const std::optional<std::string> fileContent = ReadFileToStringUVE(fullPath);
    if (!fileContent.has_value()) {
        GTEST_SKIP() << "Could not read " << fullPath
                     << " - this parity test assumes the process's working directory is the "
                        "repository root (run ctest/uve_tests from there to exercise it).";
    }
    EXPECT_EQ(*fileContent, embeddedSource);
}

INSTANTIATE_TEST_SUITE_P(
    AllBuiltInShaders, BuiltInShaderParityUVETest,
    ::testing::Values(std::pair<std::string_view, std::string_view>{"basic_2d.glsl", BuiltIn::kBasic2DSource},
                       std::pair<std::string_view, std::string_view>{"basic_3d.glsl", BuiltIn::kBasic3DSource},
                       std::pair<std::string_view, std::string_view>{"basic_3d_textured.glsl",
                                                                      BuiltIn::kBasic3DTexturedSource},
                       std::pair<std::string_view, std::string_view>{"fullscreen_quad.glsl",
                                                                      BuiltIn::kFullscreenQuadSource},
                       std::pair<std::string_view, std::string_view>{"shadow_depth.glsl",
                                                                      BuiltIn::kShadowDepthSource},
                       std::pair<std::string_view, std::string_view>{"lit_shadowed_3d.glsl",
                                                                      BuiltIn::kLitShadowed3DSource}));

} // namespace
} // namespace UVE::Render::Shader::Tests
