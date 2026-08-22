// Copyright (c) 2026 UniVex Studios. All Rights Reserved.

#include "uve/plugins/motion_query_history_uve.h"

#include "uve/math/vector3_uve.h"

#include <cmath>
#include <utility>

namespace UVE::Core {
namespace {

[[nodiscard]] MotionQueryHistoryResultUVE MakeHistoryErrorUVE(
    MotionQueryHistoryResultCodeUVE code, std::size_t index, const char* message) noexcept {
    return MotionQueryHistoryResultUVE{code, index, message};
}

[[nodiscard]] bool HasDuplicateAttributeIdUVE(
    const std::vector<MotionQueryHistoryAttributeUVE>& attributes, std::size_t beforeIndex) noexcept {
    for (std::size_t index = 0U; index < beforeIndex; ++index) {
        if (attributes[index].id == attributes[beforeIndex].id) {
            return true;
        }
    }
    return false;
}

[[nodiscard]] bool IsValidMotionQueryMirrorAxisUVE(const MotionQueryMirrorAxisUVE axis) noexcept {
    switch (axis) {
    case MotionQueryMirrorAxisUVE::X:
    case MotionQueryMirrorAxisUVE::Y:
    case MotionQueryMirrorAxisUVE::Z:
        return true;
    }
    return false;
}

void MirrorVectorComponentUVE(Math::Vector3UVE& value, MotionQueryMirrorAxisUVE axis) noexcept {
    switch (axis) {
    case MotionQueryMirrorAxisUVE::X:
        value.x = -value.x;
        break;
    case MotionQueryMirrorAxisUVE::Y:
        value.y = -value.y;
        break;
    case MotionQueryMirrorAxisUVE::Z:
        value.z = -value.z;
        break;
    }
}

} // namespace

MotionQueryHistoryResultUVE ValidateMotionQueryHistoryFrameUVE(
    const MotionQueryHistoryFrameUVE& frame) noexcept {
    TransformPoseUVE normalizedPose;
    if (!std::isfinite(frame.sample.timeSeconds) || frame.sample.timeSeconds < 0.0 ||
        !TryNormalizeTransformPoseUVE(frame.sample.pose, normalizedPose)) {
        return MakeHistoryErrorUVE(MotionQueryHistoryResultCodeUVE::InvalidFrame, 0U,
                                   "motion query history frame pose or time is invalid");
    }
    if (!ValidateMotionQueryUVE(frame.query).IsValidUVE()) {
        return MakeHistoryErrorUVE(MotionQueryHistoryResultCodeUVE::InvalidFrame, 0U,
                                   "motion query history frame query is invalid");
    }
    if (frame.attributes.size() > MotionQueryHistoryBufferUVE::kMaximumAttributesPerFrameUVE) {
        return MakeHistoryErrorUVE(MotionQueryHistoryResultCodeUVE::CapacityExceeded, 0U,
                                   "motion query history frame attributes exceed capacity");
    }
    for (std::size_t index = 0U; index < frame.attributes.size(); ++index) {
        const MotionQueryHistoryAttributeUVE& attribute = frame.attributes[index];
        if (attribute.id.empty() || attribute.id.size() > MotionQueryHistoryAttributeUVE::kMaximumIdBytesUVE ||
            !std::isfinite(attribute.value)) {
            return MakeHistoryErrorUVE(MotionQueryHistoryResultCodeUVE::InvalidFrame, index,
                                       "motion query history attribute is invalid");
        }
        if (HasDuplicateAttributeIdUVE(frame.attributes, index)) {
            return MakeHistoryErrorUVE(MotionQueryHistoryResultCodeUVE::InvalidFrame, index,
                                       "motion query history attribute IDs must be unique");
        }
    }
    return MotionQueryHistoryResultUVE{MotionQueryHistoryResultCodeUVE::Accepted, 0U, "valid"};
}

MotionQueryHistoryResultUVE ValidateMotionQueryHistoryNotifyUVE(
    const MotionQueryHistoryNotifyUVE& notify) noexcept {
    if (notify.id.empty() || notify.id.size() > MotionQueryHistoryNotifyUVE::kMaximumIdBytesUVE ||
        !std::isfinite(notify.timeSeconds) || notify.timeSeconds < 0.0 || notify.sequence == 0U ||
        notify.payload.size() > MotionQueryHistoryNotifyUVE::kMaximumPayloadBytesUVE) {
        return MakeHistoryErrorUVE(MotionQueryHistoryResultCodeUVE::InvalidNotify, 0U,
                                   "motion query history notify is invalid");
    }
    return MotionQueryHistoryResultUVE{MotionQueryHistoryResultCodeUVE::Accepted, 0U, "valid"};
}

MotionQueryHistoryResultUVE MotionQueryHistoryBufferUVE::AppendFrameUVE(
    MotionQueryHistoryFrameUVE frame) {
    const MotionQueryHistoryResultUVE validation = ValidateMotionQueryHistoryFrameUVE(frame);
    if (!validation.IsAcceptedUVE()) {
        return validation;
    }
    if (!m_frames.empty() && frame.sample.timeSeconds < m_frames.back().sample.timeSeconds) {
        return MakeHistoryErrorUVE(MotionQueryHistoryResultCodeUVE::NonMonotonicTime,
                                   m_frames.size(), "motion query history frame time must be monotonic");
    }
    if (m_frames.size() >= kMaximumFramesUVE) {
        return MakeHistoryErrorUVE(MotionQueryHistoryResultCodeUVE::CapacityExceeded,
                                   m_frames.size(), "motion query history frame capacity is full");
    }
    m_frames.push_back(std::move(frame));
    return MotionQueryHistoryResultUVE{MotionQueryHistoryResultCodeUVE::Accepted, 0U, "accepted"};
}

MotionQueryHistoryResultUVE MotionQueryHistoryBufferUVE::AppendNotifyUVE(
    MotionQueryHistoryNotifyUVE notify) {
    if (notify.sequence == 0U) {
        notify.sequence = static_cast<std::uint64_t>(m_notifies.size() + 1U);
    }
    const MotionQueryHistoryResultUVE validation = ValidateMotionQueryHistoryNotifyUVE(notify);
    if (!validation.IsAcceptedUVE()) {
        return validation;
    }
    const std::uint64_t expectedSequence = static_cast<std::uint64_t>(m_notifies.size() + 1U);
    if (notify.sequence != expectedSequence) {
        return MakeHistoryErrorUVE(MotionQueryHistoryResultCodeUVE::NonMonotonicSequence,
                                   m_notifies.size(), "motion query history notify sequence must be contiguous");
    }
    if (m_notifies.size() >= kMaximumNotifiesUVE) {
        return MakeHistoryErrorUVE(MotionQueryHistoryResultCodeUVE::CapacityExceeded,
                                   m_notifies.size(), "motion query history notify capacity is full");
    }
    if (!m_notifies.empty() && notify.timeSeconds < m_notifies.back().timeSeconds) {
        return MakeHistoryErrorUVE(MotionQueryHistoryResultCodeUVE::NonMonotonicTime,
                                   m_notifies.size(), "motion query history notify time must be monotonic");
    }
    m_notifies.push_back(std::move(notify));
    return MotionQueryHistoryResultUVE{MotionQueryHistoryResultCodeUVE::Accepted, 0U, "accepted"};
}

void MotionQueryHistoryBufferUVE::ClearUVE() noexcept {
    m_frames.clear();
    m_notifies.clear();
}

bool MotionQueryHistoryBufferUVE::TryGetPoseAtOrBeforeUVE(
    double timeSeconds, PoseSampleUVE& outSample) const noexcept {
    if (!std::isfinite(timeSeconds) || timeSeconds < 0.0 || m_frames.empty() ||
        timeSeconds < m_frames.front().sample.timeSeconds) {
        return false;
    }
    const PoseSampleUVE* selected = &m_frames.front().sample;
    for (const MotionQueryHistoryFrameUVE& frame : m_frames) {
        if (frame.sample.timeSeconds > timeSeconds) {
            break;
        }
        selected = &frame.sample;
    }
    outSample = *selected;
    return true;
}

bool TryMirrorMotionQueryHistoryFrameUVE(
    const MotionQueryHistoryFrameUVE& frame, MotionQueryMirrorSettingsUVE settings,
    MotionQueryHistoryFrameUVE& outFrame) noexcept {
    if (!IsValidMotionQueryMirrorAxisUVE(settings.axis) ||
        !ValidateMotionQueryHistoryFrameUVE(frame).IsAcceptedUVE()) {
        return false;
    }
    MotionQueryHistoryFrameUVE mirrored = frame;
    MirrorVectorComponentUVE(mirrored.sample.pose.position, settings.axis);
    MirrorVectorComponentUVE(mirrored.query.rootVelocity, settings.axis);
    MirrorVectorComponentUVE(mirrored.query.facingDirection, settings.axis);
    for (MotionTrajectorySampleUVE& sample : mirrored.query.trajectory) {
        MirrorVectorComponentUVE(sample.relativePosition, settings.axis);
    }
    outFrame = std::move(mirrored);
    return true;
}

} // namespace UVE::Core
