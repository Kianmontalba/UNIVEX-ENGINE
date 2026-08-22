// Copyright (c) 2026 UniVex Studios. All Rights Reserved.


#include "uve/asset/mesh_asset_uve.h"

#include <algorithm>
#include <cstring>
#include <filesystem>
#include <limits>
#include <memory>
#include <thread>
#include <vector>

#include <gtest/gtest.h>

#include "uve/asset/asset_database_uve.h"
#include "uve/asset/asset_handle_uve.h"
#include "uve/asset/asset_manager_uve.h"
#include "uve/asset/uve_file_envelope_uve.h"
#include "uve/debug/log_sink_uve.h"
#include "uve/debug/logger_uve.h"
#include "uve/events/event_system_uve.h"
#include "uve/threading/thread_pool_uve.h"

namespace UVE::Asset::Tests {
namespace {

[[nodiscard]] MeshAssetUVE MakeTestMeshUVE() {
    MeshAssetUVE mesh;
    mesh.vertices = {
        MeshVertexUVE{Math::Vector3UVE{0.0F, 0.0F, 0.0F}, Math::Vector3UVE{0.0F, 1.0F, 0.0F}, 0.0F, 0.0F},
        MeshVertexUVE{Math::Vector3UVE{1.0F, 0.0F, 0.0F}, Math::Vector3UVE{0.0F, 1.0F, 0.0F}, 1.0F, 0.0F},
        MeshVertexUVE{Math::Vector3UVE{0.0F, 1.0F, 0.0F}, Math::Vector3UVE{0.0F, 1.0F, 0.0F}, 0.0F, 1.0F},
    };
    mesh.indices = {0, 1, 2};
    mesh.localBounds = Math::AabbUVE{Math::Vector3UVE{0.0F, 0.0F, 0.0F}, Math::Vector3UVE{1.0F, 1.0F, 0.0F}};
    return mesh;
}

TEST(MeshAssetUVETest, GenerateMeshTangentsUVE_StandardUvTriangle_ProducesOrthonormalPositiveBasis) {
    std::vector<MeshVertexUVE> vertices = {
        MeshVertexUVE{Math::Vector3UVE{0.0F, 0.0F, 0.0F}, Math::Vector3UVE{0.0F, 0.0F, 1.0F}, 0.0F, 0.0F},
        MeshVertexUVE{Math::Vector3UVE{1.0F, 0.0F, 0.0F}, Math::Vector3UVE{0.0F, 0.0F, 1.0F}, 1.0F, 0.0F},
        MeshVertexUVE{Math::Vector3UVE{0.0F, 1.0F, 0.0F}, Math::Vector3UVE{0.0F, 0.0F, 1.0F}, 0.0F, 1.0F},
    };
    const std::vector<std::uint32_t> indices{0, 1, 2};

    GenerateMeshTangentsUVE(vertices, indices);

    for (const MeshVertexUVE& vertex : vertices) {
        EXPECT_NEAR(vertex.tangent.x, 1.0F, 0.0001F);
        EXPECT_NEAR(vertex.tangent.y, 0.0F, 0.0001F);
        EXPECT_NEAR(vertex.tangent.z, 0.0F, 0.0001F);
        EXPECT_NEAR(Math::DotUVE(vertex.normal, vertex.tangent), 0.0F, 0.0001F);
        EXPECT_FLOAT_EQ(vertex.tangentHandedness, 1.0F);
    }
}

TEST(MeshAssetUVETest, GenerateMeshTangentsUVE_MirroredUvTriangle_ProducesNegativeHandedness) {
    std::vector<MeshVertexUVE> vertices = {
        MeshVertexUVE{Math::Vector3UVE{0.0F, 0.0F, 0.0F}, Math::Vector3UVE{0.0F, 0.0F, 1.0F}, 0.0F, 0.0F},
        MeshVertexUVE{Math::Vector3UVE{1.0F, 0.0F, 0.0F}, Math::Vector3UVE{0.0F, 0.0F, 1.0F}, 0.0F, 1.0F},
        MeshVertexUVE{Math::Vector3UVE{0.0F, 1.0F, 0.0F}, Math::Vector3UVE{0.0F, 0.0F, 1.0F}, 1.0F, 0.0F},
    };
    const std::vector<std::uint32_t> indices{0, 1, 2};

    GenerateMeshTangentsUVE(vertices, indices);

    for (const MeshVertexUVE& vertex : vertices) {
        EXPECT_NEAR(Math::LengthUVE(vertex.tangent), 1.0F, 0.0001F);
        EXPECT_NEAR(Math::DotUVE(vertex.normal, vertex.tangent), 0.0F, 0.0001F);
        EXPECT_FLOAT_EQ(vertex.tangentHandedness, -1.0F);
    }
}

TEST(MeshAssetUVETest, GenerateMeshTangentsUVE_DegenerateUvs_UsesDeterministicFallback) {
    std::vector<MeshVertexUVE> vertices = {
        MeshVertexUVE{Math::Vector3UVE{0.0F, 0.0F, 0.0F}, Math::Vector3UVE{0.0F, 0.0F, 1.0F}, 0.0F, 0.0F},
        MeshVertexUVE{Math::Vector3UVE{1.0F, 0.0F, 0.0F}, Math::Vector3UVE{0.0F, 0.0F, 1.0F}, 0.0F, 0.0F},
        MeshVertexUVE{Math::Vector3UVE{0.0F, 1.0F, 0.0F}, Math::Vector3UVE{0.0F, 0.0F, 1.0F}, 0.0F, 0.0F},
    };
    const std::vector<std::uint32_t> indices{0, 1, 2};

    GenerateMeshTangentsUVE(vertices, indices);

    for (const MeshVertexUVE& vertex : vertices) {
        EXPECT_NEAR(vertex.tangent.x, 1.0F, 0.0001F);
        EXPECT_NEAR(vertex.tangent.y, 0.0F, 0.0001F);
        EXPECT_NEAR(vertex.tangent.z, 0.0F, 0.0001F);
        EXPECT_FLOAT_EQ(vertex.tangentHandedness, 1.0F);
    }
}

TEST(MeshAssetUVETest, TryGenerateMeshTangentsUVE_OverflowedSharedAccumulatorPreservesOutput) {
    std::vector<MeshVertexUVE> vertices = {
        MeshVertexUVE{Math::Vector3UVE{0.0F, 0.0F, 0.0F}, Math::Vector3UVE{0.0F, 0.0F, 1.0F}, 0.0F, 0.0F,
                      Math::Vector3UVE{0.0F, 0.0F, 1.0F}, -1.0F},
        MeshVertexUVE{Math::Vector3UVE{3.0e38F, 0.0F, 0.0F}, Math::Vector3UVE{0.0F, 0.0F, 1.0F}, 1.0F, 0.0F,
                      Math::Vector3UVE{0.0F, 1.0F, 0.0F}, -1.0F},
        MeshVertexUVE{Math::Vector3UVE{0.0F, 1.0F, 0.0F}, Math::Vector3UVE{0.0F, 0.0F, 1.0F}, 0.0F, 1.0F,
                      Math::Vector3UVE{0.0F, 1.0F, 0.0F}, -1.0F},
        MeshVertexUVE{Math::Vector3UVE{3.0e38F, 0.0F, 0.0F}, Math::Vector3UVE{0.0F, 0.0F, 1.0F}, 1.0F, 0.0F,
                      Math::Vector3UVE{0.0F, 1.0F, 0.0F}, -1.0F},
        MeshVertexUVE{Math::Vector3UVE{0.0F, 1.0F, 0.0F}, Math::Vector3UVE{0.0F, 0.0F, 1.0F}, 0.0F, 1.0F,
                      Math::Vector3UVE{0.0F, 1.0F, 0.0F}, -1.0F},
    };
    const std::vector<MeshVertexUVE> original = vertices;
    const std::vector<std::uint32_t> indices{0U, 1U, 2U, 0U, 3U, 4U};

    EXPECT_FALSE(TryGenerateMeshTangentsUVE(vertices, indices));
    ASSERT_EQ(vertices.size(), original.size());
    for (std::size_t index = 0U; index < vertices.size(); ++index) {
        EXPECT_EQ(vertices[index].position, original[index].position);
        EXPECT_EQ(vertices[index].normal, original[index].normal);
        EXPECT_EQ(vertices[index].u, original[index].u);
        EXPECT_EQ(vertices[index].v, original[index].v);
        EXPECT_EQ(vertices[index].tangent, original[index].tangent);
        EXPECT_EQ(vertices[index].tangentHandedness, original[index].tangentHandedness);
    }
}

TEST(MeshAssetUVETest, SaveThenLoad_RoundTripsByteExact) {
    const std::filesystem::path path = "uve_mesh_asset_tests_round_trip.uvemodel";
    std::filesystem::remove(path);
    const MeshAssetUVE original = MakeTestMeshUVE();
    ASSERT_TRUE(SaveMeshAssetUVE(original, path));

    MeshAssetUVE loaded;
    ASSERT_TRUE(LoadMeshAssetUVE(path, loaded));

    ASSERT_EQ(loaded.vertices.size(), original.vertices.size());
    for (std::size_t index = 0; index < original.vertices.size(); ++index) {
        EXPECT_EQ(loaded.vertices[index].position, original.vertices[index].position);
        EXPECT_EQ(loaded.vertices[index].normal, original.vertices[index].normal);
        EXPECT_EQ(loaded.vertices[index].u, original.vertices[index].u);
        EXPECT_EQ(loaded.vertices[index].v, original.vertices[index].v);
        EXPECT_NEAR(Math::LengthUVE(loaded.vertices[index].tangent), 1.0F, 0.0001F);
        EXPECT_NEAR(Math::DotUVE(loaded.vertices[index].normal, loaded.vertices[index].tangent), 0.0F, 0.0001F);
        EXPECT_TRUE(loaded.vertices[index].tangentHandedness == -1.0F ||
                    loaded.vertices[index].tangentHandedness == 1.0F);
    }
    EXPECT_EQ(loaded.indices, original.indices);
    EXPECT_EQ(loaded.localBounds, original.localBounds);

    std::filesystem::remove(path);
}

TEST(MeshAssetUVETest, LoadMeshAssetUVE_WrongAssetKind_FailsCleanlyAndLogsError) {
    const std::filesystem::path path = "uve_mesh_asset_tests_wrong_kind.uveblob";
    std::filesystem::remove(path);
    ASSERT_TRUE(WriteUveFileUVE(path, AssetKindUVE::Blob, {}));

    Debug::LoggerUVE logger;
    logger.Init(Debug::LogLevelUVE::Trace);
    auto memorySink = std::make_unique<Debug::MemorySinkUVE>();
    Debug::MemorySinkUVE* const memorySinkPtr = memorySink.get();
    logger.AddSink(std::move(memorySink));

    MeshAssetUVE mesh;
    EXPECT_FALSE(LoadMeshAssetUVE(path, mesh));

    const std::vector<Debug::LogMessageUVE> messages = memorySinkPtr->GetMessagesUVE();
    const bool foundError =
        std::any_of(messages.begin(), messages.end(), [](const Debug::LogMessageUVE& message) {
            return message.level == Debug::LogLevelUVE::Error && message.message.find("not a mesh file") != std::string::npos;
        });
    EXPECT_TRUE(foundError);

    logger.Shutdown();
    std::filesystem::remove(path);
}

TEST(MeshAssetUVETest, LoadMeshAssetUVE_ImpossibleVertexCountFailsBeforeReserve) {
    const std::filesystem::path path = "uve_mesh_asset_tests_impossible_vertex_count.uvemodel";
    std::filesystem::remove(path);
    const std::uint32_t impossibleVertexCount = std::numeric_limits<std::uint32_t>::max();
    std::vector<std::byte> payload(sizeof(impossibleVertexCount));
    std::memcpy(payload.data(), &impossibleVertexCount, sizeof(impossibleVertexCount));
    ASSERT_TRUE(WriteUveFileUVE(path, AssetKindUVE::Mesh, payload));

    MeshAssetUVE loaded = MakeTestMeshUVE();
    const MeshAssetUVE original = loaded;
    EXPECT_FALSE(LoadMeshAssetUVE(path, loaded));
    EXPECT_EQ(loaded.vertices.size(), original.vertices.size());
    EXPECT_EQ(loaded.indices, original.indices);
    EXPECT_EQ(loaded.localBounds, original.localBounds);
    std::filesystem::remove(path);
}

TEST(MeshAssetUVETest, LoadMeshAssetUVE_MissingFile_ReturnsFalse) {
    const std::filesystem::path path = "uve_mesh_asset_tests_nonexistent.uvemodel";
    std::filesystem::remove(path);

    MeshAssetUVE mesh;
    EXPECT_FALSE(LoadMeshAssetUVE(path, mesh));
}

TEST(MeshAssetUVETest, LoadMeshAssetUVE_OutOfBoundsIndex_FailsAndLogsError) {
    const std::filesystem::path path = "uve_mesh_asset_tests_bad_index.uvemodel";
    std::filesystem::remove(path);

    MeshAssetUVE invalidMesh = MakeTestMeshUVE();
    invalidMesh.indices = {0, 1, 5}; // 5 is out of bounds - only 3 vertices exist
    ASSERT_TRUE(SaveMeshAssetUVE(invalidMesh, path));

    Debug::LoggerUVE logger;
    logger.Init(Debug::LogLevelUVE::Trace);
    auto memorySink = std::make_unique<Debug::MemorySinkUVE>();
    Debug::MemorySinkUVE* const memorySinkPtr = memorySink.get();
    logger.AddSink(std::move(memorySink));

    MeshAssetUVE loaded;
    EXPECT_FALSE(LoadMeshAssetUVE(path, loaded));

    const std::vector<Debug::LogMessageUVE> messages = memorySinkPtr->GetMessagesUVE();
    const bool foundError =
        std::any_of(messages.begin(), messages.end(), [](const Debug::LogMessageUVE& message) {
            return message.level == Debug::LogLevelUVE::Error &&
                   message.message.find("out-of-bounds index") != std::string::npos;
        });
    EXPECT_TRUE(foundError);

    logger.Shutdown();
    std::filesystem::remove(path);
}

TEST(MeshAssetUVETest, EndToEnd_RegisterLoaderThenLoadUVE_ReachesLoadedWithMatchingData) {
    const std::filesystem::path path = "uve_mesh_asset_tests_end_to_end.uvemodel";
    std::filesystem::remove(path);
    const MeshAssetUVE original = MakeTestMeshUVE();
    ASSERT_TRUE(SaveMeshAssetUVE(original, path));

    Threading::ThreadPoolUVE threadPool(2);
    Events::EventSystemUVE eventSystem;
    AssetDatabaseUVE assetDatabase;
    AssetManagerUVE assetManager(threadPool, eventSystem);
    assetManager.RegisterLoaderUVE<MeshAssetUVE>(&LoadMeshAssetUVE);

    const AssetGuidUVE guid = assetDatabase.RegisterUVE(path);
    const AssetHandleUVE<MeshAssetUVE> handle = assetManager.LoadUVE<MeshAssetUVE>(guid, assetDatabase);

    bool ready = false;
    for (int iteration = 0; iteration < 200000 && !ready; ++iteration) {
        ready = handle.IsReadyUVE() || handle.HasFailedUVE();
        if (!ready) {
            std::this_thread::yield();
        }
    }
    ASSERT_TRUE(ready);
    ASSERT_TRUE(handle.IsReadyUVE());
    const MeshAssetUVE* const loaded = handle.TryGetUVE();
    ASSERT_NE(loaded, nullptr);
    EXPECT_EQ(loaded->indices, original.indices);
    ASSERT_EQ(loaded->vertices.size(), original.vertices.size());

    std::filesystem::remove(path);
}

} // namespace
} // namespace UVE::Asset::Tests
