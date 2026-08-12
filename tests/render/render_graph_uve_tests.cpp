// Copyright (c) 2026 UniVex Studios. All Rights Reserved.


#include "uve/render/render_graph_uve.h"

#include <memory>
#include <string>
#include <vector>

#include <gtest/gtest.h>

#include "uve/render/i_command_buffer_uve.h"
#include "uve/render/null_render_device_uve.h"

namespace UVE::Render::Tests {
namespace {

TEST(RenderGraphUVETest, ExecuteUVE_ImportedResourcePassesRunInStableInsertionOrder) {
    NullRenderDeviceUVE renderDevice;
    const TextureHandleUVE texture = renderDevice.CreateTextureUVE(TextureDescUVE{1, 1, TextureFormatUVE::RGBA8Unorm, 1});
    ASSERT_NE(texture, kInvalidTextureHandleUVE);

    RenderGraphUVE graph;
    const RenderGraphResourceHandleUVE resource = graph.ImportTextureUVE(texture, "TestColor");
    std::vector<std::string> executionOrder;
    graph.AddPassUVE(RenderGraphPassDescUVE{"Write", {{resource, RenderGraphResourceAccessUVE::Write}},
                                            [&executionOrder](ICommandBufferUVE&) { executionOrder.push_back("Write"); }});
    graph.AddPassUVE(RenderGraphPassDescUVE{"Read", {{resource, RenderGraphResourceAccessUVE::Read}},
                                            [&executionOrder](ICommandBufferUVE&) { executionOrder.push_back("Read"); }});

    std::unique_ptr<ICommandBufferUVE> commandBuffer = renderDevice.CreateCommandBufferUVE();
    ASSERT_NE(commandBuffer, nullptr);
    EXPECT_TRUE(graph.ExecuteUVE(*commandBuffer));
    EXPECT_EQ(executionOrder, (std::vector<std::string>{"Write", "Read"}));
    EXPECT_EQ(graph.GetPassCountUVE(), 2U);
    renderDevice.DestroyTextureUVE(texture);
}

TEST(RenderGraphUVETest, ExecuteUVE_UnknownResourceRejectsWithoutRecordingPasses) {
    NullRenderDeviceUVE renderDevice;
    RenderGraphUVE graph;
    bool recorded = false;
    graph.AddPassUVE(RenderGraphPassDescUVE{"Invalid", {{RenderGraphResourceHandleUVE{0U}, RenderGraphResourceAccessUVE::Read}},
                                            [&recorded](ICommandBufferUVE&) { recorded = true; }});

    std::unique_ptr<ICommandBufferUVE> commandBuffer = renderDevice.CreateCommandBufferUVE();
    ASSERT_NE(commandBuffer, nullptr);
    EXPECT_FALSE(graph.ExecuteUVE(*commandBuffer));
    EXPECT_FALSE(recorded);
}

} // namespace
} // namespace UVE::Render::Tests
