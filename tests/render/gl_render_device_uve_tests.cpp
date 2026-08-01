//------------------------------------------------------------------------------
// UniVex Engine (UVE) — Proprietary Game Engine
// Copyright (c) 2026 UniVex Studios. All Rights Reserved.
// Unauthorized copying, modification, distribution, or use of this code
// in whole or in part is strictly prohibited without express written
// permission from UniVex Studios.
// Violators will be prosecuted to the fullest extent of the law.
//------------------------------------------------------------------------------

#include "uve/render/gl_render_device_uve.h"

#include <algorithm>
#include <array>
#include <memory>
#include <span>
#include <string>

#include <gtest/gtest.h>

#include "uve/debug/log_sink_uve.h"
#include "uve/debug/logger_uve.h"
#include "uve/events/event_system_uve.h"
#include "uve/window/window_manager_uve.h"

namespace UVE::Render::Tests {
namespace {

// Same rationale as tests/window/window_manager_uve_tests.cpp: this sandbox's Mesa/llvmpipe GLX
// stack caps at OpenGL 4.5 Core, so tests explicitly request 4.5 rather than the production
// default (4.6) to isolate "no display available" as the only GTEST_SKIP() reason.
[[nodiscard]] Window::WindowDescUVE MakeTestWindowDescUVE() {
    Window::WindowDescUVE desc;
    desc.title = "uve_gl_render_device_uve_tests";
    desc.width = 64;
    desc.height = 64;
    desc.glVersionMajor = 4;
    desc.glVersionMinor = 5;
    return desc;
}

constexpr std::string_view kValidVertexShaderSource = R"(#version 330 core
layout(location = 0) in vec3 aPosition;
void main() {
    gl_Position = vec4(aPosition, 1.0);
}
)";

constexpr std::string_view kValidFragmentShaderSource = R"(#version 330 core
out vec4 FragColor;
void main() {
    FragColor = vec4(1.0, 1.0, 1.0, 1.0);
}
)";

constexpr std::string_view kBrokenShaderSource = R"(#version 330 core
void main() {
    this is not valid glsl :(
}
)";

// Needs a real (possibly virtual, e.g. Xvfb) GL context. Every test's fixture checks this at
// SetUp() and calls GTEST_SKIP() with a clear message if unavailable, so the same uve_tests
// binary runs cleanly with or without a display attached.
class GlRenderDeviceUVETest : public ::testing::Test {
protected:
    void SetUp() override {
        windowManager = std::make_unique<Window::WindowManagerUVE>(eventSystem, MakeTestWindowDescUVE());
        if (!windowManager->IsValidUVE()) {
            GTEST_SKIP() << "No display available for GlRenderDeviceUVE - skipping (run under "
                            "xvfb-run to exercise this test)";
        }
        renderDevice = std::make_unique<GlRenderDeviceUVE>(*windowManager);
    }

    Events::EventSystemUVE eventSystem;
    std::unique_ptr<Window::WindowManagerUVE> windowManager;
    std::unique_ptr<GlRenderDeviceUVE> renderDevice;
};

TEST_F(GlRenderDeviceUVETest, GetBackendNameUVE_ReturnsOpenGL) {
    EXPECT_EQ(renderDevice->GetBackendNameUVE(), "OpenGL");
}

TEST_F(GlRenderDeviceUVETest, CreateThenDestroyBuffer_UpdatesLiveResourceCount) {
    ASSERT_EQ(renderDevice->GetLiveResourceCountUVE(), 0U);
    const BufferHandleUVE buffer = renderDevice->CreateBufferUVE(BufferDescUVE{64, BufferUsageUVE::Vertex});
    EXPECT_NE(buffer, kInvalidBufferHandleUVE);
    EXPECT_EQ(renderDevice->GetLiveResourceCountUVE(), 1U);

    renderDevice->DestroyBufferUVE(buffer);
    EXPECT_EQ(renderDevice->GetLiveResourceCountUVE(), 0U);
}

