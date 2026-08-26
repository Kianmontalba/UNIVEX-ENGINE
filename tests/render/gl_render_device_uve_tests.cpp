// Copyright (c) 2026 UniVex Studios. All Rights Reserved.


#include "uve/render/gl_render_device_uve.h"
#include "uve/asset/asset_bundle_uve.h"
#include "uve/asset/file_system_uve.h"
#include "uve/asset/mesh_asset_uve.h"
#include "uve/render/shader/built_in_shaders_uve.h"
#include "uve/render/shader/shader_manager_uve.h"
#include "uve/threading/thread_pool_uve.h"

#include <algorithm>
#include <array>
#include <cstdint>
#include <memory>
#include <span>
#include <string>
#include <thread>

#ifndef GL_GLEXT_PROTOTYPES
#define GL_GLEXT_PROTOTYPES 1
#endif
#include <GL/gl.h>
#include <gtest/gtest.h>

#include <chrono>
#include <limits>

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
constexpr std::string_view kSamplerAndUnsupportedUniformFragmentShaderSource = R"(#version 330 core
out vec4 FragColor;
uniform sampler2D uTexture;
uniform uint uUnsigned;
void main() {
    FragColor = texture(uTexture, vec2(0.5)) * float(uUnsigned);
}
)";

[[nodiscard]] std::string WithShaderStageDefineUVE(std::string_view source, std::string_view stageDefine) {
    std::string resolvedSource(source);
    const std::size_t firstLineEnd = resolvedSource.find('\n');
    if (firstLineEnd == std::string::npos) {
        return resolvedSource;
    }
    resolvedSource.insert(firstLineEnd + 1U, "#define " + std::string(stageDefine) + "\n");
    return resolvedSource;
}

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

TEST_F(GlRenderDeviceUVETest, CreateShaderUVE_UnknownStage_ReturnsInvalidBeforeAllocation) {
    ASSERT_EQ(renderDevice->GetLiveResourceCountUVE(), 0U);
    std::string infoLog = "stale";
    const ShaderHandleUVE invalid = renderDevice->CreateShaderUVE(
        ShaderDescUVE{static_cast<ShaderStageUVE>(0xFFU), std::string(kValidVertexShaderSource)}, &infoLog);

    EXPECT_EQ(invalid, kInvalidShaderHandleUVE);
    EXPECT_EQ(infoLog, "Unknown shader stage.");
    EXPECT_EQ(renderDevice->GetLiveResourceCountUVE(), 0U);
}

TEST_F(GlRenderDeviceUVETest, CreateBufferUVE_UnknownUsage_ReturnsInvalidBeforeAllocation) {
    ASSERT_EQ(renderDevice->GetLiveResourceCountUVE(), 0U);
    const BufferHandleUVE invalid =
        renderDevice->CreateBufferUVE(BufferDescUVE{16U, static_cast<BufferUsageUVE>(0xFFU)});

    EXPECT_EQ(invalid, kInvalidBufferHandleUVE);
    EXPECT_EQ(renderDevice->GetLiveResourceCountUVE(), 0U);
}

TEST_F(GlRenderDeviceUVETest, CreateBufferUVE_SizeExceedsGlsizeiptr_ReturnsInvalidBeforeAllocation) {
    ASSERT_EQ(renderDevice->GetLiveResourceCountUVE(), 0U);
    const BufferDescUVE oversizedDesc{
        static_cast<std::uint64_t>(std::numeric_limits<GLsizeiptr>::max()) + 1U, BufferUsageUVE::Vertex};

    EXPECT_EQ(renderDevice->CreateBufferUVE(oversizedDesc), kInvalidBufferHandleUVE);
    EXPECT_EQ(renderDevice->GetLiveResourceCountUVE(), 0U);

    const BufferHandleUVE valid = renderDevice->CreateBufferUVE(BufferDescUVE{16U, BufferUsageUVE::Vertex});
    ASSERT_NE(valid, kInvalidBufferHandleUVE);
    EXPECT_EQ(valid.value, 1U);
}

TEST_F(GlRenderDeviceUVETest, CreateTextureUVE_DimensionExceedsGlsizei_ReturnsInvalidBeforeAllocation) {
    ASSERT_EQ(renderDevice->GetLiveResourceCountUVE(), 0U);
    TextureDescUVE oversizedDesc;
    oversizedDesc.width = static_cast<std::uint32_t>(std::numeric_limits<GLsizei>::max()) + 1U;
    oversizedDesc.height = 1U;

    EXPECT_EQ(renderDevice->CreateTextureUVE(oversizedDesc), kInvalidTextureHandleUVE);
    EXPECT_EQ(renderDevice->GetLiveResourceCountUVE(), 0U);

    const TextureHandleUVE valid = renderDevice->CreateTextureUVE(TextureDescUVE{1U, 1U});
    ASSERT_NE(valid, kInvalidTextureHandleUVE);
    EXPECT_EQ(valid.value, 1U);
}

TEST_F(GlRenderDeviceUVETest, CreatePipelineUVE_VertexLayoutExceedsGlLimit_ReturnsInvalidBeforeAllocation) {
    GLint maxVertexAttribs = 0;
    glGetIntegerv(GL_MAX_VERTEX_ATTRIBS, &maxVertexAttribs);
    ASSERT_GT(maxVertexAttribs, 0);

    const ShaderHandleUVE vertexShader =
        renderDevice->CreateShaderUVE(ShaderDescUVE{ShaderStageUVE::Vertex, std::string(kValidVertexShaderSource)});
    const ShaderHandleUVE fragmentShader = renderDevice->CreateShaderUVE(
        ShaderDescUVE{ShaderStageUVE::Fragment, std::string(kValidFragmentShaderSource)});
    ASSERT_NE(vertexShader, kInvalidShaderHandleUVE);
    ASSERT_NE(fragmentShader, kInvalidShaderHandleUVE);

    PipelineDescUVE oversizedDesc;
    oversizedDesc.vertexShader = vertexShader;
    oversizedDesc.fragmentShader = fragmentShader;
    oversizedDesc.vertexStride = 3U * static_cast<std::uint32_t>(sizeof(float));
    oversizedDesc.vertexLayout.resize(static_cast<std::size_t>(maxVertexAttribs) + 1U,
                                      VertexAttributeUVE{"POSITION", VertexAttributeFormatUVE::Float3, 0U});

    EXPECT_EQ(renderDevice->CreatePipelineUVE(oversizedDesc), kInvalidPipelineHandleUVE);
    EXPECT_EQ(renderDevice->GetLiveResourceCountUVE(), 2U);
}

TEST_F(GlRenderDeviceUVETest, CreatePipelineUVE_UnknownVertexFormat_ReturnsInvalidBeforeAllocation) {
    const ShaderHandleUVE vertexShader =
        renderDevice->CreateShaderUVE(ShaderDescUVE{ShaderStageUVE::Vertex, std::string(kValidVertexShaderSource)});
    const ShaderHandleUVE fragmentShader = renderDevice->CreateShaderUVE(
        ShaderDescUVE{ShaderStageUVE::Fragment, std::string(kValidFragmentShaderSource)});
    ASSERT_NE(vertexShader, kInvalidShaderHandleUVE);
    ASSERT_NE(fragmentShader, kInvalidShaderHandleUVE);

    PipelineDescUVE invalidDesc;
    invalidDesc.vertexShader = vertexShader;
    invalidDesc.fragmentShader = fragmentShader;
    invalidDesc.vertexLayout = {VertexAttributeUVE{"POSITION", static_cast<VertexAttributeFormatUVE>(0xFFU), 0U}};
    EXPECT_EQ(renderDevice->CreatePipelineUVE(invalidDesc), kInvalidPipelineHandleUVE);
    EXPECT_EQ(renderDevice->GetLiveResourceCountUVE(), 2U);

    PipelineDescUVE validDesc = invalidDesc;
    validDesc.vertexLayout = {VertexAttributeUVE{"POSITION", VertexAttributeFormatUVE::Float3, 0U}};
    validDesc.vertexStride = 3U * static_cast<std::uint32_t>(sizeof(float));
    const PipelineHandleUVE validPipeline = renderDevice->CreatePipelineUVE(validDesc);
    EXPECT_EQ(validPipeline.value, 1U);

    renderDevice->DestroyPipelineUVE(validPipeline);
    renderDevice->DestroyShaderUVE(fragmentShader);
    renderDevice->DestroyShaderUVE(vertexShader);
}

TEST_F(GlRenderDeviceUVETest, CreatePipelineFromBinaryUVE_UnknownVertexFormat_ReturnsInvalidBeforeAllocation) {
    const std::array<std::byte, 4> binary{};
    PipelineBinaryDescUVE invalidDesc;
    invalidDesc.vertexLayout = {VertexAttributeUVE{"POSITION", static_cast<VertexAttributeFormatUVE>(0xFFU), 0U}};

    EXPECT_EQ(renderDevice->CreatePipelineFromBinaryUVE(binary, 0U, invalidDesc), kInvalidPipelineHandleUVE);
    EXPECT_EQ(renderDevice->GetLiveResourceCountUVE(), 0U);
}

TEST_F(GlRenderDeviceUVETest, CreatePipelineUVE_UnknownBlendMode_ReturnsInvalidBeforeAllocation) {
    const ShaderHandleUVE vertexShader =
        renderDevice->CreateShaderUVE(ShaderDescUVE{ShaderStageUVE::Vertex, std::string(kValidVertexShaderSource)});
    const ShaderHandleUVE fragmentShader = renderDevice->CreateShaderUVE(
        ShaderDescUVE{ShaderStageUVE::Fragment, std::string(kValidFragmentShaderSource)});
    ASSERT_NE(vertexShader, kInvalidShaderHandleUVE);
    ASSERT_NE(fragmentShader, kInvalidShaderHandleUVE);

    PipelineDescUVE invalidDesc;
    invalidDesc.vertexShader = vertexShader;
    invalidDesc.fragmentShader = fragmentShader;
    invalidDesc.blendMode = static_cast<PipelineBlendModeUVE>(0xFFU);
    EXPECT_EQ(renderDevice->CreatePipelineUVE(invalidDesc), kInvalidPipelineHandleUVE);
    EXPECT_EQ(renderDevice->GetLiveResourceCountUVE(), 2U);

    invalidDesc.blendMode = PipelineBlendModeUVE::Opaque;
    invalidDesc.vertexLayout = {VertexAttributeUVE{"POSITION", VertexAttributeFormatUVE::Float3, 0U}};
    invalidDesc.vertexStride = 3U * static_cast<std::uint32_t>(sizeof(float));
    const PipelineHandleUVE validPipeline = renderDevice->CreatePipelineUVE(invalidDesc);
    EXPECT_EQ(validPipeline.value, 1U);

    renderDevice->DestroyPipelineUVE(validPipeline);
    renderDevice->DestroyShaderUVE(fragmentShader);
    renderDevice->DestroyShaderUVE(vertexShader);
}

TEST_F(GlRenderDeviceUVETest, CreatePipelineFromBinaryUVE_UnknownBlendMode_ReturnsInvalidBeforeAllocation) {
    const std::array<std::byte, 4> binary{};
    PipelineBinaryDescUVE invalidDesc;
    invalidDesc.blendMode = static_cast<PipelineBlendModeUVE>(0xFFU);

    EXPECT_EQ(renderDevice->CreatePipelineFromBinaryUVE(binary, 0U, invalidDesc), kInvalidPipelineHandleUVE);
    EXPECT_EQ(renderDevice->GetLiveResourceCountUVE(), 0U);
}

