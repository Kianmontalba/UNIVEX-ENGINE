// Copyright (c) 2026 UniVex Studios. All Rights Reserved.

#pragma once

#include <optional>

#include "uve/math/vector3_uve.h"

namespace UVE::Physics {

/// Returns the diagonal inverse inertia tensor for a box of `halfExtents` and `mass`.
/// The result is represented by its diagonal vector because this bounded slice has no rotation
/// integration or gyroscopic coupling; zero mass returns zero inverse inertia.
[[nodiscard]] std::optional<Math::Vector3UVE> ComputeBoxInverseInertiaUVE(
    float mass, Math::Vector3UVE halfExtents) noexcept;

/// Integrates one copied angular-velocity value under a caller-supplied torque for one finite
/// timestep. The diagonal inverse inertia is caller-owned state; no ECS or body persistence is
/// accessed or mutated by this helper.
[[nodiscard]] std::optional<Math::Vector3UVE> IntegrateAngularVelocityUVE(
    Math::Vector3UVE angularVelocity, Math::Vector3UVE torque,
    Math::Vector3UVE inverseInertia, float deltaTimeSeconds) noexcept;

/// Applies one copied angular impulse through a diagonal inverse inertia tensor.
[[nodiscard]] std::optional<Math::Vector3UVE> ApplyAngularImpulseUVE(
    Math::Vector3UVE angularVelocity, Math::Vector3UVE angularImpulse,
    Math::Vector3UVE inverseInertia) noexcept;

/// Returns the diagonal-inertia gyroscopic torque term `angularVelocity x (inertia * angularVelocity)`.
/// The caller decides whether and how to subtract this term during integration.
[[nodiscard]] std::optional<Math::Vector3UVE> EvaluateGyroscopicTorqueUVE(
    Math::Vector3UVE angularVelocity, Math::Vector3UVE inverseInertia) noexcept;

} // namespace UVE::Physics
