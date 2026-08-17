// Copyright (c) 2026 UniVex Studios. All Rights Reserved.


#include "uve/asset/asset_manager_uve.h"

#include <algorithm>
#include <atomic>
#include <filesystem>
#include <memory>
#include <string>
#include <thread>
#include <vector>

#include <gtest/gtest.h>

#include "uve/asset/asset_database_uve.h"
#include "uve/asset/asset_handle_uve.h"
#include "uve/asset/asset_load_completed_event_uve.h"
#include "uve/asset/blob_asset_uve.h"
#include "uve/asset/uve_file_envelope_uve.h"
#include "uve/debug/log_sink_uve.h"
#include "uve/debug/logger_uve.h"
#include "uve/events/event_system_uve.h"
#include "uve/threading/thread_pool_uve.h"

namespace UVE::Asset::Tests {
namespace {

[[nodiscard]] bool LoadBlobUVE(const std::filesystem::path& path, BlobAssetUVE& outValue) {
    auto result = ReadUveFileUVE(path);
    if (!result.has_value()) {
        return false;
    }
    outValue = std::move(result->second);
    return true;
}

/// Blocks (via bounded busy-poll, never a fixed sleep) until `handle` reaches a terminal state
/// (Loaded or Failed), or `maxIterations` is exceeded. Returns false on timeout.
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

/// A handle reaching a terminal state only guarantees AssetManagerUVE's record was updated - the
/// worker thread's subsequent UVE_ERROR()/QueueEvent() calls in the same function are not
/// synchronized with that state flip, so log assertions need their own bounded retry rather than
/// checking the sink exactly once.
[[nodiscard]] bool WaitForLogMessageUVE(Debug::MemorySinkUVE& sink, Debug::LogLevelUVE level,
                                         std::string_view substring, int maxIterations = 200000) {
    for (int iteration = 0; iteration < maxIterations; ++iteration) {
        const std::vector<Debug::LogMessageUVE> messages = sink.GetMessagesUVE();
        const bool found =
            std::any_of(messages.begin(), messages.end(), [level, substring](const Debug::LogMessageUVE& message) {
                return message.level == level && message.message.find(substring) != std::string::npos;
            });
        if (found) {
            return true;
        }
        std::this_thread::yield();
    }
    return false;
}

class AssetManagerUVETest : public ::testing::Test {
protected:
    Threading::ThreadPoolUVE threadPool{2};
    Events::EventSystemUVE eventSystem;
    AssetDatabaseUVE assetDatabase;
    AssetManagerUVE assetManager{threadPool, eventSystem};

