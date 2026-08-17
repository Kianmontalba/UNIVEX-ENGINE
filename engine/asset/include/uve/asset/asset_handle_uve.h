// Copyright (c) 2026 UniVex Studios. All Rights Reserved.


#pragma once

#include "uve/asset/asset_guid_uve.h"
#include "uve/asset/asset_load_state_uve.h"
#include "uve/asset/i_asset_manager_uve.h"

namespace UVE::Asset {

/// AssetHandleUVE<T> is a ref-counted RAII reference to one asset tracked by an IAssetManagerUVE,
/// obtained only via IAssetManagerUVE::LoadUVE<T>(). Copying increments the manager's reference
/// count for this asset; destruction (and move-from) decrements it. The referenced
/// IAssetManagerUVE& must outlive every handle to assets it produced (the same "systems don't
/// outlive their owner" contract already implicit elsewhere in the engine, e.g. IEntityManagerUVE&
/// arguments never outliving the EntityManagerUVE that owns them).
/// Thread-safety: safe to copy/move/destroy from any thread — every operation routes through
/// IAssetManagerUVE's own thread-safe ref-count primitives.
template <typename T>
class AssetHandleUVE final {
public:
    AssetHandleUVE(const AssetHandleUVE& other) noexcept : m_manager(other.m_manager), m_guid(other.m_guid) {
        m_manager->AddRefUVE(m_guid);
    }

    AssetHandleUVE(AssetHandleUVE&& other) noexcept : m_manager(other.m_manager), m_guid(other.m_guid) {
        other.m_manager = nullptr;
    }

    AssetHandleUVE& operator=(const AssetHandleUVE& other) noexcept {
        if (this != &other) {
            ReleaseIfOwnedUVE();
            m_manager = other.m_manager;
            m_guid = other.m_guid;
            m_manager->AddRefUVE(m_guid);
        }
        return *this;
    }

    AssetHandleUVE& operator=(AssetHandleUVE&& other) noexcept {
        if (this != &other) {
            ReleaseIfOwnedUVE();
            m_manager = other.m_manager;
            m_guid = other.m_guid;
            other.m_manager = nullptr;
        }
        return *this;
    }

    ~AssetHandleUVE() { ReleaseIfOwnedUVE(); }

    /// True once the underlying load has completed successfully — TryGetUVE() returns non-null.
    [[nodiscard]] bool IsReadyUVE() const { return m_manager->GetLoadStateUVE(m_guid) == AssetLoadStateUVE::Loaded; }

    /// True once the underlying load has completed and failed — TryGetUVE() will never become
    /// non-null for this handle (short of a future HotReloadUVE-triggered retry succeeding).
    [[nodiscard]] bool HasFailedUVE() const {
        return m_manager->GetLoadStateUVE(m_guid) == AssetLoadStateUVE::Failed;
    }

    /// Returns a copied diagnostic for the most recent failed load/reload, or an empty string when
    /// this handle is not failed. The copy remains valid independently of the manager record.
    [[nodiscard]] std::string GetFailureReasonUVE() const { return m_manager->GetFailureReasonUVE(m_guid); }

    /// Returns a pointer to the loaded `T`, or `nullptr` if the load hasn't finished (or failed)
    /// yet. The pointer may be invalidated by a future HotReloadUVE-triggered reload — callers
    /// that need a stable reference across frames should re-call TryGetUVE() each time.
    [[nodiscard]] T* TryGetUVE() const {
        return IsReadyUVE() ? static_cast<T*>(m_manager->GetRawPointerUVE(m_guid)) : nullptr;
    }

    [[nodiscard]] AssetGuidUVE GetGuidUVE() const noexcept { return m_guid; }

private:
    friend class IAssetManagerUVE;

    /// Constructs a handle for a reference IAssetManagerUVE::LoadUVE<T>() has already accounted
    /// for — deliberately private (only LoadUVE<T>() may call this) so no caller can construct a
    /// handle for a reference that was never actually counted.
    AssetHandleUVE(IAssetManagerUVE& manager, AssetGuidUVE guid) noexcept : m_manager(&manager), m_guid(guid) {}

    void ReleaseIfOwnedUVE() {
        if (m_manager != nullptr) {
            m_manager->ReleaseUVE(m_guid);
        }
    }

    IAssetManagerUVE* m_manager;
    AssetGuidUVE m_guid;
};

} // namespace UVE::Asset