TEST_F(GlRenderDeviceUVETest, UpdateBufferUVE_WithinBounds_Succeeds) {
    const BufferHandleUVE buffer = renderDevice->CreateBufferUVE(BufferDescUVE{16, BufferUsageUVE::Vertex});
    const std::array<std::byte, 8> data{};
    EXPECT_TRUE(renderDevice->UpdateBufferUVE(buffer, data, 0));
    renderDevice->DestroyBufferUVE(buffer);
}

TEST_F(GlRenderDeviceUVETest, UpdateBufferUVE_OutOfBounds_ReturnsFalse) {
    const BufferHandleUVE buffer = renderDevice->CreateBufferUVE(BufferDescUVE{8, BufferUsageUVE::Vertex});
    const std::array<std::byte, 16> data{};
    EXPECT_FALSE(renderDevice->UpdateBufferUVE(buffer, data, 0));
    renderDevice->DestroyBufferUVE(buffer);
}

TEST_F(GlRenderDeviceUVETest, CreateThenDestroyTexture_UpdatesLiveResourceCount) {
    ASSERT_EQ(renderDevice->GetLiveResourceCountUVE(), 0U);
    const TextureHandleUVE texture =
        renderDevice->CreateTextureUVE(TextureDescUVE{64, 64, TextureFormatUVE::RGBA8Unorm, 1});
    EXPECT_NE(texture, kInvalidTextureHandleUVE);
    EXPECT_EQ(renderDevice->GetLiveResourceCountUVE(), 1U);

    renderDevice->DestroyTextureUVE(texture);
    EXPECT_EQ(renderDevice->GetLiveResourceCountUVE(), 0U);
}

TEST_F(GlRenderDeviceUVETest, CreateShaderUVE_ValidSource_Succeeds) {
    const ShaderHandleUVE shader =
        renderDevice->CreateShaderUVE(ShaderDescUVE{ShaderStageUVE::Vertex, std::string(kValidVertexShaderSource)});
    EXPECT_NE(shader, kInvalidShaderHandleUVE);
    renderDevice->DestroyShaderUVE(shader);
}

TEST_F(GlRenderDeviceUVETest, CreateShaderUVE_BrokenSource_ReturnsInvalidAndLogsCompilerDiagnostic) {
    Debug::LoggerUVE logger;
    logger.Init(Debug::LogLevelUVE::Trace);
    auto memorySink = std::make_unique<Debug::MemorySinkUVE>();
    Debug::MemorySinkUVE* const memorySinkPtr = memorySink.get();
    logger.AddSink(std::move(memorySink));

    const ShaderHandleUVE shader =
        renderDevice->CreateShaderUVE(ShaderDescUVE{ShaderStageUVE::Fragment, std::string(kBrokenShaderSource)});
    EXPECT_EQ(shader, kInvalidShaderHandleUVE);

    const std::vector<Debug::LogMessageUVE> messages = memorySinkPtr->GetMessagesUVE();
    const bool foundCompilerDiagnostic =
        std::any_of(messages.begin(), messages.end(), [](const Debug::LogMessageUVE& message) {
            return message.level == Debug::LogLevelUVE::Error &&
                   message.message.find("compilation failed") != std::string::npos;
        });
    EXPECT_TRUE(foundCompilerDiagnostic);

    logger.Shutdown();
}

TEST_F(GlRenderDeviceUVETest, CreatePipelineUVE_UnknownShaderHandle_ReturnsInvalid) {
    PipelineDescUVE desc;
    desc.vertexShader = ShaderHandleUVE{999};
    desc.fragmentShader = ShaderHandleUVE{998};
    EXPECT_EQ(renderDevice->CreatePipelineUVE(desc), kInvalidPipelineHandleUVE);
}

