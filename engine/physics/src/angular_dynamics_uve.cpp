// Copyright (c) 2026 UniVex Studios. All Rights Reserved.

#include "uve/physics/angular_dynamics_uve.h"

#include <cmath>

namespace UVE::Physics {
namespace {

[[nodiscard]] bool IsFiniteVectorUVE(const Math::Vector3UVE& value) noexcept {
    return std::isfinite(value.x) && std::isfinite(value.y) && std::isfinite(value.z);
}

[[nodiscard]] Math::Vector3UVE MultiplyComponentsUVE(
    const Math::Vector3UVE& lhs, const Math::Vector3UVE& rhs) noexcept {
    return Math::Vector3UVE{lhs.x * rhs.x, lhs.y * rhs.y, lhs.z * rhs.z};
}

} // namespace

std::optional<Math::Vector3UVE> ComputeBoxInverseInertiaUVE(
    const float mass, const Math::Vector3UVE halfExtents) noexcept {
    if (!std::isfinite(mass) || mass < 0.0F || !IsFiniteVectorUVE(halfExtents) ||
        halfExtents.x <= 0.0F || halfExtents.y <= 0.0F || halfExtents.z <= 0.0F) {
        return std::nullopt;
    }
    if (mass == 0.0F) {
        return Math::Vector3UVE{};
    }

    const float xSquared = halfExtents.x * halfExtents.x;
    const float ySquared = halfExtents.y * halfExtents.y;
    const float zSquared = halfExtents.z * halfExtents.z;
    const float inverseMass = 1.0F / mass;
    if (!std::isfinite(xSquared) || !std::isfinite(ySquared) || !std::isfinite(zSquared) ||
        !std::isfinite(inverseMass)) {
        return std::nullopt;
    }
    const Math::Vector3UVE inertia{
        (mass / 3.0F) * (ySquared + zSquared),
        (mass / 3.0F) * (xSquared + zSquared),
        (mass / 3.0F) * (xSquared + ySquared),
    };
    if (!IsFiniteVectorUVE(inertia) || inertia.x <= 0.0F || inertia.y <= 0.0F || inertia.z <= 0.0F ||
        !std::isfinite(inverseMass)) {
        return std::nullopt;
    }
    return Math::Vector3UVE{1.0F / inertia.x, 1.0F / inertia.y, 1.0F / inertia.z};
}

std::optional<Math::Vector3UVE> IntegrateAngularVelocityUVE(
    const Math::Vector3UVE angularVelocity, const Math::Vector3UVE torque,
    const Math::Vector3UVE inverseInertia, const float deltaTimeSeconds) noexcept {
    if (!IsFiniteVectorUVE(angularVelocity) || !IsFiniteVectorUVE(torque) ||
        !IsFiniteVectorUVE(inverseInertia) || !std::isfinite(deltaTimeSeconds) || deltaTimeSeconds < 0.0F ||
        inverseInertia.x < 0.0F || inverseInertia.y < 0.0F || inverseInertia.z < 0.0F) {
        return std::nullopt;
    }
    const Math::Vector3UVE angularAcceleration = MultiplyComponentsUVE(torque, inverseInertia);
    const Math::Vector3UVE result = angularVelocity + angularAcceleration * deltaTimeSeconds;
    return IsFiniteVectorUVE(result) ? std::optional<Math::Vector3UVE>{result} : std::nullopt;
}

std::optional<Math::Vector3UVE> ApplyAngularImpulseUVE(
    const Math::Vector3UVE angularVelocity, const Math::Vector3UVE angularImpulse,
    const Math::Vector3UVE inverseInertia) noexcept {
    if (!IsFiniteVectorUVE(angularVelocity) || !IsFiniteVectorUVE(angularImpulse) ||
        !IsFiniteVectorUVE(inverseInertia) || inverseInertia.x < 0.0F || inverseInertia.y < 0.0F ||
        inverseInertia.z < 0.0F) {
        return std::nullopt;
    }
    const Math::Vector3UVE result = angularVelocity + MultiplyComponentsUVE(angularImpulse, inverseInertia);
    return IsFiniteVectorUVE(result) ? std::optional<Math::Vector3UVE>{result} : std::nullopt;
}

std::optional<Math::Vector3UVE> EvaluateGyroscopicTorqueUVE(
    const Math::Vector3UVE angularVelocity, const Math::Vector3UVE inverseInertia) noexcept {
    if (!IsFiniteVectorUVE(angularVelocity) || !IsFiniteVectorUVE(inverseInertia) ||
        inverseInertia.x < 0.0F || inverseInertia.y < 0.0F || inverseInertia.z < 0.0F) {
        return std::nullopt;
    }
    const Math::Vector3UVE angularMomentum{
        inverseInertia.x > 0.0F ? angularVelocity.x / inverseInertia.x : 0.0F,
        inverseInertia.y > 0.0F ? angularVelocity.y / inverseInertia.y : 0.0F,
        inverseInertia.z > 0.0F ? angularVelocity.z / inverseInertia.z : 0.0F};
    const Math::Vector3UVE result{
        angularVelocity.y * angularMomentum.z - angularVelocity.z * angularMomentum.y,
        angularVelocity.z * angularMomentum.x - angularVelocity.x * angularMomentum.z,
        angularVelocity.x * angularMomentum.y - angularVelocity.y * angularMomentum.x};
    return IsFiniteVectorUVE(result) ? std::optional<Math::Vector3UVE>{result} : std::nullopt;
}

} // namespace UVE::Physics
