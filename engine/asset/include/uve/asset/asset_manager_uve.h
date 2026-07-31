//------------------------------------------------------------------------------
// UniVex Engine (UVE) — Proprietary Game Engine
// Copyright (c) 2026 UniVex Studios. All Rights Reserved.
// Unauthorized copying, modification, distribution, or use of this code
// in whole or in part is strictly prohibited without express written
// permission from UniVex Studios.
// Violators will be prosecuted to the fullest extent of the law.
//------------------------------------------------------------------------------

#pragma once

#include <filesystem>
#include <memory>
#include <typeindex>

#include "uve/asset/i_asset_manager_uve.h"
#include "uve/events/i_event_system_uve.h"
#include "uve/threading/i_thread_pool_uve.h"

namespace UVE::Asset {

class IHotReloadUVE;

/// AssetManagerUVE is the concrete, engine-standard implementation of IAssetManagerUVE. Runs
/// every load on `threadPool`, publishes AssetLoadCompletedEventUVE via `eventSystem` on
/// completion (from whichever worker thread ran the load — IEventSystemUVE::QueueEvent()
/// documents this as safe), and — if `hotReload` is non-null — tells it which GUIDs are
/// currently loaded so it knows what to poll.
/// Thread-safety: thread-safe. Every method is guarded by an internal mutex, matching
/// ConfigManagerUVE's/AssetDatabaseUVE's contract. The destructor blocks until every in-flight
/// load job finishes before tearing down internal state, so a load job's captured `this` never
/// outlives the object it points to.
class AssetManagerUVE final : public IAssetManagerUVE {
public:
    /// `hotReload` may be null (hot reload disabled) — AssetManagerUVE works identically either
    /// way, simply skipping the Track/Untrack calls when it's null.
    AssetManagerUVE(Threading::IThreadPoolUVE& threadPool, Events::IEventSystemUVE& eventSystem,
                    IHotReloadUVE* hotReload = nullptr);
    ~AssetManagerUVE() override;

    AssetManagerUVE(const AssetManagerUVE&) = delete;
    AssetManagerUVE& operator=(const AssetManagerUVE&) = delete;

    void CollectGarbageUVE() override;
    void ReloadUVE(AssetGuidUVE guid, IAssetDatabaseUVE& assetDatabase) override;
    [[nodiscard]] std::size_t GetLoadedAssetCountUVE() const override;

protected:
    void RegisterLoaderErased(std::type_index type, AssetLoaderInfoUVE info) override;
    void LoadErased(AssetGuidUVE guid, std::type_index type, IAssetDatabaseUVE& assetDatabase) override;
    [[nodiscard]] AssetLoadStateUVE GetStateErased(AssetGuidUVE guid) const override;
    [[nodiscard]] void* TryGetErased(AssetGuidUVE guid) override;
    void AddRefErased(AssetGuidUVE guid) override;
    void ReleaseErased(AssetGuidUVE guid) override;

private:
    /// Shared by LoadErased()'s and ReloadUVE()'s submitted job: runs the registered loader for
    /// `type` against `path`, updates the record (destroying any previously-loaded data on a
    /// successful reload), tracks/untracks with HotReloadUVE as appropriate, and queues
    /// AssetLoadCompletedEventUVE. Runs on a IThreadPoolUVE worker thread.
    void ExecuteLoadUVE(AssetGuidUVE guid, std::type_index type, std::filesystem::path path);

    struct ImplUVE;
    std::unique_ptr<ImplUVE> m_impl;
};

} // namespace UVE::Asset
