//------------------------------------------------------------------------------
// UniVex Engine (UVE) — Proprietary Game Engine
// Copyright (c) 2026 UniVex Studios. All Rights Reserved.
// Unauthorized copying, modification, distribution, or use of this code
// in whole or in part is strictly prohibited without express written
// permission from UniVex Studios.
// Violators will be prosecuted to the fullest extent of the law.
//------------------------------------------------------------------------------

#pragma once

#include <memory>

#include "uve/render/i_render_device_uve.h"
#include "uve/render/i_render_system_uve.h"

namespace UVE::Render {

/// RenderSystemUVE is the concrete, engine-standard implementation of IRenderSystemUVE. Takes its
/// IRenderDeviceUVE& by dependency injection (same shape as
/// `FileSystemUVE(IAssetBundleUVE&)`/`AssetManagerUVE(IThreadPoolUVE&, IEventSystemUVE&, ...)`) —
/// it never constructs or owns a render device itself, so tests and future backend swaps can pass
/// in whichever IRenderDeviceUVE& they need.
class RenderSystemUVE final : public IRenderSystemUVE {
public:
    explicit RenderSystemUVE(IRenderDeviceUVE& renderDevice);
    ~RenderSystemUVE() override;

    RenderSystemUVE(const RenderSystemUVE&) = delete;
    RenderSystemUVE& operator=(const RenderSystemUVE&) = delete;

    void BeginFrameUVE() override;
    [[nodiscard]] ICommandBufferUVE& GetFrameCommandBufferUVE() override;
    void EndFrameUVE() override;
    [[nodiscard]] std::uint64_t GetFrameIndexUVE() const noexcept override;

private:
    struct ImplUVE;
    std::unique_ptr<ImplUVE> m_impl;
};

} // namespace UVE::Render
