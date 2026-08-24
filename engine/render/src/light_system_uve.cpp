// Copyright (c) 2026 UniVex Studios. All Rights Reserved.


#include "uve/render/light_system_uve.h"

#include <cstddef>

#include "uve/debug/assert_uve.h"
#include "uve/math/quaternion_uve.h"
#include "uve/scene/components/light_component_uve.h"
#include "uve/scene/components/world_transform_component_uve.h"

namespace UVE::Render {

LightListUVE LightSystemUVE::ExtractActiveLightsUVE(Scene::IEntityManagerUVE& entityManager) const {
    LightListUVE result;
    std::size_t filledCount = 0;

    entityManager.ForEachUVE<Scene::WorldTransformComponentUVE, Scene::LightComponentUVE>(
        [&](Scene::EntityUVE, const Scene::WorldTransformComponentUVE& worldTransform,
            const Scene::LightComponentUVE& light) {
            if (filledCount >= kMaxLightsUVE) {
                return;
            }
            const bool validLight = Scene::IsLightComponentValidUVE(light);
            UVE_ASSERT(validLight);
            if (!validLight) {
                return;
            }
            LightDataUVE& slot = result[filledCount];
            slot.type = light.type;
            slot.position = worldTransform.worldPosition;
            slot.direction = Math::RotateVectorUVE(worldTransform.worldRotation, Math::Vector3UVE{0.0F, 0.0F, -1.0F});
            slot.rotation = worldTransform.worldRotation;
            slot.color = light.color;
            slot.intensity = light.intensity;
            slot.range = light.range;
            slot.spotAngleDegrees = light.spotAngleDegrees;
            ++filledCount;
        });

    return result;
}

} // namespace UVE::Render
