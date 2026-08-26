// Copyright (c) 2026 UniVex Studios. All Rights Reserved.

#pragma once

namespace UVE::Editor {

/// Draws the editor-private camera orientation widget using value-only UVE inputs/outputs.
/// Returns true only when the widget changed the orientation. No scene or ECS state is touched.
[[nodiscard]] bool DrawViewportOrientationWidgetImGuIZMOUVE(float screenX, float screenY, float size,
                                                            float& yawRadians, float& pitchRadians) noexcept;

} // namespace UVE::Editor
