// Copyright (c) 2026 UniVex Studios. All Rights Reserved.

#include "uve/asset/project_change_watcher_uve.h"

#include <algorithm>
#include <cstdint>
#include <filesystem>
#include <fstream>
#include <optional>
#include <string>
#include <vector>

#include <gtest/gtest.h>

#include "uve/asset/asset_content_fingerprint_uve.h"
#include "uve/asset/asset_database_uve.h"
#include "uve/asset/derived_artifact_cache_uve.h"
#include "uve/asset/i_derived_artifact_cache_uve.h"
#include "uve/asset/project_change_watcher_uve.h"

namespace UVE::Asset::Tests {
namespace {

void WriteFixtureFileUVE(const std::filesystem::path& path, const std::string_view contents) {
    if (!path.parent_path().empty()) {
        std::filesystem::create_directories(path.parent_path());
    }
    std::ofstream file(path, std::ios::binary | std::ios::trunc);
    ASSERT_TRUE(file.is_open());
    file << contents;
    ASSERT_TRUE(file.good());
}

[[nodiscard]] const ProjectFileChangeUVE* FindChangeUVE(const ProjectChangeSnapshotUVE& snapshot,
                                                         const std::string_view relativePath,
                                                         const ProjectFileChangeKindUVE kind) {
    const auto found = std::find_if(snapshot.changes.begin(), snapshot.changes.end(),
                                    [relativePath, kind](const ProjectFileChangeUVE& change) {
                                        return change.relativePath.generic_string() == relativePath &&
                                               change.kind == kind;
                                    });
    return found == snapshot.changes.end() ? nullptr : &*found;
}

class RecordingDerivedArtifactCacheUVE final : public IDerivedArtifactCacheUVE {
public:
    [[nodiscard]] std::optional<DerivedArtifactCacheRecordUVE>
    LoadImportRecordUVE(const std::filesystem::path&) const override {
        return std::nullopt;
    }

    [[nodiscard]] bool StoreImportRecordUVE(const std::filesystem::path&,
                                             const DerivedArtifactCacheRecordUVE&) override {
        return false;
    }

    [[nodiscard]] std::size_t MarkStaleForSourceUVE(const std::filesystem::path& sourcePath) override {
        markedSources.push_back(sourcePath.lexically_normal());
        return staleRecordsPerChange;
    }

    [[nodiscard]] std::filesystem::path GetCacheRootUVE() const override { return {}; }

    std::size_t staleRecordsPerChange = 0U;
    std::vector<std::filesystem::path> markedSources;
};

class ProjectChangeWatcherUVETest : public ::testing::Test {
protected:
    const std::filesystem::path root = "uve_project_change_watcher_tests";
    AssetDatabaseUVE assetDatabase;
    RecordingDerivedArtifactCacheUVE derivedArtifactCache;

