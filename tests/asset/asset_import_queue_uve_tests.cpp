// Copyright (c) 2026 UniVex Studios. All Rights Reserved.

#include "uve/asset/asset_import_queue_uve.h"

#include <filesystem>
#include <fstream>
#include <memory>
#include <string>

#include <gtest/gtest.h>

#include "uve/asset/asset_content_fingerprint_uve.h"
#include "uve/asset/asset_database_uve.h"
#include "uve/asset/asset_importer_uve.h"
#include "uve/asset/data_table_asset_uve.h"
#include "uve/asset/data_table_importer_uve.h"
#include "uve/asset/derived_artifact_cache_uve.h"

namespace UVE::Asset::Tests {
namespace {

void WriteFixtureFileUVE(const std::filesystem::path& path, const std::string_view contents) {
    if (!path.parent_path().empty()) {
        std::filesystem::create_directories(path.parent_path());
    }
    std::ofstream file(path, std::ios::binary);
    ASSERT_TRUE(file.is_open());
    file << contents;
    ASSERT_TRUE(file.good());
}

[[nodiscard]] AssetImportRequestUVE MakeRequestUVE(const std::filesystem::path& source,
                                                    const std::filesystem::path& destination) {
    AssetImportRequestUVE request;
    request.sourcePath = source;
    request.destinationPath = destination;
    request.settings = std::make_shared<AssetImportSettingsUVE>();
    return request;
}

class AssetImportQueueUVETest : public ::testing::Test {
protected:
    const std::filesystem::path root = "uve_asset_import_queue_tests";
    const std::filesystem::path contentRoot = root / "content";
    const std::filesystem::path cacheRoot = root / "DerivedData" / "Import";
    AssetDatabaseUVE assetDatabase;
    AssetImporterUVE importer;
    DerivedArtifactCacheUVE cache{cacheRoot};
    AssetImportQueueUVE queue{importer, assetDatabase, cache};

