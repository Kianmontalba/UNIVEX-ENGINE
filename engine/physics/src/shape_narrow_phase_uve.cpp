// Copyright (c) 2026 UniVex Studios. All Rights Reserved.

#include "uve/physics/detail/shape_narrow_phase_uve.h"

#include <algorithm>
#include <array>
#include <cmath>
#include <limits>

namespace UVE::Physics::Detail {
namespace {

[[nodiscard]] bool IsFiniteVectorUVE(const Math::Vector3UVE value) noexcept {
    return std::isfinite(value.x) && std::isfinite(value.y) && std::isfinite(value.z);
}

[[nodiscard]] bool IsFiniteAabbUVE(const Math::AabbUVE& box) noexcept {
    return IsFiniteVectorUVE(box.min) && IsFiniteVectorUVE(box.max) && box.min.x < box.max.x &&
           box.min.y < box.max.y && box.min.z < box.max.z;
}

[[nodiscard]] bool TryBuildOrientedBoxFrameUVE(
    const Math::Vector3UVE boxHalfExtents, const Math::QuaternionUVE boxRotation,
    Math::QuaternionUVE& outRotation, Math::QuaternionUVE& outInverse) noexcept {
    if (!IsFiniteVectorUVE(boxHalfExtents) || boxHalfExtents.x <= 0.0F || boxHalfExtents.y <= 0.0F ||
        boxHalfExtents.z <= 0.0F || !Math::TryNormalizeUVE(boxRotation, outRotation) ||
        !Math::TryInverseUVE(outRotation, outInverse)) {
        return false;
    }
    return true;
}

} // namespace

std::optional<Math::PenetrationUVE> ComputeSphereAabbPenetrationUVE(
    const Math::AabbUVE& box, const Math::Vector3UVE sphereCenter, const float sphereRadius) noexcept {
    if (!IsFiniteAabbUVE(box) || !std::isfinite(sphereCenter.x) || !std::isfinite(sphereCenter.y) ||
        !std::isfinite(sphereCenter.z) || !std::isfinite(sphereRadius) || sphereRadius <= 0.0F) {
        return std::nullopt;
    }

    const Math::Vector3UVE closestPoint{
        std::clamp(sphereCenter.x, box.min.x, box.max.x),
        std::clamp(sphereCenter.y, box.min.y, box.max.y),
        std::clamp(sphereCenter.z, box.min.z, box.max.z),
    };
    const Math::Vector3UVE delta = closestPoint - sphereCenter;
    const float distanceSquared = Math::LengthSquaredUVE(delta);
    const float radiusSquared = sphereRadius * sphereRadius;
    if (distanceSquared > 0.0F) {
        if (distanceSquared >= radiusSquared) {
            return std::nullopt;
        }
        const float distance = std::sqrt(distanceSquared);
        return Math::PenetrationUVE{delta * (1.0F / distance), sphereRadius - distance};
    }

    const float distanceToMinX = sphereCenter.x - box.min.x;
    const float distanceToMaxX = box.max.x - sphereCenter.x;
    const float distanceToMinY = sphereCenter.y - box.min.y;
    const float distanceToMaxY = box.max.y - sphereCenter.y;
    const float distanceToMinZ = sphereCenter.z - box.min.z;
    const float distanceToMaxZ = box.max.z - sphereCenter.z;

    float nearestDistance = distanceToMinX;
    Math::Vector3UVE axis{1.0F, 0.0F, 0.0F};
    if (distanceToMaxX < nearestDistance) {
        nearestDistance = distanceToMaxX;
        axis = {-1.0F, 0.0F, 0.0F};
    }
    if (distanceToMinY < nearestDistance) {
        nearestDistance = distanceToMinY;
        axis = {0.0F, 1.0F, 0.0F};
    }
    if (distanceToMaxY < nearestDistance) {
        nearestDistance = distanceToMaxY;
        axis = {0.0F, -1.0F, 0.0F};
    }
    if (distanceToMinZ < nearestDistance) {
        nearestDistance = distanceToMinZ;
        axis = {0.0F, 0.0F, 1.0F};
    }
    if (distanceToMaxZ < nearestDistance) {
        nearestDistance = distanceToMaxZ;
        axis = {0.0F, 0.0F, -1.0F};
    }
    return Math::PenetrationUVE{axis, sphereRadius + nearestDistance};
}

std::optional<Math::PenetrationUVE> ComputeCapsuleAabbPenetrationUVE(
    const Math::AabbUVE& box, const Math::Vector3UVE segmentStart, const Math::Vector3UVE segmentEnd,
    const float capsuleRadius) noexcept {
    if (!IsFiniteAabbUVE(box) || !std::isfinite(segmentStart.x) || !std::isfinite(segmentStart.y) ||
        !std::isfinite(segmentStart.z) || !std::isfinite(segmentEnd.x) || !std::isfinite(segmentEnd.y) ||
        !std::isfinite(segmentEnd.z) || !std::isfinite(capsuleRadius) || capsuleRadius <= 0.0F) {
        return std::nullopt;
    }

    const Math::Vector3UVE direction = segmentEnd - segmentStart;
    constexpr float kBreakpointEpsilonUVE = 1.0e-6F;
    std::array<float, 8U> breakpoints{0.0F, 1.0F, 0.0F, 0.0F, 0.0F, 0.0F, 0.0F, 0.0F};
    std::size_t breakpointCount = 2U;
    const auto AddBreakpointUVE = [&](const float value) {
        if (value > kBreakpointEpsilonUVE && value < 1.0F - kBreakpointEpsilonUVE &&
            breakpointCount < breakpoints.size()) {
            breakpoints[breakpointCount++] = value;
        }
    };
    const auto AddAxisBreakpointsUVE = [&](const float start, const float delta, const float minimum,
                                           const float maximum) {
        if (std::fabs(delta) <= kBreakpointEpsilonUVE) {
            return;
        }
        AddBreakpointUVE((minimum - start) / delta);
        AddBreakpointUVE((maximum - start) / delta);
    };
    AddAxisBreakpointsUVE(segmentStart.x, direction.x, box.min.x, box.max.x);
    AddAxisBreakpointsUVE(segmentStart.y, direction.y, box.min.y, box.max.y);
    AddAxisBreakpointsUVE(segmentStart.z, direction.z, box.min.z, box.max.z);

    std::sort(breakpoints.begin(), breakpoints.begin() + static_cast<std::ptrdiff_t>(breakpointCount));
    std::size_t uniqueCount = 0U;
    for (std::size_t index = 0U; index < breakpointCount; ++index) {
        if (uniqueCount == 0U || std::fabs(breakpoints[index] - breakpoints[uniqueCount - 1U]) >
                                      kBreakpointEpsilonUVE) {
            breakpoints[uniqueCount++] = breakpoints[index];
        }
    }

    float bestDistanceSquared = std::numeric_limits<float>::infinity();
    float bestTime = 0.0F;
    const auto ConsiderTimeUVE = [&](const float time) {
        const Math::Vector3UVE point = segmentStart + direction * time;
        const Math::Vector3UVE closestPoint{
            std::clamp(point.x, box.min.x, box.max.x),
            std::clamp(point.y, box.min.y, box.max.y),
            std::clamp(point.z, box.min.z, box.max.z),
        };
        const float distanceSquared = Math::LengthSquaredUVE(closestPoint - point);
        if (distanceSquared < bestDistanceSquared) {
            bestDistanceSquared = distanceSquared;
            bestTime = time;
        }
    };

    for (std::size_t index = 0U; index < uniqueCount; ++index) {
        ConsiderTimeUVE(breakpoints[index]);
    }
    for (std::size_t index = 0U; index + 1U < uniqueCount; ++index) {
        const float intervalStart = breakpoints[index];
        const float intervalEnd = breakpoints[index + 1U];
        if (intervalEnd - intervalStart <= kBreakpointEpsilonUVE) {
            continue;
        }
        const float midpoint = (intervalStart + intervalEnd) * 0.5F;
        const Math::Vector3UVE midpointPoint = segmentStart + direction * midpoint;
        double quadraticA = 0.0;
        double quadraticB = 0.0;
        const auto AccumulateAxisUVE = [&](const float start, const float delta, const float minimum,
                                           const float maximum, const float midpointValue) {
            if (midpointValue < minimum) {
                const double constant = static_cast<double>(minimum - start);
                const double slope = static_cast<double>(delta);
                quadraticA += slope * slope;
                quadraticB -= 2.0 * constant * slope;
            } else if (midpointValue > maximum) {
                const double constant = static_cast<double>(start - maximum);
                const double slope = static_cast<double>(delta);
                quadraticA += slope * slope;
                quadraticB += 2.0 * constant * slope;
            }
        };
        AccumulateAxisUVE(segmentStart.x, direction.x, box.min.x, box.max.x, midpointPoint.x);
        AccumulateAxisUVE(segmentStart.y, direction.y, box.min.y, box.max.y, midpointPoint.y);
        AccumulateAxisUVE(segmentStart.z, direction.z, box.min.z, box.max.z, midpointPoint.z);
        if (quadraticA > 0.0) {
            const float stationary = static_cast<float>(-quadraticB / (2.0 * quadraticA));
            if (stationary > intervalStart && stationary < intervalEnd) {
                ConsiderTimeUVE(stationary);
            }
        }
    }

    const Math::Vector3UVE closestSegmentPoint = segmentStart + direction * bestTime;
    if (bestDistanceSquared >= capsuleRadius * capsuleRadius) {
        return std::nullopt;
    }
    if (bestDistanceSquared <= 0.0F) {
        return ComputeSphereAabbPenetrationUVE(box, closestSegmentPoint, capsuleRadius);
    }
    const Math::Vector3UVE closestBoxPoint{
        std::clamp(closestSegmentPoint.x, box.min.x, box.max.x),
        std::clamp(closestSegmentPoint.y, box.min.y, box.max.y),
        std::clamp(closestSegmentPoint.z, box.min.z, box.max.z),
    };
    const Math::Vector3UVE delta = closestBoxPoint - closestSegmentPoint;
    const float distance = std::sqrt(bestDistanceSquared);
    return Math::PenetrationUVE{delta * (1.0F / distance), capsuleRadius - distance};
}

std::optional<Math::PenetrationUVE> ComputeSphereSpherePenetrationUVE(
    const Math::Vector3UVE firstCenter, const float firstRadius, const Math::Vector3UVE secondCenter,
    const float secondRadius) noexcept {
    if (!std::isfinite(firstCenter.x) || !std::isfinite(firstCenter.y) || !std::isfinite(firstCenter.z) ||
        !std::isfinite(secondCenter.x) || !std::isfinite(secondCenter.y) ||
        !std::isfinite(secondCenter.z) || !std::isfinite(firstRadius) || !std::isfinite(secondRadius) ||
        firstRadius <= 0.0F || secondRadius <= 0.0F) {
        return std::nullopt;
    }

    const Math::Vector3UVE delta = secondCenter - firstCenter;
    const float distanceSquared = Math::LengthSquaredUVE(delta);
    const float combinedRadius = firstRadius + secondRadius;
    const float combinedRadiusSquared = combinedRadius * combinedRadius;
    if (distanceSquared >= combinedRadiusSquared) {
        return std::nullopt;
    }
    if (distanceSquared <= 0.0F) {
        return Math::PenetrationUVE{{1.0F, 0.0F, 0.0F}, combinedRadius};
    }

    const float distance = std::sqrt(distanceSquared);
    return Math::PenetrationUVE{delta * (1.0F / distance), combinedRadius - distance};
}

std::optional<Math::PenetrationUVE> ComputeCapsuleSpherePenetrationUVE(
    const Math::Vector3UVE capsuleSegmentStart, const Math::Vector3UVE capsuleSegmentEnd,
    const float capsuleRadius, const Math::Vector3UVE sphereCenter, const float sphereRadius) noexcept {
    if (!std::isfinite(capsuleSegmentStart.x) || !std::isfinite(capsuleSegmentStart.y) ||
        !std::isfinite(capsuleSegmentStart.z) || !std::isfinite(capsuleSegmentEnd.x) ||
        !std::isfinite(capsuleSegmentEnd.y) || !std::isfinite(capsuleSegmentEnd.z) ||
        !std::isfinite(capsuleRadius) || capsuleRadius <= 0.0F || !std::isfinite(sphereCenter.x) ||
        !std::isfinite(sphereCenter.y) || !std::isfinite(sphereCenter.z) || !std::isfinite(sphereRadius) ||
        sphereRadius <= 0.0F) {
        return std::nullopt;
    }

    const Math::Vector3UVE segment = capsuleSegmentEnd - capsuleSegmentStart;
    const float segmentLengthSquared = Math::LengthSquaredUVE(segment);
    float segmentTime = 0.0F;
    if (segmentLengthSquared > 0.0F) {
        segmentTime = std::clamp(
            Math::DotUVE(sphereCenter - capsuleSegmentStart, segment) / segmentLengthSquared, 0.0F, 1.0F);
    }
    const Math::Vector3UVE closestSegmentPoint = capsuleSegmentStart + segment * segmentTime;
    const Math::Vector3UVE delta = sphereCenter - closestSegmentPoint;
    const float distanceSquared = Math::LengthSquaredUVE(delta);
    const float combinedRadius = capsuleRadius + sphereRadius;
    if (distanceSquared >= combinedRadius * combinedRadius) {
        return std::nullopt;
    }
    if (distanceSquared <= 0.0F) {
        return Math::PenetrationUVE{{1.0F, 0.0F, 0.0F}, combinedRadius};
    }

    const float distance = std::sqrt(distanceSquared);
    return Math::PenetrationUVE{delta * (1.0F / distance), combinedRadius - distance};
}

std::optional<Math::PenetrationUVE> ComputeCapsuleCapsulePenetrationUVE(
    const Math::Vector3UVE firstSegmentStart, const Math::Vector3UVE firstSegmentEnd, const float firstRadius,
    const Math::Vector3UVE secondSegmentStart, const Math::Vector3UVE secondSegmentEnd,
    const float secondRadius) noexcept {
    const auto IsFiniteVectorUVE = [](const Math::Vector3UVE value) noexcept {
        return std::isfinite(value.x) && std::isfinite(value.y) && std::isfinite(value.z);
    };
    if (!IsFiniteVectorUVE(firstSegmentStart) || !IsFiniteVectorUVE(firstSegmentEnd) ||
        !IsFiniteVectorUVE(secondSegmentStart) || !IsFiniteVectorUVE(secondSegmentEnd) ||
        !std::isfinite(firstRadius) || firstRadius <= 0.0F || !std::isfinite(secondRadius) || secondRadius <= 0.0F) {
        return std::nullopt;
    }

    const Math::Vector3UVE firstDirection = firstSegmentEnd - firstSegmentStart;
    const Math::Vector3UVE secondDirection = secondSegmentEnd - secondSegmentStart;
    const double firstLengthSquared = static_cast<double>(Math::LengthSquaredUVE(firstDirection));
    const double secondLengthSquared = static_cast<double>(Math::LengthSquaredUVE(secondDirection));
    const double directionDot = static_cast<double>(Math::DotUVE(firstDirection, secondDirection));
    const Math::Vector3UVE offset = secondSegmentStart - firstSegmentStart;
    const double firstOffsetDot = static_cast<double>(Math::DotUVE(firstDirection, offset));
    const double secondOffsetDot = static_cast<double>(Math::DotUVE(secondDirection, offset));

    float bestFirstTime = 0.0F;
    float bestSecondTime = 0.0F;
    float bestDistanceSquared = std::numeric_limits<float>::infinity();
    const auto ConsiderPairUVE = [&](const float firstTime, const float secondTime) {
        const Math::Vector3UVE firstPoint = firstSegmentStart + firstDirection * firstTime;
        const Math::Vector3UVE secondPoint = secondSegmentStart + secondDirection * secondTime;
        const float distanceSquared = Math::LengthSquaredUVE(secondPoint - firstPoint);
        if (distanceSquared < bestDistanceSquared) {
            bestDistanceSquared = distanceSquared;
            bestFirstTime = firstTime;
            bestSecondTime = secondTime;
        }
    };
    const auto ClosestSecondTimeUVE = [&](const Math::Vector3UVE firstPoint) {
        if (secondLengthSquared <= 0.0) {
            return 0.0F;
        }
        return std::clamp(Math::DotUVE(firstPoint - secondSegmentStart, secondDirection) /
                              static_cast<float>(secondLengthSquared),
                          0.0F, 1.0F);
    };
    const auto ClosestFirstTimeUVE = [&](const Math::Vector3UVE secondPoint) {
        if (firstLengthSquared <= 0.0) {
            return 0.0F;
        }
        return std::clamp(Math::DotUVE(secondPoint - firstSegmentStart, firstDirection) /
                              static_cast<float>(firstLengthSquared),
                          0.0F, 1.0F);
    };

    ConsiderPairUVE(0.0F, ClosestSecondTimeUVE(firstSegmentStart));
    ConsiderPairUVE(1.0F, ClosestSecondTimeUVE(firstSegmentEnd));
    ConsiderPairUVE(ClosestFirstTimeUVE(secondSegmentStart), 0.0F);
    ConsiderPairUVE(ClosestFirstTimeUVE(secondSegmentEnd), 1.0F);

    const double denominator = firstLengthSquared * secondLengthSquared - directionDot * directionDot;
    constexpr double kParallelEpsilonUVE = 1.0e-12;
    if (denominator > kParallelEpsilonUVE) {
        const double firstTime = (directionDot * secondOffsetDot - secondLengthSquared * firstOffsetDot) /
                                 denominator;
        const double secondTime = (firstLengthSquared * secondOffsetDot - directionDot * firstOffsetDot) /
                                  denominator;
        if (firstTime >= 0.0 && firstTime <= 1.0 && secondTime >= 0.0 && secondTime <= 1.0) {
            ConsiderPairUVE(static_cast<float>(firstTime), static_cast<float>(secondTime));
        }
    }

    const Math::Vector3UVE firstPoint = firstSegmentStart + firstDirection * bestFirstTime;
    const Math::Vector3UVE secondPoint = secondSegmentStart + secondDirection * bestSecondTime;
    const Math::Vector3UVE delta = secondPoint - firstPoint;
    const float combinedRadius = firstRadius + secondRadius;
    if (bestDistanceSquared >= combinedRadius * combinedRadius) {
        return std::nullopt;
    }
    if (bestDistanceSquared <= 0.0F) {
        return Math::PenetrationUVE{{1.0F, 0.0F, 0.0F}, combinedRadius};
    }

    const float distance = std::sqrt(bestDistanceSquared);
    return Math::PenetrationUVE{delta * (1.0F / distance), combinedRadius - distance};
}

std::optional<Math::PenetrationUVE> ComputeSphereOrientedBoxPenetrationUVE(
    const Math::Vector3UVE boxCenter, const Math::Vector3UVE boxHalfExtents,
    const Math::QuaternionUVE boxRotation, const Math::Vector3UVE sphereCenter,
    const float sphereRadius) noexcept {
    if (!IsFiniteVectorUVE(boxCenter) || !IsFiniteVectorUVE(sphereCenter) || !std::isfinite(sphereRadius) ||
        sphereRadius <= 0.0F) {
        return std::nullopt;
    }
    Math::QuaternionUVE normalizedRotation;
    Math::QuaternionUVE inverseRotation;
    if (!TryBuildOrientedBoxFrameUVE(boxHalfExtents, boxRotation, normalizedRotation, inverseRotation)) {
        return std::nullopt;
    }

    const Math::Vector3UVE localSphereCenter =
        Math::RotateVectorUVE(inverseRotation, sphereCenter - boxCenter);
    const Math::AabbUVE localBox = Math::AabbUVE::FromCenterExtentsUVE({0.0F, 0.0F, 0.0F}, boxHalfExtents);
    const std::optional<Math::PenetrationUVE> localPenetration =
        ComputeSphereAabbPenetrationUVE(localBox, localSphereCenter, sphereRadius);
    if (!localPenetration.has_value()) {
        return std::nullopt;
    }
    return Math::PenetrationUVE{
        Math::RotateVectorUVE(normalizedRotation, localPenetration->axis), localPenetration->depth};
}

std::optional<Math::PenetrationUVE> ComputeCapsuleOrientedBoxPenetrationUVE(
    const Math::Vector3UVE boxCenter, const Math::Vector3UVE boxHalfExtents,
    const Math::QuaternionUVE boxRotation, const Math::Vector3UVE capsuleSegmentStart,
    const Math::Vector3UVE capsuleSegmentEnd, const float capsuleRadius) noexcept {
    if (!IsFiniteVectorUVE(boxCenter) || !IsFiniteVectorUVE(capsuleSegmentStart) ||
        !IsFiniteVectorUVE(capsuleSegmentEnd) || !std::isfinite(capsuleRadius) || capsuleRadius <= 0.0F) {
        return std::nullopt;
    }
    Math::QuaternionUVE normalizedRotation;
    Math::QuaternionUVE inverseRotation;
    if (!TryBuildOrientedBoxFrameUVE(boxHalfExtents, boxRotation, normalizedRotation, inverseRotation)) {
        return std::nullopt;
    }

    const Math::Vector3UVE localSegmentStart =
        Math::RotateVectorUVE(inverseRotation, capsuleSegmentStart - boxCenter);
    const Math::Vector3UVE localSegmentEnd = Math::RotateVectorUVE(inverseRotation, capsuleSegmentEnd - boxCenter);
    const Math::AabbUVE localBox = Math::AabbUVE::FromCenterExtentsUVE({0.0F, 0.0F, 0.0F}, boxHalfExtents);
    const std::optional<Math::PenetrationUVE> localPenetration = ComputeCapsuleAabbPenetrationUVE(
        localBox, localSegmentStart, localSegmentEnd, capsuleRadius);
    if (!localPenetration.has_value()) {
        return std::nullopt;
    }
    return Math::PenetrationUVE{
        Math::RotateVectorUVE(normalizedRotation, localPenetration->axis), localPenetration->depth};
}

} // namespace UVE::Physics::Detail

// EOF
