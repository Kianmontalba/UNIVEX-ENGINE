//------------------------------------------------------------------------------
// UniVex Engine (UVE) — Proprietary Game Engine
// Copyright (c) 2026 UniVex Studios. All Rights Reserved.
// Unauthorized copying, modification, distribution, or use of this code
// in whole or in part is strictly prohibited without express written
// permission from UniVex Studios.
// Violators will be prosecuted to the fullest extent of the law.
//------------------------------------------------------------------------------

#include "uve/render/shader/shader_program_uve.h"

#include <algorithm>
#include <memory>
#include <thread>
#include <variant>
#include <vector>

#include <gtest/gtest.h>

#include "uve/asset/asset_bundle_uve.h"
#include "uve/asset/file_system_uve.h"
#include "uve/events/event_system_uve.h"
#include "uve/math/matrix4x4_uve.h"
#include "uve/math/vector3_uve.h"
#include "uve/render/null_render_device_uve.h"
#include "uve/render/shader/built_in_shaders_uve.h"
#include "uve/render/shader/shader_manager_uve.h"
#include "uve/threading/thread_pool_uve.h"

namespace UVE::Render::Shader::Tests {
namespace {

// Bounded busy-poll (never a fixed sleep), matching this codebase's established
// WaitForTerminalStateUVE pattern (see tests/asset/asset_manager_uve_tests.cpp) - readiness only
// advances when UpdateUVE() is called, since ShaderManagerUVE's real compile step is
// main-thread-driven, not automatic.
[[nodiscard]] bool WaitUntilProgramReadyUVE(ShaderManagerUVE& shaderManager, const ShaderProgramUVE& program,
                                             int maxIterations = 200000) {
    for (int iteration = 0; iteration < maxIterations; ++iteration) {
        shaderManager.UpdateUVE(0.0);
        if (program.IsReadyUVE()) {
            return true;
        }
        std::this_thread::yield();
    }
    return false;
}

class ShaderProgramUVETest : public ::testing::Test {
protected:
    void SetUp() override {
        assetBundle = std::make_unique<Asset::AssetBundleUVE>();
        fileSystem = std::make_unique<Asset::FileSystemUVE>(*assetBundle);
        threadPool = std::make_unique<Threading::ThreadPoolUVE>(1);
        eventSystem = std::make_unique<Events::EventSystemUVE>();
        renderDevice = std::make_unique<NullRenderDeviceUVE>();
        shaderManager = std::make_unique<ShaderManagerUVE>(*threadPool, *eventSystem, *renderDevice, *fileSystem,
                                                             ShaderManagerConfigUVE{});
    }

    [[nodiscard]] std::shared_ptr<ShaderProgramUVE> MakeReadyProgramUVE() {
        ShaderProgramDescUVE desc;
        desc.virtualFilePath = std::string(BuiltIn::kBasic3DVirtualPath);
        desc.embeddedFallbackSourceCode = std::string(BuiltIn::kBasic3DSource);
        desc.vertexLayout = {VertexAttributeUVE{"POSITION", VertexAttributeFormatUVE::Float3, 0}};
        desc.vertexStride = 3U * static_cast<std::uint32_t>(sizeof(float));
        std::shared_ptr<ShaderProgramUVE> program = shaderManager->CreateProgramUVE(desc);
        EXPECT_TRUE(WaitUntilProgramReadyUVE(*shaderManager, *program));
        return program;
    }

