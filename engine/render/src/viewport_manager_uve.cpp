// Copyright (c) 2026 UniVex Studios. All Rights Reserved.


#include "uve/render/viewport_manager_uve.h"

#include <algorithm>
#include <cmath>

#include "uve/debug/logging_macros_uve.h"
#include "uve/scene/components/camera_component_uve.h"

namespace UVE::Render {

namespace {

[[nodiscard]] bool IsPaneRectValidUVE(const ViewportPaneUVE& pane) noexcept {
    return std::isfinite(pane.originX01) && std::isfinite(pane.originY01) && std::isfinite(pane.sizeX01) &&
           std::isfinite(pane.sizeY01) && pane.sizeX01 > 0.0F && pane.sizeY01 > 0.0F &&
           pane.originX01 >= 0.0F && pane.originY01 >= 0.0F && pane.originX01 + pane.sizeX01 <= 1.0F &&
           pane.originY01 + pane.sizeY01 <= 1.0F;
}

[[nodiscard]] bool HasLiveCameraUVE(Scene::IEntityManagerUVE& entityManager,
                                    const Scene::EntityUVE cameraEntity) {
    if (cameraEntity == Scene::kInvalidEntityUVE || !entityManager.IsAliveUVE(cameraEntity)) {
        return false;
    }
    if (!entityManager.HasComponentUVE<Scene::CameraComponentUVE>(cameraEntity)) {
        return false;
    }
    return Scene::IsCameraComponentValidUVE(entityManager.GetComponentUVE<Scene::CameraComponentUVE>(cameraEntity));
}

} // namespace

void ViewportManagerUVE::AddPaneUVE(ViewportPaneUVE pane) {
    m_panes.push_back(pane);
}

void ViewportManagerUVE::RemovePaneUVE(const std::size_t index) {
    if (index >= m_panes.size()) {
        UVE_ERROR("ViewportManagerUVE: RemovePaneUVE index {} is out of range ({} panes)", index, m_panes.size());
        return;
    }
    m_panes.erase(m_panes.begin() + static_cast<std::ptrdiff_t>(index));
}

std::size_t ViewportManagerUVE::GetPaneCountUVE() const noexcept {
    return m_panes.size();
}

const ViewportPaneUVE* ViewportManagerUVE::GetPaneUVE(const std::size_t index) const noexcept {
    if (index >= m_panes.size()) {
        return nullptr;
    }
    return &m_panes[index];
}

std::optional<std::size_t> ViewportManagerUVE::FindPaneAtNormalizedPositionUVE(const float x01,
                                                                                const float y01) const noexcept {
    if (!std::isfinite(x01) || !std::isfinite(y01)) {
        return std::nullopt;
    }
    for (std::size_t reverseIndex = m_panes.size(); reverseIndex > 0U; --reverseIndex) {
        const std::size_t index = reverseIndex - 1U;
        const ViewportPaneUVE& pane = m_panes[index];
        if (x01 >= pane.originX01 && x01 <= pane.originX01 + pane.sizeX01 && y01 >= pane.originY01 &&
            y01 <= pane.originY01 + pane.sizeY01) {
            return index;
        }
    }
    return std::nullopt;
}

void ViewportManagerUVE::RenderAllPanesUVE(IRenderer3DUVE& renderer, Scene::IEntityManagerUVE& entityManager,
                                            const std::uint32_t windowWidth, const std::uint32_t windowHeight) const {
    if (windowWidth == 0U || windowHeight == 0U) {
        return;
    }
    for (const ViewportPaneUVE& pane : m_panes) {
        if (pane.uiOnly) {
            continue;
        }
        if (!IsPaneRectValidUVE(pane)) {
            UVE_WARNING("ViewportManagerUVE: RenderAllPanesUVE skipping a pane with an invalid normalized rect "
                        "({}, {}, {}x{})",
                        pane.originX01, pane.originY01, pane.sizeX01, pane.sizeY01);
            continue;
        }
        if (!HasLiveCameraUVE(entityManager, pane.cameraEntity)) {
            UVE_WARNING("ViewportManagerUVE: RenderAllPanesUVE skipping a pane with no live camera entity");
            continue;
        }

        const auto pixelX = static_cast<std::uint32_t>(pane.originX01 * static_cast<float>(windowWidth));
        const auto pixelYFromTop = static_cast<std::uint32_t>(pane.originY01 * static_cast<float>(windowHeight));
        const std::uint32_t pixelWidth =
            std::max<std::uint32_t>(1U, static_cast<std::uint32_t>(pane.sizeX01 * static_cast<float>(windowWidth)));
        const std::uint32_t pixelHeight =
            std::max<std::uint32_t>(1U, static_cast<std::uint32_t>(pane.sizeY01 * static_cast<float>(windowHeight)));
        // Clamp instead of letting a rounding-driven off-by-one overflow the window: e.g. a pane
        // at originX01=0.5, sizeX01=0.5 against an odd windowWidth can round pixelX + pixelWidth to
        // windowWidth + 1.
        const std::uint32_t clampedWidth = std::min(pixelWidth, windowWidth - std::min(pixelX, windowWidth));
        const std::uint32_t clampedHeight = std::min(pixelHeight, windowHeight - std::min(pixelYFromTop, windowHeight));
        if (clampedWidth == 0U || clampedHeight == 0U) {
            continue;
        }

        if (!renderer.ResizeTargetsUVE(clampedWidth, clampedHeight)) {
            UVE_WARNING("ViewportManagerUVE: RenderAllPanesUVE could not resize the shared renderer target to "
                        "{}x{}; skipping this pane",
                        clampedWidth, clampedHeight);
            continue;
        }

        // ViewportRectUVE/glViewport use OpenGL's bottom-left origin; ViewportPaneUVE's
        // originY01 is top-left (this engine's other screen-space UI convention) - flip here,
        // at the one boundary between the two conventions, rather than asking every pane author
        // to think in GL's coordinate space.
        const std::uint32_t pixelYFromBottom = windowHeight - pixelYFromTop - clampedHeight;
        renderer.RenderFrameToRegionUVE(entityManager, pane.cameraEntity,
                                        ViewportRectUVE{pixelX, pixelYFromBottom, clampedWidth, clampedHeight});
    }
}

} // namespace UVE::Render
