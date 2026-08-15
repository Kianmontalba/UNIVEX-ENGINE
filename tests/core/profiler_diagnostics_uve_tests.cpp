// Copyright (c) 2026 UniVex Studios. All Rights Reserved.

#include "uve/core/profiler_diagnostics_uve.h"

#include <gtest/gtest.h>

#include <limits>

namespace UVE::Core {

TEST(ProfilerCaptureUVETest, BeginAndEndUVE_ReturnCopiedBundleWithTypedDiagnostics) {
    ProfilerCaptureUVE capture;
    ASSERT_TRUE(capture.BeginUVE("frame-42", 100U).IsAcceptedUVE());
    EXPECT_TRUE(capture.IsActiveUVE());

    ASSERT_TRUE(capture.RecordSpanUVE(DiagnosticCategoryUVE::Animation, "evaluate", 110U, 140U,
                                      7U, 42U)
                    .IsAcceptedUVE());
    ASSERT_TRUE(capture.RecordCounterUVE(DiagnosticCategoryUVE::Memory, "active-bytes", 145U,
                                         2048.0, 7U, 42U)
                    .IsAcceptedUVE());
    ASSERT_TRUE(capture.RecordBreadcrumbUVE(DiagnosticCategoryUVE::Task, "animation ready", 150U,
                                            7U, 42U)
                    .IsAcceptedUVE());

    DiagnosticRuntimeSnapshotUVE snapshot;
    snapshot.frameNumber = 42U;
    snapshot.activeBytes = 2048U;
    snapshot.workerCount = 3U;
    snapshot.pendingTaskCount = 2U;
    ASSERT_TRUE(capture.SetRuntimeSnapshotUVE(snapshot).IsAcceptedUVE());

    const DiagnosticCaptureStopResultUVE result = capture.EndUVE(200U);
    ASSERT_TRUE(result.IsAcceptedUVE());
    EXPECT_FALSE(capture.IsActiveUVE());
    EXPECT_EQ(result.bundle.sessionId, "frame-42");
    EXPECT_EQ(result.bundle.beginTimestampNanoseconds, 100U);
    EXPECT_EQ(result.bundle.endTimestampNanoseconds, 200U);
    ASSERT_EQ(result.bundle.spans.size(), 1U);
    EXPECT_EQ(result.bundle.spans[0].name, "evaluate");
    EXPECT_EQ(result.bundle.spans[0].category, DiagnosticCategoryUVE::Animation);
    ASSERT_EQ(result.bundle.counters.size(), 1U);
    EXPECT_DOUBLE_EQ(result.bundle.counters[0].value, 2048.0);
    ASSERT_EQ(result.bundle.breadcrumbs.size(), 1U);
    EXPECT_EQ(result.bundle.breadcrumbs[0].sequence, 0U);
    EXPECT_EQ(result.bundle.runtimeSnapshot.pendingTaskCount, 2U);
}

TEST(ProfilerCaptureUVETest, RecordBreadcrumbUVE_AssignsMonotonicSequenceNumbers) {
    ProfilerCaptureUVE capture;
    ASSERT_TRUE(capture.BeginUVE("breadcrumbs", 1U).IsAcceptedUVE());
    ASSERT_TRUE(capture.RecordBreadcrumbUVE(DiagnosticCategoryUVE::Cpu, "one", 2U, 1U, 1U)
                    .IsAcceptedUVE());
    ASSERT_TRUE(capture.RecordBreadcrumbUVE(DiagnosticCategoryUVE::Cpu, "two", 3U, 1U, 1U)
                    .IsAcceptedUVE());

    const DiagnosticCaptureStopResultUVE result = capture.EndUVE(4U);
    ASSERT_TRUE(result.IsAcceptedUVE());
    ASSERT_EQ(result.bundle.breadcrumbs.size(), 2U);
    EXPECT_EQ(result.bundle.breadcrumbs[0].sequence, 0U);
    EXPECT_EQ(result.bundle.breadcrumbs[1].sequence, 1U);
}

TEST(ProfilerCaptureUVETest, CaptureLimitsUVE_DropsExcessEventsAndPreservesDiagnostics) {
    DiagnosticCaptureLimitsUVE limits;
    limits.maximumSpans = 1U;
    limits.maximumCounters = 1U;
    limits.maximumBreadcrumbs = 1U;
    ProfilerCaptureUVE capture;
    ASSERT_TRUE(capture.BeginUVE("bounded", 10U, limits).IsAcceptedUVE());

    EXPECT_TRUE(capture.RecordSpanUVE(DiagnosticCategoryUVE::Render, "pass", 11U, 12U, 1U, 2U)
                    .IsAcceptedUVE());
    EXPECT_EQ(capture.RecordSpanUVE(DiagnosticCategoryUVE::Render, "pass-2", 13U, 14U, 1U, 2U).code,
              DiagnosticCaptureCodeUVE::CapacityExceeded);
    EXPECT_TRUE(capture.RecordCounterUVE(DiagnosticCategoryUVE::ECS, "entities", 15U, 4.0, 1U, 2U)
                    .IsAcceptedUVE());
    EXPECT_EQ(capture.RecordCounterUVE(DiagnosticCategoryUVE::ECS, "entities-2", 16U, 5.0, 1U, 2U).code,
              DiagnosticCaptureCodeUVE::CapacityExceeded);
    EXPECT_TRUE(capture.RecordBreadcrumbUVE(DiagnosticCategoryUVE::VM, "step", 17U, 1U, 2U)
                    .IsAcceptedUVE());
    EXPECT_EQ(capture.RecordBreadcrumbUVE(DiagnosticCategoryUVE::VM, "step-2", 18U, 1U, 2U).code,
              DiagnosticCaptureCodeUVE::CapacityExceeded);

    const DiagnosticCaptureStopResultUVE result = capture.EndUVE(20U);
    ASSERT_TRUE(result.IsAcceptedUVE());
    EXPECT_EQ(result.bundle.spans.size(), 1U);
    EXPECT_EQ(result.bundle.counters.size(), 1U);
    EXPECT_EQ(result.bundle.breadcrumbs.size(), 1U);
    EXPECT_EQ(result.bundle.droppedSpanCount, 1U);
    EXPECT_EQ(result.bundle.droppedCounterCount, 1U);
    EXPECT_EQ(result.bundle.droppedBreadcrumbCount, 1U);
}

TEST(ProfilerCaptureUVETest, CaptureLifecycleUVE_RejectsInvalidOrOutOfOrderOperations) {
    ProfilerCaptureUVE capture;
    EXPECT_EQ(capture.RecordCounterUVE(DiagnosticCategoryUVE::Cpu, "before", 1U, 1.0, 1U, 1U).code,
              DiagnosticCaptureCodeUVE::NotActive);

    DiagnosticCaptureLimitsUVE invalidLimits;
    invalidLimits.maximumSpans = 0U;
    EXPECT_EQ(capture.BeginUVE("invalid-limits", 1U, invalidLimits).code,
              DiagnosticCaptureCodeUVE::InvalidLimits);
    EXPECT_EQ(capture.BeginUVE("", 1U).code, DiagnosticCaptureCodeUVE::InvalidSession);

    ASSERT_TRUE(capture.BeginUVE("valid", 10U).IsAcceptedUVE());
    EXPECT_EQ(capture.BeginUVE("second", 11U).code, DiagnosticCaptureCodeUVE::AlreadyActive);
    EXPECT_EQ(capture.RecordSpanUVE(DiagnosticCategoryUVE::Animation, "bad", 20U, 19U, 1U, 1U).code,
              DiagnosticCaptureCodeUVE::InvalidTimestamp);
    EXPECT_EQ(capture.RecordCounterUVE(DiagnosticCategoryUVE::Animation, "nan", 20U,
                                       std::numeric_limits<double>::quiet_NaN(), 1U, 1U)
                   .code,
              DiagnosticCaptureCodeUVE::InvalidValue);
    EXPECT_EQ(capture.EndUVE(9U).code, DiagnosticCaptureCodeUVE::InvalidTimestamp);
    EXPECT_TRUE(capture.EndUVE(20U).IsAcceptedUVE());
    EXPECT_EQ(capture.EndUVE(21U).code, DiagnosticCaptureCodeUVE::NotActive);
}

} // namespace UVE::Core
