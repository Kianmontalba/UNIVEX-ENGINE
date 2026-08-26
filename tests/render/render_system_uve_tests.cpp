// Copyright (c) 2026 UniVex Studios. All Rights Reserved.


#include "uve/render/render_system_uve.h"

#include <vector>

#include <gtest/gtest.h>

#include "uve/platform/platform_uve.h"
#include "uve/render/null_render_device_uve.h"

namespace UVE::Render::Tests {
namespace {

TEST(RenderSystemUVETest, FrameIndex_StartsAtZero) {
    NullRenderDeviceUVE device;
    RenderSystemUVE renderSystem(device);

    EXPECT_EQ(renderSystem.GetFrameIndexUVE(), 0U);
}

TEST(RenderSystemUVETest, BeginFrameThenEndFrame_AdvancesFrameIndex) {
    NullRenderDeviceUVE device;
    RenderSystemUVE renderSystem(device);

    renderSystem.BeginFrameUVE();
    renderSystem.EndFrameUVE();
    EXPECT_EQ(renderSystem.GetFrameIndexUVE(), 1U);

    renderSystem.BeginFrameUVE();
    renderSystem.EndFrameUVE();
    EXPECT_EQ(renderSystem.GetFrameIndexUVE(), 2U);
}

TEST(RenderSystemUVETest, GetFrameCommandBufferUVE_ReturnsUsableCommandBufferDuringActiveFrame) {
    NullRenderDeviceUVE device;
    RenderSystemUVE renderSystem(device);

    renderSystem.BeginFrameUVE();
    ICommandBufferUVE& commandBuffer = renderSystem.GetFrameCommandBufferUVE();
    RenderPassDescUVE passDesc;
    commandBuffer.BeginRenderPassUVE(passDesc);
    commandBuffer.EndRenderPassUVE();
    renderSystem.EndFrameUVE();

    const std::vector<RecordedCommandUVE>& recorded = device.GetLastSubmittedCommandsUVE();
    ASSERT_EQ(recorded.size(), 2U);
    EXPECT_TRUE(std::holds_alternative<BeginRenderPassCommandUVE>(recorded[0]));
    EXPECT_TRUE(std::holds_alternative<EndRenderPassCommandUVE>(recorded[1]));
}

TEST(RenderSystemUVETest, EndFrameUVE_SubmitsTheSameCommandBufferThatWasRecordedInto) {
    NullRenderDeviceUVE device;
    RenderSystemUVE renderSystem(device);

    renderSystem.BeginFrameUVE();
    ICommandBufferUVE& commandBuffer = renderSystem.GetFrameCommandBufferUVE();
    commandBuffer.BeginRenderPassUVE(RenderPassDescUVE{});
    commandBuffer.EndRenderPassUVE();
    renderSystem.EndFrameUVE();

    EXPECT_FALSE(device.GetLastSubmittedCommandsUVE().empty());
}

#if UVE_DEBUG
TEST(RenderSystemUVEDeathTest, GetFrameCommandBufferUVE_WithoutActiveFrame_Asserts) {
    NullRenderDeviceUVE device;
    RenderSystemUVE renderSystem(device);

    EXPECT_DEATH({ static_cast<void>(renderSystem.GetFrameCommandBufferUVE()); }, "");
}

TEST(RenderSystemUVEDeathTest, EndFrameUVE_WithoutActiveFrame_Asserts) {
    NullRenderDeviceUVE device;
    RenderSystemUVE renderSystem(device);

    EXPECT_DEATH({ renderSystem.EndFrameUVE(); }, "");
}

TEST(RenderSystemUVEDeathTest, BeginFrameUVE_WhileFrameAlreadyActive_Asserts) {
    NullRenderDeviceUVE device;
    RenderSystemUVE renderSystem(device);

    renderSystem.BeginFrameUVE();
    EXPECT_DEATH({ renderSystem.BeginFrameUVE(); }, "");
}
#endif

#if !UVE_DEBUG
TEST(RenderSystemUVETest, Release_GetFrameCommandBufferUVE_WithoutActiveFrameIsSafe) {
    NullRenderDeviceUVE device;
    RenderSystemUVE renderSystem(device);

    ICommandBufferUVE& fallback = renderSystem.GetFrameCommandBufferUVE();
    fallback.BeginRenderPassUVE(RenderPassDescUVE{});
    fallback.EndRenderPassUVE();

    EXPECT_TRUE(device.GetLastSubmittedCommandsUVE().empty());
    EXPECT_EQ(renderSystem.GetFrameIndexUVE(), 0U);
}

TEST(RenderSystemUVETest, Release_EndFrameUVE_WithoutActiveFrameIsSafeNoOp) {
    NullRenderDeviceUVE device;
    RenderSystemUVE renderSystem(device);

    renderSystem.EndFrameUVE();

    EXPECT_TRUE(device.GetLastSubmittedCommandsUVE().empty());
    EXPECT_EQ(renderSystem.GetFrameIndexUVE(), 0U);
}

TEST(RenderSystemUVETest, Release_BeginFrameUVE_WhileActivePreservesCurrentFrame) {
    NullRenderDeviceUVE device;
    RenderSystemUVE renderSystem(device);

    renderSystem.BeginFrameUVE();
    ICommandBufferUVE* const firstBuffer = &renderSystem.GetFrameCommandBufferUVE();
    renderSystem.BeginFrameUVE();
    ICommandBufferUVE* const preservedBuffer = &renderSystem.GetFrameCommandBufferUVE();

    EXPECT_EQ(preservedBuffer, firstBuffer);
    renderSystem.EndFrameUVE();
    EXPECT_EQ(renderSystem.GetFrameIndexUVE(), 1U);
}
#endif

} // namespace
} // namespace UVE::Render::Tests
