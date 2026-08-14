// Copyright (c) 2026 UniVex Studios. All Rights Reserved.

#include <array>
#include <cstdint>
#include <sstream>
#include <string>
#include <string_view>
#include <vector>

#include <gtest/gtest.h>
#include <nlohmann/json.hpp>

#include "uve/core/engine_core_uve.h"
#include "uve/editor/editor_bridge_stdio_uve.h"

namespace UVE::Editor::Tests {
namespace {

using JsonUVE = nlohmann::json;

[[nodiscard]] Core::EngineConfigUVE MakeBridgeStdioTestConfigUVE() {
    Core::EngineConfigUVE config{};
    config.headlessUVE = true;
    config.logFilePath = "uve_editor_bridge_stdio_tests.log";
    config.settingsFilePath = "uve_editor_bridge_stdio_tests_settings.json";
    config.assetDatabaseFilePath = "uve_editor_bridge_stdio_tests_assets.json";
    config.saveDirectoryPath = "uve_editor_bridge_stdio_tests_saves";
    config.shaderCachePath = "uve_editor_bridge_stdio_tests_shader_cache";
    config.shaderSourceRealDirectoryUVE = "engine/render/shader/built_in";
    config.shaderSourceMountPrefixUVE = "shaders";
    return config;
}

void AppendFrameUVE(std::ostream& output, const JsonUVE& message) {
    const std::string body = message.dump();
    const std::uint32_t length = static_cast<std::uint32_t>(body.size());
    const std::array<char, 4U> header{
        static_cast<char>((length >> 24U) & 0xFFU), static_cast<char>((length >> 16U) & 0xFFU),
        static_cast<char>((length >> 8U) & 0xFFU), static_cast<char>(length & 0xFFU)};
    output.write(header.data(), static_cast<std::streamsize>(header.size()));
    output.write(body.data(), static_cast<std::streamsize>(body.size()));
}

[[nodiscard]] std::vector<JsonUVE> ReadFramesUVE(std::istream& input) {
    std::vector<JsonUVE> frames;
    while (true) {
        std::array<char, 4U> header{};
        input.read(header.data(), static_cast<std::streamsize>(header.size()));
        if (input.gcount() == 0 && input.eof()) {
            return frames;
        }
        EXPECT_EQ(input.gcount(), static_cast<std::streamsize>(header.size()));
        const std::uint32_t length =
            (static_cast<std::uint32_t>(static_cast<unsigned char>(header[0])) << 24U) |
            (static_cast<std::uint32_t>(static_cast<unsigned char>(header[1])) << 16U) |
            (static_cast<std::uint32_t>(static_cast<unsigned char>(header[2])) << 8U) |
            static_cast<std::uint32_t>(static_cast<unsigned char>(header[3]));
        std::string body(length, '\0');
        input.read(body.data(), static_cast<std::streamsize>(body.size()));
        EXPECT_EQ(input.gcount(), static_cast<std::streamsize>(body.size()));
        frames.push_back(JsonUVE::parse(body));
    }
}

TEST(EditorBridgeStdioUVETest, ServeUVE_HandshakesAndRoutesExistingBridgeDispatch) {
    Core::EngineCoreUVE engine(MakeBridgeStdioTestConfigUVE());
    engine.Init();
    ASSERT_TRUE(engine.Load());
    {
        EditorUVE editor(engine.GetServicesUVE(), "uve_editor_bridge_stdio_roundtrip.uvescene");
        editor.InitUVE();
        EditorBridgeUVE bridge(editor);
        EditorBridgeStdioServerUVE server(bridge);
        std::stringstream input;
        std::stringstream output;
        std::stringstream diagnostics;

        AppendFrameUVE(input, JsonUVE{{"jsonrpc", "2.0"},
                                      {"id", 1U},
                                      {"method", "bridge.hello"},
                                      {"params", {{"protocolVersion", kEditorBridgeProtocolVersionUVE}}}});
        AppendFrameUVE(input, JsonUVE{{"jsonrpc", "2.0"},
                                      {"id", 2U},
                                      {"method", "bridge.dispatch"},
                                      {"params", {{"protocolVersion", kEditorBridgeProtocolVersionUVE},
                                                  {"requestId", 42U},
                                                  {"expectedRevision", 1U},
                                                  {"kind", "createDocumentEntity"},
                                                  {"entityKind", "cube"}}}});

        EXPECT_EQ(server.ServeUVE(input, output, diagnostics), 0);
        EXPECT_TRUE(diagnostics.str().empty());
        const std::vector<JsonUVE> frames = ReadFramesUVE(output);
        ASSERT_EQ(frames.size(), 2U);
        EXPECT_TRUE(frames[0U].at("result").at("compatible").get<bool>());
        EXPECT_EQ(frames[0U].at("result").at("protocolVersion").get<std::uint32_t>(),
                  kEditorBridgeProtocolVersionUVE);
        const JsonUVE& handshakeSnapshot = frames[0U].at("result").at("snapshot");
        EXPECT_TRUE(handshakeSnapshot.at("selectedEntitiesTruncated").is_boolean());
        EXPECT_TRUE(handshakeSnapshot.at("hierarchy").at("entries").is_array());
        EXPECT_TRUE(handshakeSnapshot.at("inspector").at("eligibleDrawerIds").is_array());
        EXPECT_TRUE(handshakeSnapshot.at("contentBrowser").at("breadcrumbs").is_array());
        EXPECT_TRUE(frames[1U].at("result").at("applied").get<bool>());
        EXPECT_EQ(frames[1U].at("result").at("code").get<std::string>(), "bridge.command.applied");
        EXPECT_TRUE(frames[1U].at("result").at("createdEntity").is_object());
        EXPECT_TRUE(editor.IsSceneDirtyUVE());

        editor.ShutdownUVE();
    }
    engine.Shutdown();
}

TEST(EditorBridgeStdioUVETest, ServeUVE_ReportsIncompatibleHelloWithoutMutatingEditorState) {
    Core::EngineCoreUVE engine(MakeBridgeStdioTestConfigUVE());
    engine.Init();
    ASSERT_TRUE(engine.Load());
    {
        EditorUVE editor(engine.GetServicesUVE(), "uve_editor_bridge_stdio_incompatible.uvescene");
        editor.InitUVE();
        EditorBridgeUVE bridge(editor);
        EditorBridgeStdioServerUVE server(bridge);
        std::stringstream input;
        std::stringstream output;
        std::stringstream diagnostics;
        AppendFrameUVE(input, JsonUVE{{"jsonrpc", "2.0"},
                                      {"id", 4U},
                                      {"method", "bridge.hello"},
                                      {"params", {{"protocolVersion", 99U}}}});

        EXPECT_EQ(server.ServeUVE(input, output, diagnostics), 0);
        EXPECT_TRUE(diagnostics.str().empty());
        const std::vector<JsonUVE> frames = ReadFramesUVE(output);
        ASSERT_EQ(frames.size(), 1U);
        EXPECT_FALSE(frames.front().at("result").at("compatible").get<bool>());
        EXPECT_EQ(frames.front().at("result").at("code").get<std::string>(), "bridge.protocol.unsupported");
        EXPECT_TRUE(editor.GetDocumentRootsUVE().empty());
        EXPECT_FALSE(editor.IsSceneDirtyUVE());

        editor.ShutdownUVE();
    }
    engine.Shutdown();
}

TEST(EditorBridgeStdioUVETest, ServeUVE_RejectsMalformedJsonWithoutDispatchingEditorState) {
    Core::EngineCoreUVE engine(MakeBridgeStdioTestConfigUVE());
    engine.Init();
    ASSERT_TRUE(engine.Load());
    {
        EditorUVE editor(engine.GetServicesUVE(), "uve_editor_bridge_stdio_invalid_json.uvescene");
        editor.InitUVE();
        EditorBridgeUVE bridge(editor);
        EditorBridgeStdioServerUVE server(bridge);
        std::stringstream input;
        std::stringstream output;
        std::stringstream diagnostics;
        const std::string malformedJson{"{"};
        const std::uint32_t length = static_cast<std::uint32_t>(malformedJson.size());
        const std::array<char, 4U> header{
            static_cast<char>((length >> 24U) & 0xFFU), static_cast<char>((length >> 16U) & 0xFFU),
            static_cast<char>((length >> 8U) & 0xFFU), static_cast<char>(length & 0xFFU)};
        input.write(header.data(), static_cast<std::streamsize>(header.size()));
        input.write(malformedJson.data(), static_cast<std::streamsize>(malformedJson.size()));

        EXPECT_EQ(server.ServeUVE(input, output, diagnostics), 0);
        EXPECT_EQ(diagnostics.str(), "bridge.transport.json.invalid\n");
        const std::vector<JsonUVE> frames = ReadFramesUVE(output);
        ASSERT_EQ(frames.size(), 1U);
        EXPECT_EQ(frames.front().at("error").at("data").at("code").get<std::string>(),
                  "bridge.transport.json.invalid");
        EXPECT_TRUE(editor.GetDocumentRootsUVE().empty());
        EXPECT_FALSE(editor.IsSceneDirtyUVE());

        editor.ShutdownUVE();
    }
    engine.Shutdown();
}

TEST(EditorBridgeStdioUVETest, ServeUVE_RejectsZeroLengthFrameBeforeDispatchingEditorState) {
    Core::EngineCoreUVE engine(MakeBridgeStdioTestConfigUVE());
    engine.Init();
    ASSERT_TRUE(engine.Load());
    {
        EditorUVE editor(engine.GetServicesUVE(), "uve_editor_bridge_stdio_invalid_frame.uvescene");
        editor.InitUVE();
        EditorBridgeUVE bridge(editor);
        EditorBridgeStdioServerUVE server(bridge);
        std::stringstream input;
        std::stringstream output;
        std::stringstream diagnostics;
        const std::array<char, 4U> zeroLengthHeader{};
        input.write(zeroLengthHeader.data(), static_cast<std::streamsize>(zeroLengthHeader.size()));

        EXPECT_EQ(server.ServeUVE(input, output, diagnostics), 2);
        EXPECT_EQ(diagnostics.str(), "bridge.transport.frame.zero_length\n");
        const std::vector<JsonUVE> frames = ReadFramesUVE(output);
        ASSERT_EQ(frames.size(), 1U);
        EXPECT_EQ(frames.front().at("error").at("data").at("code").get<std::string>(),
                  "bridge.transport.frame.zero_length");
        EXPECT_TRUE(editor.GetDocumentRootsUVE().empty());
        EXPECT_FALSE(editor.IsSceneDirtyUVE());

        editor.ShutdownUVE();
    }
    engine.Shutdown();
}

TEST(EditorBridgeStdioUVETest, ServeUVE_ClassifiesTruncatedAndOversizedFramesBeforeDispatchingEditorState) {
    Core::EngineCoreUVE engine(MakeBridgeStdioTestConfigUVE());
    engine.Init();
    ASSERT_TRUE(engine.Load());
    {
        EditorUVE editor(engine.GetServicesUVE(), "uve_editor_bridge_stdio_frame_classification.uvescene");
        editor.InitUVE();
        EditorBridgeUVE bridge(editor);

        const auto verifyMalformedFrame = [&](const std::string_view bytes, const std::string_view expectedCode) {
            EditorBridgeStdioServerUVE server(bridge);
            std::stringstream input;
            std::stringstream output;
            std::stringstream diagnostics;
            input.write(bytes.data(), static_cast<std::streamsize>(bytes.size()));

            EXPECT_EQ(server.ServeUVE(input, output, diagnostics), 2);
            EXPECT_EQ(diagnostics.str(), std::string(expectedCode) + "\n");
            const std::vector<JsonUVE> frames = ReadFramesUVE(output);
            ASSERT_EQ(frames.size(), 1U);
            EXPECT_EQ(frames.front().at("error").at("data").at("code").get<std::string>(), expectedCode);
            EXPECT_TRUE(editor.GetDocumentRootsUVE().empty());
            EXPECT_FALSE(editor.IsSceneDirtyUVE());
        };

        const std::string truncatedHeader{"\x00\x01", 2U};
        verifyMalformedFrame(truncatedHeader, "bridge.transport.frame.truncated_header");

        std::string truncatedBody;
        const std::uint32_t declaredLength = 2U;
        const std::array<char, 4U> truncatedBodyHeader{
            static_cast<char>((declaredLength >> 24U) & 0xFFU), static_cast<char>((declaredLength >> 16U) & 0xFFU),
            static_cast<char>((declaredLength >> 8U) & 0xFFU), static_cast<char>(declaredLength & 0xFFU)};
        truncatedBody.append(truncatedBodyHeader.data(), truncatedBodyHeader.size());
        truncatedBody.push_back('{');
        verifyMalformedFrame(truncatedBody, "bridge.transport.frame.truncated_body");

        std::string oversizedFrame;
        const std::uint32_t oversizedLength = EditorBridgeStdioServerUVE::kMaximumFrameBytesUVE + 1U;
        const std::array<char, 4U> oversizedHeader{
            static_cast<char>((oversizedLength >> 24U) & 0xFFU), static_cast<char>((oversizedLength >> 16U) & 0xFFU),
            static_cast<char>((oversizedLength >> 8U) & 0xFFU), static_cast<char>(oversizedLength & 0xFFU)};
        oversizedFrame.append(oversizedHeader.data(), oversizedHeader.size());
        verifyMalformedFrame(oversizedFrame, "bridge.transport.frame.oversized");

        editor.ShutdownUVE();
    }
    engine.Shutdown();
}

} // namespace
} // namespace UVE::Editor::Tests
