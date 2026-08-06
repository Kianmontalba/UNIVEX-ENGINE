// Copyright (c) 2026 UniVex Studios. All Rights Reserved.


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
#include "uve/math/matrix4x4_uve.h"
#include "uve/math/vector3_uve.h"
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

constexpr std::string_view kUniformVertexShaderSource = R"(#version 330 core
layout(location = 0) in vec3 aPosition;
uniform mat4 uModel;
void main() {
    gl_Position = uModel * vec4(aPosition, 1.0);
}
)";

constexpr std::string_view kUniformFragmentShaderSource = R"(#version 330 core
out vec4 FragColor;
uniform vec3 uColor;
void main() {
    FragColor = vec4(uColor, 1.0);
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

// Depth-only render pass (Increment 26, shadow mapping's depth pre-pass): colorAttachment left
// invalid, depthAttachment a real Depth32Float texture — exercises GlCommandBufferUVE's
// depth-only FBO branch (glDrawBuffer(GL_NONE)/glReadBuffer(GL_NONE), viewport derived from the
// depth texture's own size since no color attachment is present to derive it from). Like
// FullTriangleDrawAndPresent_DoesNotCrash above, this codebase's public RHI surface doesn't expose
// glCheckFramebufferStatus/glGetError results directly, so "doesn't crash and produces a usable
// texture handle" is this test's proof, matching every other live-GL test's verification style.
TEST_F(GlRenderDeviceUVETest, DepthOnlyRenderPass_NoColorAttachment_DoesNotCrash) {
    const ShaderHandleUVE vertexShader =
        renderDevice->CreateShaderUVE(ShaderDescUVE{ShaderStageUVE::Vertex, std::string(kValidVertexShaderSource)});
    const ShaderHandleUVE fragmentShader = renderDevice->CreateShaderUVE(
        ShaderDescUVE{ShaderStageUVE::Fragment, std::string(kValidFragmentShaderSource)});

    PipelineDescUVE pipelineDesc;
    pipelineDesc.vertexShader = vertexShader;
    pipelineDesc.fragmentShader = fragmentShader;
    pipelineDesc.vertexLayout = {VertexAttributeUVE{"POSITION", VertexAttributeFormatUVE::Float3, 0}};
    pipelineDesc.vertexStride = 3U * static_cast<std::uint32_t>(sizeof(float));
    pipelineDesc.depthTestEnabled = true;
    pipelineDesc.depthWriteEnabled = true;
    const PipelineHandleUVE pipeline = renderDevice->CreatePipelineUVE(pipelineDesc);
    ASSERT_NE(pipeline, kInvalidPipelineHandleUVE);

    const TextureHandleUVE depthTexture =
        renderDevice->CreateTextureUVE(TextureDescUVE{64, 64, TextureFormatUVE::Depth32Float, 1});
    ASSERT_NE(depthTexture, kInvalidTextureHandleUVE);

    constexpr float kVertices[] = {0.0F, 0.5F, 0.0F, -0.5F, -0.5F, 0.0F, 0.5F, -0.5F, 0.0F};
    const std::span<const std::byte> vertexBytes = std::as_bytes(std::span<const float>(kVertices));
    const BufferHandleUVE vertexBuffer =
        renderDevice->CreateBufferUVE(BufferDescUVE{vertexBytes.size(), BufferUsageUVE::Vertex}, vertexBytes);

    std::unique_ptr<ICommandBufferUVE> commandBuffer = renderDevice->CreateCommandBufferUVE();
    ASSERT_NE(commandBuffer, nullptr);

    RenderPassDescUVE passDesc;
    passDesc.colorAttachment = kInvalidTextureHandleUVE;
    passDesc.depthAttachment = depthTexture;
    passDesc.depthLoadOp = LoadOpUVE::Clear;
    passDesc.clearDepth = 1.0F;

    commandBuffer->BeginRenderPassUVE(passDesc);
    commandBuffer->BindPipelineUVE(pipeline);
    commandBuffer->BindVertexBufferUVE(vertexBuffer);
    commandBuffer->DrawUVE(3);
    commandBuffer->EndRenderPassUVE();

    renderDevice->SubmitUVE(std::move(commandBuffer));

    renderDevice->DestroyBufferUVE(vertexBuffer);
    renderDevice->DestroyTextureUVE(depthTexture);
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

TEST_F(GlRenderDeviceUVETest, CreateShaderUVE_OutInfoLogParameter_CapturesLogOnBothSuccessAndFailure) {
    std::string successLog = "unset";
    const ShaderHandleUVE shader = renderDevice->CreateShaderUVE(
        ShaderDescUVE{ShaderStageUVE::Vertex, std::string(kValidVertexShaderSource)}, &successLog);
    EXPECT_NE(shader, kInvalidShaderHandleUVE);
    renderDevice->DestroyShaderUVE(shader);

    std::string failureLog;
    const ShaderHandleUVE broken = renderDevice->CreateShaderUVE(
        ShaderDescUVE{ShaderStageUVE::Fragment, std::string(kBrokenShaderSource)}, &failureLog);
    EXPECT_EQ(broken, kInvalidShaderHandleUVE);
    EXPECT_FALSE(failureLog.empty());
}

TEST_F(GlRenderDeviceUVETest, GetPipelineUniformsUVE_ReflectsDeclaredUniforms) {
    const ShaderHandleUVE vertexShader = renderDevice->CreateShaderUVE(
        ShaderDescUVE{ShaderStageUVE::Vertex, std::string(kUniformVertexShaderSource)});
    const ShaderHandleUVE fragmentShader = renderDevice->CreateShaderUVE(
        ShaderDescUVE{ShaderStageUVE::Fragment, std::string(kUniformFragmentShaderSource)});
    ASSERT_NE(vertexShader, kInvalidShaderHandleUVE);
    ASSERT_NE(fragmentShader, kInvalidShaderHandleUVE);

    PipelineDescUVE pipelineDesc;
    pipelineDesc.vertexShader = vertexShader;
    pipelineDesc.fragmentShader = fragmentShader;
    pipelineDesc.vertexLayout = {VertexAttributeUVE{"POSITION", VertexAttributeFormatUVE::Float3, 0}};
    pipelineDesc.vertexStride = 3U * static_cast<std::uint32_t>(sizeof(float));
    const PipelineHandleUVE pipeline = renderDevice->CreatePipelineUVE(pipelineDesc);
    ASSERT_NE(pipeline, kInvalidPipelineHandleUVE);

    const std::vector<UniformReflectionUVE> uniforms = renderDevice->GetPipelineUniformsUVE(pipeline);
    const bool foundModel = std::any_of(uniforms.begin(), uniforms.end(), [](const UniformReflectionUVE& uniform) {
        return uniform.name == "uModel" && uniform.type == ShaderDataTypeUVE::Mat4;
    });
    const bool foundColor = std::any_of(uniforms.begin(), uniforms.end(), [](const UniformReflectionUVE& uniform) {
        return uniform.name == "uColor" && uniform.type == ShaderDataTypeUVE::Vec3;
    });
    EXPECT_TRUE(foundModel);
    EXPECT_TRUE(foundColor);

    renderDevice->DestroyPipelineUVE(pipeline);
    renderDevice->DestroyShaderUVE(vertexShader);
    renderDevice->DestroyShaderUVE(fragmentShader);
}

TEST_F(GlRenderDeviceUVETest, GetPipelineUniformsUVE_UnknownHandle_ReturnsEmpty) {
    EXPECT_TRUE(renderDevice->GetPipelineUniformsUVE(PipelineHandleUVE{999}).empty());
}

TEST_F(GlRenderDeviceUVETest, PipelineBinaryRoundTrip_CreatePipelineFromBinaryUVE_ProducesValidPipeline) {
    const ShaderHandleUVE vertexShader =
        renderDevice->CreateShaderUVE(ShaderDescUVE{ShaderStageUVE::Vertex, std::string(kValidVertexShaderSource)});
    const ShaderHandleUVE fragmentShader = renderDevice->CreateShaderUVE(
        ShaderDescUVE{ShaderStageUVE::Fragment, std::string(kValidFragmentShaderSource)});

    PipelineDescUVE pipelineDesc;
    pipelineDesc.vertexShader = vertexShader;
    pipelineDesc.fragmentShader = fragmentShader;
    pipelineDesc.vertexLayout = {VertexAttributeUVE{"POSITION", VertexAttributeFormatUVE::Float3, 0}};
    pipelineDesc.vertexStride = 3U * static_cast<std::uint32_t>(sizeof(float));
    const PipelineHandleUVE pipeline = renderDevice->CreatePipelineUVE(pipelineDesc);
    ASSERT_NE(pipeline, kInvalidPipelineHandleUVE);

    std::vector<std::byte> binary;
    std::uint32_t format = 0;
    const bool gotBinary = renderDevice->GetPipelineBinaryUVE(pipeline, binary, format);
    renderDevice->DestroyPipelineUVE(pipeline);
    renderDevice->DestroyShaderUVE(vertexShader);
    renderDevice->DestroyShaderUVE(fragmentShader);

    if (!gotBinary || binary.empty()) {
        GTEST_SKIP() << "This sandbox's Mesa/llvmpipe driver did not return a usable program "
                        "binary (GL_PROGRAM_BINARY_LENGTH <= 0) - documented as a plain cache "
                        "miss, not a failure, so skip the round-trip assertion here.";
    }

    PipelineBinaryDescUVE binaryDesc;
    binaryDesc.vertexLayout = pipelineDesc.vertexLayout;
    binaryDesc.vertexStride = pipelineDesc.vertexStride;
    const PipelineHandleUVE restored = renderDevice->CreatePipelineFromBinaryUVE(binary, format, binaryDesc);
    EXPECT_NE(restored, kInvalidPipelineHandleUVE);
    if (restored != kInvalidPipelineHandleUVE) {
        renderDevice->DestroyPipelineUVE(restored);
    }
}

TEST_F(GlRenderDeviceUVETest, GetPipelineBinaryUVE_UnknownHandle_ReturnsFalse) {
    std::vector<std::byte> binary;
    std::uint32_t format = 0;
    EXPECT_FALSE(renderDevice->GetPipelineBinaryUVE(PipelineHandleUVE{999}, binary, format));
}

TEST_F(GlRenderDeviceUVETest, CommandBuffer_SetUniformCalls_OnBoundPipeline_DoNotCrash) {
    const ShaderHandleUVE vertexShader = renderDevice->CreateShaderUVE(
        ShaderDescUVE{ShaderStageUVE::Vertex, std::string(kUniformVertexShaderSource)});
    const ShaderHandleUVE fragmentShader = renderDevice->CreateShaderUVE(
        ShaderDescUVE{ShaderStageUVE::Fragment, std::string(kUniformFragmentShaderSource)});

    PipelineDescUVE pipelineDesc;
    pipelineDesc.vertexShader = vertexShader;
    pipelineDesc.fragmentShader = fragmentShader;
    pipelineDesc.vertexLayout = {VertexAttributeUVE{"POSITION", VertexAttributeFormatUVE::Float3, 0}};
    pipelineDesc.vertexStride = 3U * static_cast<std::uint32_t>(sizeof(float));
    const PipelineHandleUVE pipeline = renderDevice->CreatePipelineUVE(pipelineDesc);
    ASSERT_NE(pipeline, kInvalidPipelineHandleUVE);

    std::unique_ptr<ICommandBufferUVE> commandBuffer = renderDevice->CreateCommandBufferUVE();
    RenderPassDescUVE passDesc;
    passDesc.colorAttachment = kInvalidTextureHandleUVE;
    passDesc.colorLoadOp = LoadOpUVE::DontCare;
    passDesc.depthLoadOp = LoadOpUVE::DontCare;

    commandBuffer->BeginRenderPassUVE(passDesc);
    commandBuffer->BindPipelineUVE(pipeline);
    commandBuffer->SetUniformMatrix4x4UVE("uModel", Math::Matrix4x4UVE::IdentityUVE());
    commandBuffer->SetUniformVector3UVE("uColor", Math::Vector3UVE{1.0F, 0.0F, 0.0F});
    commandBuffer->EndRenderPassUVE();

    renderDevice->SubmitUVE(std::move(commandBuffer));

    renderDevice->DestroyPipelineUVE(pipeline);
    renderDevice->DestroyShaderUVE(vertexShader);
    renderDevice->DestroyShaderUVE(fragmentShader);

    SUCCEED();
}

} // namespace
} // namespace UVE::Render::Tests