    void SetUp() override { std::filesystem::remove_all(root); }
    void TearDown() override { std::filesystem::remove_all(root); }
};

TEST_F(AssetImportQueueUVETest, EnqueueAndTickUVE_ProcessExactlyOneFifoJobPerTick) {
    const std::filesystem::path firstSource = root / "source_first.txt";
    const std::filesystem::path secondSource = root / "source_second.txt";
    const std::filesystem::path firstDestination = contentRoot / "first.txt";
    const std::filesystem::path secondDestination = contentRoot / "second.txt";
    WriteFixtureFileUVE(firstSource, "first bytes");
    WriteFixtureFileUVE(secondSource, "second bytes");
    std::filesystem::create_directories(contentRoot);

    const std::optional<AssetImportJobIdUVE> firstId = queue.EnqueueUVE(MakeRequestUVE(firstSource, firstDestination));
    const std::optional<AssetImportJobIdUVE> secondId = queue.EnqueueUVE(MakeRequestUVE(secondSource, secondDestination));
    ASSERT_TRUE(firstId.has_value());
    ASSERT_TRUE(secondId.has_value());
    EXPECT_LT(firstId->value, secondId->value);

    ASSERT_TRUE(queue.TickUVE());
    std::vector<AssetImportJobUVE> jobs = queue.GetJobsUVE();
    ASSERT_EQ(jobs.size(), 2U);
    EXPECT_EQ(jobs[0].state, AssetImportJobStateUVE::Succeeded);
    EXPECT_EQ(jobs[0].attemptCount, 1U);
    EXPECT_EQ(jobs[1].state, AssetImportJobStateUVE::Queued);
    EXPECT_FALSE(std::filesystem::exists(secondDestination));

    ASSERT_TRUE(queue.TickUVE());
    jobs = queue.GetJobsUVE();
    EXPECT_EQ(jobs[1].state, AssetImportJobStateUVE::Succeeded);
    EXPECT_EQ(jobs[1].attemptCount, 1U);
    EXPECT_TRUE(std::filesystem::exists(secondDestination));
    EXPECT_FALSE(queue.TickUVE());
}

TEST_F(AssetImportQueueUVETest, RetryUVE_ReturnsFailedJobToTailWithoutJumpingQueuedWork) {
    const std::filesystem::path failingSource = root / "failing.unsupported";
    const std::filesystem::path succeedingSource = root / "succeeding.txt";
    const std::filesystem::path failingDestination = contentRoot / "failing.unsupported";
    const std::filesystem::path succeedingDestination = contentRoot / "succeeding.txt";
    WriteFixtureFileUVE(failingSource, "unsupported bytes");
    WriteFixtureFileUVE(succeedingSource, "good bytes");
    std::filesystem::create_directories(contentRoot);

    const std::optional<AssetImportJobIdUVE> failingId =
        queue.EnqueueUVE(MakeRequestUVE(failingSource, failingDestination));
    const std::optional<AssetImportJobIdUVE> succeedingId =
        queue.EnqueueUVE(MakeRequestUVE(succeedingSource, succeedingDestination));
    ASSERT_TRUE(failingId.has_value());
    ASSERT_TRUE(succeedingId.has_value());

    ASSERT_TRUE(queue.TickUVE());
    std::vector<AssetImportJobUVE> jobs = queue.GetJobsUVE();
    ASSERT_EQ(jobs[0].state, AssetImportJobStateUVE::Failed);
    EXPECT_EQ(jobs[0].attemptCount, 1U);
    ASSERT_TRUE(queue.RetryUVE(*failingId));

    ASSERT_TRUE(queue.TickUVE());
    jobs = queue.GetJobsUVE();
    EXPECT_EQ(jobs[1].id, *succeedingId);
    EXPECT_EQ(jobs[1].state, AssetImportJobStateUVE::Succeeded);
    EXPECT_EQ(jobs[0].state, AssetImportJobStateUVE::Queued);
    EXPECT_EQ(jobs[0].attemptCount, 1U);

    ASSERT_TRUE(queue.TickUVE());
    jobs = queue.GetJobsUVE();
    EXPECT_EQ(jobs[0].id, *failingId);
    EXPECT_EQ(jobs[0].state, AssetImportJobStateUVE::Failed);
    EXPECT_EQ(jobs[0].attemptCount, 2U);
}

TEST_F(AssetImportQueueUVETest, CacheUVE_HitsForMatchingBytesAndInvalidatesOutOfBandDestinationDrift) {
    const std::filesystem::path source = root / "source.custom";
    const std::filesystem::path destination = contentRoot / "output.custom";
    WriteFixtureFileUVE(source, "source bytes");
    std::filesystem::create_directories(contentRoot);

    int importerCallCount = 0;
    importer.RegisterImporterUVE("custom", [&importerCallCount](const std::filesystem::path& sourcePath,
                                                                   const std::filesystem::path& destinationPath,
                                                                   const AssetImportSettingsUVE&) {
        ++importerCallCount;
        std::ifstream input(sourcePath, std::ios::binary);
        std::ofstream output(destinationPath, std::ios::binary);
        output << input.rdbuf();
        return input.good() || input.eof();
    });

    const std::optional<AssetImportJobIdUVE> firstId = queue.EnqueueUVE(MakeRequestUVE(source, destination));
    ASSERT_TRUE(firstId.has_value());
    ASSERT_TRUE(queue.TickUVE());
    EXPECT_EQ(importerCallCount, 1);
    std::vector<AssetImportJobUVE> jobs = queue.GetJobsUVE();
    ASSERT_TRUE(jobs[0].resultGuid.has_value());
    EXPECT_FALSE(jobs[0].cacheHit);

    const std::optional<AssetImportJobIdUVE> secondId = queue.EnqueueUVE(MakeRequestUVE(source, destination));
    ASSERT_TRUE(secondId.has_value());
    ASSERT_TRUE(queue.TickUVE());
    EXPECT_EQ(importerCallCount, 1);
    jobs = queue.GetJobsUVE();
    EXPECT_EQ(jobs[1].state, AssetImportJobStateUVE::Succeeded);
    EXPECT_TRUE(jobs[1].cacheHit);
    EXPECT_EQ(jobs[1].resultGuid, jobs[0].resultGuid);

    WriteFixtureFileUVE(destination, "out-of-band destination drift");
    const std::optional<AssetImportJobIdUVE> thirdId = queue.EnqueueUVE(MakeRequestUVE(source, destination));
    ASSERT_TRUE(thirdId.has_value());
    ASSERT_TRUE(queue.TickUVE());
    EXPECT_EQ(importerCallCount, 2);
    jobs = queue.GetJobsUVE();
    EXPECT_EQ(jobs[2].state, AssetImportJobStateUVE::Succeeded);
    EXPECT_FALSE(jobs[2].cacheHit);
    EXPECT_EQ(*ComputeAssetContentFingerprintUVE(destination), *ComputeAssetContentFingerprintUVE(source));
}

TEST_F(AssetImportQueueUVETest, DataTableSettingsFingerprintForcesCacheMissWhenSchemaChanges) {
    const std::filesystem::path source = root / "weapons.csv";
    const std::filesystem::path destination = contentRoot / "weapons.uvetable";
    WriteFixtureFileUVE(source, "id,damage\npistol,25\n");
    std::filesystem::create_directories(contentRoot);

    importer.RegisterImporterUVE(".csv", [](const std::filesystem::path& sourcePath,
                                             const std::filesystem::path& destinationPath,
                                             const AssetImportSettingsUVE& settings) {
        const auto* const dataTableSettings = dynamic_cast<const DataTableImportSettingsUVE*>(&settings);
        if (dataTableSettings == nullptr) {
            return false;
        }
        std::ifstream input(sourcePath, std::ios::binary);
        std::string document((std::istreambuf_iterator<char>(input)), std::istreambuf_iterator<char>());
        if (!input.good() && !input.eof()) {
            return false;
        }
        DataTableUVE table(dataTableSettings->tableName);
        for (const DataTableColumnUVE& column : dataTableSettings->columns) {
            if (!table.DefineColumnUVE(column.name, column.type)) {
                return false;
            }
        }
        return table.ImportCsvUVE(document) && SaveDataTableAssetUVE(table, destinationPath);
    });

    auto integerSettings = std::make_shared<DataTableImportSettingsUVE>();
    integerSettings->tableName = "weapons";
    integerSettings->columns = {DataTableColumnUVE{"damage", DataTableColumnTypeUVE::Integer}};
    auto numberSettings = std::make_shared<DataTableImportSettingsUVE>(*integerSettings);
    numberSettings->columns.front().type = DataTableColumnTypeUVE::Number;
    EXPECT_NE(integerSettings->GetCacheVersionUVE(), numberSettings->GetCacheVersionUVE());

    AssetImportRequestUVE firstRequest;
    firstRequest.sourcePath = source;
    firstRequest.destinationPath = destination;
    firstRequest.settings = integerSettings;
    ASSERT_TRUE(queue.EnqueueUVE(std::move(firstRequest)).has_value());
    ASSERT_TRUE(queue.TickUVE());

    AssetImportRequestUVE secondRequest;
    secondRequest.sourcePath = source;
    secondRequest.destinationPath = destination;
    secondRequest.settings = numberSettings;
    ASSERT_TRUE(queue.EnqueueUVE(std::move(secondRequest)).has_value());
    ASSERT_TRUE(queue.TickUVE());

    const std::vector<AssetImportJobUVE> jobs = queue.GetJobsUVE();
    ASSERT_EQ(jobs.size(), 2U);
    EXPECT_FALSE(jobs[0].cacheHit);
    EXPECT_FALSE(jobs[1].cacheHit);
    ASSERT_TRUE(jobs[0].request.settingsVersion != jobs[1].request.settingsVersion);
    DataTableUVE imported;
    ASSERT_TRUE(LoadDataTableAssetUVE(destination, imported));
    ASSERT_EQ(imported.GetSnapshotUVE().columns.size(), 1U);
    EXPECT_EQ(imported.GetSnapshotUVE().columns.front().type, DataTableColumnTypeUVE::Number);
}

TEST_F(AssetImportQueueUVETest, SnapshotAndCacheContracts_RejectInvalidRequestAndReturnCopies) {
    AssetImportRequestUVE invalidRequest;
    EXPECT_FALSE(queue.EnqueueUVE(std::move(invalidRequest)).has_value());

    const std::filesystem::path source = root / "source.txt";
    const std::filesystem::path destination = contentRoot / "output.txt";
    WriteFixtureFileUVE(source, "source bytes");
    std::filesystem::create_directories(contentRoot);
    ASSERT_TRUE(queue.EnqueueUVE(MakeRequestUVE(source, destination)).has_value());

    std::vector<AssetImportJobUVE> firstSnapshot = queue.GetJobsUVE();
    ASSERT_EQ(firstSnapshot.size(), 1U);
    firstSnapshot[0].state = AssetImportJobStateUVE::Succeeded;
    firstSnapshot[0].request.sourcePath = "caller-mutated";
    const std::vector<AssetImportJobUVE> secondSnapshot = queue.GetJobsUVE();
    EXPECT_EQ(secondSnapshot[0].state, AssetImportJobStateUVE::Queued);
    EXPECT_NE(secondSnapshot[0].request.sourcePath, std::filesystem::path("caller-mutated"));

    const DerivedArtifactCacheRecordUVE invalidRecord{kDerivedArtifactCacheSchemaVersionUVE,
                                                       source,
                                                       root / "different-destination.txt",
                                                       {},
                                                       {},
                                                       "generic-v1",
                                                       AssetGuidUVE{1U}};
    EXPECT_FALSE(cache.StoreImportRecordUVE(destination, invalidRecord));
    EXPECT_FALSE(std::filesystem::exists(cacheRoot));
}

TEST(AssetContentFingerprintUVETest, ComputeUVE_IsTimestampIndependentAndByteSensitive) {
    const std::filesystem::path root = "uve_asset_content_fingerprint_tests";
    const std::filesystem::path first = root / "first.bin";
    const std::filesystem::path second = root / "second.bin";
    std::filesystem::remove_all(root);
    WriteFixtureFileUVE(first, "same bytes");
    WriteFixtureFileUVE(second, "same bytes");

    const std::optional<AssetContentFingerprintUVE> firstFingerprint = ComputeAssetContentFingerprintUVE(first);
    const std::optional<AssetContentFingerprintUVE> secondFingerprint = ComputeAssetContentFingerprintUVE(second);
    ASSERT_TRUE(firstFingerprint.has_value());
    ASSERT_TRUE(secondFingerprint.has_value());
    EXPECT_EQ(*firstFingerprint, *secondFingerprint);

    WriteFixtureFileUVE(second, "different bytes");
    const std::optional<AssetContentFingerprintUVE> changedFingerprint = ComputeAssetContentFingerprintUVE(second);
    ASSERT_TRUE(changedFingerprint.has_value());
    EXPECT_NE(*firstFingerprint, *changedFingerprint);

    std::filesystem::remove_all(root);
}

// Increment 61 stale-metadata regressions remain intentionally beside queue cache-hit coverage:
// the cache owns persistence and the queue owns acceptance of a persisted record.
TEST_F(AssetImportQueueUVETest, DerivedArtifactCacheUVE_MarkStaleForSourceUVE_IsTargetedAndIdempotent) {
    const std::filesystem::path firstSource = root / "first-source.custom";
    const std::filesystem::path secondSource = root / "second-source.custom";
    const std::filesystem::path firstDestination = contentRoot / "first-output.custom";
    const std::filesystem::path secondDestination = contentRoot / "second-output.custom";
    WriteFixtureFileUVE(firstSource, "first source");
    WriteFixtureFileUVE(secondSource, "second source");
    WriteFixtureFileUVE(firstDestination, "first destination");
    WriteFixtureFileUVE(secondDestination, "second destination");

    const std::optional<AssetContentFingerprintUVE> firstSourceFingerprint =
        ComputeAssetContentFingerprintUVE(firstSource);
    const std::optional<AssetContentFingerprintUVE> secondSourceFingerprint =
        ComputeAssetContentFingerprintUVE(secondSource);
    const std::optional<AssetContentFingerprintUVE> firstDestinationFingerprint =
        ComputeAssetContentFingerprintUVE(firstDestination);
    const std::optional<AssetContentFingerprintUVE> secondDestinationFingerprint =
        ComputeAssetContentFingerprintUVE(secondDestination);
    ASSERT_TRUE(firstSourceFingerprint.has_value());
    ASSERT_TRUE(secondSourceFingerprint.has_value());
    ASSERT_TRUE(firstDestinationFingerprint.has_value());
    ASSERT_TRUE(secondDestinationFingerprint.has_value());

    const DerivedArtifactCacheRecordUVE firstRecord{kDerivedArtifactCacheSchemaVersionUVE,
                                                     firstSource,
                                                     firstDestination,
                                                     *firstSourceFingerprint,
                                                     *firstDestinationFingerprint,
                                                     "generic-v1",
                                                     AssetGuidUVE{1U}};
    const DerivedArtifactCacheRecordUVE secondRecord{kDerivedArtifactCacheSchemaVersionUVE,
                                                      secondSource,
                                                      secondDestination,
                                                      *secondSourceFingerprint,
                                                      *secondDestinationFingerprint,
                                                      "generic-v1",
                                                      AssetGuidUVE{2U}};
    ASSERT_TRUE(cache.StoreImportRecordUVE(firstDestination, firstRecord));
    ASSERT_TRUE(cache.StoreImportRecordUVE(secondDestination, secondRecord));

    EXPECT_EQ(cache.MarkStaleForSourceUVE(firstSource), 1U);
    const std::optional<DerivedArtifactCacheRecordUVE> staleFirst = cache.LoadImportRecordUVE(firstDestination);
    const std::optional<DerivedArtifactCacheRecordUVE> freshSecond = cache.LoadImportRecordUVE(secondDestination);
    ASSERT_TRUE(staleFirst.has_value());
    ASSERT_TRUE(freshSecond.has_value());
    EXPECT_TRUE(staleFirst->stale);
    EXPECT_FALSE(freshSecond->stale);
    EXPECT_EQ(cache.MarkStaleForSourceUVE(root / "unrelated-source.custom"), 0U);
    EXPECT_EQ(cache.MarkStaleForSourceUVE(firstSource), 0U);
    EXPECT_FALSE(cache.LoadImportRecordUVE(secondDestination)->stale);
}

TEST_F(AssetImportQueueUVETest, CacheUVE_StoreRejectsCallerStaleRecordAndRetainsFreshArtifact) {
    const std::filesystem::path source = root / "source.custom";
    const std::filesystem::path destination = contentRoot / "output.custom";
    WriteFixtureFileUVE(source, "source bytes");
    WriteFixtureFileUVE(destination, "output bytes");

    const std::optional<AssetContentFingerprintUVE> sourceFingerprint =
        ComputeAssetContentFingerprintUVE(source);
    const std::optional<AssetContentFingerprintUVE> destinationFingerprint =
        ComputeAssetContentFingerprintUVE(destination);
    ASSERT_TRUE(sourceFingerprint.has_value());
    ASSERT_TRUE(destinationFingerprint.has_value());

    const DerivedArtifactCacheRecordUVE freshRecord{kDerivedArtifactCacheSchemaVersionUVE,
                                                     source,
                                                     destination,
                                                     *sourceFingerprint,
                                                     *destinationFingerprint,
                                                     "generic-v1",
                                                     AssetGuidUVE{1U},
                                                     false};
    ASSERT_TRUE(cache.StoreImportRecordUVE(destination, freshRecord));

    DerivedArtifactCacheRecordUVE staleCallerRecord = freshRecord;
    staleCallerRecord.stale = true;
    EXPECT_FALSE(cache.StoreImportRecordUVE(destination, staleCallerRecord));

    const std::optional<DerivedArtifactCacheRecordUVE> retainedRecord = cache.LoadImportRecordUVE(destination);
    ASSERT_TRUE(retainedRecord.has_value());
    EXPECT_FALSE(retainedRecord->stale);
    EXPECT_EQ(retainedRecord->sourceFingerprint, freshRecord.sourceFingerprint);
    EXPECT_EQ(retainedRecord->destinationFingerprint, freshRecord.destinationFingerprint);
}

TEST_F(AssetImportQueueUVETest, CacheUVE_StaleRecordForcesFreshImportAndSuccessfulWriteClearsStaleState) {
    const std::filesystem::path source = root / "source.custom";
    const std::filesystem::path destination = contentRoot / "output.custom";
    WriteFixtureFileUVE(source, "source bytes");
    std::filesystem::create_directories(contentRoot);

    int importerCallCount = 0;
    importer.RegisterImporterUVE("custom", [&importerCallCount](const std::filesystem::path& sourcePath,
                                                                   const std::filesystem::path& destinationPath,
                                                                   const AssetImportSettingsUVE&) {
        ++importerCallCount;
        std::ifstream input(sourcePath, std::ios::binary);
        std::ofstream output(destinationPath, std::ios::binary | std::ios::trunc);
        output << input.rdbuf();
        return input.good() || input.eof();
    });

    ASSERT_TRUE(queue.EnqueueUVE(MakeRequestUVE(source, destination)).has_value());
    ASSERT_TRUE(queue.TickUVE());
    EXPECT_EQ(importerCallCount, 1);
    ASSERT_EQ(cache.MarkStaleForSourceUVE(source), 1U);
    const std::optional<DerivedArtifactCacheRecordUVE> staleRecord = cache.LoadImportRecordUVE(destination);
    ASSERT_TRUE(staleRecord.has_value());
    EXPECT_TRUE(staleRecord->stale);

    ASSERT_TRUE(queue.EnqueueUVE(MakeRequestUVE(source, destination)).has_value());
    ASSERT_TRUE(queue.TickUVE());
    EXPECT_EQ(importerCallCount, 2);
    const std::vector<AssetImportJobUVE> jobsAfterFreshImport = queue.GetJobsUVE();
    ASSERT_EQ(jobsAfterFreshImport.size(), 2U);
    EXPECT_FALSE(jobsAfterFreshImport[1].cacheHit);
    const std::optional<DerivedArtifactCacheRecordUVE> refreshedRecord = cache.LoadImportRecordUVE(destination);
    ASSERT_TRUE(refreshedRecord.has_value());
    EXPECT_FALSE(refreshedRecord->stale);

    ASSERT_TRUE(queue.EnqueueUVE(MakeRequestUVE(source, destination)).has_value());
    ASSERT_TRUE(queue.TickUVE());
    EXPECT_EQ(importerCallCount, 2);
    const std::vector<AssetImportJobUVE> finalJobs = queue.GetJobsUVE();
    ASSERT_EQ(finalJobs.size(), 3U);
    EXPECT_TRUE(finalJobs[2].cacheHit);
}

} // namespace
} // namespace UVE::Asset::Tests
