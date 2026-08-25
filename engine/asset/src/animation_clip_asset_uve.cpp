// Copyright (c) 2026 UniVex Studios. All Rights Reserved.

#include "uve/asset/animation_clip_asset_uve.h"

#include <cmath>
#include <exception>
#include <limits>
#include <optional>
#include <string>
#include <string_view>
#include <utility>
#include <vector>

#include <nlohmann/json.hpp>

#include "uve/asset/uve_file_envelope_uve.h"
#include "uve/debug/logging_macros_uve.h"

namespace UVE::Asset {
namespace {

using JsonUVE = nlohmann::json;
constexpr std::string_view kAnimationSchemaUVE = "uve-animation-v1";

[[nodiscard]] bool IsFiniteVectorUVE(const Math::Vector3UVE& value) noexcept {
    return std::isfinite(value.x) && std::isfinite(value.y) && std::isfinite(value.z);
}

[[nodiscard]] bool IsFinitePoseUVE(const AnimationAssetPoseUVE& pose) noexcept {
    return IsFiniteVectorUVE(pose.position) && Math::IsFiniteUVE(pose.rotation) &&
           IsFiniteVectorUVE(pose.scale);
}

[[nodiscard]] JsonUVE ToVectorJsonUVE(const Math::Vector3UVE& value) {
    return JsonUVE::array({value.x, value.y, value.z});
}

[[nodiscard]] JsonUVE ToQuaternionJsonUVE(const Math::QuaternionUVE& value) {
    return JsonUVE::array({value.x, value.y, value.z, value.w});
}

[[nodiscard]] bool ReadVectorJsonUVE(const JsonUVE& value, Math::Vector3UVE& outVector) {
    if (!value.is_array() || value.size() != 3U) {
        return false;
    }
    const Math::Vector3UVE candidate{value.at(0).get<float>(), value.at(1).get<float>(), value.at(2).get<float>()};
    if (!IsFiniteVectorUVE(candidate)) {
        return false;
    }
    outVector = candidate;
    return true;
}

[[nodiscard]] bool ReadQuaternionJsonUVE(const JsonUVE& value, Math::QuaternionUVE& outRotation) {
    if (!value.is_array() || value.size() != 4U) {
        return false;
    }
    const Math::QuaternionUVE candidate{value.at(0).get<float>(), value.at(1).get<float>(),
                                       value.at(2).get<float>(), value.at(3).get<float>()};
    Math::QuaternionUVE normalized;
    if (!Math::TryNormalizeUVE(candidate, normalized)) {
        return false;
    }
    outRotation = normalized;
    return true;
}

[[nodiscard]] bool ReadPoseJsonUVE(const JsonUVE& value, AnimationAssetPoseUVE& outPose) {
    if (!value.is_object() || !value.contains("position") || !value.contains("rotation") ||
        !value.contains("scale")) {
        return false;
    }
    AnimationAssetPoseUVE candidate;
    if (!ReadVectorJsonUVE(value.at("position"), candidate.position) ||
        !ReadQuaternionJsonUVE(value.at("rotation"), candidate.rotation) ||
        !ReadVectorJsonUVE(value.at("scale"), candidate.scale)) {
        return false;
    }
    outPose = candidate;
    return true;
}

} // namespace

bool IsAnimationClipAssetValidUVE(const AnimationClipAssetUVE& clip) noexcept {
    if (clip.clipId.empty() || clip.clipId.size() > kMaximumAnimationAssetIdentifierBytesUVE ||
        !std::isfinite(clip.durationSeconds) || clip.durationSeconds <= 0.0 || clip.samples.empty() ||
        clip.samples.size() > kMaximumAnimationAssetSamplesUVE ||
        clip.events.size() > kMaximumAnimationAssetEventsUVE) {
        return false;
    }
    double previousTime = -std::numeric_limits<double>::infinity();
    for (const AnimationAssetSampleUVE& sample : clip.samples) {
        if (!std::isfinite(sample.timeSeconds) || sample.timeSeconds < 0.0 ||
            sample.timeSeconds > clip.durationSeconds || sample.timeSeconds < previousTime ||
            !IsFinitePoseUVE(sample.pose)) {
            return false;
        }
        Math::QuaternionUVE normalized;
        if (!Math::TryNormalizeUVE(sample.pose.rotation, normalized)) {
            return false;
        }
        previousTime = sample.timeSeconds;
    }
    previousTime = -std::numeric_limits<double>::infinity();
    for (const AnimationAssetEventUVE& event : clip.events) {
        if (!std::isfinite(event.timeSeconds) || event.timeSeconds < 0.0 ||
            event.timeSeconds > clip.durationSeconds || event.timeSeconds < previousTime || event.eventId.empty() ||
            event.eventId.size() > kMaximumAnimationAssetIdentifierBytesUVE ||
            event.eventId.find('\0') != std::string::npos) {
            return false;
        }
        previousTime = event.timeSeconds;
    }
    return true;
}

bool SaveAnimationClipAssetUVE(const AnimationClipAssetUVE& clip, const std::filesystem::path& path) {
    if (!IsAnimationClipAssetValidUVE(clip)) {
        UVE_ERROR("AnimationClipAssetUVE: refusing to save invalid clip to {}", path.string());
        return false;
    }
    JsonUVE document{{"schema", kAnimationSchemaUVE}, {"clipId", clip.clipId},
                     {"durationSeconds", clip.durationSeconds}, {"samples", JsonUVE::array()},
                     {"events", JsonUVE::array()}};
    for (const AnimationAssetSampleUVE& sample : clip.samples) {
        document["samples"].push_back({{"timeSeconds", sample.timeSeconds},
                                       {"pose", {{"position", ToVectorJsonUVE(sample.pose.position)},
                                                  {"rotation", ToQuaternionJsonUVE(sample.pose.rotation)},
                                                  {"scale", ToVectorJsonUVE(sample.pose.scale)}}}});
    }
    for (const AnimationAssetEventUVE& event : clip.events) {
        document["events"].push_back({{"timeSeconds", event.timeSeconds}, {"eventId", event.eventId}});
    }
    const std::string serialized = document.dump();
    if (serialized.empty() || serialized.size() > kMaximumAnimationAssetPayloadBytesUVE) {
        UVE_ERROR("AnimationClipAssetUVE: serialized payload is empty or oversized for {}", path.string());
        return false;
    }
    const auto* const bytes = reinterpret_cast<const std::byte*>(serialized.data());
    return WriteUveFileUVE(path, AssetKindUVE::Animation, std::vector<std::byte>(bytes, bytes + serialized.size()));
}

bool LoadAnimationClipAssetUVE(const std::filesystem::path& path, AnimationClipAssetUVE& outClip) {
    const std::optional<std::pair<UveFileHeaderUVE, std::vector<std::byte>>> file = ReadUveFileUVE(path);
    if (!file.has_value() || file->first.assetType != AssetKindUVE::Animation || file->second.empty() ||
        file->second.size() > kMaximumAnimationAssetPayloadBytesUVE) {
        UVE_ERROR("AnimationClipAssetUVE: invalid animation envelope {}", path.string());
        return false;
    }
    try {
        const std::string serialized(reinterpret_cast<const char*>(file->second.data()), file->second.size());
        const JsonUVE document = JsonUVE::parse(serialized);
        if (!document.is_object() || document.value("schema", "") != kAnimationSchemaUVE ||
            !document.contains("clipId") || !document.contains("durationSeconds") ||
            !document.contains("samples") || !document.contains("events")) {
            return false;
        }
        AnimationClipAssetUVE candidate;
        candidate.clipId = document.at("clipId").get<std::string>();
        candidate.durationSeconds = document.at("durationSeconds").get<double>();
        const JsonUVE& samples = document.at("samples");
        const JsonUVE& events = document.at("events");
        if (!samples.is_array() || !events.is_array() || samples.size() > kMaximumAnimationAssetSamplesUVE ||
            events.size() > kMaximumAnimationAssetEventsUVE) {
            return false;
        }
        candidate.samples.reserve(samples.size());
        for (const JsonUVE& value : samples) {
            if (!value.is_object() || !value.contains("timeSeconds") || !value.contains("pose")) {
                return false;
            }
            AnimationAssetSampleUVE sample;
            sample.timeSeconds = value.at("timeSeconds").get<double>();
            if (!ReadPoseJsonUVE(value.at("pose"), sample.pose)) {
                return false;
            }
            candidate.samples.push_back(std::move(sample));
        }
        candidate.events.reserve(events.size());
        for (const JsonUVE& value : events) {
            if (!value.is_object() || !value.contains("timeSeconds") || !value.contains("eventId")) {
                return false;
            }
            candidate.events.push_back(
                AnimationAssetEventUVE{value.at("timeSeconds").get<double>(), value.at("eventId").get<std::string>()});
        }
        if (!IsAnimationClipAssetValidUVE(candidate)) {
            return false;
        }
        outClip = std::move(candidate);
        return true;
    } catch (const std::exception& exception) {
        UVE_ERROR("AnimationClipAssetUVE: failed to parse {}: {}", path.string(), exception.what());
        return false;
    } catch (...) {
        UVE_ERROR("AnimationClipAssetUVE: failed to parse {} with an unknown exception", path.string());
        return false;
    }
}

} // namespace UVE::Asset