    void SetUp() override { assetManager.RegisterLoaderUVE<BlobAssetUVE>(LoadBlobUVE); }
};

TEST_F(AssetManagerUVETest, LoadUVE_RealFile_BecomesReadyWithMatchingBytes) {
    const std::filesystem::path path = "uve_asset_manager_tests_basic.uveblob";
    std::filesystem::remove(path);
    const std::string text = "hello asset manager";
    const auto* const textBytes = reinterpret_cast<const std::byte*>(text.data());
    ASSERT_TRUE(WriteUveFileUVE(path, AssetKindUVE::Blob, std::vector<std::byte>(textBytes, textBytes + text.size())));

    const AssetGuidUVE guid = assetDatabase.RegisterUVE(path);
    const AssetHandleUVE<BlobAssetUVE> handle = assetManager.LoadUVE<BlobAssetUVE>(guid, assetDatabase);

    ASSERT_TRUE(WaitForTerminalStateUVE(handle));
    ASSERT_TRUE(handle.IsReadyUVE());
    const BlobAssetUVE* const blob = handle.TryGetUVE();
    ASSERT_NE(blob, nullptr);
    const std::string roundTripped(reinterpret_cast<const char*>(blob->data()), blob->size());
    EXPECT_EQ(roundTripped, text);

    std::filesystem::remove(path);
}

TEST_F(AssetManagerUVETest, LoadUVE_CalledTwiceForSameGuid_LoadsOnlyOnce) {
    const std::filesystem::path path = "uve_asset_manager_tests_shared.uveblob";
    std::filesystem::remove(path);
    ASSERT_TRUE(WriteUveFileUVE(path, AssetKindUVE::Blob, {}));
    const AssetGuidUVE guid = assetDatabase.RegisterUVE(path);

    static std::atomic<int> loadCount{0};
    loadCount = 0;
    assetManager.RegisterLoaderUVE<BlobAssetUVE>([](const std::filesystem::path& loaderPath, BlobAssetUVE& out) {
        ++loadCount;
        return LoadBlobUVE(loaderPath, out);
    });

    const AssetHandleUVE<BlobAssetUVE> first = assetManager.LoadUVE<BlobAssetUVE>(guid, assetDatabase);
    const AssetHandleUVE<BlobAssetUVE> second = assetManager.LoadUVE<BlobAssetUVE>(guid, assetDatabase);
    ASSERT_TRUE(WaitForTerminalStateUVE(first));
    ASSERT_TRUE(WaitForTerminalStateUVE(second));

    EXPECT_EQ(loadCount.load(), 1);

    std::filesystem::remove(path);
}

TEST_F(AssetManagerUVETest, CollectGarbageUVE_DoesNotUnloadWhileAnotherHandleIsAlive) {
    const std::filesystem::path path = "uve_asset_manager_tests_refcount.uveblob";
    std::filesystem::remove(path);
    ASSERT_TRUE(WriteUveFileUVE(path, AssetKindUVE::Blob, {}));
    const AssetGuidUVE guid = assetDatabase.RegisterUVE(path);

    AssetHandleUVE<BlobAssetUVE> keepAlive = assetManager.LoadUVE<BlobAssetUVE>(guid, assetDatabase);
    ASSERT_TRUE(WaitForTerminalStateUVE(keepAlive));
    {
        const AssetHandleUVE<BlobAssetUVE> temporary = assetManager.LoadUVE<BlobAssetUVE>(guid, assetDatabase);
        static_cast<void>(temporary);
    }

    assetManager.CollectGarbageUVE();
    EXPECT_EQ(assetManager.GetLoadedAssetCountUVE(), 1U);
    EXPECT_TRUE(keepAlive.IsReadyUVE());

    std::filesystem::remove(path);
}

TEST_F(AssetManagerUVETest, CollectGarbageUVE_UnloadsOnceLastHandleIsReleased) {
    const std::filesystem::path path = "uve_asset_manager_tests_gc.uveblob";
    std::filesystem::remove(path);
    ASSERT_TRUE(WriteUveFileUVE(path, AssetKindUVE::Blob, {}));
    const AssetGuidUVE guid = assetDatabase.RegisterUVE(path);

    {
        const AssetHandleUVE<BlobAssetUVE> handle = assetManager.LoadUVE<BlobAssetUVE>(guid, assetDatabase);
        ASSERT_TRUE(WaitForTerminalStateUVE(handle));
        EXPECT_EQ(assetManager.GetLoadedAssetCountUVE(), 1U);
    }

    assetManager.CollectGarbageUVE();
    EXPECT_EQ(assetManager.GetLoadedAssetCountUVE(), 0U);

    std::filesystem::remove(path);
}

TEST_F(AssetManagerUVETest, LoadUVE_UnknownGuid_BecomesFailedAndLogsError) {
    Debug::LoggerUVE logger;
    logger.Init(Debug::LogLevelUVE::Trace);
    auto memorySink = std::make_unique<Debug::MemorySinkUVE>();
    Debug::MemorySinkUVE* const memorySinkPtr = memorySink.get();
    logger.AddSink(std::move(memorySink));

    const AssetHandleUVE<BlobAssetUVE> handle =
        assetManager.LoadUVE<BlobAssetUVE>(AssetGuidUVE{424242}, assetDatabase);
    ASSERT_TRUE(WaitForTerminalStateUVE(handle));
    EXPECT_TRUE(handle.HasFailedUVE());
    EXPECT_EQ(handle.TryGetUVE(), nullptr);
    EXPECT_EQ(handle.GetFailureReasonUVE(), "asset database resolved no path");
    EXPECT_EQ(assetManager.GetFailureReasonUVE(AssetGuidUVE{424242}), "asset database resolved no path");

    EXPECT_TRUE(WaitForLogMessageUVE(*memorySinkPtr, Debug::LogLevelUVE::Error, "failed to load asset"));

    logger.Shutdown();
}

TEST_F(AssetManagerUVETest, LoadUVE_LoaderRejectsExistingFile_ReportsReasonAndSuccessfulReloadClearsIt) {
    const std::filesystem::path path = "uve_asset_manager_tests_failure_reason.uveblob";
    std::filesystem::remove(path);
    ASSERT_TRUE(WriteUveFileUVE(path, AssetKindUVE::Blob, {}));
    const AssetGuidUVE guid = assetDatabase.RegisterUVE(path);

    assetManager.RegisterLoaderUVE<BlobAssetUVE>([](const std::filesystem::path&, BlobAssetUVE&) { return false; });
    const AssetHandleUVE<BlobAssetUVE> handle = assetManager.LoadUVE<BlobAssetUVE>(guid, assetDatabase);
    ASSERT_TRUE(WaitForTerminalStateUVE(handle));
    EXPECT_TRUE(handle.HasFailedUVE());
    EXPECT_EQ(handle.GetFailureReasonUVE(), "registered asset loader rejected file");

    assetManager.RegisterLoaderUVE<BlobAssetUVE>(LoadBlobUVE);
    assetManager.ReloadUVE(guid, assetDatabase);
    ASSERT_TRUE(WaitForTerminalStateUVE(handle));
    EXPECT_TRUE(handle.IsReadyUVE());
    EXPECT_TRUE(handle.GetFailureReasonUVE().empty());

    std::filesystem::remove(path);
}

TEST_F(AssetManagerUVETest, LoadCompletion_PublishesAssetLoadCompletedEvent) {
    const std::filesystem::path path = "uve_asset_manager_tests_event.uveblob";
    std::filesystem::remove(path);
    ASSERT_TRUE(WriteUveFileUVE(path, AssetKindUVE::Blob, {}));
    const AssetGuidUVE guid = assetDatabase.RegisterUVE(path);

    std::atomic<bool> received{false};
    std::atomic<bool> receivedSuccess{false};
    eventSystem.Subscribe<AssetLoadCompletedEventUVE>([&](const AssetLoadCompletedEventUVE& event) {
        if (event.guid == guid) {
            received = true;
            receivedSuccess = event.success;
        }
    });

    const AssetHandleUVE<BlobAssetUVE> handle = assetManager.LoadUVE<BlobAssetUVE>(guid, assetDatabase);
    ASSERT_TRUE(WaitForTerminalStateUVE(handle));

    for (int iteration = 0; iteration < 200000 && !received.load(); ++iteration) {
        eventSystem.DispatchQueuedUVE();
        std::this_thread::yield();
    }

    EXPECT_TRUE(received.load());
    EXPECT_TRUE(receivedSuccess.load());

    std::filesystem::remove(path);
}

} // namespace
} // namespace UVE::Asset::Tests
