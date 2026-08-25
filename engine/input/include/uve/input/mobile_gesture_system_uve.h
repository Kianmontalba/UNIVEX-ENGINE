// Copyright (c) 2026 UniVex Studios. All Rights Reserved.
#pragma once

#include "uve/input/i_mobile_gesture_system_uve.h"

namespace UVE::Input {

class MobileGestureSystemUVE final : public IMobileGestureSystemUVE {
public:
    explicit MobileGestureSystemUVE(IMobileInputSystemUVE& mobileInput,
                                    MobileGestureRecognizerConfigUVE config = {}) noexcept;

    void UpdateUVE(float frameDeltaSeconds) override;
    [[nodiscard]] MobileGestureReportUVE GetLastReportUVE() const noexcept override;
    void ResetUVE() noexcept override;

private:
    IMobileInputSystemUVE& m_mobileInput;
    MobileGestureRecognizerUVE m_recognizer;
    MobileGestureReportUVE m_lastReport{};
};

} // namespace UVE::Input
