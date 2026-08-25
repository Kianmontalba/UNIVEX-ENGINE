// Copyright (c) 2026 UniVex Studios. All Rights Reserved.
#include "uve/input/mobile_gesture_system_uve.h"

namespace UVE::Input {

MobileGestureSystemUVE::MobileGestureSystemUVE(IMobileInputSystemUVE& mobileInput,
                                               const MobileGestureRecognizerConfigUVE config) noexcept
    : m_mobileInput(mobileInput), m_recognizer(config) {}

void MobileGestureSystemUVE::UpdateUVE(const float frameDeltaSeconds) {
    m_lastReport = m_recognizer.ConsumeSnapshotUVE(m_mobileInput.GetSnapshotUVE(), frameDeltaSeconds);
}

MobileGestureReportUVE MobileGestureSystemUVE::GetLastReportUVE() const noexcept {
    return m_lastReport;
}

void MobileGestureSystemUVE::ResetUVE() noexcept {
    m_recognizer.ResetUVE();
    m_lastReport = MobileGestureReportUVE{};
}

} // namespace UVE::Input
