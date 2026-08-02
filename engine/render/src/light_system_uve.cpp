//------------------------------------------------------------------------------
// UniVex Engine (UVE) — Proprietary Game Engine
// Copyright (c) 2026 UniVex Studios. All Rights Reserved.
// Unauthorized copying, modification, distribution, or use of this code
// in whole or in part is strictly prohibited without express written
// permission from UniVex Studios.
// Violators will be prosecuted to the fullest extent of the law.
//------------------------------------------------------------------------------

#include "uve/render/light_system_uve.h"

#include "uve/math/quaternion_uve.h"
#include "uve/scene/components/light_component_uve.h"
#include "uve/scene/components/world_transform_component_uve.h"

namespace UVE::Render {

DirectionalLightDataUVE LightSystemUVE::ExtractActiveLightUVE(Scene::IEntityManagerUVE& entityManager) const {
    DirectionalLightDataUVE result;
    bool foundUVE = false;

    entityManager.ForEachUVE<Scene::WorldTransformComponentUVE, Scene::LightComponentUVE>(
        [&](Scene::EntityUVE, const Scene::WorldTransformComponentUVE& worldTransform,
            const Scene::LightComponentUVE& light) {
            if (foundUVE) {
                return;
            }
            foundUVE = true;
            result.direction =
                Math::RotateVectorUVE(worldTransform.worldRotation, Math::Vector3UVE{0.0F, 0.0F, -1.0F});
            result.color = light.color;
            result.intensity = light.intensity;
        });

    return result;
}

} // namespace UVE::Render