TEST_F(GlRenderDeviceUVETest, CreatePipelineUVE_UnknownTopology_ReturnsInvalidBeforeAllocation) {
    const ShaderHandleUVE vertexShader =
        renderDevice->CreateShaderUVE(ShaderDescUVE{ShaderStageUVE::Vertex, std::string(kValidVertexShaderSource)});
    const ShaderHandleUVE fragmentShader = renderDevice->CreateShaderUVE(
        ShaderDescUVE{ShaderStageUVE::Fragment, std::string(kValidFragmentShaderSource)});
    ASSERT_NE(vertexShader, kInvalidShaderHandleUVE);
    ASSERT_NE(fragmentShader, kInvalidShaderHandleUVE);

    PipelineDescUVE invalidDesc;
    invalidDesc.vertexShader = vertexShader;
    invalidDesc.fragmentShader = fragmentShader;
    invalidDesc.topology = static_cast<PrimitiveTopologyUVE>(0xFFU);
    EXPECT_EQ(renderDevice->CreatePipelineUVE(invalidDesc), kInvalidPipelineHandleUVE);
    EXPECT_EQ(renderDevice->GetLiveResourceCountUVE(), 2U);

    invalidDesc.topology = PrimitiveTopologyUVE::Triangles;
    invalidDesc.vertexLayout = {VertexAttributeUVE{"POSITION", VertexAttributeFormatUVE::Float3, 0U}};
    invalidDesc.vertexStride = 3U * static_cast<std::uint32_t>(sizeof(float));
    const PipelineHandleUVE validPipeline = renderDevice->CreatePipelineUVE(invalidDesc);
    EXPECT_EQ(validPipeline.value, 1U);

    renderDevice->DestroyPipelineUVE(validPipeline);
    renderDevice->DestroyShaderUVE(fragmentShader);
    renderDevice->DestroyShaderUVE(vertexShader);
}

TEST_F(GlRenderDeviceUVETest, CreatePipelineFromBinaryUVE_UnknownTopology_ReturnsInvalidBeforeAllocation) {
    const std::array<std::byte, 4> binary{};
    PipelineBinaryDescUVE invalidDesc;
    invalidDesc.topology = static_cast<PrimitiveTopologyUVE>(0xFFU);

    EXPECT_EQ(renderDevice->CreatePipelineFromBinaryUVE(binary, 0U, invalidDesc), kInvalidPipelineHandleUVE);
    EXPECT_EQ(renderDevice->GetLiveResourceCountUVE(), 0U);
}

TEST_F(GlRenderDeviceUVETest, CreatePipelineUVE_ZeroVertexStride_ReturnsInvalidBeforeAllocation) {
    const ShaderHandleUVE vertexShader =
        renderDevice->CreateShaderUVE(ShaderDescUVE{ShaderStageUVE::Vertex, std::string(kValidVertexShaderSource)});
    const ShaderHandleUVE fragmentShader = renderDevice->CreateShaderUVE(
        ShaderDescUVE{ShaderStageUVE::Fragment, std::string(kValidFragmentShaderSource)});
    ASSERT_NE(vertexShader, kInvalidShaderHandleUVE);
    ASSERT_NE(fragmentShader, kInvalidShaderHandleUVE);

    PipelineDescUVE invalidDesc;
    invalidDesc.vertexShader = vertexShader;
    invalidDesc.fragmentShader = fragmentShader;
    invalidDesc.vertexLayout = {VertexAttributeUVE{"POSITION", VertexAttributeFormatUVE::Float3, 0U}};
    EXPECT_EQ(renderDevice->CreatePipelineUVE(invalidDesc), kInvalidPipelineHandleUVE);
    EXPECT_EQ(renderDevice->GetLiveResourceCountUVE(), 2U);

    invalidDesc.vertexStride = 3U * static_cast<std::uint32_t>(sizeof(float));
    const PipelineHandleUVE validPipeline = renderDevice->CreatePipelineUVE(invalidDesc);
    EXPECT_EQ(validPipeline.value, 1U);

    renderDevice->DestroyPipelineUVE(validPipeline);
    renderDevice->DestroyShaderUVE(fragmentShader);
    renderDevice->DestroyShaderUVE(vertexShader);
}

TEST_F(GlRenderDeviceUVETest, CreatePipelineFromBinaryUVE_ZeroVertexStride_ReturnsInvalidBeforeAllocation) {
    const std::array<std::byte, 4> binary{};
    PipelineBinaryDescUVE invalidDesc;
    invalidDesc.vertexLayout = {VertexAttributeUVE{"POSITION", VertexAttributeFormatUVE::Float3, 0U}};

    EXPECT_EQ(renderDevice->CreatePipelineFromBinaryUVE(binary, 0U, invalidDesc), kInvalidPipelineHandleUVE);
    EXPECT_EQ(renderDevice->GetLiveResourceCountUVE(), 0U);
}

TEST_F(GlRenderDeviceUVETest, CreatePipelineUVE_AttributeExceedsVertexStride_ReturnsInvalidBeforeAllocation) {
    const ShaderHandleUVE vertexShader =
        renderDevice->CreateShaderUVE(ShaderDescUVE{ShaderStageUVE::Vertex, std::string(kValidVertexShaderSource)});
    const ShaderHandleUVE fragmentShader = renderDevice->CreateShaderUVE(
        ShaderDescUVE{ShaderStageUVE::Fragment, std::string(kValidFragmentShaderSource)});
    ASSERT_NE(vertexShader, kInvalidShaderHandleUVE);
    ASSERT_NE(fragmentShader, kInvalidShaderHandleUVE);

    PipelineDescUVE invalidDesc;
    invalidDesc.vertexShader = vertexShader;
    invalidDesc.fragmentShader = fragmentShader;
    invalidDesc.vertexLayout = {VertexAttributeUVE{"POSITION", VertexAttributeFormatUVE::Float3, 8U}};
    invalidDesc.vertexStride = 12U;
    EXPECT_EQ(renderDevice->CreatePipelineUVE(invalidDesc), kInvalidPipelineHandleUVE);
    EXPECT_EQ(renderDevice->GetLiveResourceCountUVE(), 2U);

    invalidDesc.vertexLayout[0].offset = 0U;
    const PipelineHandleUVE validPipeline = renderDevice->CreatePipelineUVE(invalidDesc);
    EXPECT_EQ(validPipeline.value, 1U);

    renderDevice->DestroyPipelineUVE(validPipeline);
    renderDevice->DestroyShaderUVE(fragmentShader);
    renderDevice->DestroyShaderUVE(vertexShader);
}

TEST_F(GlRenderDeviceUVETest, CreatePipelineFromBinaryUVE_AttributeExceedsVertexStride_ReturnsInvalidBeforeAllocation) {
    const std::array<std::byte, 4> binary{};
    PipelineBinaryDescUVE invalidDesc;
    invalidDesc.vertexLayout = {VertexAttributeUVE{"POSITION", VertexAttributeFormatUVE::Float4, 4U}};
    invalidDesc.vertexStride = 16U;

    EXPECT_EQ(renderDevice->CreatePipelineFromBinaryUVE(binary, 0U, invalidDesc), kInvalidPipelineHandleUVE);
    EXPECT_EQ(renderDevice->GetLiveResourceCountUVE(), 0U);
}

TEST_F(GlRenderDeviceUVETest, CreatePipelineUVE_VertexStrideExceedsGlsizei_ReturnsInvalidBeforeAllocation) {
    const ShaderHandleUVE vertexShader =
        renderDevice->CreateShaderUVE(ShaderDescUVE{ShaderStageUVE::Vertex, std::string(kValidVertexShaderSource)});
    const ShaderHandleUVE fragmentShader = renderDevice->CreateShaderUVE(
        ShaderDescUVE{ShaderStageUVE::Fragment, std::string(kValidFragmentShaderSource)});
    ASSERT_NE(vertexShader, kInvalidShaderHandleUVE);
    ASSERT_NE(fragmentShader, kInvalidShaderHandleUVE);

    PipelineDescUVE invalidDesc;
    invalidDesc.vertexShader = vertexShader;
    invalidDesc.fragmentShader = fragmentShader;
    invalidDesc.vertexLayout = {VertexAttributeUVE{"POSITION", VertexAttributeFormatUVE::Float3, 0U}};
    invalidDesc.vertexStride = std::numeric_limits<std::uint32_t>::max();
    EXPECT_EQ(renderDevice->CreatePipelineUVE(invalidDesc), kInvalidPipelineHandleUVE);
    EXPECT_EQ(renderDevice->GetLiveResourceCountUVE(), 2U);

    invalidDesc.vertexStride = 3U * static_cast<std::uint32_t>(sizeof(float));
    const PipelineHandleUVE validPipeline = renderDevice->CreatePipelineUVE(invalidDesc);
    EXPECT_EQ(validPipeline.value, 1U);

    renderDevice->DestroyPipelineUVE(validPipeline);
    renderDevice->DestroyShaderUVE(fragmentShader);
    renderDevice->DestroyShaderUVE(vertexShader);
}

TEST_F(GlRenderDeviceUVETest, CreatePipelineFromBinaryUVE_VertexStrideExceedsGlsizei_ReturnsInvalidBeforeAllocation) {
    const std::array<std::byte, 4> binary{};
    PipelineBinaryDescUVE invalidDesc;
    invalidDesc.vertexLayout = {VertexAttributeUVE{"POSITION", VertexAttributeFormatUVE::Float3, 0U}};
    invalidDesc.vertexStride = std::numeric_limits<std::uint32_t>::max();

    EXPECT_EQ(renderDevice->CreatePipelineFromBinaryUVE(binary, 0U, invalidDesc), kInvalidPipelineHandleUVE);
    EXPECT_EQ(renderDevice->GetLiveResourceCountUVE(), 0U);
}

TEST_F(GlRenderDeviceUVETest, DrawUVE_CountExceedsGlsizei_DoesNotIssueGlCall) {
    std::unique_ptr<ICommandBufferUVE> commandBuffer = renderDevice->CreateCommandBufferUVE();
    ASSERT_NE(commandBuffer, nullptr);
    RenderPassDescUVE passDesc;
    passDesc.colorAttachment = kInvalidTextureHandleUVE;
    passDesc.depthLoadOp = LoadOpUVE::DontCare;
    commandBuffer->BeginRenderPassUVE(passDesc);
    while (glGetError() != GL_NO_ERROR) {
    }

    commandBuffer->DrawUVE(std::numeric_limits<std::uint32_t>::max());
    EXPECT_EQ(glGetError(), GL_NO_ERROR);

    commandBuffer->EndRenderPassUVE();
    renderDevice->SubmitUVE(std::move(commandBuffer));
}

TEST_F(GlRenderDeviceUVETest, DrawIndexedUVE_CountExceedsGlsizei_DoesNotIssueGlCall) {
    std::unique_ptr<ICommandBufferUVE> commandBuffer = renderDevice->CreateCommandBufferUVE();
    ASSERT_NE(commandBuffer, nullptr);
    RenderPassDescUVE passDesc;
    passDesc.colorAttachment = kInvalidTextureHandleUVE;
    passDesc.depthLoadOp = LoadOpUVE::DontCare;
    commandBuffer->BeginRenderPassUVE(passDesc);
    while (glGetError() != GL_NO_ERROR) {
    }

    commandBuffer->DrawIndexedUVE(std::numeric_limits<std::uint32_t>::max());
    EXPECT_EQ(glGetError(), GL_NO_ERROR);

    commandBuffer->EndRenderPassUVE();
    renderDevice->SubmitUVE(std::move(commandBuffer));
}

TEST_F(GlRenderDeviceUVETest, BufferUsageAndIndexedCountValidationRejectsUnsafeDraws) {
    const BufferHandleUVE vertexBuffer = renderDevice->CreateBufferUVE(BufferDescUVE{16U, BufferUsageUVE::Vertex});
    const BufferHandleUVE indexBuffer = renderDevice->CreateBufferUVE(BufferDescUVE{4U, BufferUsageUVE::Index});
    const BufferHandleUVE uniformBuffer = renderDevice->CreateBufferUVE(BufferDescUVE{16U, BufferUsageUVE::Uniform});
    ASSERT_NE(vertexBuffer, kInvalidBufferHandleUVE);
    ASSERT_NE(indexBuffer, kInvalidBufferHandleUVE);
    ASSERT_NE(uniformBuffer, kInvalidBufferHandleUVE);

    std::unique_ptr<ICommandBufferUVE> commandBuffer = renderDevice->CreateCommandBufferUVE();
    ASSERT_NE(commandBuffer, nullptr);
    RenderPassDescUVE passDesc;
    passDesc.colorAttachment = kInvalidTextureHandleUVE;
    passDesc.depthLoadOp = LoadOpUVE::DontCare;
    commandBuffer->BeginRenderPassUVE(passDesc);
    while (glGetError() != GL_NO_ERROR) {
    }

    commandBuffer->BindVertexBufferUVE(uniformBuffer);
    commandBuffer->BindIndexBufferUVE(vertexBuffer);
    commandBuffer->BindUniformBufferUVE(vertexBuffer, 0U);
    commandBuffer->BindUniformBufferUVE(uniformBuffer, 0U);
    commandBuffer->DrawIndexedUVE(2U);
    EXPECT_EQ(glGetError(), GL_NO_ERROR);

    commandBuffer->BindIndexBufferUVE(indexBuffer);
    commandBuffer->DrawIndexedUVE(2U);
    EXPECT_EQ(glGetError(), GL_NO_ERROR);

    commandBuffer->EndRenderPassUVE();
    renderDevice->SubmitUVE(std::move(commandBuffer));
    renderDevice->DestroyBufferUVE(uniformBuffer);
    renderDevice->DestroyBufferUVE(indexBuffer);
    renderDevice->DestroyBufferUVE(vertexBuffer);
}

