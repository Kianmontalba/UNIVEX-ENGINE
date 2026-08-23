// Copyright (c) 2026 UniVex Studios. All Rights Reserved.


#include "uve/asset/asset_manager_uve.h"

#include <exception>
#include <functional>
#include <mutex>
#include <string>
#include <system_error>
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

struct AssetManagerRecordUVE {
    std::type_index type{typeid(void)};
    AssetLoadStateUVE state = AssetLoadStateUVE::Loading;
    void* data = nullptr;
    std::function<void(void*)> destroy;
    std::string failureReason;
    int refCount = 0;
};

} // namespace

struct AssetManagerUVE::ImplUVE {
    Threading::IThreadPoolUVE& threadPool;
    Events::IEventSystemUVE& eventSystem;
    IHotReloadUVE* hotReload;
    mutable std::mutex mutex;
    std::unordered_map<std::type_index, AssetLoaderInfoUVE> loaders;
    std::unordered_map<AssetGuidUVE, AssetManagerRecordUVE> records;
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
    bool missingLoader = false;
    {
        const std::lock_guard<std::mutex> lock(m_impl->mutex);
        const auto existingIt = m_impl->records.find(guid);
        if (existingIt != m_impl->records.end()) {
            ++existingIt->second.refCount;
            return;
        }

        const auto loaderIt = m_impl->loaders.find(type);
        AssetManagerRecordUVE record;
        record.type = type;
        record.refCount = 1;
        if (loaderIt == m_impl->loaders.end()) {
            record.state = AssetLoadStateUVE::Failed;
            record.failureReason = "no loader registered for asset type";
            missingLoader = true;
        } else {
            record.state = AssetLoadStateUVE::Loading;
            record.destroy = loaderIt->second.destroy;
        }
        m_impl->records.emplace(guid, std::move(record));
    }

    if (missingLoader) {
        UVE_ERROR("AssetManagerUVE: no loader registered for requested asset type (GUID {})", guid.value);
        m_impl->eventSystem.QueueEvent(AssetLoadCompletedEventUVE{guid, false});
        return;
    }

    std::filesystem::path path;
    try {
        path = assetDatabase.ResolveUVE(guid);
    } catch (const std::exception& exception) {
        FailResolutionUVE(guid, "asset database resolver threw: " + std::string(exception.what()));
        return;
    } catch (...) {
        FailResolutionUVE(guid, "asset database resolver threw an unknown exception");
        return;
    }
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
        it->second.failureReason.clear();
        type = it->second.type;
    }

    std::filesystem::path path;
    try {
        path = assetDatabase.ResolveUVE(guid);
    } catch (const std::exception& exception) {
        FailResolutionUVE(guid, "asset database resolver threw: " + std::string(exception.what()));
        return;
    } catch (...) {
        FailResolutionUVE(guid, "asset database resolver threw an unknown exception");
        return;
    }
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
    std::string failureReason;
    if (!path.empty() && loadFunc) {
        try {
            newData = loadFunc(path, success);
        } catch (const std::exception& exception) {
            failureReason = "asset loader threw: " + std::string(exception.what());
        } catch (...) {
            failureReason = "asset loader threw an unknown exception";
        }
    }

    if (path.empty()) {
        failureReason = "asset database resolved no path";
    } else if (!loadFunc) {
        failureReason = "no loader registered for asset type";
    } else {
        std::error_code existsError;
        if (!std::filesystem::exists(path, existsError)) {
            failureReason = existsError ? "asset file existence check failed: " + existsError.message()
                                        : "asset file does not exist";
        } else if (!success && failureReason.empty()) {
            failureReason = "registered asset loader rejected file";
        }
    }

    void* oldData = nullptr;
    std::function<void(void*)> destroyFunc;
    {
        const std::lock_guard<std::mutex> lock(m_impl->mutex);
        const auto it = m_impl->records.find(guid);
        if (it != m_impl->records.end()) {
            oldData = it->second.data;
            if (success) {
                destroyFunc = it->second.destroy;
                it->second.data = newData;
                it->second.state = AssetLoadStateUVE::Loaded;
                it->second.failureReason.clear();
            } else if (oldData != nullptr) {
                // A failed reload is failure-atomic: retain the last-known-good data and expose
                // the reload diagnostic while keeping the handle ready for existing consumers.
                it->second.state = AssetLoadStateUVE::Loaded;
                it->second.failureReason = failureReason;
            } else {
                it->second.data = nullptr;
                it->second.state = AssetLoadStateUVE::Failed;
                it->second.failureReason = failureReason;
            }
        }
    }
    // Free the previous value only after releasing the lock, and only on a successful reload.
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

void AssetManagerUVE::FailResolutionUVE(AssetGuidUVE guid, std::string failureReason) {
    {
        const std::lock_guard<std::mutex> lock(m_impl->mutex);
        const auto it = m_impl->records.find(guid);
        if (it == m_impl->records.end()) {
            return;
        }
        it->second.state = it->second.data != nullptr ? AssetLoadStateUVE::Loaded : AssetLoadStateUVE::Failed;
        it->second.failureReason = std::move(failureReason);
    }
    UVE_ERROR("AssetManagerUVE: failed to resolve asset GUID {}", guid.value);
    m_impl->eventSystem.QueueEvent(AssetLoadCompletedEventUVE{guid, false});
}

void AssetManagerUVE::CollectGarbageUVE() {
    std::vector<AssetManagerRecordUVE> toDestroy;
    {
        const std::lock_guard<std::mutex> lock(m_impl->mutex);
        for (auto it = m_impl->records.begin(); it != m_impl->records.end();) {
            AssetManagerRecordUVE& record = it->second;
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
    for (AssetManagerRecordUVE& record : toDestroy) {
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

std::string AssetManagerUVE::GetFailureReasonErased(AssetGuidUVE guid) const {
    const std::lock_guard<std::mutex> lock(m_impl->mutex);
    const auto it = m_impl->records.find(guid);
    return it == m_impl->records.end() ? std::string{} : it->second.failureReason;
}

void* AssetManagerUVE::TryGetErased(AssetGuidUVE guid) {
    const std::lock_guard<std::mutex> lock(m_impl->mutex);
    const auto it = m_impl->records.find(guid);
    return it == m_impl->records.end() ? nullptr : it->second.data;
}

void AssetManagerUVE::AddRefErased(AssetGuidUVE guid) {
    const std::lock_guard<std::mutex> lock(m_impl->mutex);
    const auto it = m_impl->records.find(guid);
    if (it == m_impl->records.end()) {
        UVE_ASSERT(it != m_impl->records.end());
        return;
    }
    ++it->second.refCount;
}

void AssetManagerUVE::ReleaseErased(AssetGuidUVE guid) {
    const std::lock_guard<std::mutex> lock(m_impl->mutex);
    const auto it = m_impl->records.find(guid);
    if (it == m_impl->records.end() || it->second.refCount == 0U) {
        UVE_ASSERT(it != m_impl->records.end());
        if (it != m_impl->records.end()) {
            UVE_ASSERT(it->second.refCount > 0);
        }
        return;
    }
    --it->second.refCount;
}

} // namespace UVE::Asset
