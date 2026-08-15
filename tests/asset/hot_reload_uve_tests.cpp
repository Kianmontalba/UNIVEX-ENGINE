// Copyright (c) 2026 UniVex Studios. All Rights Reserved.


#include "uve/asset/hot_reload_uve.h"

#include <algorithm>
#include <atomic>
#include <chrono>
#include <filesystem>
#include <memory>
#include <string>
#include <thread>
#include <vector>

#include <gtest/gtest.h>

#include "uve/asset/asset_database_uve.h"
#include "uve/asset/asset_handle_uve.h"
#include "uve/asset/asset_manager_uve.h"
#include "uve/asset/asset_reloaded_event_uve.h"
#include "uve/asset/blob_asset_uve.h"
#include "uve/asset/uve_file_envelope_uve.h"
#include "uve/debug/log_sink_uve.h"
#include "uve/debug/logger_uve.h"
#include "uve/events/event_system_uve.h"
#include "uve/threading/thread_pool_uve.h"

namespace UVE::Asset::Tests {
namespace {

[[nodiscard]] std::vector<std::byte> MakePayloadUVE(std::string_view text) {
    const auto* const bytes = reinterpret_cast<const std::byte*>(text.data());
    return std::vector<std::byte>(bytes, bytes + text.size());
}

[[nodiscard]] bool LoadBlobUVE(const std::filesystem::path& path, BlobAssetUVE& outValue) {
    auto result = ReadUveFileUVE(path);
    if (!result.has_value()) {
        return false;
    }
    outValue = std::move(result->second);
    return true;
}

template <typename T>
[[nodiscard]] bool WaitForTerminalStateUVE(const AssetHandleUVE<T>& handle, int maxIterations = 200000) {
    for (int iteration = 0; iteration < maxIterations; ++iteration) {
        if (handle.IsReadyUVE() || handle.HasFailedUVE()) {
            return true;
        }
        std::this_thread::yield();
    }
    return false;
}

class HotReloadUVETest : public ::testing::Test {
protected:
    Threading::ThreadPoolUVE threadPool{2};
    Events::EventSystemUVE eventSystem;
    AssetDatabaseUVE assetDatabase;
    HotReloadUVE hotReload{eventSystem, 0.05};
    AssetManagerUVE assetManager{threadPool, eventSystem, &hotReload};

