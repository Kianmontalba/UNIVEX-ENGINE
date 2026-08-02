//------------------------------------------------------------------------------
// UniVex Engine (UVE) — Proprietary Game Engine
// Copyright (c) 2026 UniVex Studios. All Rights Reserved.
// Unauthorized copying, modification, distribution, or use of this code
// in whole or in part is strictly prohibited without express written
// permission from UniVex Studios.
// Violators will be prosecuted to the fullest extent of the law.
//------------------------------------------------------------------------------

#pragma once

#include "uve/render/i_light_system_uve.h"

namespace UVE::Render {

/// LightSystemUVE is the concrete, engine-standard implementation of ILightSystemUVE. Deliberately
/// stateless (no members, no PIMPL) — mirrors CameraSystemUVE's/MeshRendererUVE's precedent for
/// utility services that always take the IEntityManagerUVE& they operate on as an explicit
/// parameter rather than owning a reference to one.
class LightSystemUVE final : public ILightSystemUVE {
public:
    [[nodiscard]] LightListUVE ExtractActiveLightsUVE(Scene::IEntityManagerUVE& entityManager) const override;
};

} // namespace UVE::Render