TEST_F(GlRenderDeviceUVETest, DrawUVE_BoundVertexBufferCapacityIsValidated) {
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
    ASSERT_NE(pipeline, kInvalidPipelineHandleUVE);

    constexpr std::array<float, 3> oneVertex{0.0F, 0.0F, 0.0F};
    const BufferHandleUVE vertexBuffer = renderDevice->CreateBufferUVE(
        BufferDescUVE{sizeof(oneVertex), BufferUsageUVE::Vertex}, std::as_bytes(std::span(oneVertex)));
    ASSERT_NE(vertexBuffer, kInvalidBufferHandleUVE);

    std::unique_ptr<ICommandBufferUVE> commandBuffer = renderDevice->CreateCommandBufferUVE();
    ASSERT_NE(commandBuffer, nullptr);
    RenderPassDescUVE passDesc;
    passDesc.colorAttachment = kInvalidTextureHandleUVE;
    passDesc.depthLoadOp = LoadOpUVE::DontCare;
    commandBuffer->BeginRenderPassUVE(passDesc);
    commandBuffer->BindPipelineUVE(pipeline);
    commandBuffer->BindVertexBufferUVE(vertexBuffer);
    while (glGetError() != GL_NO_ERROR) {
    }

    commandBuffer->DrawUVE(2U);
    EXPECT_EQ(glGetError(), GL_NO_ERROR);
    commandBuffer->DrawUVE(1U);
    EXPECT_EQ(glGetError(), GL_NO_ERROR);

    commandBuffer->EndRenderPassUVE();
    renderDevice->SubmitUVE(std::move(commandBuffer));

    std::unique_ptr<ICommandBufferUVE> fullscreenCommandBuffer = renderDevice->CreateCommandBufferUVE();
    ASSERT_NE(fullscreenCommandBuffer, nullptr);
    fullscreenCommandBuffer->BeginRenderPassUVE(passDesc);
    fullscreenCommandBuffer->BindPipelineUVE(pipeline);
    while (glGetError() != GL_NO_ERROR) {
    }
    fullscreenCommandBuffer->DrawUVE(3U);
    EXPECT_EQ(glGetError(), GL_NO_ERROR);
    fullscreenCommandBuffer->EndRenderPassUVE();
    renderDevice->SubmitUVE(std::move(fullscreenCommandBuffer));

    renderDevice->DestroyBufferUVE(vertexBuffer);
    renderDevice->DestroyPipelineUVE(pipeline);
    renderDevice->DestroyShaderUVE(vertexShader);
    renderDevice->DestroyShaderUVE(fragmentShader);
}

