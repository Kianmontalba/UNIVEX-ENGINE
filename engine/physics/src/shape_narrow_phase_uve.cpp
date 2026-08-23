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
    const double deltaX = static_cast<double>(delta.x);
    const double deltaY = static_cast<double>(delta.y);
    const double deltaZ = static_cast<double>(delta.z);
    const double distanceSquared = deltaX * deltaX + deltaY * deltaY + deltaZ * deltaZ;
    const double radiusSquared = static_cast<double>(sphereRadius) * static_cast<double>(sphereRadius);
    if (distanceSquared > 0.0) {
        if (distanceSquared >= radiusSquared) {
            return std::nullopt;
        }
        const float distance = static_cast<float>(std::sqrt(distanceSquared));
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

    const double directionX = static_cast<double>(segmentEnd.x) - static_cast<double>(segmentStart.x);
    const double directionY = static_cast<double>(segmentEnd.y) - static_cast<double>(segmentStart.y);
    const double directionZ = static_cast<double>(segmentEnd.z) - static_cast<double>(segmentStart.z);
    if (!std::isfinite(directionX) || !std::isfinite(directionY) || !std::isfinite(directionZ)) {
        return std::nullopt;
    }
    constexpr double kBreakpointEpsilonUVE = 1.0e-6;
    std::array<double, 8U> breakpoints{0.0, 1.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0};
    std::size_t breakpointCount = 2U;
    const auto AddBreakpointUVE = [&](const double value) {
        if (std::isfinite(value) && value > kBreakpointEpsilonUVE && value < 1.0 - kBreakpointEpsilonUVE &&
            breakpointCount < breakpoints.size()) {
            breakpoints[breakpointCount++] = value;
        }
    };
    const auto AddAxisBreakpointsUVE = [&](const double start, const double delta, const float minimum,
                                           const float maximum) {
        if (std::fabs(delta) <= kBreakpointEpsilonUVE) {
            return;
        }
        AddBreakpointUVE((static_cast<double>(minimum) - start) / delta);
        AddBreakpointUVE((static_cast<double>(maximum) - start) / delta);
    };
    AddAxisBreakpointsUVE(segmentStart.x, directionX, box.min.x, box.max.x);
    AddAxisBreakpointsUVE(segmentStart.y, directionY, box.min.y, box.max.y);
    AddAxisBreakpointsUVE(segmentStart.z, directionZ, box.min.z, box.max.z);

    std::sort(breakpoints.begin(), breakpoints.begin() + static_cast<std::ptrdiff_t>(breakpointCount));
    std::size_t uniqueCount = 0U;
    for (std::size_t index = 0U; index < breakpointCount; ++index) {
        if (uniqueCount == 0U || std::fabs(breakpoints[index] - breakpoints[uniqueCount - 1U]) >
                                      kBreakpointEpsilonUVE) {
            breakpoints[uniqueCount++] = breakpoints[index];
        }
    }

    double bestDistanceSquared = std::numeric_limits<double>::infinity();
    double bestTime = 0.0;
    double bestDeltaX = 0.0;
    double bestDeltaY = 0.0;
    double bestDeltaZ = 0.0;
    const auto ConsiderTimeUVE = [&](const double time) {
        const double pointX = static_cast<double>(segmentStart.x) + directionX * time;
        const double pointY = static_cast<double>(segmentStart.y) + directionY * time;
        const double pointZ = static_cast<double>(segmentStart.z) + directionZ * time;
        const double closestX = std::clamp(pointX, static_cast<double>(box.min.x), static_cast<double>(box.max.x));
        const double closestY = std::clamp(pointY, static_cast<double>(box.min.y), static_cast<double>(box.max.y));
        const double closestZ = std::clamp(pointZ, static_cast<double>(box.min.z), static_cast<double>(box.max.z));
        const double deltaX = closestX - pointX;
        const double deltaY = closestY - pointY;
        const double deltaZ = closestZ - pointZ;
        const double scale = std::max(std::fabs(deltaX), std::max(std::fabs(deltaY), std::fabs(deltaZ)));
        if (!std::isfinite(scale)) {
            return;
        }
        double distanceSquared = 0.0;
        if (scale != 0.0) {
            const double scaledDistanceSquared = (deltaX / scale) * (deltaX / scale) +
                                                 (deltaY / scale) * (deltaY / scale) +
                                                 (deltaZ / scale) * (deltaZ / scale);
            if (!std::isfinite(scaledDistanceSquared)) {
                return;
            }
            distanceSquared = scale * scale * scaledDistanceSquared;
            if (!std::isfinite(distanceSquared)) {
                return;
            }
        }
        if (distanceSquared < bestDistanceSquared) {
            bestDistanceSquared = distanceSquared;
            bestTime = time;
            bestDeltaX = deltaX;
            bestDeltaY = deltaY;
            bestDeltaZ = deltaZ;
        }
    };

    for (std::size_t index = 0U; index < uniqueCount; ++index) {
        ConsiderTimeUVE(breakpoints[index]);
    }
    for (std::size_t index = 0U; index + 1U < uniqueCount; ++index) {
        const double intervalStart = breakpoints[index];
        const double intervalEnd = breakpoints[index + 1U];
        if (intervalEnd - intervalStart <= kBreakpointEpsilonUVE) {
            continue;
        }
        const double midpoint = (intervalStart + intervalEnd) * 0.5;
        const double midpointX = static_cast<double>(segmentStart.x) + directionX * midpoint;
        const double midpointY = static_cast<double>(segmentStart.y) + directionY * midpoint;
        const double midpointZ = static_cast<double>(segmentStart.z) + directionZ * midpoint;
        double quadraticA = 0.0;
        double quadraticB = 0.0;
        const auto AccumulateAxisUVE = [&](const double start, const double delta, const float minimum,
                                           const float maximum, const double midpointValue) {
            if (midpointValue < static_cast<double>(minimum)) {
                const double constant = static_cast<double>(minimum) - start;
                quadraticA += delta * delta;
                quadraticB -= 2.0 * constant * delta;
            } else if (midpointValue > static_cast<double>(maximum)) {
                const double constant = start - static_cast<double>(maximum);
                quadraticA += delta * delta;
                quadraticB += 2.0 * constant * delta;
            }
        };
        AccumulateAxisUVE(static_cast<double>(segmentStart.x), directionX, box.min.x, box.max.x, midpointX);
        AccumulateAxisUVE(static_cast<double>(segmentStart.y), directionY, box.min.y, box.max.y, midpointY);
        AccumulateAxisUVE(static_cast<double>(segmentStart.z), directionZ, box.min.z, box.max.z, midpointZ);
        if (quadraticA > 0.0 && std::isfinite(quadraticA) && std::isfinite(quadraticB)) {
            const double stationary = -quadraticB / (2.0 * quadraticA);
            if (std::isfinite(stationary) && stationary > intervalStart && stationary < intervalEnd) {
                ConsiderTimeUVE(stationary);
            }
        }
    }

    if (!std::isfinite(bestDistanceSquared)) {
        return std::nullopt;
    }
    const double capsuleRadiusDouble = static_cast<double>(capsuleRadius);
    const double capsuleRadiusSquared = capsuleRadiusDouble * capsuleRadiusDouble;
    const double maximumFloat = static_cast<double>(std::numeric_limits<float>::max());
    if (!std::isfinite(capsuleRadiusSquared) || bestDistanceSquared >= capsuleRadiusSquared) {
        return std::nullopt;
    }
    const double closestX = static_cast<double>(segmentStart.x) + directionX * bestTime;
    const double closestY = static_cast<double>(segmentStart.y) + directionY * bestTime;
    const double closestZ = static_cast<double>(segmentStart.z) + directionZ * bestTime;
    if (bestDistanceSquared <= 0.0) {
        const Math::Vector3UVE closestSegmentPoint{
            static_cast<float>(closestX), static_cast<float>(closestY), static_cast<float>(closestZ)};
        if (!IsFiniteVectorUVE(closestSegmentPoint)) {
            return std::nullopt;
        }
        return ComputeSphereAabbPenetrationUVE(box, closestSegmentPoint, capsuleRadius);
    }

    const double distance = std::sqrt(bestDistanceSquared);
    const double depth = capsuleRadiusDouble - distance;
    if (!std::isfinite(distance) || !std::isfinite(depth) || depth <= 0.0 || depth > maximumFloat) {
        return std::nullopt;
    }
    const double inverseDistance = 1.0 / distance;
    const Math::Vector3UVE axis{
        static_cast<float>(bestDeltaX * inverseDistance),
        static_cast<float>(bestDeltaY * inverseDistance),
        static_cast<float>(bestDeltaZ * inverseDistance),
    };
    if (!IsFiniteVectorUVE(axis)) {
        return std::nullopt;
    }
    return Math::PenetrationUVE{axis, static_cast<float>(depth)};
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

    const double deltaX = static_cast<double>(secondCenter.x) - static_cast<double>(firstCenter.x);
    const double deltaY = static_cast<double>(secondCenter.y) - static_cast<double>(firstCenter.y);
    const double deltaZ = static_cast<double>(secondCenter.z) - static_cast<double>(firstCenter.z);
    const double scale = std::max(std::fabs(deltaX), std::max(std::fabs(deltaY), std::fabs(deltaZ)));
    if (!std::isfinite(scale)) {
        return std::nullopt;
    }
    double distanceSquared = 0.0;
    if (scale != 0.0) {
        const double scaledDistanceSquared = (deltaX / scale) * (deltaX / scale) +
                                             (deltaY / scale) * (deltaY / scale) +
                                             (deltaZ / scale) * (deltaZ / scale);
        if (!std::isfinite(scaledDistanceSquared)) {
            return std::nullopt;
        }
        distanceSquared = (scale * scale) * scaledDistanceSquared;
        if (!std::isfinite(distanceSquared)) {
            return std::nullopt;
        }
    }

    const double combinedRadius = static_cast<double>(firstRadius) + static_cast<double>(secondRadius);
    const double maximumFloat = static_cast<double>(std::numeric_limits<float>::max());
    if (!std::isfinite(combinedRadius) || combinedRadius <= 0.0 || combinedRadius > maximumFloat) {
        return std::nullopt;
    }
    const double combinedRadiusSquared = combinedRadius * combinedRadius;
    if (!std::isfinite(combinedRadiusSquared) || distanceSquared >= combinedRadiusSquared) {
        return std::nullopt;
    }
    if (distanceSquared == 0.0) {
        return Math::PenetrationUVE{{1.0F, 0.0F, 0.0F}, static_cast<float>(combinedRadius)};
    }

    const double distance = std::sqrt(distanceSquared);
    const double depth = combinedRadius - distance;
    if (!std::isfinite(distance) || !std::isfinite(depth) || depth <= 0.0 || depth > maximumFloat) {
        return std::nullopt;
    }
    const double inverseDistance = 1.0 / distance;
    const Math::Vector3UVE axis{
        static_cast<float>(deltaX * inverseDistance),
        static_cast<float>(deltaY * inverseDistance),
        static_cast<float>(deltaZ * inverseDistance),
    };
    if (!IsFiniteVectorUVE(axis)) {
        return std::nullopt;
    }
    return Math::PenetrationUVE{axis, static_cast<float>(depth)};
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

    const double segmentX = static_cast<double>(capsuleSegmentEnd.x) -
                            static_cast<double>(capsuleSegmentStart.x);
    const double segmentY = static_cast<double>(capsuleSegmentEnd.y) -
                            static_cast<double>(capsuleSegmentStart.y);
    const double segmentZ = static_cast<double>(capsuleSegmentEnd.z) -
                            static_cast<double>(capsuleSegmentStart.z);
    const double segmentLengthSquared = segmentX * segmentX + segmentY * segmentY + segmentZ * segmentZ;
    if (!std::isfinite(segmentLengthSquared)) {
        return std::nullopt;
    }
    double segmentTime = 0.0;
    if (segmentLengthSquared > 0.0) {
        const double offsetX = static_cast<double>(sphereCenter.x) -
                               static_cast<double>(capsuleSegmentStart.x);
        const double offsetY = static_cast<double>(sphereCenter.y) -
                               static_cast<double>(capsuleSegmentStart.y);
        const double offsetZ = static_cast<double>(sphereCenter.z) -
                               static_cast<double>(capsuleSegmentStart.z);
        const double projection = (offsetX * segmentX + offsetY * segmentY + offsetZ * segmentZ) /
                                  segmentLengthSquared;
        if (!std::isfinite(projection)) {
            return std::nullopt;
        }
        segmentTime = std::clamp(projection, 0.0, 1.0);
    }

    const double closestX = static_cast<double>(capsuleSegmentStart.x) + segmentX * segmentTime;
    const double closestY = static_cast<double>(capsuleSegmentStart.y) + segmentY * segmentTime;
    const double closestZ = static_cast<double>(capsuleSegmentStart.z) + segmentZ * segmentTime;
    const double deltaX = static_cast<double>(sphereCenter.x) - closestX;
    const double deltaY = static_cast<double>(sphereCenter.y) - closestY;
    const double deltaZ = static_cast<double>(sphereCenter.z) - closestZ;
    const double scale = std::max(std::fabs(deltaX), std::max(std::fabs(deltaY), std::fabs(deltaZ)));
    if (!std::isfinite(scale)) {
        return std::nullopt;
    }
    double distanceSquared = 0.0;
    if (scale != 0.0) {
        const double scaledDistanceSquared = (deltaX / scale) * (deltaX / scale) +
                                             (deltaY / scale) * (deltaY / scale) +
                                             (deltaZ / scale) * (deltaZ / scale);
        if (!std::isfinite(scaledDistanceSquared)) {
            return std::nullopt;
        }
        distanceSquared = (scale * scale) * scaledDistanceSquared;
        if (!std::isfinite(distanceSquared)) {
            return std::nullopt;
        }
    }

    const double combinedRadius = static_cast<double>(capsuleRadius) + static_cast<double>(sphereRadius);
    const double maximumFloat = static_cast<double>(std::numeric_limits<float>::max());
    if (!std::isfinite(combinedRadius) || combinedRadius <= 0.0 || combinedRadius > maximumFloat) {
        return std::nullopt;
    }
    const double combinedRadiusSquared = combinedRadius * combinedRadius;
    if (!std::isfinite(combinedRadiusSquared) || distanceSquared >= combinedRadiusSquared) {
        return std::nullopt;
    }
    if (distanceSquared == 0.0) {
        return Math::PenetrationUVE{{1.0F, 0.0F, 0.0F}, static_cast<float>(combinedRadius)};
    }

    const double distance = std::sqrt(distanceSquared);
    const double depth = combinedRadius - distance;
    if (!std::isfinite(distance) || !std::isfinite(depth) || depth <= 0.0 || depth > maximumFloat) {
        return std::nullopt;
    }
    const double inverseDistance = 1.0 / distance;
    const Math::Vector3UVE axis{
        static_cast<float>(deltaX * inverseDistance),
        static_cast<float>(deltaY * inverseDistance),
        static_cast<float>(deltaZ * inverseDistance),
    };
    if (!IsFiniteVectorUVE(axis)) {
        return std::nullopt;
    }
    return Math::PenetrationUVE{axis, static_cast<float>(depth)};
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

    const double firstDirectionX = static_cast<double>(firstSegmentEnd.x) - static_cast<double>(firstSegmentStart.x);
    const double firstDirectionY = static_cast<double>(firstSegmentEnd.y) - static_cast<double>(firstSegmentStart.y);
    const double firstDirectionZ = static_cast<double>(firstSegmentEnd.z) - static_cast<double>(firstSegmentStart.z);
    const double secondDirectionX = static_cast<double>(secondSegmentEnd.x) - static_cast<double>(secondSegmentStart.x);
    const double secondDirectionY = static_cast<double>(secondSegmentEnd.y) - static_cast<double>(secondSegmentStart.y);
    const double secondDirectionZ = static_cast<double>(secondSegmentEnd.z) - static_cast<double>(secondSegmentStart.z);
    const double firstLengthSquared = firstDirectionX * firstDirectionX + firstDirectionY * firstDirectionY +
                                      firstDirectionZ * firstDirectionZ;
    const double secondLengthSquared = secondDirectionX * secondDirectionX + secondDirectionY * secondDirectionY +
                                       secondDirectionZ * secondDirectionZ;
    const double directionDot = firstDirectionX * secondDirectionX + firstDirectionY * secondDirectionY +
                                firstDirectionZ * secondDirectionZ;
    const double offsetX = static_cast<double>(secondSegmentStart.x) - static_cast<double>(firstSegmentStart.x);
    const double offsetY = static_cast<double>(secondSegmentStart.y) - static_cast<double>(firstSegmentStart.y);
    const double offsetZ = static_cast<double>(secondSegmentStart.z) - static_cast<double>(firstSegmentStart.z);
    const double firstOffsetDot = firstDirectionX * offsetX + firstDirectionY * offsetY + firstDirectionZ * offsetZ;
    const double secondOffsetDot = secondDirectionX * offsetX + secondDirectionY * offsetY + secondDirectionZ * offsetZ;
    if (!std::isfinite(firstLengthSquared) || !std::isfinite(secondLengthSquared) || !std::isfinite(directionDot) ||
        !std::isfinite(firstOffsetDot) || !std::isfinite(secondOffsetDot)) {
        return std::nullopt;
    }

    double bestFirstTime = 0.0;
    double bestSecondTime = 0.0;
    double bestDistanceSquared = std::numeric_limits<double>::infinity();
    double bestDeltaX = 0.0;
    double bestDeltaY = 0.0;
    double bestDeltaZ = 0.0;
    const auto ConsiderPairUVE = [&](const double firstTime, const double secondTime) {
        const double firstPointX = static_cast<double>(firstSegmentStart.x) + firstDirectionX * firstTime;
        const double firstPointY = static_cast<double>(firstSegmentStart.y) + firstDirectionY * firstTime;
        const double firstPointZ = static_cast<double>(firstSegmentStart.z) + firstDirectionZ * firstTime;
        const double secondPointX = static_cast<double>(secondSegmentStart.x) + secondDirectionX * secondTime;
        const double secondPointY = static_cast<double>(secondSegmentStart.y) + secondDirectionY * secondTime;
        const double secondPointZ = static_cast<double>(secondSegmentStart.z) + secondDirectionZ * secondTime;
        const double deltaX = secondPointX - firstPointX;
        const double deltaY = secondPointY - firstPointY;
        const double deltaZ = secondPointZ - firstPointZ;
        const double scale = std::max(std::fabs(deltaX), std::max(std::fabs(deltaY), std::fabs(deltaZ)));
        if (!std::isfinite(scale)) {
            return;
        }
        double distanceSquared = 0.0;
        if (scale != 0.0) {
            const double scaledDistanceSquared = (deltaX / scale) * (deltaX / scale) +
                                                 (deltaY / scale) * (deltaY / scale) +
                                                 (deltaZ / scale) * (deltaZ / scale);
            if (!std::isfinite(scaledDistanceSquared)) {
                return;
            }
            distanceSquared = (scale * scale) * scaledDistanceSquared;
            if (!std::isfinite(distanceSquared)) {
                return;
            }
        }
        if (distanceSquared < bestDistanceSquared) {
            bestDistanceSquared = distanceSquared;
            bestFirstTime = firstTime;
            bestSecondTime = secondTime;
            bestDeltaX = deltaX;
            bestDeltaY = deltaY;
            bestDeltaZ = deltaZ;
        }
    };
    const auto ClosestSecondTimeUVE = [&](const double pointX, const double pointY, const double pointZ) {
        if (secondLengthSquared <= 0.0) {
            return 0.0;
        }
        const double relativeX = pointX - static_cast<double>(secondSegmentStart.x);
        const double relativeY = pointY - static_cast<double>(secondSegmentStart.y);
        const double relativeZ = pointZ - static_cast<double>(secondSegmentStart.z);
        return std::clamp((relativeX * secondDirectionX + relativeY * secondDirectionY +
                           relativeZ * secondDirectionZ) /
                              secondLengthSquared,
                          0.0, 1.0);
    };
    const auto ClosestFirstTimeUVE = [&](const double pointX, const double pointY, const double pointZ) {
        if (firstLengthSquared <= 0.0) {
            return 0.0;
        }
        const double relativeX = pointX - static_cast<double>(firstSegmentStart.x);
        const double relativeY = pointY - static_cast<double>(firstSegmentStart.y);
        const double relativeZ = pointZ - static_cast<double>(firstSegmentStart.z);
        return std::clamp((relativeX * firstDirectionX + relativeY * firstDirectionY +
                           relativeZ * firstDirectionZ) /
                              firstLengthSquared,
                          0.0, 1.0);
    };

    ConsiderPairUVE(0.0, ClosestSecondTimeUVE(firstSegmentStart.x, firstSegmentStart.y, firstSegmentStart.z));
    ConsiderPairUVE(1.0, ClosestSecondTimeUVE(firstSegmentEnd.x, firstSegmentEnd.y, firstSegmentEnd.z));
    ConsiderPairUVE(ClosestFirstTimeUVE(secondSegmentStart.x, secondSegmentStart.y, secondSegmentStart.z), 0.0);
    ConsiderPairUVE(ClosestFirstTimeUVE(secondSegmentEnd.x, secondSegmentEnd.y, secondSegmentEnd.z), 1.0);

    const double denominator = firstLengthSquared * secondLengthSquared - directionDot * directionDot;
    constexpr double kParallelEpsilonUVE = 1.0e-12;
    if (denominator > kParallelEpsilonUVE) {
        const double firstTime = (directionDot * secondOffsetDot - secondLengthSquared * firstOffsetDot) /
                                 denominator;
        const double secondTime = (firstLengthSquared * secondOffsetDot - directionDot * firstOffsetDot) /
                                  denominator;
        if (std::isfinite(firstTime) && std::isfinite(secondTime) && firstTime >= 0.0 && firstTime <= 1.0 &&
            secondTime >= 0.0 && secondTime <= 1.0) {
            ConsiderPairUVE(firstTime, secondTime);
        }
    }
    if (!std::isfinite(bestDistanceSquared)) {
        return std::nullopt;
    }

    const double combinedRadius = static_cast<double>(firstRadius) + static_cast<double>(secondRadius);
    const double maximumFloat = static_cast<double>(std::numeric_limits<float>::max());
    if (!std::isfinite(combinedRadius) || combinedRadius <= 0.0 || combinedRadius > maximumFloat) {
        return std::nullopt;
    }
    const double combinedRadiusSquared = combinedRadius * combinedRadius;
    if (!std::isfinite(combinedRadiusSquared) || bestDistanceSquared >= combinedRadiusSquared) {
        return std::nullopt;
    }
    if (bestDistanceSquared == 0.0) {
        return Math::PenetrationUVE{{1.0F, 0.0F, 0.0F}, static_cast<float>(combinedRadius)};
    }

    const double distance = std::sqrt(bestDistanceSquared);
    const double depth = combinedRadius - distance;
    if (!std::isfinite(distance) || !std::isfinite(depth) || depth <= 0.0 || depth > maximumFloat) {
        return std::nullopt;
    }
    const double inverseDistance = 1.0 / distance;
    const Math::Vector3UVE axis{
        static_cast<float>(bestDeltaX * inverseDistance),
        static_cast<float>(bestDeltaY * inverseDistance),
        static_cast<float>(bestDeltaZ * inverseDistance),
    };
    if (!IsFiniteVectorUVE(axis)) {
        return std::nullopt;
    }
    return Math::PenetrationUVE{axis, static_cast<float>(depth)};
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

    const double centerDeltaX = static_cast<double>(sphereCenter.x) - static_cast<double>(boxCenter.x);
    const double centerDeltaY = static_cast<double>(sphereCenter.y) - static_cast<double>(boxCenter.y);
    const double centerDeltaZ = static_cast<double>(sphereCenter.z) - static_cast<double>(boxCenter.z);
    const double centerScale = std::max(1.0, std::max(std::fabs(centerDeltaX),
                                                      std::max(std::fabs(centerDeltaY), std::fabs(centerDeltaZ))));
    if (!std::isfinite(centerScale)) {
        return std::nullopt;
    }
    const Math::Vector3UVE scaledSphereOffset{
        static_cast<float>(centerDeltaX / centerScale),
        static_cast<float>(centerDeltaY / centerScale),
        static_cast<float>(centerDeltaZ / centerScale),
    };
    const Math::Vector3UVE scaledHalfExtents{
        static_cast<float>(static_cast<double>(boxHalfExtents.x) / centerScale),
        static_cast<float>(static_cast<double>(boxHalfExtents.y) / centerScale),
        static_cast<float>(static_cast<double>(boxHalfExtents.z) / centerScale),
    };
    const float scaledSphereRadius = static_cast<float>(static_cast<double>(sphereRadius) / centerScale);
    if (!IsFiniteVectorUVE(scaledSphereOffset) || !IsFiniteVectorUVE(scaledHalfExtents) ||
        !std::isfinite(scaledSphereRadius) || scaledSphereRadius <= 0.0F ||
        scaledHalfExtents.x <= 0.0F || scaledHalfExtents.y <= 0.0F || scaledHalfExtents.z <= 0.0F) {
        return std::nullopt;
    }
    const Math::Vector3UVE localSphereCenter = Math::RotateVectorUVE(inverseRotation, scaledSphereOffset);
    const Math::AabbUVE localBox =
        Math::AabbUVE::FromCenterExtentsUVE({0.0F, 0.0F, 0.0F}, scaledHalfExtents);
    const std::optional<Math::PenetrationUVE> localPenetration =
        ComputeSphereAabbPenetrationUVE(localBox, localSphereCenter, scaledSphereRadius);
    if (!localPenetration.has_value()) {
        return std::nullopt;
    }
    const double worldDepth = static_cast<double>(localPenetration->depth) * centerScale;
    const double maximumFloat = static_cast<double>(std::numeric_limits<float>::max());
    const Math::Vector3UVE worldAxis = Math::RotateVectorUVE(normalizedRotation, localPenetration->axis);
    if (!std::isfinite(worldDepth) || worldDepth <= 0.0 || worldDepth > maximumFloat ||
        !IsFiniteVectorUVE(worldAxis)) {
        return std::nullopt;
    }
    return Math::PenetrationUVE{worldAxis, static_cast<float>(worldDepth)};
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

    const double startOffsetX = static_cast<double>(capsuleSegmentStart.x) - static_cast<double>(boxCenter.x);
    const double startOffsetY = static_cast<double>(capsuleSegmentStart.y) - static_cast<double>(boxCenter.y);
    const double startOffsetZ = static_cast<double>(capsuleSegmentStart.z) - static_cast<double>(boxCenter.z);
    const double endOffsetX = static_cast<double>(capsuleSegmentEnd.x) - static_cast<double>(boxCenter.x);
    const double endOffsetY = static_cast<double>(capsuleSegmentEnd.y) - static_cast<double>(boxCenter.y);
    const double endOffsetZ = static_cast<double>(capsuleSegmentEnd.z) - static_cast<double>(boxCenter.z);
    const double geometryScale = std::max(
        1.0,
        std::max(std::fabs(startOffsetX),
                 std::max(std::fabs(startOffsetY),
                          std::max(std::fabs(startOffsetZ),
                                   std::max(std::fabs(endOffsetX),
                                            std::max(std::fabs(endOffsetY),
                                                     std::max(std::fabs(endOffsetZ),
                                                              std::max(static_cast<double>(boxHalfExtents.x),
                                                                       std::max(static_cast<double>(boxHalfExtents.y),
                                                                                std::max(static_cast<double>(boxHalfExtents.z),
                                                                                         static_cast<double>(capsuleRadius)))))))))));
    if (!std::isfinite(geometryScale)) {
        return std::nullopt;
    }
    const Math::Vector3UVE scaledSegmentStart{
        static_cast<float>(startOffsetX / geometryScale),
        static_cast<float>(startOffsetY / geometryScale),
        static_cast<float>(startOffsetZ / geometryScale),
    };
    const Math::Vector3UVE scaledSegmentEnd{
        static_cast<float>(endOffsetX / geometryScale),
        static_cast<float>(endOffsetY / geometryScale),
        static_cast<float>(endOffsetZ / geometryScale),
    };
    const Math::Vector3UVE scaledHalfExtents{
        static_cast<float>(static_cast<double>(boxHalfExtents.x) / geometryScale),
        static_cast<float>(static_cast<double>(boxHalfExtents.y) / geometryScale),
        static_cast<float>(static_cast<double>(boxHalfExtents.z) / geometryScale),
    };
    const float scaledCapsuleRadius = static_cast<float>(static_cast<double>(capsuleRadius) / geometryScale);
    if (!IsFiniteVectorUVE(scaledSegmentStart) || !IsFiniteVectorUVE(scaledSegmentEnd) ||
        !IsFiniteVectorUVE(scaledHalfExtents) || !std::isfinite(scaledCapsuleRadius) ||
        scaledCapsuleRadius <= 0.0F || scaledHalfExtents.x <= 0.0F || scaledHalfExtents.y <= 0.0F ||
        scaledHalfExtents.z <= 0.0F) {
        return std::nullopt;
    }
    const Math::Vector3UVE localSegmentStart = Math::RotateVectorUVE(
        inverseRotation, scaledSegmentStart);
    const Math::Vector3UVE localSegmentEnd = Math::RotateVectorUVE(inverseRotation, scaledSegmentEnd);
    const Math::AabbUVE localBox =
        Math::AabbUVE::FromCenterExtentsUVE({0.0F, 0.0F, 0.0F}, scaledHalfExtents);
    const std::optional<Math::PenetrationUVE> localPenetration = ComputeCapsuleAabbPenetrationUVE(
        localBox, localSegmentStart, localSegmentEnd, scaledCapsuleRadius);
    if (!localPenetration.has_value()) {
        return std::nullopt;
    }
    const double worldDepth = static_cast<double>(localPenetration->depth) * geometryScale;
    const double maximumFloat = static_cast<double>(std::numeric_limits<float>::max());
    const Math::Vector3UVE worldAxis = Math::RotateVectorUVE(normalizedRotation, localPenetration->axis);
    if (!std::isfinite(worldDepth) || worldDepth <= 0.0 || worldDepth > maximumFloat ||
        !IsFiniteVectorUVE(worldAxis)) {
        return std::nullopt;
    }
    return Math::PenetrationUVE{worldAxis, static_cast<float>(worldDepth)};
}

