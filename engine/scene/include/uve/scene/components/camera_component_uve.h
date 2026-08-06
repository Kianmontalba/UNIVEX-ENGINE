// Copyright (c) 2026 UniVex Studios. All Rights Reserved.


#pragma once

namespace UVE::Scene {

/// One of the master spec's named built-in components (Part 7.3). Deliberately minimal
/// placeholder data — the universally-understood basics of a perspective camera, nothing
/// rendering-pipeline-specific (projection matrices, render targets, etc.) invented ahead of
/// CameraSystemUVE (Part 7.2).
struct CameraComponentUVE final {
    float fieldOfViewDegrees = 60.0F;
    float nearPlane = 0.1F;
    float farPlane = 1000.0F;
};

} // namespace UVE::Scene
