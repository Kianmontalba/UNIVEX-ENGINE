//------------------------------------------------------------------------------
// UniVex Engine (UVE) — Proprietary Game Engine
// Copyright (c) 2026 UniVex Studios. All Rights Reserved.
// Unauthorized copying, modification, distribution, or use of this code
// in whole or in part is strictly prohibited without express written
// permission from UniVex Studios.
// Violators will be prosecuted to the fullest extent of the law.
//------------------------------------------------------------------------------

#include "uve/asset/asset_manager_uve.h"

#include <functional>
#include <mutex>
#include <typeindex>
#include <unordered_map>
#include <utility>
#include <vector>

#include "uve/asset/asset_load_completed_event_uve.h"
#include "uve/asset/i_hot_reload_uve.h"
#include "uve/debug/assert_uve.h"
#include "uve/debug/logging_macros_uve.h"
#include "uve/threading/job_counter_uve.h"

namespace UVE::Asset {

namespace {

struct AssetRecordUVE {
    std::type_index type{typeid(void)};
    AssetLoadStateUVE state = AssetLoadStateUVE::Loading;
    void* data = nullptr;
    std::function<void(void*)> destroy;
    int refCount = 0;
};

} // namespace

struct AssetManagerUVE::ImplUVE {
    Threading::IThreadPoolUVE& threadPool;
    Events::IEventSystemUVE& eventSystem;
    IHotReloadUVE* hotReload;
    mutable std::mutex mutex;
    std::unordered_map<std::type_index, AssetLoaderInfoUVE> loaders;
    std::unordered_map<AssetGuidUVE, AssetRecordUVE> records;
    Threading::JobCounterUVE pendingJobs;
};

AssetManagerUVE::AssetManagerUVE(Threading::IThreadPoolUVE& threadPool, Events::IEventSystemUVE& eventSystem,
                                  IHotReloadUVE* hotReload)
    : m_impl(std::make_unique<ImplUVE>(threadPool, eventSystem, hotReload)) {}

AssetManagerUVE::~AssetManagerUVE() {
    // Every in-flight job captures `this` — wait for all of them to finish before m_impl (and
    // this object) is torn down, or a still-running job would dereference a dangling pointer.
    m_impl->pendingJobs.WaitUVE();
}

void AssetManagerUVE::RegisterLoaderErased(std::type_index type, AssetLoaderInfoUVE info) {
    const std::lock_guard<std::mutex> lock(m_impl->mutex);
    m_impl->loaders[type] = std::move(info);
}

void AssetManagerUVE::LoadErased(AssetGuidUVE guid, std::type_index type, IAssetDatabaseUVE& assetDatabase) {
    {
        const std::lock_guard<std::mutex> lock(m_impl->mutex);
        const auto existingIt = m_impl->records.find(guid);
        if (existingIt != m_impl->records.end()) {
            ++existingIt->second.refCount;
            return;
        }

        const auto loaderIt = m_impl->loaders.find(type);
        UVE_ASSERT(loaderIt != m_impl->loaders.end());

        AssetRecordUVE record;
        record.type = type;
        record.state = AssetLoadStateUVE::Loading;
        record.refCount = 1;
        record.destroy = loaderIt->second.destroy;
        m_impl->records.emplace(guid, std::move(record));
    }

    const std::filesystem::path path = assetDatabase.ResolveUVE(guid);
    m_impl->threadPool.SubmitUVE([this, guid, type, path]() { ExecuteLoadUVE(guid, type, path); },
                                  m_impl->pendingJobs);
}

void AssetManagerUVE::ReloadUVE(AssetGuidUVE guid, IAssetDatabaseUVE& assetDatabase) {
    std::type_index type(typeid(void));
    {
        const std::lock_guard<std::mutex> lock(m_impl->mutex);
        const auto it = m_impl->records.find(guid);
        if (it == m_impl->records.end()) {
            UVE_WARNING("AssetManagerUVE: ReloadUVE() called for untracked GUID {}", guid.value);
            return;
        }
        if (it->second.state == AssetLoadStateUVE::Loading) {
            UVE_WARNING("AssetManagerUVE: ReloadUVE() called for GUID {} while still loading - skipping",
                        guid.value);
            return;
        }
        it->second.state = AssetLoadStateUVE::Loading;
        type = it->second.type;
    }

    const std::filesystem::path path = assetDatabase.ResolveUVE(guid);
    m_impl->threadPool.SubmitUVE([this, guid, type, path]() { ExecuteLoadUVE(guid, type, path); },
                                  m_impl->pendingJobs);
}

void AssetManagerUVE::ExecuteLoadUVE(AssetGuidUVE guid, std::type_index type, std::filesystem::path path) {
    std::function<void*(const std::filesystem::path&, bool&)> loadFunc;
    {
        const std::lock_guard<std::mutex> lock(m_impl->mutex);
        const auto loaderIt = m_impl->loaders.find(type);
        if (loaderIt != m_impl->loaders.end()) {
            loadFunc = loaderIt->second.load;
        }
    }

    bool success = false;
    void* newData = nullptr;
    if (!path.empty() && loadFunc) {
        newData = loadFunc(path, success);
    }

    void* oldData = nullptr;
    std::function<void(void*)> destroyFunc;
    {
        const std::lock_guard<std::mutex> lock(m_impl->mutex);
        const auto it = m_impl->records.find(guid);
        if (it != m_impl->records.end()) {
            oldData = it->second.data;
            destroyFunc = it->second.destroy;
            it->second.data = success ? newData : nullptr;
            it->second.state = success ? AssetLoadStateUVE::Loaded : AssetLoadStateUVE::Failed;
        }
    }
    // Free the previous value only after releasing the lock, and only on a successful reload
    // that replaced it (oldData is null on a first-ever load).
    if (oldData != nullptr && destroyFunc) {
        destroyFunc(oldData);
    }

    if (!success) {
        UVE_ERROR("AssetManagerUVE: failed to load asset GUID {} from \"{}\"", guid.value, path.string());
    } else if (m_impl->hotReload != nullptr) {
        m_impl->hotReload->TrackUVE(guid);
    }

    m_impl->eventSystem.QueueEvent(AssetLoadCompletedEventUVE{guid, success});
}

void AssetManagerUVE::CollectGarbageUVE() {
    std::vector<AssetRecordUVE> toDestroy;
    {
        const std::lock_guard<std::mutex> lock(m_impl->mutex);
        for (auto it = m_impl->records.begin(); it != m_impl->records.end();) {
            AssetRecordUVE& record = it->second;
            if (record.refCount == 0 && record.state != AssetLoadStateUVE::Loading) {
                if (m_impl->hotReload != nullptr) {
                    m_impl->hotReload->UntrackUVE(it->first);
                }
                toDestroy.push_back(std::move(record));
                it = m_impl->records.erase(it);
            } else {
                ++it;
            }
        }
    }
    for (AssetRecordUVE& record : toDestroy) {
        if (record.data != nullptr && record.destroy) {
            record.destroy(record.data);
        }
    }
}

std::size_t AssetManagerUVE::GetLoadedAssetCountUVE() const {
    const std::lock_guard<std::mutex> lock(m_impl->mutex);
    std::size_t count = 0;
    for (const auto& [guid, record] : m_impl->records) {
        if (record.state == AssetLoadStateUVE::Loaded) {
            ++count;
        }
    }
    return count;
}

AssetLoadStateUVE AssetManagerUVE::GetStateErased(AssetGuidUVE guid) const {
    const std::lock_guard<std::mutex> lock(m_impl->mutex);
    const auto it = m_impl->records.find(guid);
    return it == m_impl->records.end() ? AssetLoadStateUVE::NotLoaded : it->second.state;
}

void* AssetManagerUVE::TryGetErased(AssetGuidUVE guid) {
    const std::lock_guard<std::mutex> lock(m_impl->mutex);
    const auto it = m_impl->records.find(guid);
    return it == m_impl->records.end() ? nullptr : it->second.data;
}

void AssetManagerUVE::AddRefErased(AssetGuidUVE guid) {
    const std::lock_guard<std::mutex> lock(m_impl->mutex);
    const auto it = m_impl->records.find(guid);
    UVE_ASSERT(it != m_impl->records.end());
    ++it->second.refCount;
}

void AssetManagerUVE::ReleaseErased(AssetGuidUVE guid) {
    const std::lock_guard<std::mutex> lock(m_impl->mutex);
    const auto it = m_impl->records.find(guid);
    UVE_ASSERT(it != m_impl->records.end());
    UVE_ASSERT(it->second.refCount > 0);
    --it->second.refCount;
}

} // namespace UVE::Asset
