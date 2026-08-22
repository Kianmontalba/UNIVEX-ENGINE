// Copyright (c) 2026 UniVex Studios. All Rights Reserved.

#pragma once

#include "uve/plugins/motion_query_uve.h"

#include <cstddef>
#include <cstdint>
#include <string>
#include <vector>

namespace UVE::Core {

enum class MotionQueryHistoryAttributeKindUVE : std::uint8_t {
    RootSpeed = 0,
    FacingYaw,
    CustomScalar,
};

struct MotionQueryHistoryAttributeUVE final {
    static constexpr std::size_t kMaximumIdBytesUVE = 128U;

    std::string id;
    MotionQueryHistoryAttributeKindUVE kind = MotionQueryHistoryAttributeKindUVE::CustomScalar;
    float value = 0.0F;

    [[nodiscard]] bool operator==(const MotionQueryHistoryAttributeUVE&) const = default;
};

struct MotionQueryHistoryFrameUVE final {
    PoseSampleUVE sample;
    MotionQueryUVE query;
    std::vector<MotionQueryHistoryAttributeUVE> attributes;

};

struct MotionQueryHistoryNotifyUVE final {
    static constexpr std::size_t kMaximumIdBytesUVE = 128U;
    static constexpr std::size_t kMaximumPayloadBytesUVE = 256U;

    std::string id;
    double timeSeconds = 0.0;
    std::uint64_t sequence = 0U;
    std::string payload;

    [[nodiscard]] bool operator==(const MotionQueryHistoryNotifyUVE&) const = default;
};

enum class MotionQueryHistoryResultCodeUVE : std::uint8_t {
    Accepted = 0,
    InvalidFrame,
    CapacityExceeded,
    NonMonotonicTime,
    InvalidNotify,
    NonMonotonicSequence,
    InvalidSampleTime,
    NoSampleAtOrBefore,
};

struct MotionQueryHistoryResultUVE final {
    MotionQueryHistoryResultCodeUVE code = MotionQueryHistoryResultCodeUVE::InvalidFrame;
    std::size_t index = 0U;
    std::string message;

    [[nodiscard]] bool IsAcceptedUVE() const noexcept {
        return code == MotionQueryHistoryResultCodeUVE::Accepted;
    }
};

class MotionQueryHistoryBufferUVE final {
public:
    static constexpr std::size_t kMaximumFramesUVE = 128U;
    static constexpr std::size_t kMaximumAttributesPerFrameUVE = 64U;
    static constexpr std::size_t kMaximumNotifiesUVE = 256U;

    [[nodiscard]] MotionQueryHistoryResultUVE AppendFrameUVE(MotionQueryHistoryFrameUVE frame);
    [[nodiscard]] MotionQueryHistoryResultUVE AppendNotifyUVE(MotionQueryHistoryNotifyUVE notify);
    void ClearUVE() noexcept;

    [[nodiscard]] bool TryGetPoseAtOrBeforeUVE(double timeSeconds,
                                                PoseSampleUVE& outSample) const noexcept;
    [[nodiscard]] const std::vector<MotionQueryHistoryFrameUVE>& GetFramesUVE() const noexcept {
        return m_frames;
    }
    [[nodiscard]] const std::vector<MotionQueryHistoryNotifyUVE>& GetNotifiesUVE() const noexcept {
        return m_notifies;
    }

private:
    std::vector<MotionQueryHistoryFrameUVE> m_frames;
    std::vector<MotionQueryHistoryNotifyUVE> m_notifies;
};

enum class MotionQueryMirrorAxisUVE : std::uint8_t {
    X = 0,
    Y,
    Z,
};

struct MotionQueryMirrorSettingsUVE final {
    MotionQueryMirrorAxisUVE axis = MotionQueryMirrorAxisUVE::X;

    [[nodiscard]] bool operator==(const MotionQueryMirrorSettingsUVE&) const = default;
};

[[nodiscard]] MotionQueryHistoryResultUVE ValidateMotionQueryHistoryFrameUVE(
    const MotionQueryHistoryFrameUVE& frame) noexcept;

[[nodiscard]] MotionQueryHistoryResultUVE ValidateMotionQueryHistoryNotifyUVE(
    const MotionQueryHistoryNotifyUVE& notify) noexcept;

/// Mirrors a validated history frame across the requested X/Y/Z axis into staged output. Returns
/// false for an unknown raw axis or invalid frame and leaves `outFrame` unchanged on failure.
[[nodiscard]] bool TryMirrorMotionQueryHistoryFrameUVE(
    const MotionQueryHistoryFrameUVE& frame, MotionQueryMirrorSettingsUVE settings,
    MotionQueryHistoryFrameUVE& outFrame) noexcept;

} // namespace UVE::Core