    void SetUp() override { std::filesystem::remove_all(root); }
    void TearDown() override { std::filesystem::remove_all(root); }
};

TEST_F(ProjectChangeWatcherUVETest, PollNowUVE_InitialBaselineEmitsNoChanges) {
    WriteFixtureFileUVE(root / "existing.txt", "existing");
    ProjectChangeWatcherUVE watcher(root, 1.0, 16U);

    ASSERT_TRUE(watcher.PollNowUVE(assetDatabase, derivedArtifactCache));
    const ProjectChangeSnapshotUVE snapshot = watcher.GetSnapshotUVE();
    EXPECT_EQ(snapshot.successfulScanGeneration, 1U);
    EXPECT_TRUE(snapshot.changes.empty());
    EXPECT_FALSE(snapshot.rescanRequired);
    EXPECT_TRUE(derivedArtifactCache.markedSources.empty());
}

TEST_F(ProjectChangeWatcherUVETest, PollNowUVE_NormalizesRelativeTrailingSeparatorBeforeBaselineScan) {
    WriteFixtureFileUVE(root / "existing.txt", "existing");
    const std::filesystem::path configuredRoot = root.generic_string() + "/";
    ProjectChangeWatcherUVE watcher(configuredRoot, 1.0, 16U);

    ASSERT_TRUE(watcher.PollNowUVE(assetDatabase, derivedArtifactCache));
    const ProjectChangeSnapshotUVE snapshot = watcher.GetSnapshotUVE();
    EXPECT_EQ(snapshot.contentRoot, root);
    EXPECT_EQ(snapshot.successfulScanGeneration, 1U);
    EXPECT_TRUE(snapshot.changes.empty());
    EXPECT_FALSE(snapshot.lastScanDiagnostic.has_value());
}

TEST_F(ProjectChangeWatcherUVETest, PollUVE_DefersScanUntilConfiguredInterval) {
    const std::filesystem::path source = root / "tracked.txt";
    WriteFixtureFileUVE(source, "before");
    ProjectChangeWatcherUVE watcher(root, 1.0, 16U);
    ASSERT_TRUE(watcher.PollNowUVE(assetDatabase, derivedArtifactCache));

    WriteFixtureFileUVE(source, "after");
    EXPECT_FALSE(watcher.PollUVE(0.4, assetDatabase, derivedArtifactCache));
    EXPECT_TRUE(watcher.GetSnapshotUVE().changes.empty());
    EXPECT_TRUE(watcher.PollUVE(0.6, assetDatabase, derivedArtifactCache));

    const ProjectChangeSnapshotUVE snapshot = watcher.GetSnapshotUVE();
    ASSERT_NE(FindChangeUVE(snapshot, "tracked.txt", ProjectFileChangeKindUVE::Modified), nullptr);
}

TEST_F(ProjectChangeWatcherUVETest, PollNowUVE_JournalsCreateModifyAndRemoveEvents) {
    const std::filesystem::path source = root / "tracked.txt";
    ProjectChangeWatcherUVE watcher(root, 60.0, 16U);
    ASSERT_TRUE(watcher.PollNowUVE(assetDatabase, derivedArtifactCache));

    WriteFixtureFileUVE(source, "created");
    ASSERT_TRUE(watcher.PollNowUVE(assetDatabase, derivedArtifactCache));
    ProjectChangeSnapshotUVE snapshot = watcher.GetSnapshotUVE();
    ASSERT_NE(FindChangeUVE(snapshot, "tracked.txt", ProjectFileChangeKindUVE::Created), nullptr);
    watcher.AcknowledgeThroughUVE(snapshot.latestSequence);

    WriteFixtureFileUVE(source, "modified");
    ASSERT_TRUE(watcher.PollNowUVE(assetDatabase, derivedArtifactCache));
    snapshot = watcher.GetSnapshotUVE();
    ASSERT_NE(FindChangeUVE(snapshot, "tracked.txt", ProjectFileChangeKindUVE::Modified), nullptr);
    watcher.AcknowledgeThroughUVE(snapshot.latestSequence);

    ASSERT_TRUE(std::filesystem::remove(source));
    ASSERT_TRUE(watcher.PollNowUVE(assetDatabase, derivedArtifactCache));
    snapshot = watcher.GetSnapshotUVE();
    ASSERT_NE(FindChangeUVE(snapshot, "tracked.txt", ProjectFileChangeKindUVE::Removed), nullptr);
}

TEST_F(ProjectChangeWatcherUVETest, RemovedEvent_RetainsGuidCapturedByLastSuccessfulBaseline) {
    const std::filesystem::path source = root / "registered.txt";
    WriteFixtureFileUVE(source, "registered");
    const AssetGuidUVE guid = assetDatabase.RegisterUVE(source);
    ASSERT_NE(guid, kInvalidAssetGuidUVE);

    ProjectChangeWatcherUVE watcher(root, 0.0, 16U);
    ASSERT_TRUE(watcher.PollNowUVE(assetDatabase, derivedArtifactCache));
    ASSERT_TRUE(std::filesystem::remove(source));
    ASSERT_TRUE(watcher.PollNowUVE(assetDatabase, derivedArtifactCache));

    const ProjectChangeSnapshotUVE snapshot = watcher.GetSnapshotUVE();
    const ProjectFileChangeUVE* const removed =
        FindChangeUVE(snapshot, "registered.txt", ProjectFileChangeKindUVE::Removed);
    ASSERT_NE(removed, nullptr);
    EXPECT_EQ(removed->registeredAssetGuid, guid);
}

TEST_F(ProjectChangeWatcherUVETest, PollNowUVE_UsesDeterministicLexicalChangeOrdering) {
    ProjectChangeWatcherUVE watcher(root, 0.0, 16U);
    ASSERT_TRUE(watcher.PollNowUVE(assetDatabase, derivedArtifactCache));

    WriteFixtureFileUVE(root / "zeta.txt", "zeta");
    WriteFixtureFileUVE(root / "alpha.txt", "alpha");
    ASSERT_TRUE(watcher.PollNowUVE(assetDatabase, derivedArtifactCache));

    const ProjectChangeSnapshotUVE snapshot = watcher.GetSnapshotUVE();
    ASSERT_EQ(snapshot.changes.size(), 2U);
    EXPECT_EQ(snapshot.changes[0].relativePath.generic_string(), "alpha.txt");
    EXPECT_EQ(snapshot.changes[1].relativePath.generic_string(), "zeta.txt");
    EXPECT_LT(snapshot.changes[0].sequence, snapshot.changes[1].sequence);
}

TEST_F(ProjectChangeWatcherUVETest, PollNowUVE_DoesNotFollowOrJournalSymlinkEntries) {
    const std::filesystem::path outside = root.parent_path() / "uve_project_change_watcher_tests_outside";
    std::filesystem::remove_all(outside);
    WriteFixtureFileUVE(root / "safe.txt", "safe");
    WriteFixtureFileUVE(outside / "outside.txt", "outside");

    ProjectChangeWatcherUVE watcher(root, 0.0, 16U);
    ASSERT_TRUE(watcher.PollNowUVE(assetDatabase, derivedArtifactCache));

    std::error_code errorCode;
    std::filesystem::create_directory_symlink(outside, root / "outside-link", errorCode);
    if (errorCode) {
        std::filesystem::remove_all(outside);
        GTEST_SKIP() << "Host does not permit test symlink creation: " << errorCode.message();
    }

    ASSERT_TRUE(watcher.PollNowUVE(assetDatabase, derivedArtifactCache));
    const ProjectChangeSnapshotUVE snapshot = watcher.GetSnapshotUVE();
    EXPECT_TRUE(snapshot.changes.empty());
    EXPECT_TRUE(derivedArtifactCache.markedSources.empty());
    std::filesystem::remove_all(outside);
}

TEST_F(ProjectChangeWatcherUVETest, MissingRoot_BaselinesAsEmptyAndLaterCreationIsObserved) {
    ProjectChangeWatcherUVE watcher(root, 0.0, 16U);
    ASSERT_TRUE(watcher.PollNowUVE(assetDatabase, derivedArtifactCache));
    ProjectChangeSnapshotUVE snapshot = watcher.GetSnapshotUVE();
    EXPECT_TRUE(snapshot.changes.empty());
    EXPECT_EQ(snapshot.successfulScanGeneration, 1U);

    WriteFixtureFileUVE(root / "later.txt", "later");
    ASSERT_TRUE(watcher.PollNowUVE(assetDatabase, derivedArtifactCache));
    snapshot = watcher.GetSnapshotUVE();
    ASSERT_NE(FindChangeUVE(snapshot, "later.txt", ProjectFileChangeKindUVE::Created), nullptr);
}

TEST_F(ProjectChangeWatcherUVETest, FailedScan_RetainsLastSuccessfulBaselineAndReportsDiagnostic) {
    const std::filesystem::path outside = root.parent_path() / "uve_project_change_watcher_tests_failure_outside";
    std::filesystem::remove_all(outside);
    WriteFixtureFileUVE(root / "tracked.txt", "tracked");
    WriteFixtureFileUVE(outside / "outside.txt", "outside");

    ProjectChangeWatcherUVE watcher(root, 0.0, 16U);
    ASSERT_TRUE(watcher.PollNowUVE(assetDatabase, derivedArtifactCache));
    const ProjectChangeSnapshotUVE goodSnapshot = watcher.GetSnapshotUVE();

    std::filesystem::remove_all(root);
    std::error_code errorCode;
    std::filesystem::create_directory_symlink(outside, root, errorCode);
    if (errorCode) {
        std::filesystem::remove_all(outside);
        GTEST_SKIP() << "Host does not permit root symlink creation: " << errorCode.message();
    }

    EXPECT_FALSE(watcher.PollNowUVE(assetDatabase, derivedArtifactCache));
    const ProjectChangeSnapshotUVE failedSnapshot = watcher.GetSnapshotUVE();
    EXPECT_EQ(failedSnapshot.successfulScanGeneration, goodSnapshot.successfulScanGeneration);
    EXPECT_TRUE(failedSnapshot.changes.empty());
    ASSERT_TRUE(failedSnapshot.lastScanDiagnostic.has_value());
    EXPECT_FALSE(failedSnapshot.lastScanDiagnostic->empty());

    std::filesystem::remove(root, errorCode);
    std::filesystem::remove_all(outside);
}

TEST_F(ProjectChangeWatcherUVETest, RingBufferOverflow_PreservesNewestEntriesAndContinuesTargetedStaleMarking) {
    ProjectChangeWatcherUVE watcher(root, 0.0, 1U);
    derivedArtifactCache.staleRecordsPerChange = 3U;
    ASSERT_TRUE(watcher.PollNowUVE(assetDatabase, derivedArtifactCache));

    WriteFixtureFileUVE(root / "first.txt", "first");
    ASSERT_TRUE(watcher.PollNowUVE(assetDatabase, derivedArtifactCache));
    WriteFixtureFileUVE(root / "second.txt", "second");
    ASSERT_TRUE(watcher.PollNowUVE(assetDatabase, derivedArtifactCache));
    WriteFixtureFileUVE(root / "first.txt", "first modified");
    ASSERT_TRUE(watcher.PollNowUVE(assetDatabase, derivedArtifactCache));

    const ProjectChangeSnapshotUVE snapshot = watcher.GetSnapshotUVE();
    ASSERT_EQ(snapshot.changes.size(), 1U);
    EXPECT_EQ(snapshot.changes.front().relativePath.generic_string(), "first.txt");
    EXPECT_EQ(snapshot.changes.front().kind, ProjectFileChangeKindUVE::Modified);
    EXPECT_EQ(snapshot.changes.front().staleArtifactCount, 3U);
    EXPECT_TRUE(snapshot.rescanRequired);
    ASSERT_EQ(derivedArtifactCache.markedSources.size(), 3U);
    EXPECT_EQ(derivedArtifactCache.markedSources.back(), (root / "first.txt").lexically_normal());
}

TEST_F(ProjectChangeWatcherUVETest, ModifiedSource_MarksMatchingDerivedRecordStaleAndReportsCount) {
    const std::filesystem::path contentRoot = root / "content";
    const std::filesystem::path cacheRoot = root / "DerivedData" / "Import";
    const std::filesystem::path source = contentRoot / "source.custom";
    const std::filesystem::path destination = contentRoot / "output.custom";
    WriteFixtureFileUVE(source, "before");
    WriteFixtureFileUVE(destination, "output");

    const std::optional<AssetContentFingerprintUVE> sourceFingerprint = ComputeAssetContentFingerprintUVE(source);
    const std::optional<AssetContentFingerprintUVE> destinationFingerprint =
        ComputeAssetContentFingerprintUVE(destination);
    ASSERT_TRUE(sourceFingerprint.has_value());
    ASSERT_TRUE(destinationFingerprint.has_value());

    DerivedArtifactCacheUVE cache(cacheRoot);
    const DerivedArtifactCacheRecordUVE record{kDerivedArtifactCacheSchemaVersionUVE,
                                                source,
                                                destination,
                                                *sourceFingerprint,
                                                *destinationFingerprint,
                                                "generic-v1",
                                                AssetGuidUVE{77U}};
    ASSERT_TRUE(cache.StoreImportRecordUVE(destination, record));

    ProjectChangeWatcherUVE watcher(contentRoot, 0.0, 16U);
    ASSERT_TRUE(watcher.PollNowUVE(assetDatabase, cache));
    WriteFixtureFileUVE(source, "after");
    ASSERT_TRUE(watcher.PollNowUVE(assetDatabase, cache));

    const ProjectChangeSnapshotUVE snapshot = watcher.GetSnapshotUVE();
    const ProjectFileChangeUVE* const change =
        FindChangeUVE(snapshot, "source.custom", ProjectFileChangeKindUVE::Modified);
    ASSERT_NE(change, nullptr);
    EXPECT_EQ(change->staleArtifactCount, 1U);
    const std::optional<DerivedArtifactCacheRecordUVE> staleRecord = cache.LoadImportRecordUVE(destination);
    ASSERT_TRUE(staleRecord.has_value());
    EXPECT_TRUE(staleRecord->stale);
}

TEST_F(ProjectChangeWatcherUVETest, GetSnapshotUVE_ReturnsCopyIsolationAndPollNowBypassesInterval) {
    ProjectChangeWatcherUVE watcher(root, 3600.0, 16U);
    ASSERT_TRUE(watcher.PollNowUVE(assetDatabase, derivedArtifactCache));
    WriteFixtureFileUVE(root / "explicit.txt", "explicit");

    ASSERT_TRUE(watcher.PollNowUVE(assetDatabase, derivedArtifactCache));
    ProjectChangeSnapshotUVE callerSnapshot = watcher.GetSnapshotUVE();
    ASSERT_EQ(callerSnapshot.changes.size(), 1U);
    callerSnapshot.changes.front().relativePath = "caller-mutated.txt";
    callerSnapshot.changes.clear();

    const ProjectChangeSnapshotUVE retainedSnapshot = watcher.GetSnapshotUVE();
    ASSERT_EQ(retainedSnapshot.changes.size(), 1U);
    EXPECT_EQ(retainedSnapshot.changes.front().relativePath.generic_string(), "explicit.txt");
}

TEST_F(ProjectChangeWatcherUVETest, Acknowledgements_PreserveOverflowBoundaryUntilExplicitRescanAcknowledgement) {
    ProjectChangeWatcherUVE watcher(root, 0.0, 1U);
    ASSERT_TRUE(watcher.PollNowUVE(assetDatabase, derivedArtifactCache));
    WriteFixtureFileUVE(root / "first.txt", "first");
    ASSERT_TRUE(watcher.PollNowUVE(assetDatabase, derivedArtifactCache));
    WriteFixtureFileUVE(root / "second.txt", "second");
    ASSERT_TRUE(watcher.PollNowUVE(assetDatabase, derivedArtifactCache));

    const ProjectChangeSnapshotUVE overflowSnapshot = watcher.GetSnapshotUVE();
    ASSERT_TRUE(overflowSnapshot.rescanRequired);
    watcher.AcknowledgeThroughUVE(overflowSnapshot.latestSequence);
    const ProjectChangeSnapshotUVE acknowledgedSnapshot = watcher.GetSnapshotUVE();
    EXPECT_TRUE(acknowledgedSnapshot.changes.empty());
    EXPECT_TRUE(acknowledgedSnapshot.rescanRequired);

    watcher.AcknowledgeRescanUVE();
    const ProjectChangeSnapshotUVE rescanAcknowledgedSnapshot = watcher.GetSnapshotUVE();
    EXPECT_TRUE(rescanAcknowledgedSnapshot.changes.empty());
    EXPECT_FALSE(rescanAcknowledgedSnapshot.rescanRequired);
}

} // namespace
} // namespace UVE::Asset::Tests
