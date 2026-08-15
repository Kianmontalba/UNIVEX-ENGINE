// Copyright (c) 2026 UniVex Studios. All Rights Reserved.

#include "uve/core/profiler_diagnostics_uve.h"

#include <cmath>
#include <utility>

namespace UVE::Core {
namespace {

[[nodiscard]] bool IsValidCategoryUVE(DiagnosticCategoryUVE category) noexcept {
    return static_cast<std::uint8_t>(category) <
           static_cast<std::uint8_t>(DiagnosticCategoryUVE::Count);
}

[[nodiscard]] bool IsValidLimitsUVE(const DiagnosticCaptureLimitsUVE& limits) noexcept {
    return limits.maximumSpans > 0U &&
           limits.maximumSpans <= DiagnosticCaptureLimitsUVE::kMaximumSpansUVE &&
           limits.maximumCounters > 0U &&
           limits.maximumCounters <= DiagnosticCaptureLimitsUVE::kMaximumCountersUVE &&
           limits.maximumBreadcrumbs > 0U &&
           limits.maximumBreadcrumbs <= DiagnosticCaptureLimitsUVE::kMaximumBreadcrumbsUVE;
}

[[nodiscard]] DiagnosticCaptureResultUVE MakeResultUVE(DiagnosticCaptureCodeUVE code,
                                                        const char* message) {
    return DiagnosticCaptureResultUVE{code, message};
}

} // namespace

DiagnosticCaptureResultUVE ProfilerCaptureUVE::BeginUVE(
    std::string_view sessionId, std::uint64_t beginTimestampNanoseconds,
    DiagnosticCaptureLimitsUVE limits) {
    std::lock_guard<std::mutex> lock(m_mutex);
    if (m_active) {
        return MakeResultUVE(DiagnosticCaptureCodeUVE::AlreadyActive,
                             "diagnostic capture session is already active");
    }
    if (sessionId.empty() || sessionId.size() > kMaximumSessionIdBytesUVE) {
        return MakeResultUVE(DiagnosticCaptureCodeUVE::InvalidSession,
                             "diagnostic capture session identifier is empty or too long");
    }
    if (!IsValidLimitsUVE(limits)) {
        return MakeResultUVE(DiagnosticCaptureCodeUVE::InvalidLimits,
                             "diagnostic capture limits are outside bounded capacity");
    }

    m_limits = limits;
    m_bundle = DiagnosticCaptureBundleUVE{};
    m_bundle.sessionId = sessionId;
    m_bundle.beginTimestampNanoseconds = beginTimestampNanoseconds;
    m_nextBreadcrumbSequence = 0U;
    m_active = true;
    return MakeResultUVE(DiagnosticCaptureCodeUVE::Accepted, "accepted");
}

DiagnosticCaptureResultUVE ProfilerCaptureUVE::RecordSpanUVE(
    DiagnosticCategoryUVE category, std::string_view name, std::uint64_t beginTimestampNanoseconds,
    std::uint64_t endTimestampNanoseconds, std::uint64_t threadId, std::uint64_t frameNumber) {
    std::lock_guard<std::mutex> lock(m_mutex);
    if (!m_active) {
        return MakeResultUVE(DiagnosticCaptureCodeUVE::NotActive,
                             "diagnostic capture session is not active");
    }
    if (!IsValidCategoryUVE(category)) {
        return MakeResultUVE(DiagnosticCaptureCodeUVE::InvalidCategory,
                             "diagnostic span category is invalid");
    }
    if (name.empty() || name.size() > kMaximumNameBytesUVE) {
        return MakeResultUVE(DiagnosticCaptureCodeUVE::InvalidName,
                             "diagnostic span name is empty or too long");
    }
    if (endTimestampNanoseconds < beginTimestampNanoseconds) {
        return MakeResultUVE(DiagnosticCaptureCodeUVE::InvalidTimestamp,
                             "diagnostic span end precedes its begin timestamp");
    }
    if (m_bundle.spans.size() >= m_limits.maximumSpans) {
        ++m_bundle.droppedSpanCount;
        return MakeResultUVE(DiagnosticCaptureCodeUVE::CapacityExceeded,
                             "diagnostic span capacity exceeded; event was dropped");
    }

    m_bundle.spans.push_back(DiagnosticSpanUVE{category, std::string(name),
                                               beginTimestampNanoseconds,
                                               endTimestampNanoseconds, threadId, frameNumber});
    return MakeResultUVE(DiagnosticCaptureCodeUVE::Accepted, "accepted");
}

DiagnosticCaptureResultUVE ProfilerCaptureUVE::RecordCounterUVE(
    DiagnosticCategoryUVE category, std::string_view name, std::uint64_t timestampNanoseconds,
    double value, std::uint64_t threadId, std::uint64_t frameNumber) {
    std::lock_guard<std::mutex> lock(m_mutex);
    if (!m_active) {
        return MakeResultUVE(DiagnosticCaptureCodeUVE::NotActive,
                             "diagnostic capture session is not active");
    }
    if (!IsValidCategoryUVE(category)) {
        return MakeResultUVE(DiagnosticCaptureCodeUVE::InvalidCategory,
                             "diagnostic counter category is invalid");
    }
    if (name.empty() || name.size() > kMaximumNameBytesUVE) {
        return MakeResultUVE(DiagnosticCaptureCodeUVE::InvalidName,
                             "diagnostic counter name is empty or too long");
    }
    if (!std::isfinite(value)) {
        return MakeResultUVE(DiagnosticCaptureCodeUVE::InvalidValue,
                             "diagnostic counter value must be finite");
    }
    if (m_bundle.counters.size() >= m_limits.maximumCounters) {
        ++m_bundle.droppedCounterCount;
        return MakeResultUVE(DiagnosticCaptureCodeUVE::CapacityExceeded,
                             "diagnostic counter capacity exceeded; event was dropped");
    }

    m_bundle.counters.push_back(DiagnosticCounterUVE{category, std::string(name),
                                                     timestampNanoseconds, value, threadId,
                                                     frameNumber});
    return MakeResultUVE(DiagnosticCaptureCodeUVE::Accepted, "accepted");
}

DiagnosticCaptureResultUVE ProfilerCaptureUVE::RecordBreadcrumbUVE(
    DiagnosticCategoryUVE category, std::string_view message, std::uint64_t timestampNanoseconds,
    std::uint64_t threadId, std::uint64_t frameNumber) {
    std::lock_guard<std::mutex> lock(m_mutex);
    if (!m_active) {
        return MakeResultUVE(DiagnosticCaptureCodeUVE::NotActive,
                             "diagnostic capture session is not active");
    }
    if (!IsValidCategoryUVE(category)) {
        return MakeResultUVE(DiagnosticCaptureCodeUVE::InvalidCategory,
                             "diagnostic breadcrumb category is invalid");
    }
    if (message.empty() || message.size() > kMaximumMessageBytesUVE) {
        return MakeResultUVE(DiagnosticCaptureCodeUVE::InvalidMessage,
                             "diagnostic breadcrumb message is empty or too long");
    }
    if (m_bundle.breadcrumbs.size() >= m_limits.maximumBreadcrumbs) {
        ++m_bundle.droppedBreadcrumbCount;
        return MakeResultUVE(DiagnosticCaptureCodeUVE::CapacityExceeded,
                             "diagnostic breadcrumb capacity exceeded; event was dropped");
    }

    m_bundle.breadcrumbs.push_back(DiagnosticBreadcrumbUVE{
        m_nextBreadcrumbSequence++, category, std::string(message), timestampNanoseconds,
        threadId, frameNumber});
    return MakeResultUVE(DiagnosticCaptureCodeUVE::Accepted, "accepted");
}

DiagnosticCaptureResultUVE ProfilerCaptureUVE::SetRuntimeSnapshotUVE(
    DiagnosticRuntimeSnapshotUVE snapshot) {
    std::lock_guard<std::mutex> lock(m_mutex);
    if (!m_active) {
        return MakeResultUVE(DiagnosticCaptureCodeUVE::NotActive,
                             "diagnostic capture session is not active");
    }
    m_bundle.runtimeSnapshot = snapshot;
    return MakeResultUVE(DiagnosticCaptureCodeUVE::Accepted, "accepted");
}

DiagnosticCaptureStopResultUVE ProfilerCaptureUVE::EndUVE(
    std::uint64_t endTimestampNanoseconds) {
    std::lock_guard<std::mutex> lock(m_mutex);
    if (!m_active) {
        return DiagnosticCaptureStopResultUVE{DiagnosticCaptureCodeUVE::NotActive, {},
                                              "diagnostic capture session is not active"};
    }
    if (endTimestampNanoseconds < m_bundle.beginTimestampNanoseconds) {
        return DiagnosticCaptureStopResultUVE{DiagnosticCaptureCodeUVE::InvalidTimestamp, {},
                                              "diagnostic capture end precedes begin timestamp"};
    }

    m_bundle.endTimestampNanoseconds = endTimestampNanoseconds;
    DiagnosticCaptureBundleUVE completedBundle = std::move(m_bundle);
    m_bundle = DiagnosticCaptureBundleUVE{};
    m_active = false;
    m_nextBreadcrumbSequence = 0U;
    return DiagnosticCaptureStopResultUVE{DiagnosticCaptureCodeUVE::Accepted,
                                          std::move(completedBundle), "accepted"};
}

bool ProfilerCaptureUVE::IsActiveUVE() const noexcept {
    std::lock_guard<std::mutex> lock(m_mutex);
    return m_active;
}

} // namespace UVE::Core
