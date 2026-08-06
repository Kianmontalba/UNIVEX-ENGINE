// Copyright (c) 2026 UniVex Studios. All Rights Reserved.


#pragma once

#include <cstddef>
#include <filesystem>
#include <functional>
#include <typeindex>
#include <utility>

#include "uve/asset/asset_guid_uve.h"
#include "uve/asset/asset_load_state_uve.h"
#include "uve/asset/i_asset_database_uve.h"

namespace UVE::Asset {

template <typename T>
class AssetHandleUVE;

/// Type-erased loader registration record: how AssetManagerUVE actually loads/destroys a value
/// of some registered type `T`, without IAssetManagerUVE's header needing to be templated on `T`
/// itself. Internal plumbing between RegisterLoaderUVE<T>()/LoadUVE<T>() and the protected
/// erased primitives below — ordinary callers never construct one directly.
struct AssetLoaderInfoUVE {
    /// Allocates a new `T`, runs the registered loader against a resolved file path, and returns
    /// the result as `void*` with the bool out-parameter reflecting success. On failure the
    /// allocated object is destroyed before returning and the returned pointer is null.
    std::function<void*(const std::filesystem::path&, bool&)> load;
    /// Destroys a `void*` previously returned by `load`, equivalent to `delete static_cast<T*>(ptr)`.
    std::function<void(void*)> destroy;
};

/// IAssetManagerUVE is the engine's async asset loader: register a loader function once per C++
/// type, then LoadUVE<T>(guid, assetDatabase) any number of times to get a ref-counted
/// AssetHandleUVE<T>. The underlying load runs once on a Threading::IThreadPoolUVE worker
/// (repeat LoadUVE() calls for an already-loading/loaded/failed GUID just add a reference and
/// reuse the same record); CollectGarbageUVE() unloads assets whose reference count has dropped
/// to zero. Mirrors IEventSystemUVE's/IEntityManagerUVE's shape: public non-virtual member
/// templates route through protected pure-virtual type-erased primitives, since a template
/// method cannot itself be virtual.
/// Thread-safety: thread-safe — every method here (and the ref-count operations AssetHandleUVE
/// drives through the public non-virtual wrappers below) may be called concurrently from any
/// thread, including from inside a load job running on an IThreadPoolUVE worker.
/// CollectGarbageUVE() is intended to be called once per frame from the main/engine thread (see
/// EngineCoreUVE::Update(), alongside SceneGraphUVE::UpdateUVE()) purely by engine-loop
/// convention, not because calling it elsewhere would be unsafe.
class IAssetManagerUVE {
public:
    virtual ~IAssetManagerUVE() = default;

    /// Registers the loader function used by every future LoadUVE<T>() call: reads the resolved
    /// file path and fills `outValue`, returning true on success or false on failure (a
    /// missing/corrupt file, say) — must never throw. Registering a second loader for the same
    /// `T` replaces the first.
    template <typename T>
    void RegisterLoaderUVE(std::function<bool(const std::filesystem::path&, T&)> loadFunc) {
        AssetLoaderInfoUVE info;
        info.load = [loadFunc](const std::filesystem::path& path, bool& outSuccess) -> void* {
            auto* const value = new T();
            outSuccess = loadFunc(path, *value);
            if (!outSuccess) {
                delete value;
                return nullptr;
            }
            return value;
        };
        info.destroy = [](void* pointer) { delete static_cast<T*>(pointer); };
        RegisterLoaderErased(std::type_index(typeid(T)), std::move(info));
    }

    /// Resolves `guid` via `assetDatabase` and begins (or reuses an already in-flight/completed)
    /// an asynchronous load of it as type `T`, returning a ref-counted handle immediately — this
    /// call never blocks; poll AssetHandleUVE::IsReadyUVE()/TryGetUVE() on the returned handle.
    /// `T` must already be registered via RegisterLoaderUVE<T>().
    template <typename T>
    [[nodiscard]] AssetHandleUVE<T> LoadUVE(AssetGuidUVE guid, IAssetDatabaseUVE& assetDatabase) {
        LoadErased(guid, std::type_index(typeid(T)), assetDatabase);
        return AssetHandleUVE<T>(*this, guid);
    }

    /// Unloads (runs the registered destroy function for) every currently tracked asset whose
    /// reference count has reached zero. A record whose state is still `Loading` is never
    /// collected even at refcount zero — the in-flight load job implicitly holds it alive until
    /// it finishes.
    virtual void CollectGarbageUVE() = 0;

    /// Re-runs the previously-registered loader for `guid` in place, replacing its currently
    /// loaded data on success — used by HotReloadUVE when it detects the on-disk asset file has
    /// changed. Logs a warning and no-ops if `guid` isn't currently tracked, or if it's still
    /// `Loading`. No type parameter needed: the record already remembers which registered loader
    /// produced it.
    virtual void ReloadUVE(AssetGuidUVE guid, IAssetDatabaseUVE& assetDatabase) = 0;

    /// Number of asset records currently tracked with state `Loaded` (debugging/query only).
    [[nodiscard]] virtual std::size_t GetLoadedAssetCountUVE() const = 0;

    /// Public non-virtual wrappers around the protected erased primitives below, so
    /// AssetHandleUVE<T> — a separate class template, not a member of this class hierarchy — can
    /// reach them (mirrors IEntityManagerUVE::GetComponentPointerUVE()'s exact same reasoning).
    [[nodiscard]] AssetLoadStateUVE GetLoadStateUVE(AssetGuidUVE guid) const { return GetStateErased(guid); }
    [[nodiscard]] void* GetRawPointerUVE(AssetGuidUVE guid) { return TryGetErased(guid); }
    void AddRefUVE(AssetGuidUVE guid) { AddRefErased(guid); }
    void ReleaseUVE(AssetGuidUVE guid) { ReleaseErased(guid); }

protected:
    virtual void RegisterLoaderErased(std::type_index type, AssetLoaderInfoUVE info) = 0;
    virtual void LoadErased(AssetGuidUVE guid, std::type_index type, IAssetDatabaseUVE& assetDatabase) = 0;
    [[nodiscard]] virtual AssetLoadStateUVE GetStateErased(AssetGuidUVE guid) const = 0;
    [[nodiscard]] virtual void* TryGetErased(AssetGuidUVE guid) = 0;
    virtual void AddRefErased(AssetGuidUVE guid) = 0;
    virtual void ReleaseErased(AssetGuidUVE guid) = 0;
};

} // namespace UVE::Asset