std::optional<Math::PenetrationUVE> ComputeOrientedBoxOrientedBoxPenetrationUVE(
    const Math::Vector3UVE firstCenter, const Math::Vector3UVE firstHalfExtents,
    const Math::QuaternionUVE firstRotation, const Math::Vector3UVE secondCenter,
    const Math::Vector3UVE secondHalfExtents, const Math::QuaternionUVE secondRotation) noexcept {
    if (!IsFiniteVectorUVE(firstCenter) || !IsFiniteVectorUVE(secondCenter)) {
        return std::nullopt;
    }
    Math::QuaternionUVE normalizedFirstRotation;
    Math::QuaternionUVE inverseFirstRotation;
    Math::QuaternionUVE normalizedSecondRotation;
    Math::QuaternionUVE inverseSecondRotation;
    if (!TryBuildOrientedBoxFrameUVE(firstHalfExtents, firstRotation, normalizedFirstRotation,
                                     inverseFirstRotation) ||
        !TryBuildOrientedBoxFrameUVE(secondHalfExtents, secondRotation, normalizedSecondRotation,
                                     inverseSecondRotation)) {
        return std::nullopt;
    }

    const std::array<Math::Vector3UVE, 3> firstAxes{
        Math::RotateVectorUVE(normalizedFirstRotation, {1.0F, 0.0F, 0.0F}),
        Math::RotateVectorUVE(normalizedFirstRotation, {0.0F, 1.0F, 0.0F}),
        Math::RotateVectorUVE(normalizedFirstRotation, {0.0F, 0.0F, 1.0F}),
    };
    const std::array<Math::Vector3UVE, 3> secondAxes{
        Math::RotateVectorUVE(normalizedSecondRotation, {1.0F, 0.0F, 0.0F}),
        Math::RotateVectorUVE(normalizedSecondRotation, {0.0F, 1.0F, 0.0F}),
        Math::RotateVectorUVE(normalizedSecondRotation, {0.0F, 0.0F, 1.0F}),
    };
    const double centerDeltaX = static_cast<double>(secondCenter.x) - static_cast<double>(firstCenter.x);
    const double centerDeltaY = static_cast<double>(secondCenter.y) - static_cast<double>(firstCenter.y);
    const double centerDeltaZ = static_cast<double>(secondCenter.z) - static_cast<double>(firstCenter.z);
    if (!std::isfinite(centerDeltaX) || !std::isfinite(centerDeltaY) || !std::isfinite(centerDeltaZ)) {
        return std::nullopt;
    }
    constexpr double kAxisEpsilonUVE = 1.0e-6;
    (void)inverseFirstRotation;
    (void)inverseSecondRotation;

    double minimumOverlap = std::numeric_limits<double>::infinity();
    Math::Vector3UVE minimumAxis{1.0F, 0.0F, 0.0F};
    const auto TestAxisUVE = [&](Math::Vector3UVE axis) noexcept {
        const double axisLengthSquared = static_cast<double>(axis.x) * static_cast<double>(axis.x) +
                                         static_cast<double>(axis.y) * static_cast<double>(axis.y) +
                                         static_cast<double>(axis.z) * static_cast<double>(axis.z);
        if (!std::isfinite(axisLengthSquared)) {
            return false;
        }
        if (axisLengthSquared <= kAxisEpsilonUVE * kAxisEpsilonUVE) {
            return true;
        }
        const double inverseAxisLength = 1.0 / std::sqrt(axisLengthSquared);
        axis = {
            static_cast<float>(static_cast<double>(axis.x) * inverseAxisLength),
            static_cast<float>(static_cast<double>(axis.y) * inverseAxisLength),
            static_cast<float>(static_cast<double>(axis.z) * inverseAxisLength),
        };
        const double firstRadius =
            std::fabs(static_cast<double>(Math::DotUVE(axis, firstAxes[0]))) * static_cast<double>(firstHalfExtents.x) +
            std::fabs(static_cast<double>(Math::DotUVE(axis, firstAxes[1]))) * static_cast<double>(firstHalfExtents.y) +
            std::fabs(static_cast<double>(Math::DotUVE(axis, firstAxes[2]))) * static_cast<double>(firstHalfExtents.z);
        const double secondRadius =
            std::fabs(static_cast<double>(Math::DotUVE(axis, secondAxes[0]))) * static_cast<double>(secondHalfExtents.x) +
            std::fabs(static_cast<double>(Math::DotUVE(axis, secondAxes[1]))) * static_cast<double>(secondHalfExtents.y) +
            std::fabs(static_cast<double>(Math::DotUVE(axis, secondAxes[2]))) * static_cast<double>(secondHalfExtents.z);
        const double centerProjection = centerDeltaX * static_cast<double>(axis.x) +
                                        centerDeltaY * static_cast<double>(axis.y) +
                                        centerDeltaZ * static_cast<double>(axis.z);
        const double overlap = firstRadius + secondRadius - std::fabs(centerProjection);
        if (!std::isfinite(firstRadius) || !std::isfinite(secondRadius) || !std::isfinite(overlap) || overlap <= 0.0) {
            return false;
        }
        if (overlap < minimumOverlap) {
            minimumOverlap = overlap;
            minimumAxis = centerProjection < 0.0 ? -axis : axis;
        }
        return true;
    };

    for (const Math::Vector3UVE axis : firstAxes) {
        if (!TestAxisUVE(axis)) {
            return std::nullopt;
        }
    }
    for (const Math::Vector3UVE axis : secondAxes) {
        if (!TestAxisUVE(axis)) {
            return std::nullopt;
        }
    }
    for (const Math::Vector3UVE firstAxis : firstAxes) {
        for (const Math::Vector3UVE secondAxis : secondAxes) {
            if (!TestAxisUVE(Math::CrossUVE(firstAxis, secondAxis))) {
                return std::nullopt;
            }
        }
    }
    const double maximumFloat = static_cast<double>(std::numeric_limits<float>::max());
    if (!std::isfinite(minimumOverlap) || minimumOverlap > maximumFloat || !IsFiniteVectorUVE(minimumAxis)) {
        return std::nullopt;
    }
    return Math::PenetrationUVE{minimumAxis, static_cast<float>(minimumOverlap)};
}

} // namespace UVE::Physics::Detail

// EOF
