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

    const double doubleMass = static_cast<double>(mass);
    const double xSquared = static_cast<double>(halfExtents.x) * static_cast<double>(halfExtents.x);
    const double ySquared = static_cast<double>(halfExtents.y) * static_cast<double>(halfExtents.y);
    const double zSquared = static_cast<double>(halfExtents.z) * static_cast<double>(halfExtents.z);
    const double inverseMass = 1.0 / doubleMass;
    if (!std::isfinite(xSquared) || !std::isfinite(ySquared) || !std::isfinite(zSquared) ||
        !std::isfinite(inverseMass)) {
        return std::nullopt;
    }
    const double inertiaX = (doubleMass / 3.0) * (ySquared + zSquared);
    const double inertiaY = (doubleMass / 3.0) * (xSquared + zSquared);
    const double inertiaZ = (doubleMass / 3.0) * (xSquared + ySquared);
    const double inverseInertiaX = 1.0 / inertiaX;
    const double inverseInertiaY = 1.0 / inertiaY;
    const double inverseInertiaZ = 1.0 / inertiaZ;
    const Math::Vector3UVE result{
        static_cast<float>(inverseInertiaX),
        static_cast<float>(inverseInertiaY),
        static_cast<float>(inverseInertiaZ),
    };
    if (!std::isfinite(inertiaX) || !std::isfinite(inertiaY) || !std::isfinite(inertiaZ) ||
        inertiaX <= 0.0 || inertiaY <= 0.0 || inertiaZ <= 0.0 ||
        !std::isfinite(inverseInertiaX) || !std::isfinite(inverseInertiaY) ||
        !std::isfinite(inverseInertiaZ) || !IsFiniteVectorUVE(result) ||
        result.x <= 0.0F || result.y <= 0.0F || result.z <= 0.0F) {
        return std::nullopt;
    }
    return result;
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
