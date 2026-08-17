// Copyright (c) 2026 UniVex Studios. All Rights Reserved.

#pragma once

#include <cstddef>
#include <map>
#include <string>
#include <string_view>
#include <vector>

namespace UVE::Audio {

inline constexpr std::size_t kMaximumAudioMixerGroupsUVE = 32U;
inline constexpr std::size_t kMaximumAudioMixerGroupNameBytesUVE = 64U;
inline constexpr float kMinimumAudioMixerPitchMultiplierUVE = 0.25F;
inline constexpr float kMaximumAudioMixerPitchMultiplierUVE = 4.0F;
inline constexpr std::string_view kMasterAudioMixerGroupNameUVE = "Master";

struct AudioMixerGroupSnapshotUVE final {
    std::string name;
    float volumeMultiplier = 1.0F;
    float pitchMultiplier = 1.0F;
    std::size_t sourceCount = 0U;

    [[nodiscard]] bool operator==(const AudioMixerGroupSnapshotUVE&) const noexcept = default;
};

struct AudioMixerDiagnosticsUVE final {
    std::size_t groupCount = 0U;
    std::size_t inspectedGroupCount = 0U;
    std::size_t routedSourceCount = 0U;
    bool truncated = false;
    std::vector<AudioMixerGroupSnapshotUVE> groups;

    [[nodiscard]] bool operator==(const AudioMixerDiagnosticsUVE&) const noexcept = default;
};

/// Owns bounded named mixer groups above the audio device. Master always exists and cannot be
/// removed, while its multipliers remain explicitly tunable like other groups. Group multipliers are
/// validated value state; source routing is represented by copied
/// source counts and deterministic name-ordered diagnostics. Effects, streaming, clip resolution,
/// and backend/device ownership remain outside this increment.
class AudioMixerGroupUVE final {
public:
    AudioMixerGroupUVE();

    [[nodiscard]] bool RegisterGroupUVE(std::string_view name, float volumeMultiplier = 1.0F,
                                        float pitchMultiplier = 1.0F);
    [[nodiscard]] bool RemoveGroupUVE(std::string_view name);
    [[nodiscard]] bool HasGroupUVE(std::string_view name) const;
    [[nodiscard]] std::string ResolveGroupNameUVE(std::string_view name) const;

    [[nodiscard]] bool SetGroupVolumeUVE(std::string_view name, float volumeMultiplier);
    [[nodiscard]] bool SetGroupPitchUVE(std::string_view name, float pitchMultiplier);
    [[nodiscard]] float GetGroupVolumeUVE(std::string_view name) const;
    [[nodiscard]] float GetGroupPitchUVE(std::string_view name) const;

    [[nodiscard]] bool AttachSourceUVE(std::string_view name);
    [[nodiscard]] bool DetachSourceUVE(std::string_view name);
    [[nodiscard]] AudioMixerDiagnosticsUVE GetDiagnosticsUVE(std::size_t maximumGroups = kMaximumAudioMixerGroupsUVE) const;

private:
    struct GroupStateUVE final {
        float volumeMultiplier = 1.0F;
        float pitchMultiplier = 1.0F;
        std::size_t sourceCount = 0U;
    };

    [[nodiscard]] static bool IsValidNameUVE(std::string_view name) noexcept;
    [[nodiscard]] static bool IsValidVolumeMultiplierUVE(float volumeMultiplier) noexcept;
    [[nodiscard]] static bool IsValidPitchMultiplierUVE(float pitchMultiplier) noexcept;

    std::map<std::string, GroupStateUVE> m_groups;
};

} // namespace UVE::Audio