TEST_F(GlRenderDeviceUVETest, CreatePipelineUVE_ValidShaders_Succeeds) {
    const ShaderHandleUVE vertexShader =
        renderDevice->CreateShaderUVE(ShaderDescUVE{ShaderStageUVE::Vertex, std::string(kValidVertexShaderSource)});
    const ShaderHandleUVE fragmentShader = renderDevice->CreateShaderUVE(
        ShaderDescUVE{ShaderStageUVE::Fragment, std::string(kValidFragmentShaderSource)});
    ASSERT_NE(vertexShader, kInvalidShaderHandleUVE);
    ASSERT_NE(fragmentShader, kInvalidShaderHandleUVE);

    PipelineDescUVE pipelineDesc;
    pipelineDesc.vertexShader = vertexShader;
    pipelineDesc.fragmentShader = fragmentShader;
    pipelineDesc.vertexLayout = {VertexAttributeUVE{"POSITION", VertexAttributeFormatUVE::Float3, 0}};
    pipelineDesc.vertexStride = 3U * static_cast<std::uint32_t>(sizeof(float));

    const PipelineHandleUVE pipeline = renderDevice->CreatePipelineUVE(pipelineDesc);
    EXPECT_NE(pipeline, kInvalidPipelineHandleUVE);

    renderDevice->DestroyPipelineUVE(pipeline);
    renderDevice->DestroyShaderUVE(vertexShader);
    renderDevice->DestroyShaderUVE(fragmentShader);
}

TEST_F(GlRenderDeviceUVETest, FullTriangleDrawAndPresent_DoesNotCrash) {
    const ShaderHandleUVE vertexShader =
        renderDevice->CreateShaderUVE(ShaderDescUVE{ShaderStageUVE::Vertex, std::string(kValidVertexShaderSource)});
    const ShaderHandleUVE fragmentShader = renderDevice->CreateShaderUVE(
        ShaderDescUVE{ShaderStageUVE::Fragment, std::string(kValidFragmentShaderSource)});

    PipelineDescUVE pipelineDesc;
    pipelineDesc.vertexShader = vertexShader;
    pipelineDesc.fragmentShader = fragmentShader;
    pipelineDesc.vertexLayout = {VertexAttributeUVE{"POSITION", VertexAttributeFormatUVE::Float3, 0}};
    pipelineDesc.vertexStride = 3U * static_cast<std::uint32_t>(sizeof(float));
    pipelineDesc.depthTestEnabled = false;
    pipelineDesc.depthWriteEnabled = false;
    const PipelineHandleUVE pipeline = renderDevice->CreatePipelineUVE(pipelineDesc);
    ASSERT_NE(pipeline, kInvalidPipelineHandleUVE);

    constexpr float kVertices[] = {0.0F, 0.5F, 0.0F, -0.5F, -0.5F, 0.0F, 0.5F, -0.5F, 0.0F};
    const std::span<const std::byte> vertexBytes = std::as_bytes(std::span<const float>(kVertices));
    const BufferHandleUVE vertexBuffer =
        renderDevice->CreateBufferUVE(BufferDescUVE{vertexBytes.size(), BufferUsageUVE::Vertex}, vertexBytes);

    std::unique_ptr<ICommandBufferUVE> commandBuffer = renderDevice->CreateCommandBufferUVE();
    ASSERT_NE(commandBuffer, nullptr);

    RenderPassDescUVE passDesc;
    passDesc.colorAttachment = kInvalidTextureHandleUVE;
    passDesc.colorLoadOp = LoadOpUVE::Clear;
    passDesc.clearColor = {0.05F, 0.05F, 0.05F, 1.0F};
    passDesc.depthLoadOp = LoadOpUVE::DontCare;

    commandBuffer->BeginRenderPassUVE(passDesc);
    commandBuffer->BindPipelineUVE(pipeline);
    commandBuffer->BindVertexBufferUVE(vertexBuffer);
    commandBuffer->DrawUVE(3);
    commandBuffer->EndRenderPassUVE();

    renderDevice->SubmitUVE(std::move(commandBuffer));
    renderDevice->PresentUVE();

    renderDevice->DestroyBufferUVE(vertexBuffer);
    renderDevice->DestroyPipelineUVE(pipeline);
    renderDevice->DestroyShaderUVE(vertexShader);
    renderDevice->DestroyShaderUVE(fragmentShader);

    SUCCEED();
}

TEST_F(GlRenderDeviceUVETest, RepeatedPresentCalls_DoNotChangeLiveResourceCount) {
    ASSERT_EQ(renderDevice->GetLiveResourceCountUVE(), 0U);
    for (int i = 0; i < 5; ++i) {
        renderDevice->PresentUVE();
    }
    EXPECT_EQ(renderDevice->GetLiveResourceCountUVE(), 0U);
}

} // namespace
} // namespace UVE::Render::Tests
