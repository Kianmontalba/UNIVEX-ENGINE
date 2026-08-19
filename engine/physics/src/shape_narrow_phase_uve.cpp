// Copyright (c) 2026 UniVex Studios. All Rights Reserved.

#include "uve/physics/detail/shape_narrow_phase_uve.h"

#include <algorithm>
#include <array>
#include <cmath>
#include <limits>

namespace UVE::Physics::Detail {
namespace {

[[nodiscard]] bool IsFiniteAabbUVE(const Math::AabbUVE& box) noexcept {
    return std::isfinite(box.min.x) && std::isfinite(box.min.y) && std::isfinite(box.min.z) &&
           std::isfinite(box.max.x) && std::isfinite(box.max.y) && std::isfinite(box.max.z) &&
           box.min.x < box.max.x && box.min.y < box.max.y && box.min.z < box.max.z;
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

} // namespace UVE::Physics::Detail

// EOF
