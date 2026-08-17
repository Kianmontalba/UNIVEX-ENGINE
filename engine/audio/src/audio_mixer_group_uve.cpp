// Copyright (c) 2026 UniVex Studios. All Rights Reserved.

#include "uve/audio/audio_mixer_group_uve.h"

#include <algorithm>
#include <cmath>
#include <limits>

namespace UVE::Audio {

AudioMixerGroupUVE::AudioMixerGroupUVE() {
    m_groups.emplace(std::string(kMasterAudioMixerGroupNameUVE), GroupStateUVE{});
}

bool AudioMixerGroupUVE::IsValidNameUVE(const std::string_view name) noexcept {
    return !name.empty() && name.size() <= kMaximumAudioMixerGroupNameBytesUVE &&
           name.find('\0') == std::string_view::npos;
}

bool AudioMixerGroupUVE::IsValidVolumeMultiplierUVE(const float volumeMultiplier) noexcept {
    return std::isfinite(volumeMultiplier) && volumeMultiplier >= 0.0F && volumeMultiplier <= 1.0F;
}

bool AudioMixerGroupUVE::IsValidPitchMultiplierUVE(const float pitchMultiplier) noexcept {
    return std::isfinite(pitchMultiplier) && pitchMultiplier >= kMinimumAudioMixerPitchMultiplierUVE &&
           pitchMultiplier <= kMaximumAudioMixerPitchMultiplierUVE;
}

bool AudioMixerGroupUVE::RegisterGroupUVE(const std::string_view name, const float volumeMultiplier,
                                          const float pitchMultiplier) {
    if (!IsValidNameUVE(name) || !IsValidVolumeMultiplierUVE(volumeMultiplier) ||
        !IsValidPitchMultiplierUVE(pitchMultiplier) || m_groups.size() >= kMaximumAudioMixerGroupsUVE ||
        m_groups.contains(std::string(name))) {
        return false;
    }
    m_groups.emplace(std::string(name), GroupStateUVE{volumeMultiplier, pitchMultiplier, 0U});
    return true;
}

bool AudioMixerGroupUVE::RemoveGroupUVE(const std::string_view name) {
    if (name == kMasterAudioMixerGroupNameUVE) {
        return false;
    }
    const auto iterator = m_groups.find(std::string(name));
    if (iterator == m_groups.end() || iterator->second.sourceCount != 0U) {
        return false;
    }
    m_groups.erase(iterator);
    return true;
}

bool AudioMixerGroupUVE::HasGroupUVE(const std::string_view name) const {
    return m_groups.contains(std::string(name));
}

std::string AudioMixerGroupUVE::ResolveGroupNameUVE(const std::string_view name) const {
    return HasGroupUVE(name) ? std::string(name) : std::string(kMasterAudioMixerGroupNameUVE);
}

bool AudioMixerGroupUVE::SetGroupVolumeUVE(const std::string_view name, const float volumeMultiplier) {
    if (!IsValidVolumeMultiplierUVE(volumeMultiplier)) {
        return false;
    }
    const auto iterator = m_groups.find(std::string(name));
    if (iterator == m_groups.end()) {
        return false;
    }
    iterator->second.volumeMultiplier = volumeMultiplier;
    return true;
}

bool AudioMixerGroupUVE::SetGroupPitchUVE(const std::string_view name, const float pitchMultiplier) {
    if (!IsValidPitchMultiplierUVE(pitchMultiplier)) {
        return false;
    }
    const auto iterator = m_groups.find(std::string(name));
    if (iterator == m_groups.end()) {
        return false;
    }
    iterator->second.pitchMultiplier = pitchMultiplier;
    return true;
}

float AudioMixerGroupUVE::GetGroupVolumeUVE(const std::string_view name) const {
    const auto iterator = m_groups.find(std::string(name));
    return iterator == m_groups.end() ? 1.0F : iterator->second.volumeMultiplier;
}

float AudioMixerGroupUVE::GetGroupPitchUVE(const std::string_view name) const {
    const auto iterator = m_groups.find(std::string(name));
    return iterator == m_groups.end() ? 1.0F : iterator->second.pitchMultiplier;
}

bool AudioMixerGroupUVE::AttachSourceUVE(const std::string_view name) {
    const auto iterator = m_groups.find(std::string(name));
    if (iterator == m_groups.end() || iterator->second.sourceCount == std::numeric_limits<std::size_t>::max()) {
        return false;
    }
    ++iterator->second.sourceCount;
    return true;
}

bool AudioMixerGroupUVE::DetachSourceUVE(const std::string_view name) {
    const auto iterator = m_groups.find(std::string(name));
    if (iterator == m_groups.end() || iterator->second.sourceCount == 0U) {
        return false;
    }
    --iterator->second.sourceCount;
    return true;
}

AudioMixerDiagnosticsUVE AudioMixerGroupUVE::GetDiagnosticsUVE(const std::size_t maximumGroups) const {
    AudioMixerDiagnosticsUVE diagnostics;
    diagnostics.groupCount = m_groups.size();
    diagnostics.inspectedGroupCount = std::min(maximumGroups, m_groups.size());
    diagnostics.truncated = diagnostics.inspectedGroupCount < diagnostics.groupCount;
    diagnostics.groups.reserve(diagnostics.inspectedGroupCount);

    std::size_t inspected = 0U;
    for (const auto& [name, state] : m_groups) {
        diagnostics.routedSourceCount += state.sourceCount;
        if (inspected >= maximumGroups) {
            continue;
        }
        diagnostics.groups.push_back(AudioMixerGroupSnapshotUVE{name, state.volumeMultiplier,
                                                                 state.pitchMultiplier, state.sourceCount});
        ++inspected;
    }
    return diagnostics;
}

} // namespace UVE::Audio
