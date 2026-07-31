//------------------------------------------------------------------------------
// UniVex Engine (UVE) — Proprietary Game Engine
// Copyright (c) 2026 UniVex Studios. All Rights Reserved.
// Unauthorized copying, modification, distribution, or use of this code
// in whole or in part is strictly prohibited without express written
// permission from UniVex Studios.
// Violators will be prosecuted to the fullest extent of the law.
//------------------------------------------------------------------------------

#include "uve/render/null_render_device_uve.h"

#include <algorithm>
#include <array>
#include <cstddef>
#include <memory>
#include <variant>
#include <vector>

#include <gtest/gtest.h>

#include "uve/debug/log_sink_uve.h"
#include "uve/debug/logger_uve.h"
#include "uve/platform/platform_uve.h"

namespace UVE::Render::Tests {
namespace {

TEST(NullRenderDeviceUVETest, CreateBufferUVE_ReturnsUniqueHandles) {
    NullRenderDeviceUVE device;
    const BufferHandleUVE first = device.CreateBufferUVE(BufferDescUVE{16, BufferUsageUVE::Vertex});
    const BufferHandleUVE second = device.CreateBufferUVE(BufferDescUVE{16, BufferUsageUVE::Vertex});

    EXPECT_NE(first, kInvalidBufferHandleUVE);
    EXPECT_NE(second, kInvalidBufferHandleUVE);
    EXPECT_NE(first, second);
}

TEST(NullRenderDeviceUVETest, DestroyBufferUVE_UnknownHandle_LogsErrorSafely) {
    Debug::LoggerUVE logger;
    logger.Init(Debug::LogLevelUVE::Trace);
    auto memorySink = std::make_unique<Debug::MemorySinkUVE>();
    Debug::MemorySinkUVE* const memorySinkPtr = memorySink.get();
    logger.AddSink(std::move(memorySink));

    NullRenderDeviceUVE device;
    device.DestroyBufferUVE(BufferHandleUVE{999});

    const std::vector<Debug::LogMessageUVE> messages = memorySinkPtr->GetMessagesUVE();
    const bool foundError =
        std::any_of(messages.begin(), messages.end(), [](const Debug::LogMessageUVE& message) {
            return message.level == Debug::LogLevelUVE::Error;
        });
    EXPECT_TRUE(foundError);

    logger.Shutdown();
}

TEST(NullRenderDeviceUVETest, UpdateBufferUVE_UnknownHandle_ReturnsFalseAndLogsError) {
    Debug::LoggerUVE logger;
    logger.Init(Debug::LogLevelUVE::Trace);
    auto memorySink = std::make_unique<Debug::MemorySinkUVE>();
    Debug::MemorySinkUVE* const memorySinkPtr = memorySink.get();
    logger.AddSink(std::move(memorySink));

    NullRenderDeviceUVE device;
    const std::array<std::byte, 4> data{};
    EXPECT_FALSE(device.UpdateBufferUVE(BufferHandleUVE{999}, data, 0));

    const std::vector<Debug::LogMessageUVE> messages = memorySinkPtr->GetMessagesUVE();
    const bool foundError =
        std::any_of(messages.begin(), messages.end(), [](const Debug::LogMessageUVE& message) {
            return message.level == Debug::LogLevelUVE::Error;
        });
    EXPECT_TRUE(foundError);

    logger.Shutdown();
}

TEST(NullRenderDeviceUVETest, UpdateBufferUVE_WriteWithinSize_ReturnsTrue) {
    NullRenderDeviceUVE device;
    const BufferHandleUVE buffer = device.CreateBufferUVE(BufferDescUVE{16, BufferUsageUVE::Uniform});
    const std::array<std::byte, 4> data{};

    EXPECT_TRUE(device.UpdateBufferUVE(buffer, data, 0));
    EXPECT_TRUE(device.UpdateBufferUVE(buffer, data, 12));
}

TEST(NullRenderDeviceUVETest, UpdateBufferUVE_WriteExceedingSize_ReturnsFalseAndLogsError) {
    Debug::LoggerUVE logger;
    logger.Init(Debug::LogLevelUVE::Trace);
    auto memorySink = std::make_unique<Debug::MemorySinkUVE>();
    Debug::MemorySinkUVE* const memorySinkPtr = memorySink.get();
    logger.AddSink(std::move(memorySink));

    NullRenderDeviceUVE device;
    const BufferHandleUVE buffer = device.CreateBufferUVE(BufferDescUVE{4, BufferUsageUVE::Uniform});
    const std::array<std::byte, 8> data{};
    EXPECT_FALSE(device.UpdateBufferUVE(buffer, data, 0));

    const std::vector<Debug::LogMessageUVE> messages = memorySinkPtr->GetMessagesUVE();
    const bool foundError =
        std::any_of(messages.begin(), messages.end(), [](const Debug::LogMessageUVE& message) {
            return message.level == Debug::LogLevelUVE::Error;
        });
    EXPECT_TRUE(foundError);

    logger.Shutdown();
}

TEST(NullRenderDeviceUVETest, CreateTextureUVE_ReturnsUniqueHandles) {
    NullRenderDeviceUVE device;
    const TextureHandleUVE first = device.CreateTextureUVE(TextureDescUVE{64, 64, TextureFormatUVE::RGBA8Unorm});
    const TextureHandleUVE second = device.CreateTextureUVE(TextureDescUVE{64, 64, TextureFormatUVE::RGBA8Unorm});

    EXPECT_NE(first, kInvalidTextureHandleUVE);
    EXPECT_NE(second, kInvalidTextureHandleUVE);
    EXPECT_NE(first, second);
}

TEST(NullRenderDeviceUVETest, CreateShaderUVE_ReturnsUniqueHandles) {
    NullRenderDeviceUVE device;
    const ShaderHandleUVE first = device.CreateShaderUVE(ShaderDescUVE{ShaderStageUVE::Vertex, "vs"});
    const ShaderHandleUVE second = device.CreateShaderUVE(ShaderDescUVE{ShaderStageUVE::Fragment, "fs"});

    EXPECT_NE(first, kInvalidShaderHandleUVE);
    EXPECT_NE(second, kInvalidShaderHandleUVE);
    EXPECT_NE(first, second);
}

TEST(NullRenderDeviceUVETest, CreatePipelineUVE_WithLiveShaders_Succeeds) {
    NullRenderDeviceUVE device;
    const ShaderHandleUVE vertexShader = device.CreateShaderUVE(ShaderDescUVE{ShaderStageUVE::Vertex, "vs"});
    const ShaderHandleUVE fragmentShader = device.CreateShaderUVE(ShaderDescUVE{ShaderStageUVE::Fragment, "fs"});

    PipelineDescUVE desc;
    desc.vertexShader = vertexShader;
    desc.fragmentShader = fragmentShader;

    EXPECT_NE(device.CreatePipelineUVE(desc), kInvalidPipelineHandleUVE);
}

TEST(NullRenderDeviceUVETest, CreatePipelineUVE_UnknownShaderHandle_ReturnsInvalidAndLogsError) {
    Debug::LoggerUVE logger;
    logger.Init(Debug::LogLevelUVE::Trace);
    auto memorySink = std::make_unique<Debug::MemorySinkUVE>();
    Debug::MemorySinkUVE* const memorySinkPtr = memorySink.get();
    logger.AddSink(std::move(memorySink));

    NullRenderDeviceUVE device;
    PipelineDescUVE desc;
    desc.vertexShader = ShaderHandleUVE{999};
    desc.fragmentShader = ShaderHandleUVE{998};
    EXPECT_EQ(device.CreatePipelineUVE(desc), kInvalidPipelineHandleUVE);

    const std::vector<Debug::LogMessageUVE> messages = memorySinkPtr->GetMessagesUVE();
    const bool foundError =
        std::any_of(messages.begin(), messages.end(), [](const Debug::LogMessageUVE& message) {
            return message.level == Debug::LogLevelUVE::Error;
        });
    EXPECT_TRUE(foundError);

    logger.Shutdown();
}

TEST(NullRenderDeviceUVETest, GetLiveResourceCountUVE_TracksCreateAndDestroy) {
    NullRenderDeviceUVE device;
    EXPECT_EQ(device.GetLiveResourceCountUVE(), 0U);

    const BufferHandleUVE buffer = device.CreateBufferUVE(BufferDescUVE{16, BufferUsageUVE::Vertex});
    EXPECT_EQ(device.GetLiveResourceCountUVE(), 1U);

    const TextureHandleUVE texture = device.CreateTextureUVE(TextureDescUVE{4, 4, TextureFormatUVE::RGBA8Unorm});
    EXPECT_EQ(device.GetLiveResourceCountUVE(), 2U);

    device.DestroyBufferUVE(buffer);
    EXPECT_EQ(device.GetLiveResourceCountUVE(), 1U);

    device.DestroyTextureUVE(texture);
    EXPECT_EQ(device.GetLiveResourceCountUVE(), 0U);
}

TEST(NullRenderDeviceUVETest, GetBackendNameUVE_ReturnsNull) {
    NullRenderDeviceUVE device;
    EXPECT_EQ(device.GetBackendNameUVE(), "Null");
}

TEST(NullRenderDeviceUVETest, CommandBufferRecordingThenSubmit_ProducesExpectedCommandSequenceInOrder) {
    NullRenderDeviceUVE device;
    const ShaderHandleUVE vertexShader = device.CreateShaderUVE(ShaderDescUVE{ShaderStageUVE::Vertex, "vs"});
    const ShaderHandleUVE fragmentShader = device.CreateShaderUVE(ShaderDescUVE{ShaderStageUVE::Fragment, "fs"});
    PipelineDescUVE pipelineDesc;
    pipelineDesc.vertexShader = vertexShader;
    pipelineDesc.fragmentShader = fragmentShader;
    const PipelineHandleUVE pipeline = device.CreatePipelineUVE(pipelineDesc);
    ASSERT_NE(pipeline, kInvalidPipelineHandleUVE);

    const BufferHandleUVE vertexBuffer = device.CreateBufferUVE(BufferDescUVE{64, BufferUsageUVE::Vertex});
    const BufferHandleUVE indexBuffer = device.CreateBufferUVE(BufferDescUVE{12, BufferUsageUVE::Index});
    const TextureHandleUVE colorTarget = device.CreateTextureUVE(TextureDescUVE{64, 64, TextureFormatUVE::RGBA8Unorm});

    std::unique_ptr<ICommandBufferUVE> commandBuffer = device.CreateCommandBufferUVE();
    RenderPassDescUVE passDesc;
    passDesc.colorAttachment = colorTarget;
    commandBuffer->BeginRenderPassUVE(passDesc);
    commandBuffer->BindPipelineUVE(pipeline);
    commandBuffer->BindVertexBufferUVE(vertexBuffer, 0);
    commandBuffer->BindIndexBufferUVE(indexBuffer);
    commandBuffer->DrawIndexedUVE(3, 1);
    commandBuffer->EndRenderPassUVE();

    device.SubmitUVE(std::move(commandBuffer));

    const std::vector<RecordedCommandUVE>& recorded = device.GetLastSubmittedCommandsUVE();
    ASSERT_EQ(recorded.size(), 6U);
    EXPECT_TRUE(std::holds_alternative<BeginRenderPassCommandUVE>(recorded[0]));
    EXPECT_TRUE(std::holds_alternative<BindPipelineCommandUVE>(recorded[1]));
    EXPECT_TRUE(std::holds_alternative<BindVertexBufferCommandUVE>(recorded[2]));
    EXPECT_TRUE(std::holds_alternative<BindIndexBufferCommandUVE>(recorded[3]));
    EXPECT_TRUE(std::holds_alternative<DrawIndexedCommandUVE>(recorded[4]));
    EXPECT_TRUE(std::holds_alternative<EndRenderPassCommandUVE>(recorded[5]));

    EXPECT_EQ(std::get<BeginRenderPassCommandUVE>(recorded[0]).desc.colorAttachment, colorTarget);
    EXPECT_EQ(std::get<BindPipelineCommandUVE>(recorded[1]).pipeline, pipeline);
    EXPECT_EQ(std::get<BindVertexBufferCommandUVE>(recorded[2]).buffer, vertexBuffer);
    EXPECT_EQ(std::get<BindIndexBufferCommandUVE>(recorded[3]).buffer, indexBuffer);
    EXPECT_EQ(std::get<DrawIndexedCommandUVE>(recorded[4]).indexCount, 3U);
    EXPECT_EQ(std::get<DrawIndexedCommandUVE>(recorded[4]).instanceCount, 1U);
}

TEST(NullRenderDeviceUVETest, GetLastSubmittedCommandsUVE_BeforeAnySubmit_IsEmpty) {
    NullRenderDeviceUVE device;
    EXPECT_TRUE(device.GetLastSubmittedCommandsUVE().empty());
}

#if UVE_DEBUG
TEST(NullRenderDeviceUVEDeathTest, CommandBuffer_NestedBeginRenderPass_Asserts) {
    NullRenderDeviceUVE device;
    std::unique_ptr<ICommandBufferUVE> commandBuffer = device.CreateCommandBufferUVE();
    commandBuffer->BeginRenderPassUVE(RenderPassDescUVE{});
    EXPECT_DEATH({ commandBuffer->BeginRenderPassUVE(RenderPassDescUVE{}); }, "");
}

TEST(NullRenderDeviceUVEDeathTest, CommandBuffer_DrawOutsideRenderPass_Asserts) {
    NullRenderDeviceUVE device;
    std::unique_ptr<ICommandBufferUVE> commandBuffer = device.CreateCommandBufferUVE();
    EXPECT_DEATH({ commandBuffer->DrawUVE(3); }, "");
}

TEST(NullRenderDeviceUVEDeathTest, CommandBuffer_EndRenderPassWithoutBegin_Asserts) {
    NullRenderDeviceUVE device;
    std::unique_ptr<ICommandBufferUVE> commandBuffer = device.CreateCommandBufferUVE();
    EXPECT_DEATH({ commandBuffer->EndRenderPassUVE(); }, "");
}
#endif

} // namespace
} // namespace UVE::Render::Tests
