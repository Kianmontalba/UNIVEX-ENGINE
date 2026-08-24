// Copyright (c) 2026 UniVex Studios. All Rights Reserved.


#pragma once

#include <memory>

#include "uve/asset/i_hot_reload_uve.h"
#include "uve/events/i_event_system_uve.h"

namespace UVE::Asset {

/// HotReloadUVE is the concrete, engine-standard implementation of IHotReloadUVE. Polls tracked
/// assets' mtimes every `pollIntervalSeconds` of accumulated PollUVE() deltaTimeSeconds (default
/// 1.0s, see EngineConfigUVE::hotReloadPollIntervalSecondsUVE) rather than watching the
/// filesystem natively — no OS-native watching backend (inotify/ReadDirectoryChangesW/FSEvents)
/// exists anywhere in engine/platform/ yet.
/// Thread-safety: TrackUVE()/UntrackUVE() are thread-safe (guarded by an internal mutex) since
/// AssetManagerUVE calls them from its own worker-thread completion path; PollUVE() is intended
/// to be called once per frame from the main/engine thread only (see EngineCoreUVE::Update()).
class HotReloadUVE final : public IHotReloadUVE {
public:
    explicit HotReloadUVE(Events::IEventSystemUVE& eventSystem, double pollIntervalSeconds = 1.0);
    ~HotReloadUVE() override;

    HotReloadUVE(const HotReloadUVE&) = delete;
    HotReloadUVE& operator=(const HotReloadUVE&) = delete;

    void TrackUVE(AssetGuidUVE guid) override;
    void UntrackUVE(AssetGuidUVE guid) override;
    /// Negative or non-finite deltas are ignored before accumulator or reload-state mutation.
    void PollUVE(IAssetManagerUVE& assetManager, IAssetDatabaseUVE& assetDatabase,
                 double deltaTimeSeconds) override;

private:
    struct ImplUVE;
    std::unique_ptr<ImplUVE> m_impl;
};

} // namespace UVE::Asset