    std::unique_ptr<Asset::IAssetBundleUVE> assetBundle;
    std::unique_ptr<Asset::IFileSystemUVE> fileSystem;
    std::unique_ptr<Threading::IThreadPoolUVE> threadPool;
    std::unique_ptr<Events::IEventSystemUVE> eventSystem;
    std::unique_ptr<IRenderDeviceUVE> renderDevice;
    std::unique_ptr<ShaderManagerUVE> shaderManager;
};

TEST_F(ShaderProgramUVETest, CreateProgramUVE_EmbeddedFallback_BecomesReadyAndValid) {
    const std::shared_ptr<ShaderProgramUVE> program = MakeReadyProgramUVE();
    ASSERT_TRUE(program->IsReadyUVE());
    EXPECT_TRUE(program->IsValidUVE());
    EXPECT_NE(program->GetPipelineHandleUVE(), kInvalidPipelineHandleUVE);
}

TEST_F(ShaderProgramUVETest, FindUniformUVE_UnknownName_ReturnsNullopt) {
    const std::shared_ptr<ShaderProgramUVE> program = MakeReadyProgramUVE();
    EXPECT_FALSE(program->FindUniformUVE("uDefinitelyNotDeclared").has_value());
}

TEST_F(ShaderProgramUVETest, ApplyToUVE_InvalidProgram_LogsWarningAndSkipsBindWithoutCrashing) {
    // A freshly-constructed program (never ready, never linked) is !IsValidUVE() - ApplyToUVE()
    // must be a safe no-op rather than binding a stale/invalid pipeline handle.
    ShaderProgramDescUVE desc;
    desc.virtualFilePath = "shaders/definitely_missing_and_broken.glsl";
    desc.embeddedFallbackSourceCode = "not glsl at all {{{";
    const std::shared_ptr<ShaderProgramUVE> program = shaderManager->CreateProgramUVE(desc);
    ASSERT_FALSE(program->IsValidUVE());

    std::unique_ptr<ICommandBufferUVE> commandBuffer = renderDevice->CreateCommandBufferUVE();
    commandBuffer->BeginRenderPassUVE(RenderPassDescUVE{});
    program->ApplyToUVE(*commandBuffer);
    commandBuffer->EndRenderPassUVE();
    SUCCEED();
}

TEST_F(ShaderProgramUVETest, ApplyToUVE_BindsPipelineThenFlushesEachPendingUniformKind) {
    const std::shared_ptr<ShaderProgramUVE> program = MakeReadyProgramUVE();

    program->SetMatrix4x4UVE("uModel", Math::Matrix4x4UVE::IdentityUVE());
    program->SetVector3UVE("uColor", Math::Vector3UVE{0.0F, 0.83137255F, 1.0F});
    program->SetFloatUVE("uSomeFloat", 3.5F);
    program->SetIntUVE("uSomeInt", 7);
    program->SetBoolUVE("uSomeBool", true);

    std::unique_ptr<ICommandBufferUVE> commandBuffer = renderDevice->CreateCommandBufferUVE();
    commandBuffer->BeginRenderPassUVE(RenderPassDescUVE{});
    program->ApplyToUVE(*commandBuffer);
    commandBuffer->EndRenderPassUVE();
    renderDevice->SubmitUVE(std::move(commandBuffer));

    auto* const nullDevice = static_cast<NullRenderDeviceUVE*>(renderDevice.get());
    const std::vector<RecordedCommandUVE>& recorded = nullDevice->GetLastSubmittedCommandsUVE();

    ASSERT_GE(recorded.size(), 2U);
    EXPECT_TRUE(std::holds_alternative<BeginRenderPassCommandUVE>(recorded[0]));
    ASSERT_TRUE(std::holds_alternative<BindPipelineCommandUVE>(recorded[1]));
    EXPECT_EQ(std::get<BindPipelineCommandUVE>(recorded[1]).pipeline, program->GetPipelineHandleUVE());

    bool foundMatrix = false;
    bool foundVector3 = false;
    bool foundFloat = false;
    bool foundInt = false;
    bool foundBool = false;
    for (const RecordedCommandUVE& command : recorded) {
        if (const auto* matrixCommand = std::get_if<SetUniformMatrix4x4CommandUVE>(&command)) {
            if (matrixCommand->name == "uModel") {
                foundMatrix = true;
            }
        } else if (const auto* vectorCommand = std::get_if<SetUniformVector3CommandUVE>(&command)) {
            if (vectorCommand->name == "uColor") {
                EXPECT_FLOAT_EQ(vectorCommand->value.y, 0.83137255F);
                foundVector3 = true;
            }
        } else if (const auto* floatCommand = std::get_if<SetUniformFloatCommandUVE>(&command)) {
            if (floatCommand->name == "uSomeFloat") {
                EXPECT_FLOAT_EQ(floatCommand->value, 3.5F);
                foundFloat = true;
            }
        } else if (const auto* intCommand = std::get_if<SetUniformIntCommandUVE>(&command)) {
            if (intCommand->name == "uSomeInt") {
                EXPECT_EQ(intCommand->value, 7);
                foundInt = true;
            }
        } else if (const auto* boolCommand = std::get_if<SetUniformBoolCommandUVE>(&command)) {
            if (boolCommand->name == "uSomeBool") {
                EXPECT_TRUE(boolCommand->value);
                foundBool = true;
            }
        }
    }
    EXPECT_TRUE(foundMatrix);
    EXPECT_TRUE(foundVector3);
    EXPECT_TRUE(foundFloat);
    EXPECT_TRUE(foundInt);
    EXPECT_TRUE(foundBool);
}

TEST_F(ShaderProgramUVETest, SetFloatUVE_CalledTwiceForSameName_LastValueWins) {
    const std::shared_ptr<ShaderProgramUVE> program = MakeReadyProgramUVE();
    program->SetFloatUVE("uRepeated", 1.0F);
    program->SetFloatUVE("uRepeated", 9.0F);

    std::unique_ptr<ICommandBufferUVE> commandBuffer = renderDevice->CreateCommandBufferUVE();
    commandBuffer->BeginRenderPassUVE(RenderPassDescUVE{});
    program->ApplyToUVE(*commandBuffer);
    commandBuffer->EndRenderPassUVE();
    renderDevice->SubmitUVE(std::move(commandBuffer));

    auto* const nullDevice = static_cast<NullRenderDeviceUVE*>(renderDevice.get());
    int matchCount = 0;
    for (const RecordedCommandUVE& command : nullDevice->GetLastSubmittedCommandsUVE()) {
        if (const auto* floatCommand = std::get_if<SetUniformFloatCommandUVE>(&command)) {
            if (floatCommand->name == "uRepeated") {
                EXPECT_FLOAT_EQ(floatCommand->value, 9.0F);
                ++matchCount;
            }
        }
    }
    EXPECT_EQ(matchCount, 1);
}

} // namespace
} // namespace UVE::Render::Shader::Tests
