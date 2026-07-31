//------------------------------------------------------------------------------
// UniVex Engine (UVE) — Proprietary Game Engine
// Copyright (c) 2026 UniVex Studios. All Rights Reserved.
// Unauthorized copying, modification, distribution, or use of this code
// in whole or in part is strictly prohibited without express written
// permission from UniVex Studios.
// Violators will be prosecuted to the fullest extent of the law.
//------------------------------------------------------------------------------

#include "uve/asset/hot_reload_uve.h"

#include <filesystem>
#include <mutex>
#include <system_error>
#include <unordered_map>
#include <vector>

#include "uve/asset/asset_reloaded_event_uve.h"
#include "uve/debug/logging_macros_uve.h"

namespace UVE::Asset {

struct HotReloadUVE::ImplUVE {
    Events::IEventSystemUVE& eventSystem;
    double pollIntervalSeconds;
    double accumulatedSeconds = 0.0;
    mutable std::mutex mutex;
    std::unordered_map<AssetGuidUVE, std::filesystem::file_time_type> lastKnownWriteTimes;
};

HotReloadUVE::HotReloadUVE(Events::IEventSystemUVE& eventSystem, double pollIntervalSeconds)
    : m_impl(std::make_unique<ImplUVE>(eventSystem, pollIntervalSeconds)) {}

HotReloadUVE::~HotReloadUVE() = default;

void HotReloadUVE::TrackUVE(AssetGuidUVE guid) {
    const std::lock_guard<std::mutex> lock(m_impl->mutex);
    // A default-constructed (epoch) write time is deliberately used as "not yet observed" - the
    // first PollUVE() after tracking initializes it from the real file instead of treating it as
    // an immediate change (see the `lastWriteTime == file_time_type{}` check below).
    m_impl->lastKnownWriteTimes[guid] = std::filesystem::file_time_type{};
}

void HotReloadUVE::UntrackUVE(AssetGuidUVE guid) {
    const std::lock_guard<std::mutex> lock(m_impl->mutex);
    m_impl->lastKnownWriteTimes.erase(guid);
}

void HotReloadUVE::PollUVE(IAssetManagerUVE& assetManager, IAssetDatabaseUVE& assetDatabase,
                            double deltaTimeSeconds) {
    m_impl->accumulatedSeconds += deltaTimeSeconds;
    if (m_impl->accumulatedSeconds < m_impl->pollIntervalSeconds) {
        return;
    }
    m_impl->accumulatedSeconds = 0.0;

    std::vector<AssetGuidUVE> toReload;
    {
        const std::lock_guard<std::mutex> lock(m_impl->mutex);
        for (auto& [guid, lastWriteTime] : m_impl->lastKnownWriteTimes) {
            const std::filesystem::path path = assetDatabase.ResolveUVE(guid);
            if (path.empty()) {
                UVE_WARNING("HotReloadUVE: tracked asset GUID {} no longer resolves to a path - skipping",
                            guid.value);
                continue;
            }

            std::error_code errorCode;
            const std::filesystem::file_time_type currentWriteTime =
                std::filesystem::last_write_time(path, errorCode);
            if (errorCode) {
                UVE_WARNING("HotReloadUVE: tracked asset GUID {} at \"{}\" is missing - skipping", guid.value,
                            path.string());
                continue;
            }

            if (lastWriteTime == std::filesystem::file_time_type{}) {
                lastWriteTime = currentWriteTime; // first observation since TrackUVE(), not a change
                continue;
            }
            if (currentWriteTime != lastWriteTime) {
                lastWriteTime = currentWriteTime;
                toReload.push_back(guid);
            }
        }
    }

    for (AssetGuidUVE guid : toReload) {
        assetManager.ReloadUVE(guid, assetDatabase);
        m_impl->eventSystem.QueueEvent(AssetReloadedEventUVE{guid});
    }
}

} // namespace UVE::Asset