    void SetUp() override { assetManager.RegisterLoaderUVE<BlobAssetUVE>(LoadBlobUVE); }
};

TEST_F(HotReloadUVETest, PollUVE_DetectsOnDiskChange_ReloadsAssetAndPublishesEvent) {
    const std::filesystem::path path = "uve_hot_reload_tests_change.uveblob";
    std::filesystem::remove(path);
    ASSERT_TRUE(WriteUveFileUVE(path, AssetKindUVE::Blob, MakePayloadUVE("version1")));
    const AssetGuidUVE guid = assetDatabase.RegisterUVE(path);

    const AssetHandleUVE<BlobAssetUVE> handle = assetManager.LoadUVE<BlobAssetUVE>(guid, assetDatabase);
    ASSERT_TRUE(WaitForTerminalStateUVE(handle));
    ASSERT_TRUE(handle.IsReadyUVE());

    // First poll (well over the 0.05s interval) merely establishes the tracked baseline mtime -
    // it must not itself be treated as a change.
    hotReload.PollUVE(assetManager, assetDatabase, 10.0);

    ASSERT_TRUE(WriteUveFileUVE(path, AssetKindUVE::Blob, MakePayloadUVE("version2")));
    const auto farFuture = std::filesystem::file_time_type::clock::now() + std::chrono::hours(1);
    std::filesystem::last_write_time(path, farFuture);

    std::atomic<bool> reloadEventReceived{false};
    eventSystem.Subscribe<AssetReloadedEventUVE>([&reloadEventReceived, guid](const AssetReloadedEventUVE& event) {
        if (event.guid == guid) {
            reloadEventReceived = true;
        }
    });

    hotReload.PollUVE(assetManager, assetDatabase, 10.0);
    const auto reloadDeadline = std::chrono::steady_clock::now() + std::chrono::seconds{2};
    bool contentReloaded = false;
    while (std::chrono::steady_clock::now() < reloadDeadline &&
           (!reloadEventReceived.load() || !contentReloaded)) {
        eventSystem.DispatchQueuedUVE();
        if (handle.IsReadyUVE()) {
            const BlobAssetUVE* const blob = handle.TryGetUVE();
            if (blob != nullptr) {
                const std::string content(reinterpret_cast<const char*>(blob->data()), blob->size());
                contentReloaded = content == "version2";
            }
        }
        std::this_thread::yield();
    }
    EXPECT_TRUE(reloadEventReceived.load());
    EXPECT_TRUE(contentReloaded);

    std::filesystem::remove(path);
}

TEST_F(HotReloadUVETest, PollUVE_BelowConfiguredInterval_NeverReloads) {
    HotReloadUVE localHotReload(eventSystem, 1000.0);
    AssetManagerUVE localAssetManager(threadPool, eventSystem, &localHotReload);
    localAssetManager.RegisterLoaderUVE<BlobAssetUVE>(LoadBlobUVE);

    const std::filesystem::path path = "uve_hot_reload_tests_below_interval.uveblob";
    std::filesystem::remove(path);
    ASSERT_TRUE(WriteUveFileUVE(path, AssetKindUVE::Blob, MakePayloadUVE("version1")));
    const AssetGuidUVE guid = assetDatabase.RegisterUVE(path);

    const AssetHandleUVE<BlobAssetUVE> handle = localAssetManager.LoadUVE<BlobAssetUVE>(guid, assetDatabase);
    ASSERT_TRUE(WaitForTerminalStateUVE(handle));

    ASSERT_TRUE(WriteUveFileUVE(path, AssetKindUVE::Blob, MakePayloadUVE("version2")));
    const auto farFuture = std::filesystem::file_time_type::clock::now() + std::chrono::hours(1);
    std::filesystem::last_write_time(path, farFuture);

    localHotReload.PollUVE(localAssetManager, assetDatabase, 1.0); // far below the 1000s interval

    const BlobAssetUVE* const blob = handle.TryGetUVE();
    ASSERT_NE(blob, nullptr);
    const std::string content(reinterpret_cast<const char*>(blob->data()), blob->size());
    EXPECT_EQ(content, "version1");

    std::filesystem::remove(path);
}

TEST_F(HotReloadUVETest, PollUVE_TrackedFileDeleted_LogsWarningAndDoesNotCrash) {
    const std::filesystem::path path = "uve_hot_reload_tests_deleted.uveblob";
    std::filesystem::remove(path);
    ASSERT_TRUE(WriteUveFileUVE(path, AssetKindUVE::Blob, MakePayloadUVE("version1")));
    const AssetGuidUVE guid = assetDatabase.RegisterUVE(path);

    const AssetHandleUVE<BlobAssetUVE> handle = assetManager.LoadUVE<BlobAssetUVE>(guid, assetDatabase);
    ASSERT_TRUE(WaitForTerminalStateUVE(handle));
    std::filesystem::remove(path);

    Debug::LoggerUVE logger;
    logger.Init(Debug::LogLevelUVE::Trace);
    auto memorySink = std::make_unique<Debug::MemorySinkUVE>();
    Debug::MemorySinkUVE* const memorySinkPtr = memorySink.get();
    logger.AddSink(std::move(memorySink));

    hotReload.PollUVE(assetManager, assetDatabase, 10.0);

    const std::vector<Debug::LogMessageUVE> messages = memorySinkPtr->GetMessagesUVE();
    const bool foundWarning =
        std::any_of(messages.begin(), messages.end(), [](const Debug::LogMessageUVE& message) {
            return message.level == Debug::LogLevelUVE::Warning &&
                   message.message.find("missing") != std::string::npos;
        });
    EXPECT_TRUE(foundWarning);

    logger.Shutdown();
}

} // namespace
} // namespace UVE::Asset::Tests