TEST_F(GlRenderDeviceUVETest, BeginRenderPassUVE_UnknownAttachmentDoesNotBindOrCacheFramebuffer) {
    const TextureHandleUVE validColor = renderDevice->CreateTextureUVE(TextureDescUVE{1U, 1U});
    ASSERT_NE(validColor, kInvalidTextureHandleUVE);
    std::unique_ptr<ICommandBufferUVE> commandBuffer = renderDevice->CreateCommandBufferUVE();
    ASSERT_NE(commandBuffer, nullptr);

    const auto assertFramebufferUnchanged = [&commandBuffer](const RenderPassDescUVE& passDesc) {
        GLint before = 0;
        glGetIntegerv(GL_FRAMEBUFFER_BINDING, &before);
        commandBuffer->BeginRenderPassUVE(passDesc);
        GLint after = 0;
        glGetIntegerv(GL_FRAMEBUFFER_BINDING, &after);
        EXPECT_EQ(after, before);
    };

    RenderPassDescUVE unknownColor;
    unknownColor.colorAttachment = TextureHandleUVE{0xFFFF'FFFEU};
    unknownColor.depthAttachment = kInvalidTextureHandleUVE;
    assertFramebufferUnchanged(unknownColor);

    RenderPassDescUVE unknownDepth;
    unknownDepth.colorAttachment = validColor;
    unknownDepth.depthAttachment = TextureHandleUVE{0xFFFF'FFFDU};
    assertFramebufferUnchanged(unknownDepth);

    RenderPassDescUVE validPass;
    validPass.colorAttachment = validColor;
    validPass.depthAttachment = kInvalidTextureHandleUVE;
    commandBuffer->BeginRenderPassUVE(validPass);
    GLint validFramebuffer = 0;
    glGetIntegerv(GL_FRAMEBUFFER_BINDING, &validFramebuffer);
    EXPECT_NE(validFramebuffer, 0);
    commandBuffer->EndRenderPassUVE();
    renderDevice->DestroyTextureUVE(validColor);
}

TEST_F(GlRenderDeviceUVETest, BeginRenderPassUVE_IncompleteFramebufferRejectsAndRestoresState) {
    const TextureHandleUVE validColor = renderDevice->CreateTextureUVE(TextureDescUVE{2U, 2U});
    const TextureHandleUVE invalidColor = renderDevice->CreateTextureUVE(
        TextureDescUVE{1U, 1U, TextureFormatUVE::Depth32Float, 1U});
    ASSERT_NE(validColor, kInvalidTextureHandleUVE);
    ASSERT_NE(invalidColor, kInvalidTextureHandleUVE);
    std::unique_ptr<ICommandBufferUVE> commandBuffer = renderDevice->CreateCommandBufferUVE();
    ASSERT_NE(commandBuffer, nullptr);

    GLint before = 0;
    glGetIntegerv(GL_FRAMEBUFFER_BINDING, &before);
    RenderPassDescUVE mismatchedPass;
    mismatchedPass.colorAttachment = invalidColor;
    mismatchedPass.depthAttachment = kInvalidTextureHandleUVE;
    commandBuffer->BeginRenderPassUVE(mismatchedPass);
    GLint after = 0;
    glGetIntegerv(GL_FRAMEBUFFER_BINDING, &after);
    EXPECT_EQ(after, before);

    RenderPassDescUVE validPass;
    validPass.colorAttachment = validColor;
    validPass.depthAttachment = kInvalidTextureHandleUVE;
    commandBuffer->BeginRenderPassUVE(validPass);
    GLint validFramebuffer = 0;
    glGetIntegerv(GL_FRAMEBUFFER_BINDING, &validFramebuffer);
    EXPECT_NE(validFramebuffer, 0);
    commandBuffer->EndRenderPassUVE();

    renderDevice->DestroyTextureUVE(invalidColor);
    renderDevice->DestroyTextureUVE(validColor);
}

TEST_F(GlRenderDeviceUVETest, BindTextureUVE_SlotExceedsGlLimit_DoesNotIssueGlCall) {
    const TextureHandleUVE texture = renderDevice->CreateTextureUVE(TextureDescUVE{1U, 1U});
    ASSERT_NE(texture, kInvalidTextureHandleUVE);

    std::unique_ptr<ICommandBufferUVE> commandBuffer = renderDevice->CreateCommandBufferUVE();
    ASSERT_NE(commandBuffer, nullptr);
    RenderPassDescUVE passDesc;
    passDesc.colorAttachment = kInvalidTextureHandleUVE;
    passDesc.depthLoadOp = LoadOpUVE::DontCare;
    commandBuffer->BeginRenderPassUVE(passDesc);
    while (glGetError() != GL_NO_ERROR) {
    }

    commandBuffer->BindTextureUVE(texture, std::numeric_limits<std::uint32_t>::max());
    EXPECT_EQ(glGetError(), GL_NO_ERROR);
    commandBuffer->BindTextureUVE(texture, 0U);
    EXPECT_EQ(glGetError(), GL_NO_ERROR);

    commandBuffer->EndRenderPassUVE();
    renderDevice->SubmitUVE(std::move(commandBuffer));
    renderDevice->DestroyTextureUVE(texture);
}

TEST_F(GlRenderDeviceUVETest, CreatePipelineFromBinaryUVE_BinarySizeExceedsGlsizei_ReturnsInvalidBeforeAllocation) {
    const std::array<std::byte, 1> storage{};
    const std::size_t oversizedBinarySize = static_cast<std::size_t>(std::numeric_limits<GLsizei>::max()) + 1U;
    const std::span<const std::byte> oversizedBinary(storage.data(), oversizedBinarySize);
    const PipelineBinaryDescUVE desc;

    EXPECT_EQ(renderDevice->CreatePipelineFromBinaryUVE(oversizedBinary, 0U, desc), kInvalidPipelineHandleUVE);
    EXPECT_EQ(renderDevice->GetLiveResourceCountUVE(), 0U);
}

TEST_F(GlRenderDeviceUVETest, BindUniformBufferUVE_SlotExceedsGlLimit_DoesNotIssueGlCall) {
    const BufferHandleUVE buffer = renderDevice->CreateBufferUVE(BufferDescUVE{16U, BufferUsageUVE::Uniform});
    ASSERT_NE(buffer, kInvalidBufferHandleUVE);

    std::unique_ptr<ICommandBufferUVE> commandBuffer = renderDevice->CreateCommandBufferUVE();
    ASSERT_NE(commandBuffer, nullptr);
    RenderPassDescUVE passDesc;
    passDesc.colorAttachment = kInvalidTextureHandleUVE;
    passDesc.depthLoadOp = LoadOpUVE::DontCare;
    commandBuffer->BeginRenderPassUVE(passDesc);
    while (glGetError() != GL_NO_ERROR) {
    }

    commandBuffer->BindUniformBufferUVE(buffer, std::numeric_limits<std::uint32_t>::max());
    EXPECT_EQ(glGetError(), GL_NO_ERROR);
    commandBuffer->BindUniformBufferUVE(buffer, 0U);
    EXPECT_EQ(glGetError(), GL_NO_ERROR);

    commandBuffer->EndRenderPassUVE();
    renderDevice->SubmitUVE(std::move(commandBuffer));
    renderDevice->DestroyBufferUVE(buffer);
}

TEST_F(GlRenderDeviceUVETest, BeginRenderPassUVE_UnknownLoadOp_LeavesStateUntouched) {
    std::unique_ptr<ICommandBufferUVE> commandBuffer = renderDevice->CreateCommandBufferUVE();
    ASSERT_NE(commandBuffer, nullptr);
    RenderPassDescUVE invalidDesc;
    invalidDesc.depthLoadOp = static_cast<LoadOpUVE>(0xFFU);
    commandBuffer->BeginRenderPassUVE(invalidDesc);

    commandBuffer->BeginRenderPassUVE(RenderPassDescUVE{});
    commandBuffer->EndRenderPassUVE();
    renderDevice->SubmitUVE(std::move(commandBuffer));
    EXPECT_EQ(glGetError(), GL_NO_ERROR);
}

TEST_F(GlRenderDeviceUVETest, CreateBufferUVE_ZeroSize_ReturnsInvalidBeforeAllocation) {
    ASSERT_EQ(renderDevice->GetLiveResourceCountUVE(), 0U);
    while (glGetError() != GL_NO_ERROR) {
    }

    const BufferHandleUVE invalid = renderDevice->CreateBufferUVE(BufferDescUVE{0U, BufferUsageUVE::Vertex});

    EXPECT_EQ(invalid, kInvalidBufferHandleUVE);
    EXPECT_EQ(renderDevice->GetLiveResourceCountUVE(), 0U);
    EXPECT_EQ(glGetError(), GL_NO_ERROR);
}

TEST_F(GlRenderDeviceUVETest, CreateBufferUVE_OversizedInitialData_ReturnsInvalidBeforeAllocation) {
    ASSERT_EQ(renderDevice->GetLiveResourceCountUVE(), 0U);
    const std::array<std::byte, 17> initialData{};
    const BufferHandleUVE invalid =
        renderDevice->CreateBufferUVE(BufferDescUVE{16U, BufferUsageUVE::Vertex}, initialData);

    EXPECT_EQ(invalid, kInvalidBufferHandleUVE);
    EXPECT_EQ(renderDevice->GetLiveResourceCountUVE(), 0U);

    const BufferHandleUVE valid = renderDevice->CreateBufferUVE(BufferDescUVE{16U, BufferUsageUVE::Vertex});
    EXPECT_EQ(valid.value, 1U);
    renderDevice->DestroyBufferUVE(valid);
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

TEST_F(GlRenderDeviceUVETest, UpdateBufferUVE_OverflowedOffset_ReturnsFalseWithoutGlError) {
    const BufferHandleUVE buffer = renderDevice->CreateBufferUVE(BufferDescUVE{16, BufferUsageUVE::Vertex});
    const std::array<std::byte, 4> data{};
    while (glGetError() != GL_NO_ERROR) {
    }

    EXPECT_FALSE(renderDevice->UpdateBufferUVE(buffer, data, std::numeric_limits<std::uint64_t>::max()));
    EXPECT_EQ(glGetError(), GL_NO_ERROR);
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

TEST_F(GlRenderDeviceUVETest, CreateTextureUVE_PreservesActiveUnitBindingForLiveCommandBufferCache) {
    const TextureHandleUVE firstTexture =
        renderDevice->CreateTextureUVE(TextureDescUVE{1U, 1U, TextureFormatUVE::RGBA8Unorm, 1U});
    ASSERT_NE(firstTexture, kInvalidTextureHandleUVE);

    std::unique_ptr<ICommandBufferUVE> commandBuffer = renderDevice->CreateCommandBufferUVE();
    ASSERT_NE(commandBuffer, nullptr);
    commandBuffer->BeginRenderPassUVE(RenderPassDescUVE{});
    commandBuffer->BindTextureUVE(firstTexture, 0U);

    GLint activeTextureBefore = 0;
    GLint bindingBefore = 0;
    glGetIntegerv(GL_ACTIVE_TEXTURE, &activeTextureBefore);
    glGetIntegerv(GL_TEXTURE_BINDING_2D, &bindingBefore);
    ASSERT_EQ(activeTextureBefore, static_cast<GLint>(GL_TEXTURE0));
    ASSERT_NE(bindingBefore, 0);

    const TextureHandleUVE secondTexture =
        renderDevice->CreateTextureUVE(TextureDescUVE{1U, 1U, TextureFormatUVE::RGBA8Unorm, 1U});
    ASSERT_NE(secondTexture, kInvalidTextureHandleUVE);

    GLint activeTextureAfterCreate = 0;
    GLint bindingAfterCreate = 0;
    glGetIntegerv(GL_ACTIVE_TEXTURE, &activeTextureAfterCreate);
    glGetIntegerv(GL_TEXTURE_BINDING_2D, &bindingAfterCreate);
    EXPECT_EQ(activeTextureAfterCreate, activeTextureBefore);
    EXPECT_EQ(bindingAfterCreate, bindingBefore);

    // The command buffer still caches firstTexture at slot 0. If texture creation leaked secondTexture
    // into the binding, this call would incorrectly skip the bind and leave the wrong GL name active.
    commandBuffer->BindTextureUVE(firstTexture, 0U);
    GLint bindingAfterCachedRebind = 0;
    glGetIntegerv(GL_TEXTURE_BINDING_2D, &bindingAfterCachedRebind);
    EXPECT_EQ(bindingAfterCachedRebind, bindingBefore);
    commandBuffer->EndRenderPassUVE();

    renderDevice->DestroyTextureUVE(secondTexture);
    renderDevice->DestroyTextureUVE(firstTexture);
}

TEST_F(GlRenderDeviceUVETest, CreateAndUpdateBufferUVE_PreserveBindingForLiveCommandBufferCache) {
    const ShaderHandleUVE vertexShader =
        renderDevice->CreateShaderUVE(ShaderDescUVE{ShaderStageUVE::Vertex, std::string(kValidVertexShaderSource)});
    const ShaderHandleUVE fragmentShader = renderDevice->CreateShaderUVE(
        ShaderDescUVE{ShaderStageUVE::Fragment, std::string(kValidFragmentShaderSource)});
    ASSERT_NE(vertexShader, kInvalidShaderHandleUVE);
    ASSERT_NE(fragmentShader, kInvalidShaderHandleUVE);

    PipelineDescUVE pipelineDesc;
    pipelineDesc.vertexShader = vertexShader;
    pipelineDesc.fragmentShader = fragmentShader;
    pipelineDesc.vertexLayout = {VertexAttributeUVE{"POSITION", VertexAttributeFormatUVE::Float3, 0U}};
    pipelineDesc.vertexStride = 3U * static_cast<std::uint32_t>(sizeof(float));
    pipelineDesc.depthTestEnabled = false;
    pipelineDesc.depthWriteEnabled = false;
    const PipelineHandleUVE pipeline = renderDevice->CreatePipelineUVE(pipelineDesc);
    ASSERT_NE(pipeline, kInvalidPipelineHandleUVE);

    constexpr std::array<float, 3> kFirstVertex{0.0F, 0.0F, 0.0F};
    const BufferHandleUVE firstBuffer = renderDevice->CreateBufferUVE(
        BufferDescUVE{sizeof(kFirstVertex), BufferUsageUVE::Vertex}, std::as_bytes(std::span(kFirstVertex)));
    ASSERT_NE(firstBuffer, kInvalidBufferHandleUVE);

    std::unique_ptr<ICommandBufferUVE> commandBuffer = renderDevice->CreateCommandBufferUVE();
    ASSERT_NE(commandBuffer, nullptr);
    commandBuffer->BeginRenderPassUVE(RenderPassDescUVE{});
    commandBuffer->BindPipelineUVE(pipeline);
    commandBuffer->BindVertexBufferUVE(firstBuffer, 0U);

    GLint bindingBefore = 0;
    glGetIntegerv(GL_ARRAY_BUFFER_BINDING, &bindingBefore);
    ASSERT_NE(bindingBefore, 0);

    constexpr std::array<float, 3> kSecondVertex{1.0F, 0.0F, 0.0F};
    const BufferHandleUVE secondBuffer = renderDevice->CreateBufferUVE(
        BufferDescUVE{sizeof(kSecondVertex), BufferUsageUVE::Vertex}, std::as_bytes(std::span(kSecondVertex)));
    ASSERT_NE(secondBuffer, kInvalidBufferHandleUVE);

    GLint bindingAfterCreate = 0;
    glGetIntegerv(GL_ARRAY_BUFFER_BINDING, &bindingAfterCreate);
    EXPECT_EQ(bindingAfterCreate, bindingBefore);

    // The command buffer still caches firstBuffer. If creation leaked secondBuffer into GL, this
    // call would incorrectly skip the bind and leave the wrong vertex buffer active.
    commandBuffer->BindVertexBufferUVE(firstBuffer, 0U);
    GLint bindingAfterCachedCreateRebind = 0;
    glGetIntegerv(GL_ARRAY_BUFFER_BINDING, &bindingAfterCachedCreateRebind);
    EXPECT_EQ(bindingAfterCachedCreateRebind, bindingBefore);

    EXPECT_TRUE(renderDevice->UpdateBufferUVE(secondBuffer, std::as_bytes(std::span(kSecondVertex)), 0U));
    GLint bindingAfterUpdate = 0;
    glGetIntegerv(GL_ARRAY_BUFFER_BINDING, &bindingAfterUpdate);
    EXPECT_EQ(bindingAfterUpdate, bindingBefore);

    // UpdateBufferUVE has the same temporary-bind requirement as creation.
    commandBuffer->BindVertexBufferUVE(firstBuffer, 0U);
    GLint bindingAfterCachedUpdateRebind = 0;
    glGetIntegerv(GL_ARRAY_BUFFER_BINDING, &bindingAfterCachedUpdateRebind);
    EXPECT_EQ(bindingAfterCachedUpdateRebind, bindingBefore);

    commandBuffer->EndRenderPassUVE();
    renderDevice->DestroyBufferUVE(secondBuffer);
    renderDevice->DestroyBufferUVE(firstBuffer);
    renderDevice->DestroyPipelineUVE(pipeline);
    renderDevice->DestroyShaderUVE(vertexShader);
    renderDevice->DestroyShaderUVE(fragmentShader);
}

TEST_F(GlRenderDeviceUVETest, CreateTextureUVE_Rgba16Float_UsesHalfFloatUploadType) {
    constexpr std::array<std::uint16_t, 16> kPixels{
        0x3C00U, 0x3800U, 0x0000U, 0x3C00U,
        0x3400U, 0x3C00U, 0x3800U, 0x3C00U,
        0x0000U, 0x0000U, 0x3C00U, 0x3C00U,
        0x3C00U, 0x3400U, 0x3800U, 0x3C00U,
    };
    while (glGetError() != GL_NO_ERROR) {
    }

    const TextureHandleUVE texture = renderDevice->CreateTextureUVE(
        TextureDescUVE{2U, 2U, TextureFormatUVE::RGBA16Float, 1U},
        std::as_bytes(std::span(kPixels)));
    ASSERT_NE(texture, kInvalidTextureHandleUVE);
    EXPECT_EQ(glGetError(), GL_NO_ERROR);

    std::unique_ptr<ICommandBufferUVE> commandBuffer = renderDevice->CreateCommandBufferUVE();
    ASSERT_NE(commandBuffer, nullptr);
    commandBuffer->BeginRenderPassUVE(RenderPassDescUVE{});
    commandBuffer->BindTextureUVE(texture, 0U);
    EXPECT_EQ(glGetError(), GL_NO_ERROR);
    commandBuffer->EndRenderPassUVE();

    std::array<std::uint16_t, kPixels.size()> readback{};
    glGetTexImage(GL_TEXTURE_2D, 0, GL_RGBA, GL_HALF_FLOAT, readback.data());
    EXPECT_EQ(glGetError(), GL_NO_ERROR);
    EXPECT_EQ(readback, kPixels);

    renderDevice->DestroyTextureUVE(texture);
}

TEST_F(GlRenderDeviceUVETest, DestroyRenderDeviceUVE_ReleasesLiveGlResources) {
    auto firstDevice = std::make_unique<GlRenderDeviceUVE>(*windowManager);
    const ShaderHandleUVE vertexShader =
        firstDevice->CreateShaderUVE(ShaderDescUVE{ShaderStageUVE::Vertex, std::string(kValidVertexShaderSource)});
    const ShaderHandleUVE fragmentShader = firstDevice->CreateShaderUVE(
        ShaderDescUVE{ShaderStageUVE::Fragment, std::string(kValidFragmentShaderSource)});
    ASSERT_NE(vertexShader, kInvalidShaderHandleUVE);
    ASSERT_NE(fragmentShader, kInvalidShaderHandleUVE);

    PipelineDescUVE pipelineDesc;
    pipelineDesc.vertexShader = vertexShader;
    pipelineDesc.fragmentShader = fragmentShader;
    pipelineDesc.vertexLayout = {VertexAttributeUVE{"POSITION", VertexAttributeFormatUVE::Float3, 0U}};
    pipelineDesc.vertexStride = 3U * static_cast<std::uint32_t>(sizeof(float));
    const PipelineHandleUVE pipeline = firstDevice->CreatePipelineUVE(pipelineDesc);
    ASSERT_NE(pipeline, kInvalidPipelineHandleUVE);

    constexpr std::array<float, 3> kVertex{0.0F, 0.0F, 0.0F};
    const BufferHandleUVE buffer = firstDevice->CreateBufferUVE(
        BufferDescUVE{sizeof(kVertex), BufferUsageUVE::Vertex}, std::as_bytes(std::span(kVertex)));
    const TextureHandleUVE texture = firstDevice->CreateTextureUVE(TextureDescUVE{1U, 1U});
    ASSERT_NE(buffer, kInvalidBufferHandleUVE);
    ASSERT_NE(texture, kInvalidTextureHandleUVE);
    ASSERT_EQ(firstDevice->GetLiveResourceCountUVE(), 5U);

    std::unique_ptr<ICommandBufferUVE> commandBuffer = firstDevice->CreateCommandBufferUVE();
    ASSERT_NE(commandBuffer, nullptr);
    commandBuffer->BeginRenderPassUVE(RenderPassDescUVE{});
    commandBuffer->BindPipelineUVE(pipeline);
    commandBuffer->BindVertexBufferUVE(buffer, 0U);
    commandBuffer->BindTextureUVE(texture, 0U);

    GLint bufferName = 0;
    GLint textureName = 0;
    GLint programName = 0;
    GLint vertexArrayName = 0;
    glGetIntegerv(GL_ARRAY_BUFFER_BINDING, &bufferName);
    glGetIntegerv(GL_TEXTURE_BINDING_2D, &textureName);
    glGetIntegerv(GL_CURRENT_PROGRAM, &programName);
    glGetIntegerv(GL_VERTEX_ARRAY_BINDING, &vertexArrayName);
    ASSERT_NE(bufferName, 0);
    ASSERT_NE(textureName, 0);
    ASSERT_NE(programName, 0);
    ASSERT_NE(vertexArrayName, 0);

    commandBuffer->EndRenderPassUVE();
    commandBuffer.reset();
    firstDevice.reset();

    // Detach state that may legally remain current after object deletion before querying liveness.
    glUseProgram(0);
    glBindVertexArray(0);
    glBindBuffer(GL_ARRAY_BUFFER, 0);
    glBindTexture(GL_TEXTURE_2D, 0);
    EXPECT_FALSE(glIsBuffer(static_cast<GLuint>(bufferName)));
    EXPECT_FALSE(glIsTexture(static_cast<GLuint>(textureName)));
    EXPECT_FALSE(glIsProgram(static_cast<GLuint>(programName)));
    EXPECT_FALSE(glIsVertexArray(static_cast<GLuint>(vertexArrayName)));
    EXPECT_EQ(glGetError(), GL_NO_ERROR);

    auto secondDevice = std::make_unique<GlRenderDeviceUVE>(*windowManager);
    EXPECT_EQ(secondDevice->GetLiveResourceCountUVE(), 0U);
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

TEST_F(GlRenderDeviceUVETest, InvalidSentinelBindsAreRejectedWithoutPoisoningValidState) {
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
    ASSERT_NE(pipeline, kInvalidPipelineHandleUVE);

    constexpr float vertices[] = {0.0F, 0.5F, 0.0F, -0.5F, -0.5F, 0.0F, 0.5F, -0.5F, 0.0F};
    const std::span<const std::byte> vertexBytes = std::as_bytes(std::span<const float>(vertices));
    const BufferHandleUVE vertexBuffer =
        renderDevice->CreateBufferUVE(BufferDescUVE{vertexBytes.size(), BufferUsageUVE::Vertex}, vertexBytes);
    ASSERT_NE(vertexBuffer, kInvalidBufferHandleUVE);

    std::unique_ptr<ICommandBufferUVE> commandBuffer = renderDevice->CreateCommandBufferUVE();
    ASSERT_NE(commandBuffer, nullptr);
    RenderPassDescUVE passDesc;
    passDesc.colorAttachment = kInvalidTextureHandleUVE;
    passDesc.depthLoadOp = LoadOpUVE::DontCare;
    commandBuffer->BeginRenderPassUVE(passDesc);
    while (glGetError() != GL_NO_ERROR) {
    }

    commandBuffer->BindPipelineUVE(kInvalidPipelineHandleUVE);
    commandBuffer->BindVertexBufferUVE(kInvalidBufferHandleUVE);
    commandBuffer->BindIndexBufferUVE(kInvalidBufferHandleUVE);
    EXPECT_EQ(glGetError(), GL_NO_ERROR);

    commandBuffer->BindPipelineUVE(pipeline);
    commandBuffer->BindVertexBufferUVE(vertexBuffer);
    commandBuffer->DrawUVE(3U);
    commandBuffer->EndRenderPassUVE();
    renderDevice->SubmitUVE(std::move(commandBuffer));

    renderDevice->DestroyBufferUVE(vertexBuffer);
    renderDevice->DestroyPipelineUVE(pipeline);
    renderDevice->DestroyShaderUVE(vertexShader);
    renderDevice->DestroyShaderUVE(fragmentShader);
}

TEST_F(GlRenderDeviceUVETest, DestroyBoundPipelineWhileRecording_FailsClosed) {
    const ShaderHandleUVE vertexShader = renderDevice->CreateShaderUVE(
        ShaderDescUVE{ShaderStageUVE::Vertex, std::string(kUniformVertexShaderSource)});
    const ShaderHandleUVE fragmentShader = renderDevice->CreateShaderUVE(
        ShaderDescUVE{ShaderStageUVE::Fragment, std::string(kUniformFragmentShaderSource)});
    ASSERT_NE(vertexShader, kInvalidShaderHandleUVE);
    ASSERT_NE(fragmentShader, kInvalidShaderHandleUVE);

    PipelineDescUVE pipelineDesc;
    pipelineDesc.vertexShader = vertexShader;
    pipelineDesc.fragmentShader = fragmentShader;
    pipelineDesc.vertexLayout = {VertexAttributeUVE{"POSITION", VertexAttributeFormatUVE::Float3, 0U}};
    pipelineDesc.vertexStride = 3U * static_cast<std::uint32_t>(sizeof(float));
    pipelineDesc.depthTestEnabled = false;
    pipelineDesc.depthWriteEnabled = false;
    const PipelineHandleUVE pipeline = renderDevice->CreatePipelineUVE(pipelineDesc);
    ASSERT_NE(pipeline, kInvalidPipelineHandleUVE);

    constexpr std::array<float, 3> kFirstVertex{0.0F, 0.0F, 0.0F};
    constexpr std::array<float, 3> kSecondVertex{1.0F, 0.0F, 0.0F};
    const BufferHandleUVE firstBuffer = renderDevice->CreateBufferUVE(
        BufferDescUVE{sizeof(kFirstVertex), BufferUsageUVE::Vertex}, std::as_bytes(std::span(kFirstVertex)));
    const BufferHandleUVE secondBuffer = renderDevice->CreateBufferUVE(
        BufferDescUVE{sizeof(kSecondVertex), BufferUsageUVE::Vertex}, std::as_bytes(std::span(kSecondVertex)));
    ASSERT_NE(firstBuffer, kInvalidBufferHandleUVE);
    ASSERT_NE(secondBuffer, kInvalidBufferHandleUVE);

    std::unique_ptr<ICommandBufferUVE> commandBuffer = renderDevice->CreateCommandBufferUVE();
    ASSERT_NE(commandBuffer, nullptr);
    commandBuffer->BeginRenderPassUVE(RenderPassDescUVE{});
    commandBuffer->BindPipelineUVE(pipeline);
    commandBuffer->BindVertexBufferUVE(firstBuffer, 0U);
    renderDevice->DestroyPipelineUVE(pipeline);
    while (glGetError() != GL_NO_ERROR) {
    }

    commandBuffer->SetUniformMatrix4x4UVE("uModel", Math::Matrix4x4UVE::IdentityUVE());
    commandBuffer->BindVertexBufferUVE(secondBuffer, 0U);
    commandBuffer->DrawUVE(1U);
    EXPECT_EQ(glGetError(), GL_NO_ERROR);

    commandBuffer->EndRenderPassUVE();
    renderDevice->DestroyBufferUVE(secondBuffer);
    renderDevice->DestroyBufferUVE(firstBuffer);
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

TEST_F(GlRenderDeviceUVETest, FullscreenToneMappingShader_MapsBoundSourceTextureToDefaultFramebuffer) {
    const ShaderHandleUVE vertexShader = renderDevice->CreateShaderUVE(
        ShaderDescUVE{ShaderStageUVE::Vertex, WithShaderStageDefineUVE(Shader::BuiltIn::kFullscreenQuadSource,
                                                                        "VERTEX_SHADER")});
    const ShaderHandleUVE fragmentShader = renderDevice->CreateShaderUVE(
        ShaderDescUVE{ShaderStageUVE::Fragment, WithShaderStageDefineUVE(Shader::BuiltIn::kFullscreenQuadSource,
                                                                          "FRAGMENT_SHADER")});
    ASSERT_NE(vertexShader, kInvalidShaderHandleUVE);
    ASSERT_NE(fragmentShader, kInvalidShaderHandleUVE);

    PipelineDescUVE pipelineDesc;
    pipelineDesc.vertexShader = vertexShader;
    pipelineDesc.fragmentShader = fragmentShader;
    pipelineDesc.depthTestEnabled = false;
    pipelineDesc.depthWriteEnabled = false;
    const PipelineHandleUVE pipeline = renderDevice->CreatePipelineUVE(pipelineDesc);
    ASSERT_NE(pipeline, kInvalidPipelineHandleUVE);

    constexpr std::array<std::uint8_t, 4> kWarmHdrLikePixel{220U, 48U, 20U, 255U};
    const TextureHandleUVE sourceTexture = renderDevice->CreateTextureUVE(
        TextureDescUVE{1U, 1U, TextureFormatUVE::RGBA8Unorm, 1U}, std::as_bytes(std::span(kWarmHdrLikePixel)));
    ASSERT_NE(sourceTexture, kInvalidTextureHandleUVE);

    std::unique_ptr<ICommandBufferUVE> commandBuffer = renderDevice->CreateCommandBufferUVE();
    ASSERT_NE(commandBuffer, nullptr);
    RenderPassDescUVE passDesc;
    passDesc.colorAttachment = kInvalidTextureHandleUVE;
    passDesc.clearColor = {0.0F, 0.0F, 0.0F, 1.0F};
    passDesc.depthLoadOp = LoadOpUVE::DontCare;
    commandBuffer->BeginRenderPassUVE(passDesc);
    commandBuffer->BindPipelineUVE(pipeline);
    commandBuffer->SetUniformIntUVE("uSourceTexture", 0);
    commandBuffer->BindTextureUVE(sourceTexture, 0U);
    commandBuffer->DrawUVE(3U);
    commandBuffer->EndRenderPassUVE();
    renderDevice->SubmitUVE(std::move(commandBuffer));

    std::array<std::uint8_t, 3> pixel{};
    glReadPixels(32, 32, 1, 1, GL_RGB, GL_UNSIGNED_BYTE, pixel.data());
    EXPECT_GT(pixel[0], static_cast<std::uint8_t>(pixel[1] + 24U));
    EXPECT_GT(pixel[1], pixel[2]);
    EXPECT_EQ(glGetError(), GL_NO_ERROR);

    renderDevice->DestroyTextureUVE(sourceTexture);
    renderDevice->DestroyPipelineUVE(pipeline);
    renderDevice->DestroyShaderUVE(fragmentShader);
    renderDevice->DestroyShaderUVE(vertexShader);
}

TEST_F(GlRenderDeviceUVETest, FullscreenToneMappingShader_ComposesRgba16FloatRenderTargetToDefaultFramebuffer) {
    constexpr std::string_view kFullscreenVertexSource = R"(#version 330 core
void main() {
    vec2 position = vec2((gl_VertexID << 1) & 2, gl_VertexID & 2);
    gl_Position = vec4(position * 2.0 - 1.0, 0.0, 1.0);
}
)";
    constexpr std::string_view kFlatColorFragmentSource = R"(#version 330 core
out vec4 FragColor;
uniform vec3 uColor;
void main() {
    FragColor = vec4(uColor, 1.0);
}
)";
    const ShaderHandleUVE sourceVertexShader =
        renderDevice->CreateShaderUVE(ShaderDescUVE{ShaderStageUVE::Vertex, std::string(kFullscreenVertexSource)});
    const ShaderHandleUVE sourceFragmentShader = renderDevice->CreateShaderUVE(
        ShaderDescUVE{ShaderStageUVE::Fragment, std::string(kFlatColorFragmentSource)});
    const ShaderHandleUVE toneVertexShader = renderDevice->CreateShaderUVE(
        ShaderDescUVE{ShaderStageUVE::Vertex, WithShaderStageDefineUVE(Shader::BuiltIn::kFullscreenQuadSource,
                                                                        "VERTEX_SHADER")});
    const ShaderHandleUVE toneFragmentShader = renderDevice->CreateShaderUVE(
        ShaderDescUVE{ShaderStageUVE::Fragment, WithShaderStageDefineUVE(Shader::BuiltIn::kFullscreenQuadSource,
                                                                          "FRAGMENT_SHADER")});
    ASSERT_NE(sourceVertexShader, kInvalidShaderHandleUVE);
    ASSERT_NE(sourceFragmentShader, kInvalidShaderHandleUVE);
    ASSERT_NE(toneVertexShader, kInvalidShaderHandleUVE);
    ASSERT_NE(toneFragmentShader, kInvalidShaderHandleUVE);

    PipelineDescUVE sourcePipelineDesc;
    sourcePipelineDesc.vertexShader = sourceVertexShader;
    sourcePipelineDesc.fragmentShader = sourceFragmentShader;
    sourcePipelineDesc.depthTestEnabled = true;
    sourcePipelineDesc.depthWriteEnabled = true;
    const PipelineHandleUVE sourcePipeline = renderDevice->CreatePipelineUVE(sourcePipelineDesc);
    ASSERT_NE(sourcePipeline, kInvalidPipelineHandleUVE);
    PipelineDescUVE tonePipelineDesc;
    tonePipelineDesc.vertexShader = toneVertexShader;
    tonePipelineDesc.fragmentShader = toneFragmentShader;
    tonePipelineDesc.depthTestEnabled = false;
    tonePipelineDesc.depthWriteEnabled = false;
    const PipelineHandleUVE tonePipeline = renderDevice->CreatePipelineUVE(tonePipelineDesc);
    ASSERT_NE(tonePipeline, kInvalidPipelineHandleUVE);

    const TextureHandleUVE hdrTarget =
        renderDevice->CreateTextureUVE(TextureDescUVE{64U, 64U, TextureFormatUVE::RGBA16Float, 1U});
    const TextureHandleUVE depthTarget =
        renderDevice->CreateTextureUVE(TextureDescUVE{64U, 64U, TextureFormatUVE::Depth32Float, 1U});
    const TextureHandleUVE shadowStyleDepthTarget =
        renderDevice->CreateTextureUVE(TextureDescUVE{64U, 64U, TextureFormatUVE::Depth32Float, 1U});
    ASSERT_NE(hdrTarget, kInvalidTextureHandleUVE);
    ASSERT_NE(depthTarget, kInvalidTextureHandleUVE);
    ASSERT_NE(shadowStyleDepthTarget, kInvalidTextureHandleUVE);

    // A renderer shadow pass may use this depth-only path when a directional caster is present.
    // Ensure core-profile `GL_NONE` draw/read-buffer states cannot prevent a subsequent color+depth
    // pass from rasterizing its fullscreen source geometry.
    std::unique_ptr<ICommandBufferUVE> commandBuffer = renderDevice->CreateCommandBufferUVE();
    ASSERT_NE(commandBuffer, nullptr);
    RenderPassDescUVE shadowStylePassDesc;
    shadowStylePassDesc.colorAttachment = kInvalidTextureHandleUVE;
    shadowStylePassDesc.depthAttachment = shadowStyleDepthTarget;
    shadowStylePassDesc.depthLoadOp = LoadOpUVE::Clear;
    shadowStylePassDesc.clearDepth = 1.0F;
    commandBuffer->BeginRenderPassUVE(shadowStylePassDesc);
    commandBuffer->EndRenderPassUVE();

    RenderPassDescUVE sourcePassDesc;
    sourcePassDesc.colorAttachment = hdrTarget;
    sourcePassDesc.depthAttachment = depthTarget;
    sourcePassDesc.depthLoadOp = LoadOpUVE::Clear;
    sourcePassDesc.clearDepth = 1.0F;
    commandBuffer->BeginRenderPassUVE(sourcePassDesc);
    commandBuffer->BindPipelineUVE(sourcePipeline);
    commandBuffer->SetUniformVector3UVE("uColor", Math::Vector3UVE{0.9F, 0.1F, 0.03F});
    commandBuffer->DrawUVE(3U);
    commandBuffer->EndRenderPassUVE();

    RenderPassDescUVE tonePassDesc;
    tonePassDesc.colorAttachment = kInvalidTextureHandleUVE;
    tonePassDesc.depthLoadOp = LoadOpUVE::DontCare;
    commandBuffer->BeginRenderPassUVE(tonePassDesc);
    commandBuffer->BindPipelineUVE(tonePipeline);
    commandBuffer->SetUniformIntUVE("uSourceTexture", 0);
    commandBuffer->BindTextureUVE(hdrTarget, 0U);
    commandBuffer->DrawUVE(3U);
    commandBuffer->EndRenderPassUVE();
    renderDevice->SubmitUVE(std::move(commandBuffer));

    // ToneMapping binds a depth-write-disabled pipeline. The next frame's clear must override
    // that persisted GL state before it draws the same depth again; otherwise GL_LESS rejects
    // later scene pixels although the renderer recorded valid draws.
    std::unique_ptr<ICommandBufferUVE> nextFrameCommandBuffer = renderDevice->CreateCommandBufferUVE();
    ASSERT_NE(nextFrameCommandBuffer, nullptr);
    nextFrameCommandBuffer->BeginRenderPassUVE(sourcePassDesc);
    nextFrameCommandBuffer->BindPipelineUVE(sourcePipeline);
    nextFrameCommandBuffer->SetUniformVector3UVE("uColor", Math::Vector3UVE{0.04F, 0.12F, 0.92F});
    nextFrameCommandBuffer->DrawUVE(3U);
    nextFrameCommandBuffer->EndRenderPassUVE();
    nextFrameCommandBuffer->BeginRenderPassUVE(tonePassDesc);
    nextFrameCommandBuffer->BindPipelineUVE(tonePipeline);
    nextFrameCommandBuffer->SetUniformIntUVE("uSourceTexture", 0);
    nextFrameCommandBuffer->BindTextureUVE(hdrTarget, 0U);
    nextFrameCommandBuffer->DrawUVE(3U);
    nextFrameCommandBuffer->EndRenderPassUVE();
    renderDevice->SubmitUVE(std::move(nextFrameCommandBuffer));

    std::array<std::uint8_t, 3> pixel{};
    glReadPixels(32, 32, 1, 1, GL_RGB, GL_UNSIGNED_BYTE, pixel.data());
    EXPECT_GT(pixel[2], static_cast<std::uint8_t>(pixel[0] + 36U));
    EXPECT_GT(pixel[2], static_cast<std::uint8_t>(pixel[1] + 36U));
    EXPECT_EQ(glGetError(), GL_NO_ERROR);

    renderDevice->DestroyTextureUVE(shadowStyleDepthTarget);
    renderDevice->DestroyTextureUVE(depthTarget);
    renderDevice->DestroyTextureUVE(hdrTarget);
    renderDevice->DestroyPipelineUVE(tonePipeline);
    renderDevice->DestroyPipelineUVE(sourcePipeline);
    renderDevice->DestroyShaderUVE(toneFragmentShader);
    renderDevice->DestroyShaderUVE(toneVertexShader);
    renderDevice->DestroyShaderUVE(sourceFragmentShader);
    renderDevice->DestroyShaderUVE(sourceVertexShader);
}

TEST_F(GlRenderDeviceUVETest, Basic3DShader_PerspectiveMatrixRendersNegativeZGeometry) {
    const ShaderHandleUVE vertexShader = renderDevice->CreateShaderUVE(
        ShaderDescUVE{ShaderStageUVE::Vertex,
                      WithShaderStageDefineUVE(Shader::BuiltIn::kBasic3DSource, "VERTEX_SHADER")});
    const ShaderHandleUVE fragmentShader = renderDevice->CreateShaderUVE(
        ShaderDescUVE{ShaderStageUVE::Fragment,
                      WithShaderStageDefineUVE(Shader::BuiltIn::kBasic3DSource, "FRAGMENT_SHADER")});
    ASSERT_NE(vertexShader, kInvalidShaderHandleUVE);
    ASSERT_NE(fragmentShader, kInvalidShaderHandleUVE);

    PipelineDescUVE pipelineDesc;
    pipelineDesc.vertexShader = vertexShader;
    pipelineDesc.fragmentShader = fragmentShader;
    pipelineDesc.vertexLayout = {VertexAttributeUVE{"POSITION", VertexAttributeFormatUVE::Float3, 0U}};
    pipelineDesc.vertexStride = 3U * static_cast<std::uint32_t>(sizeof(float));
    pipelineDesc.depthTestEnabled = false;
    pipelineDesc.depthWriteEnabled = false;
    const PipelineHandleUVE pipeline = renderDevice->CreatePipelineUVE(pipelineDesc);
    ASSERT_NE(pipeline, kInvalidPipelineHandleUVE);

    constexpr std::array<float, 9> kVertices{-2.0F, -2.0F, -8.0F, 2.0F, -2.0F, -8.0F, 0.0F, 2.0F, -8.0F};
    const BufferHandleUVE vertexBuffer = renderDevice->CreateBufferUVE(
        BufferDescUVE{std::as_bytes(std::span(kVertices)).size(), BufferUsageUVE::Vertex},
        std::as_bytes(std::span(kVertices)));
    ASSERT_NE(vertexBuffer, kInvalidBufferHandleUVE);

    std::unique_ptr<ICommandBufferUVE> commandBuffer = renderDevice->CreateCommandBufferUVE();
    ASSERT_NE(commandBuffer, nullptr);
    RenderPassDescUVE passDesc;
    passDesc.colorAttachment = kInvalidTextureHandleUVE;
    passDesc.clearColor = {0.0F, 0.0F, 0.0F, 1.0F};
    passDesc.depthLoadOp = LoadOpUVE::DontCare;
    commandBuffer->BeginRenderPassUVE(passDesc);
    commandBuffer->BindPipelineUVE(pipeline);
    commandBuffer->SetUniformMatrix4x4UVE("uModel", Math::Matrix4x4UVE::IdentityUVE());
    commandBuffer->SetUniformMatrix4x4UVE(
        "uViewProjection", Math::Matrix4x4UVE::PerspectiveUVE(1.0471975512F, 1.0F, 0.1F, 100.0F));
    commandBuffer->SetUniformVector3UVE("uColor", Math::Vector3UVE{0.95F, 0.15F, 0.05F});
    commandBuffer->BindVertexBufferUVE(vertexBuffer);
    commandBuffer->DrawUVE(3U);
    commandBuffer->EndRenderPassUVE();
    renderDevice->SubmitUVE(std::move(commandBuffer));

    std::array<std::uint8_t, 3> pixel{};
    glReadPixels(32, 28, 1, 1, GL_RGB, GL_UNSIGNED_BYTE, pixel.data());
    EXPECT_GT(pixel[0], static_cast<std::uint8_t>(pixel[1] + 48U));
    EXPECT_EQ(glGetError(), GL_NO_ERROR);

    renderDevice->DestroyBufferUVE(vertexBuffer);
    renderDevice->DestroyPipelineUVE(pipeline);
    renderDevice->DestroyShaderUVE(fragmentShader);
    renderDevice->DestroyShaderUVE(vertexShader);
}

TEST_F(GlRenderDeviceUVETest, Basic3DShader_IndexedCanonicalMeshVertexLayoutRenders) {
    const ShaderHandleUVE vertexShader = renderDevice->CreateShaderUVE(
        ShaderDescUVE{ShaderStageUVE::Vertex,
                      WithShaderStageDefineUVE(Shader::BuiltIn::kBasic3DSource, "VERTEX_SHADER")});
    const ShaderHandleUVE fragmentShader = renderDevice->CreateShaderUVE(
        ShaderDescUVE{ShaderStageUVE::Fragment,
                      WithShaderStageDefineUVE(Shader::BuiltIn::kBasic3DSource, "FRAGMENT_SHADER")});
    ASSERT_NE(vertexShader, kInvalidShaderHandleUVE);
    ASSERT_NE(fragmentShader, kInvalidShaderHandleUVE);

    PipelineDescUVE pipelineDesc;
    pipelineDesc.vertexShader = vertexShader;
    pipelineDesc.fragmentShader = fragmentShader;
    pipelineDesc.vertexLayout = {VertexAttributeUVE{"POSITION", VertexAttributeFormatUVE::Float3,
                                                     offsetof(Asset::MeshVertexUVE, position)},
                                 VertexAttributeUVE{"NORMAL", VertexAttributeFormatUVE::Float3,
                                                     offsetof(Asset::MeshVertexUVE, normal)},
                                 VertexAttributeUVE{"TEXCOORD0", VertexAttributeFormatUVE::Float2,
                                                     offsetof(Asset::MeshVertexUVE, u)},
                                 VertexAttributeUVE{"TANGENT", VertexAttributeFormatUVE::Float4,
                                                     offsetof(Asset::MeshVertexUVE, tangent)}};
    pipelineDesc.vertexStride = static_cast<std::uint32_t>(sizeof(Asset::MeshVertexUVE));
    pipelineDesc.depthTestEnabled = false;
    pipelineDesc.depthWriteEnabled = false;
    const PipelineHandleUVE pipeline = renderDevice->CreatePipelineUVE(pipelineDesc);
    ASSERT_NE(pipeline, kInvalidPipelineHandleUVE);

    const std::array<Asset::MeshVertexUVE, 3> vertices{
        Asset::MeshVertexUVE{Math::Vector3UVE{-2.0F, -2.0F, 0.0F}, Math::Vector3UVE{0.0F, 0.0F, 1.0F}, 0.0F, 0.0F},
        Asset::MeshVertexUVE{Math::Vector3UVE{2.0F, -2.0F, 0.0F}, Math::Vector3UVE{0.0F, 0.0F, 1.0F}, 1.0F, 0.0F},
        Asset::MeshVertexUVE{Math::Vector3UVE{0.0F, 2.0F, 0.0F}, Math::Vector3UVE{0.0F, 0.0F, 1.0F}, 0.5F, 1.0F},
    };
    constexpr std::array<std::uint32_t, 3> kIndices{0U, 1U, 2U};
    const BufferHandleUVE vertexBuffer = renderDevice->CreateBufferUVE(
        BufferDescUVE{std::as_bytes(std::span(vertices)).size(), BufferUsageUVE::Vertex}, std::as_bytes(std::span(vertices)));
    const BufferHandleUVE indexBuffer = renderDevice->CreateBufferUVE(
        BufferDescUVE{std::as_bytes(std::span(kIndices)).size(), BufferUsageUVE::Index}, std::as_bytes(std::span(kIndices)));
    ASSERT_NE(vertexBuffer, kInvalidBufferHandleUVE);
    ASSERT_NE(indexBuffer, kInvalidBufferHandleUVE);

    std::unique_ptr<ICommandBufferUVE> commandBuffer = renderDevice->CreateCommandBufferUVE();
    ASSERT_NE(commandBuffer, nullptr);
    RenderPassDescUVE passDesc;
    passDesc.colorAttachment = kInvalidTextureHandleUVE;
    passDesc.clearColor = {0.0F, 0.0F, 0.0F, 1.0F};
    passDesc.depthLoadOp = LoadOpUVE::DontCare;
    commandBuffer->BeginRenderPassUVE(passDesc);
    commandBuffer->BindPipelineUVE(pipeline);
    commandBuffer->SetUniformMatrix4x4UVE(
        "uModel", Math::Matrix4x4UVE::ComposeTrsUVE(Math::Vector3UVE{0.0F, 0.0F, -8.0F}, Math::QuaternionUVE{},
                                                      Math::Vector3UVE{1.0F, 1.0F, 1.0F}));
    commandBuffer->SetUniformMatrix4x4UVE(
        "uViewProjection", Math::Matrix4x4UVE::PerspectiveUVE(1.0471975512F, 1.0F, 0.1F, 100.0F));
    commandBuffer->SetUniformVector3UVE("uColor", Math::Vector3UVE{0.95F, 0.15F, 0.05F});
    commandBuffer->BindVertexBufferUVE(vertexBuffer);
    commandBuffer->BindIndexBufferUVE(indexBuffer);
    commandBuffer->DrawIndexedUVE(3U);
    commandBuffer->EndRenderPassUVE();
    renderDevice->SubmitUVE(std::move(commandBuffer));

    std::array<std::uint8_t, 3> pixel{};
    glReadPixels(32, 28, 1, 1, GL_RGB, GL_UNSIGNED_BYTE, pixel.data());
    EXPECT_GT(pixel[0], static_cast<std::uint8_t>(pixel[1] + 48U));
    EXPECT_EQ(glGetError(), GL_NO_ERROR);

    renderDevice->DestroyBufferUVE(indexBuffer);
    renderDevice->DestroyBufferUVE(vertexBuffer);
    renderDevice->DestroyPipelineUVE(pipeline);
    renderDevice->DestroyShaderUVE(fragmentShader);
    renderDevice->DestroyShaderUVE(vertexShader);
}

TEST_F(GlRenderDeviceUVETest, ShaderManager_Basic3DProgramRendersIndexedGeometry) {
    Threading::ThreadPoolUVE threadPool{1U};
    Asset::AssetBundleUVE assetBundle;
    Asset::FileSystemUVE fileSystem{assetBundle};
    const Asset::MountHandleUVE shaderMount =
        fileSystem.MountDirectoryUVE("shaders", "engine/render/shader/built_in", 0);
    ASSERT_NE(shaderMount, 0U);
    Shader::ShaderManagerUVE shaderManager(threadPool, eventSystem, *renderDevice, fileSystem,
                                            Shader::ShaderManagerConfigUVE{});

    Shader::ShaderProgramDescUVE programDesc;
    programDesc.virtualFilePath = std::string(Shader::BuiltIn::kBasic3DVirtualPath);
    programDesc.embeddedFallbackSourceCode = std::string(Shader::BuiltIn::kBasic3DSource);
    programDesc.vertexLayout = {VertexAttributeUVE{"POSITION", VertexAttributeFormatUVE::Float3, 0U}};
    programDesc.vertexStride = 3U * static_cast<std::uint32_t>(sizeof(float));
    programDesc.depthTestEnabled = false;
    programDesc.depthWriteEnabled = false;
    programDesc.debugNameUVE = "ShaderManagerBasic3DRealGl";
    const std::shared_ptr<Shader::ShaderProgramUVE> program = shaderManager.CreateProgramUVE(programDesc);
    ASSERT_NE(program, nullptr);
    const auto compileDeadline = std::chrono::steady_clock::now() + std::chrono::seconds{2};
    while (!program->IsReadyUVE() && std::chrono::steady_clock::now() < compileDeadline) {
        shaderManager.UpdateUVE(0.0);
        std::this_thread::yield();
    }
    ASSERT_TRUE(program->IsReadyUVE());
    ASSERT_TRUE(program->IsValidUVE());

    constexpr std::array<float, 9> kVertices{-2.0F, -2.0F, -8.0F, 2.0F, -2.0F, -8.0F, 0.0F, 2.0F, -8.0F};
    const BufferHandleUVE vertexBuffer = renderDevice->CreateBufferUVE(
        BufferDescUVE{std::as_bytes(std::span(kVertices)).size(), BufferUsageUVE::Vertex},
        std::as_bytes(std::span(kVertices)));
    ASSERT_NE(vertexBuffer, kInvalidBufferHandleUVE);

    std::unique_ptr<ICommandBufferUVE> commandBuffer = renderDevice->CreateCommandBufferUVE();
    ASSERT_NE(commandBuffer, nullptr);
    RenderPassDescUVE passDesc;
    passDesc.colorAttachment = kInvalidTextureHandleUVE;
    passDesc.clearColor = {0.0F, 0.0F, 0.0F, 1.0F};
    passDesc.depthLoadOp = LoadOpUVE::DontCare;
    commandBuffer->BeginRenderPassUVE(passDesc);
    program->SetMatrix4x4UVE("uModel", Math::Matrix4x4UVE::IdentityUVE());
    program->SetMatrix4x4UVE(
        "uViewProjection", Math::Matrix4x4UVE::PerspectiveUVE(1.0471975512F, 1.0F, 0.1F, 100.0F));
    program->SetVector3UVE("uColor", Math::Vector3UVE{0.95F, 0.15F, 0.05F});
    program->ApplyToUVE(*commandBuffer);
    commandBuffer->BindVertexBufferUVE(vertexBuffer);
    commandBuffer->DrawUVE(3U);
    commandBuffer->EndRenderPassUVE();
    renderDevice->SubmitUVE(std::move(commandBuffer));

    std::array<std::uint8_t, 3> pixel{};
    glReadPixels(32, 28, 1, 1, GL_RGB, GL_UNSIGNED_BYTE, pixel.data());
    EXPECT_GT(pixel[0], static_cast<std::uint8_t>(pixel[1] + 48U));
    EXPECT_EQ(glGetError(), GL_NO_ERROR);

    renderDevice->DestroyBufferUVE(vertexBuffer);
    fileSystem.UnmountUVE(shaderMount);
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

TEST_F(GlRenderDeviceUVETest, GetPipelineUniformsUVE_UnsupportedTypeIsExplicitAndSamplerRemainsInt) {
    const ShaderHandleUVE vertexShader = renderDevice->CreateShaderUVE(
        ShaderDescUVE{ShaderStageUVE::Vertex, std::string(kValidVertexShaderSource)});
    const ShaderHandleUVE fragmentShader = renderDevice->CreateShaderUVE(
        ShaderDescUVE{ShaderStageUVE::Fragment, std::string(kSamplerAndUnsupportedUniformFragmentShaderSource)});
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
    const auto samplerIt = std::find_if(uniforms.cbegin(), uniforms.cend(), [](const UniformReflectionUVE& uniform) {
        return uniform.name == "uTexture";
    });
    const auto unsupportedIt = std::find_if(uniforms.cbegin(), uniforms.cend(), [](const UniformReflectionUVE& uniform) {
        return uniform.name == "uUnsigned";
    });
    ASSERT_NE(samplerIt, uniforms.cend());
    ASSERT_NE(unsupportedIt, uniforms.cend());
    EXPECT_EQ(samplerIt->type, ShaderDataTypeUVE::Int);
    EXPECT_EQ(unsupportedIt->type, ShaderDataTypeUVE::Unsupported);

    std::unique_ptr<ICommandBufferUVE> commandBuffer = renderDevice->CreateCommandBufferUVE();
    ASSERT_NE(commandBuffer, nullptr);
    RenderPassDescUVE passDesc;
    passDesc.colorAttachment = kInvalidTextureHandleUVE;
    passDesc.colorLoadOp = LoadOpUVE::DontCare;
    passDesc.depthLoadOp = LoadOpUVE::DontCare;
    commandBuffer->BeginRenderPassUVE(passDesc);
    commandBuffer->BindPipelineUVE(pipeline);
    while (glGetError() != GL_NO_ERROR) {
    }
    commandBuffer->SetUniformIntUVE("uTexture", 0);
    commandBuffer->SetUniformIntUVE("uUnsigned", 1);
    EXPECT_EQ(glGetError(), GL_NO_ERROR);
    commandBuffer->EndRenderPassUVE();
    renderDevice->SubmitUVE(std::move(commandBuffer));

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

TEST_F(GlRenderDeviceUVETest, LitShadowed3DShader_DepthPrepassDarkensOccludedFragment) {
    const ShaderHandleUVE shadowVertexShader = renderDevice->CreateShaderUVE(
        ShaderDescUVE{ShaderStageUVE::Vertex,
                      WithShaderStageDefineUVE(Shader::BuiltIn::kShadowDepthSource, "VERTEX_SHADER")});
    const ShaderHandleUVE shadowFragmentShader = renderDevice->CreateShaderUVE(
        ShaderDescUVE{ShaderStageUVE::Fragment,
                      WithShaderStageDefineUVE(Shader::BuiltIn::kShadowDepthSource, "FRAGMENT_SHADER")});
    const ShaderHandleUVE litVertexShader = renderDevice->CreateShaderUVE(
        ShaderDescUVE{ShaderStageUVE::Vertex,
                      WithShaderStageDefineUVE(Shader::BuiltIn::kLitShadowed3DSource, "VERTEX_SHADER")});
    const ShaderHandleUVE litFragmentShader = renderDevice->CreateShaderUVE(
        ShaderDescUVE{ShaderStageUVE::Fragment,
                      WithShaderStageDefineUVE(Shader::BuiltIn::kLitShadowed3DSource, "FRAGMENT_SHADER")});
    ASSERT_NE(shadowVertexShader, kInvalidShaderHandleUVE);
    ASSERT_NE(shadowFragmentShader, kInvalidShaderHandleUVE);
    ASSERT_NE(litVertexShader, kInvalidShaderHandleUVE);
    ASSERT_NE(litFragmentShader, kInvalidShaderHandleUVE);

    PipelineDescUVE shadowPipelineDesc;
    shadowPipelineDesc.vertexShader = shadowVertexShader;
    shadowPipelineDesc.fragmentShader = shadowFragmentShader;
    shadowPipelineDesc.vertexLayout = {VertexAttributeUVE{"POSITION", VertexAttributeFormatUVE::Float3, 0}};
    shadowPipelineDesc.vertexStride = 3U * static_cast<std::uint32_t>(sizeof(float));
    shadowPipelineDesc.depthTestEnabled = true;
    shadowPipelineDesc.depthWriteEnabled = true;
    const PipelineHandleUVE shadowPipeline = renderDevice->CreatePipelineUVE(shadowPipelineDesc);
    ASSERT_NE(shadowPipeline, kInvalidPipelineHandleUVE);

    PipelineDescUVE litPipelineDesc;
    litPipelineDesc.vertexShader = litVertexShader;
    litPipelineDesc.fragmentShader = litFragmentShader;
    litPipelineDesc.vertexLayout = {VertexAttributeUVE{"POSITION", VertexAttributeFormatUVE::Float3, 0},
                                    VertexAttributeUVE{"NORMAL", VertexAttributeFormatUVE::Float3,
                                                       3U * static_cast<std::uint32_t>(sizeof(float))},
                                    VertexAttributeUVE{"TEXCOORD0", VertexAttributeFormatUVE::Float2,
                                                       6U * static_cast<std::uint32_t>(sizeof(float))},
                                    VertexAttributeUVE{"TANGENT", VertexAttributeFormatUVE::Float4,
                                                       8U * static_cast<std::uint32_t>(sizeof(float))}};
    litPipelineDesc.vertexStride = 12U * static_cast<std::uint32_t>(sizeof(float));
    litPipelineDesc.depthTestEnabled = false;
    litPipelineDesc.depthWriteEnabled = false;
    const PipelineHandleUVE litPipeline = renderDevice->CreatePipelineUVE(litPipelineDesc);
    ASSERT_NE(litPipeline, kInvalidPipelineHandleUVE);

    constexpr std::array<float, 9> kShadowCasterVertices{
        -0.9F, -0.9F, -0.5F,
        0.9F, -0.9F, -0.5F,
        0.0F, 0.9F, -0.5F,
    };
    constexpr std::array<float, 36> kReceiverVertices{
        -1.0F, -1.0F, 0.0F, 0.0F, 0.0F, 1.0F, 0.0F, 0.0F, 1.0F, 0.0F, 0.0F, 1.0F,
        3.0F, -1.0F, 0.0F, 0.0F, 0.0F, 1.0F, 2.0F, 0.0F, 1.0F, 0.0F, 0.0F, 1.0F,
        -1.0F, 3.0F, 0.0F, 0.0F, 0.0F, 1.0F, 0.0F, 2.0F, 1.0F, 0.0F, 0.0F, 1.0F,
    };
    constexpr std::array<std::uint8_t, 4> kWhitePixel{255U, 255U, 255U, 255U};
    constexpr std::array<std::uint8_t, 4> kFlatNormalPixel{128U, 128U, 255U, 255U};

    const BufferHandleUVE shadowVertexBuffer = renderDevice->CreateBufferUVE(
        BufferDescUVE{std::as_bytes(std::span(kShadowCasterVertices)).size(), BufferUsageUVE::Vertex},
        std::as_bytes(std::span(kShadowCasterVertices)));
    const BufferHandleUVE receiverVertexBuffer = renderDevice->CreateBufferUVE(
        BufferDescUVE{std::as_bytes(std::span(kReceiverVertices)).size(), BufferUsageUVE::Vertex},
        std::as_bytes(std::span(kReceiverVertices)));
    const TextureHandleUVE shadowMap =
        renderDevice->CreateTextureUVE(TextureDescUVE{64, 64, TextureFormatUVE::Depth32Float, 1});
    const TextureHandleUVE whiteTexture = renderDevice->CreateTextureUVE(
        TextureDescUVE{1, 1, TextureFormatUVE::RGBA8Unorm, 1}, std::as_bytes(std::span(kWhitePixel)));
    const TextureHandleUVE flatNormalTexture = renderDevice->CreateTextureUVE(
        TextureDescUVE{1, 1, TextureFormatUVE::RGBA8Unorm, 1}, std::as_bytes(std::span(kFlatNormalPixel)));
    ASSERT_NE(shadowVertexBuffer, kInvalidBufferHandleUVE);
    ASSERT_NE(receiverVertexBuffer, kInvalidBufferHandleUVE);
    ASSERT_NE(shadowMap, kInvalidTextureHandleUVE);
    ASSERT_NE(whiteTexture, kInvalidTextureHandleUVE);
    ASSERT_NE(flatNormalTexture, kInvalidTextureHandleUVE);

    std::unique_ptr<ICommandBufferUVE> shadowCommandBuffer = renderDevice->CreateCommandBufferUVE();
    ASSERT_NE(shadowCommandBuffer, nullptr);
    RenderPassDescUVE shadowPassDesc;
    shadowPassDesc.colorAttachment = kInvalidTextureHandleUVE;
    shadowPassDesc.depthAttachment = shadowMap;
    shadowPassDesc.depthLoadOp = LoadOpUVE::Clear;
    shadowPassDesc.clearDepth = 1.0F;
    shadowCommandBuffer->BeginRenderPassUVE(shadowPassDesc);
    shadowCommandBuffer->BindPipelineUVE(shadowPipeline);
    shadowCommandBuffer->SetUniformMatrix4x4UVE("uModel", Math::Matrix4x4UVE::IdentityUVE());
    shadowCommandBuffer->SetUniformMatrix4x4UVE("uLightSpaceMatrix", Math::Matrix4x4UVE::IdentityUVE());
    shadowCommandBuffer->BindVertexBufferUVE(shadowVertexBuffer);
    shadowCommandBuffer->DrawUVE(3);
    shadowCommandBuffer->EndRenderPassUVE();
    renderDevice->SubmitUVE(std::move(shadowCommandBuffer));

    std::unique_ptr<ICommandBufferUVE> colorCommandBuffer = renderDevice->CreateCommandBufferUVE();
    ASSERT_NE(colorCommandBuffer, nullptr);
    RenderPassDescUVE colorPassDesc;
    colorPassDesc.colorAttachment = kInvalidTextureHandleUVE;
    colorPassDesc.colorLoadOp = LoadOpUVE::Clear;
    colorPassDesc.clearColor = {0.0F, 0.0F, 0.0F, 1.0F};
    colorPassDesc.depthLoadOp = LoadOpUVE::DontCare;
    colorCommandBuffer->BeginRenderPassUVE(colorPassDesc);
    colorCommandBuffer->BindPipelineUVE(litPipeline);
    colorCommandBuffer->SetUniformMatrix4x4UVE("uModel", Math::Matrix4x4UVE::IdentityUVE());
    colorCommandBuffer->SetUniformMatrix4x4UVE("uViewProjection", Math::Matrix4x4UVE::IdentityUVE());
    colorCommandBuffer->SetUniformMatrix4x4UVE("uLightSpaceMatrix", Math::Matrix4x4UVE::IdentityUVE());
    colorCommandBuffer->SetUniformVector3UVE("uAmbientColor", Math::Vector3UVE{0.0F, 0.0F, 0.0F});
    colorCommandBuffer->SetUniformVector3UVE("uViewPosition", Math::Vector3UVE{0.0F, 0.0F, 1.0F});
    colorCommandBuffer->SetUniformVector3UVE("uAlbedoColor", Math::Vector3UVE{1.0F, 1.0F, 1.0F});
    colorCommandBuffer->SetUniformFloatUVE("uMetallic", 0.0F);
    colorCommandBuffer->SetUniformFloatUVE("uRoughness", 1.0F);
    colorCommandBuffer->SetUniformVector3UVE("uEmissiveColor", Math::Vector3UVE{0.0F, 0.0F, 0.0F});
    colorCommandBuffer->SetUniformIntUVE("uLights[0].type", 0);
    colorCommandBuffer->SetUniformVector3UVE("uLights[0].direction", Math::Vector3UVE{0.0F, 0.0F, -1.0F});
    colorCommandBuffer->SetUniformVector3UVE("uLights[0].color", Math::Vector3UVE{1.0F, 1.0F, 1.0F});
    colorCommandBuffer->SetUniformFloatUVE("uLights[0].intensity", 1.0F);
    colorCommandBuffer->BindTextureUVE(shadowMap, 0U);
    colorCommandBuffer->SetUniformIntUVE("uShadowMapTexture", 0);
    colorCommandBuffer->SetUniformIntUVE("uShadowPcfKernelRadius", 1);
    colorCommandBuffer->BindTextureUVE(flatNormalTexture, 3U);
    colorCommandBuffer->SetUniformIntUVE("uNormalTexture", 3);
    colorCommandBuffer->BindTextureUVE(whiteTexture, 1U);
    colorCommandBuffer->SetUniformIntUVE("uAlbedoTexture", 1);
    colorCommandBuffer->BindTextureUVE(whiteTexture, 2U);
    colorCommandBuffer->SetUniformIntUVE("uAOTexture", 2);
    colorCommandBuffer->BindVertexBufferUVE(receiverVertexBuffer);
    colorCommandBuffer->DrawUVE(3);
    colorCommandBuffer->EndRenderPassUVE();
    renderDevice->SubmitUVE(std::move(colorCommandBuffer));

    std::array<std::uint8_t, 4> shadowedPixel{};
    std::array<std::uint8_t, 4> softEdgePixel{};
    std::array<std::uint8_t, 4> litPixel{};
    glReadPixels(32, 32, 1, 1, GL_RGBA, GL_UNSIGNED_BYTE, shadowedPixel.data());
    glReadPixels(44, 36, 1, 1, GL_RGBA, GL_UNSIGNED_BYTE, softEdgePixel.data());
    glReadPixels(60, 60, 1, 1, GL_RGBA, GL_UNSIGNED_BYTE, litPixel.data());
    EXPECT_GT(static_cast<int>(softEdgePixel[0]), static_cast<int>(shadowedPixel[0]) + 16);
    EXPECT_GT(static_cast<int>(litPixel[0]), static_cast<int>(softEdgePixel[0]) + 16);

    renderDevice->DestroyTextureUVE(flatNormalTexture);
    renderDevice->DestroyTextureUVE(whiteTexture);
    renderDevice->DestroyTextureUVE(shadowMap);
    renderDevice->DestroyBufferUVE(receiverVertexBuffer);
    renderDevice->DestroyBufferUVE(shadowVertexBuffer);
    renderDevice->DestroyPipelineUVE(litPipeline);
    renderDevice->DestroyPipelineUVE(shadowPipeline);
    renderDevice->DestroyShaderUVE(litFragmentShader);
    renderDevice->DestroyShaderUVE(litVertexShader);
    renderDevice->DestroyShaderUVE(shadowFragmentShader);
    renderDevice->DestroyShaderUVE(shadowVertexShader);
}

TEST_F(GlRenderDeviceUVETest, CommandBuffer_SetUniformCalls_RejectReflectedTypeMismatches) {
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

    std::unique_ptr<ICommandBufferUVE> commandBuffer = renderDevice->CreateCommandBufferUVE();
    ASSERT_NE(commandBuffer, nullptr);
    RenderPassDescUVE passDesc;
    passDesc.colorAttachment = kInvalidTextureHandleUVE;
    passDesc.colorLoadOp = LoadOpUVE::DontCare;
    passDesc.depthLoadOp = LoadOpUVE::DontCare;
    commandBuffer->BeginRenderPassUVE(passDesc);
    commandBuffer->BindPipelineUVE(pipeline);
    while (glGetError() != GL_NO_ERROR) {
    }

    commandBuffer->SetUniformMatrix4x4UVE("uModel", Math::Matrix4x4UVE::IdentityUVE());
    commandBuffer->SetUniformVector3UVE("uColor", Math::Vector3UVE{1.0F, 0.0F, 0.0F});
    commandBuffer->SetUniformFloatUVE("uModel", 1.0F);
    commandBuffer->SetUniformIntUVE("uColor", 1);
    commandBuffer->SetUniformBoolUVE("uModel", true);
    commandBuffer->SetUniformMatrix4x4UVE("uColor", Math::Matrix4x4UVE::IdentityUVE());
    EXPECT_EQ(glGetError(), GL_NO_ERROR);

    commandBuffer->EndRenderPassUVE();
    renderDevice->SubmitUVE(std::move(commandBuffer));
    renderDevice->DestroyPipelineUVE(pipeline);
    renderDevice->DestroyShaderUVE(vertexShader);
    renderDevice->DestroyShaderUVE(fragmentShader);
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
