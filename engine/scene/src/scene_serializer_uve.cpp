// Copyright (c) 2026 UniVex Studios. All Rights Reserved.


#include "uve/scene/scene_serializer_uve.h"

#include <cstddef>
#include <cstdint>
#include <functional>
#include <optional>
#include <string>
#include <typeindex>
#include <unordered_map>
#include <utility>
#include <vector>

#include <nlohmann/json.hpp>

#include "uve/asset/asset_guid_uve.h"
#include "uve/asset/uve_file_envelope_uve.h"
#include "uve/debug/logging_macros_uve.h"
#include "uve/math/quaternion_uve.h"
#include "uve/math/vector3_uve.h"
#include "uve/scene/components/audio_source_component_uve.h"
#include "uve/scene/components/camera_component_uve.h"
#include "uve/scene/components/collider_component_uve.h"
#include "uve/scene/components/hierarchy_component_uve.h"
#include "uve/scene/components/light_component_uve.h"
#include "uve/scene/components/mesh_component_uve.h"
#include "uve/scene/components/particle_emitter_component_uve.h"
#include "uve/scene/components/prefab_instance_component_uve.h"
#include "uve/scene/components/rigid_body_component_uve.h"
#include "uve/scene/components/script_component_uve.h"
#include "uve/scene/components/transform_component_uve.h"
#include "uve/scene/components/world_transform_component_uve.h"

namespace UVE::Scene {

namespace {

// --- Math JSON helpers ---------------------------------------------------------------------

[[nodiscard]] nlohmann::json ToJsonUVE(const Math::Vector3UVE& vector) {
    return nlohmann::json::array({vector.x, vector.y, vector.z});
}

[[nodiscard]] Math::Vector3UVE Vector3FromJsonUVE(const nlohmann::json& json) {
    return Math::Vector3UVE{json.at(0).get<float>(), json.at(1).get<float>(), json.at(2).get<float>()};
}

[[nodiscard]] nlohmann::json ToJsonUVE(const Math::QuaternionUVE& rotation) {
    return nlohmann::json::array({rotation.x, rotation.y, rotation.z, rotation.w});
}

[[nodiscard]] Math::QuaternionUVE QuaternionFromJsonUVE(const nlohmann::json& json) {
    return Math::QuaternionUVE{json.at(0).get<float>(), json.at(1).get<float>(), json.at(2).get<float>(),
                                json.at(3).get<float>()};
}

// --- Per-component-type JSON (de)serialization ---------------------------------------------
//
// One ToJsonUVE(const T&) overload plus one fromJson lambda per serializable component type.
// HierarchyComponentUVE and WorldTransformComponentUVE are deliberately absent here: the former
// needs the file-local-id remapping only SaveUVE()/LoadUVE() itself has the context to do, and
// the latter is derived/cached data that is never serialized at all.
//
// Adding a new built-in component type? Register its JSON (de)serialization here too.

[[nodiscard]] nlohmann::json ToJsonUVE(const TransformComponentUVE& component) {
    return {
        {"localPosition", ToJsonUVE(component.localPosition)},
        {"localRotation", ToJsonUVE(component.localRotation)},
        {"localScale", ToJsonUVE(component.localScale)},
    };
}

[[nodiscard]] nlohmann::json ToJsonUVE(const MeshComponentUVE& component) {
    return {{"meshGuid", component.meshGuid.value}, {"materialGuid", component.materialGuid.value}};
}

[[nodiscard]] nlohmann::json ToJsonUVE(const LightComponentUVE& component) {
    return {{"color", ToJsonUVE(component.color)},
            {"intensity", component.intensity},
            {"type", static_cast<std::uint8_t>(component.type)},
            {"range", component.range},
            {"spotAngleDegrees", component.spotAngleDegrees}};
}

[[nodiscard]] nlohmann::json ToJsonUVE(const CameraComponentUVE& component) {
    return {
        {"fieldOfViewDegrees", component.fieldOfViewDegrees},
        {"nearPlane", component.nearPlane},
        {"farPlane", component.farPlane},
    };
}

[[nodiscard]] nlohmann::json ToJsonUVE(const ColliderComponentUVE& component) {
    return {{"halfExtents", ToJsonUVE(component.halfExtents)},
            {"collisionLayer", component.collisionLayer},
            {"collisionMask", component.collisionMask},
            {"friction", component.friction},
            {"restitution", component.restitution},
            {"density", component.density}};
}

[[nodiscard]] nlohmann::json ToJsonUVE(const RigidBodyComponentUVE& component) {
    return {{"mass", component.mass},
            {"isKinematic", component.isKinematic},
            {"velocity", ToJsonUVE(component.velocity)},
            {"drag", component.drag},
            {"gravityScale", component.gravityScale}};
}

[[nodiscard]] nlohmann::json ToJsonUVE(const AudioSourceComponentUVE& component) {
    return {{"audioAssetPath", component.audioAssetPath},
            {"volume", component.volume},
            {"looping", component.looping},
            {"pitch", component.pitch},
            {"spatial", component.spatial},
            {"minDistance", component.minDistance},
            {"maxDistance", component.maxDistance},
            {"attenuationCurve", static_cast<std::uint8_t>(component.attenuationCurve)},
            {"playOnAwake", component.playOnAwake}};
}

[[nodiscard]] nlohmann::json ToJsonUVE(const ScriptComponentUVE& component) {
    return {{"scriptAssetPath", component.scriptAssetPath}};
}

[[nodiscard]] nlohmann::json ToJsonUVE(const ParticleEmitterComponentUVE& component) {
    return {{"maxParticles", component.maxParticles}};
}

[[nodiscard]] nlohmann::json ToJsonUVE(const PrefabInstanceComponentUVE& component) {
    return {{"sourcePrefabGuid", component.sourcePrefabGuid.value}};
}

/// One entry in the component-serializer table: `toJson` reads `entity`'s component of the
/// registered type via IEntityManagerUVE::GetComponentPointerUVE(); `fromJson` adds a fresh
/// component (built from `json`) to `entity`.
struct ComponentRegistrationUVE {
    std::type_index typeIndex;
    std::function<nlohmann::json(IEntityManagerUVE&, EntityUVE)> toJson;
    std::function<void(IEntityManagerUVE&, EntityUVE, const nlohmann::json&)> fromJson;
};

template <typename T, typename FromJsonFunc>
[[nodiscard]] ComponentRegistrationUVE MakeRegistrationUVE(FromJsonFunc fromJsonFunc) {
    return ComponentRegistrationUVE{
        std::type_index(typeid(T)),
        [](IEntityManagerUVE& entityManager, EntityUVE entity) -> nlohmann::json {
            return ToJsonUVE(entityManager.GetComponentUVE<T>(entity));
        },
        [fromJsonFunc](IEntityManagerUVE& entityManager, EntityUVE entity, const nlohmann::json& json) {
            entityManager.AddComponentUVE<T>(entity, fromJsonFunc(json));
        },
    };
}

[[nodiscard]] const std::unordered_map<std::string, ComponentRegistrationUVE>& GetRegistrationsByNameUVE() {
    static const std::unordered_map<std::string, ComponentRegistrationUVE> registrations = [] {
        std::unordered_map<std::string, ComponentRegistrationUVE> table;

        table.emplace("TransformComponentUVE", MakeRegistrationUVE<TransformComponentUVE>([](const nlohmann::json& json) {
                          return TransformComponentUVE{Vector3FromJsonUVE(json.at("localPosition")),
                                                        QuaternionFromJsonUVE(json.at("localRotation")),
                                                        Vector3FromJsonUVE(json.at("localScale"))};
                      }));
        table.emplace("MeshComponentUVE", MakeRegistrationUVE<MeshComponentUVE>([](const nlohmann::json& json) {
                          return MeshComponentUVE{Asset::AssetGuidUVE{json.at("meshGuid").get<std::uint64_t>()},
                                                   Asset::AssetGuidUVE{json.at("materialGuid").get<std::uint64_t>()}};
                      }));
        table.emplace("LightComponentUVE", MakeRegistrationUVE<LightComponentUVE>([](const nlohmann::json& json) {
                          LightComponentUVE light;
                          light.color = Vector3FromJsonUVE(json.at("color"));
                          light.intensity = json.at("intensity").get<float>();
                          light.type = static_cast<LightTypeUVE>(
                              json.value("type", static_cast<std::uint8_t>(LightTypeUVE::Directional)));
                          light.range = json.value("range", 10.0F);
                          light.spotAngleDegrees = json.value("spotAngleDegrees", 45.0F);
                          return light;
                      }));
        table.emplace("CameraComponentUVE", MakeRegistrationUVE<CameraComponentUVE>([](const nlohmann::json& json) {
                          return CameraComponentUVE{json.at("fieldOfViewDegrees").get<float>(),
                                                     json.at("nearPlane").get<float>(),
                                                     json.at("farPlane").get<float>()};
                      }));
        table.emplace("ColliderComponentUVE", MakeRegistrationUVE<ColliderComponentUVE>([](const nlohmann::json& json) {
                          ColliderComponentUVE collider;
                          collider.halfExtents = Vector3FromJsonUVE(json.at("halfExtents"));
                          collider.collisionLayer = json.value("collisionLayer", std::uint32_t{1});
                          collider.collisionMask = json.value("collisionMask", std::uint32_t{0xFFFFFFFFU});
                          collider.friction = json.value("friction", 0.0F);
                          collider.restitution = json.value("restitution", 0.0F);
                          collider.density = json.value("density", 1.0F);
                          return collider;
                      }));
        table.emplace("RigidBodyComponentUVE", MakeRegistrationUVE<RigidBodyComponentUVE>([](const nlohmann::json& json) {
                          RigidBodyComponentUVE rigidBody;
                          rigidBody.mass = json.at("mass").get<float>();
                          rigidBody.isKinematic = json.at("isKinematic").get<bool>();
                          rigidBody.velocity =
                              json.contains("velocity") ? Vector3FromJsonUVE(json.at("velocity")) : Math::Vector3UVE{};
                          rigidBody.drag = json.value("drag", 0.0F);
                          rigidBody.gravityScale = json.value("gravityScale", 1.0F);
                          return rigidBody;
                      }));
        table.emplace("AudioSourceComponentUVE",
                      MakeRegistrationUVE<AudioSourceComponentUVE>([](const nlohmann::json& json) {
                          AudioSourceComponentUVE source;
                          source.audioAssetPath = json.at("audioAssetPath").get<std::string>();
                          source.volume = json.at("volume").get<float>();
                          source.looping = json.value("looping", false);
                          source.pitch = json.value("pitch", 1.0F);
                          source.spatial = json.value("spatial", true);
                          source.minDistance = json.value("minDistance", 1.0F);
                          source.maxDistance = json.value("maxDistance", 25.0F);
                          source.attenuationCurve = static_cast<AudioAttenuationCurveUVE>(
                              json.value("attenuationCurve", std::uint8_t{0}));
                          source.playOnAwake = json.value("playOnAwake", true);
                          return source;
                      }));
        table.emplace("ScriptComponentUVE", MakeRegistrationUVE<ScriptComponentUVE>([](const nlohmann::json& json) {
                          return ScriptComponentUVE{json.at("scriptAssetPath").get<std::string>()};
                      }));
        table.emplace("ParticleEmitterComponentUVE",
                      MakeRegistrationUVE<ParticleEmitterComponentUVE>([](const nlohmann::json& json) {
                          return ParticleEmitterComponentUVE{json.at("maxParticles").get<std::uint32_t>()};
                      }));
        table.emplace("PrefabInstanceComponentUVE",
                      MakeRegistrationUVE<PrefabInstanceComponentUVE>([](const nlohmann::json& json) {
                          return PrefabInstanceComponentUVE{
                              Asset::AssetGuidUVE{json.at("sourcePrefabGuid").get<std::uint64_t>()}};
                      }));

        return table;
    }();
    return registrations;
}

[[nodiscard]] const std::string* FindNameForTypeIndexUVE(std::type_index typeIndex) {
    static const std::unordered_map<std::type_index, std::string> namesByType = [] {
        std::unordered_map<std::type_index, std::string> map;
        for (const auto& [name, registration] : GetRegistrationsByNameUVE()) {
            map.emplace(registration.typeIndex, name);
        }
        return map;
    }();
    const auto it = namesByType.find(typeIndex);
    return it == namesByType.end() ? nullptr : &it->second;
}

/// Appends `root` and every descendant reachable via HierarchyComponentUVE.parent to
/// `outEntities`, depth-first. SceneSerializerUVE deliberately doesn't depend on
/// ISceneGraphUVE (keeping it usable standalone), so this duplicates SceneGraphUVE::
/// GetChildrenUVE()'s O(n)-scan-per-level approach rather than sharing it.
void CollectSubtreeUVE(IEntityManagerUVE& entityManager, EntityUVE root, std::vector<EntityUVE>& outEntities) {
    outEntities.push_back(root);
    std::vector<EntityUVE> children;
    entityManager.ForEachUVE<HierarchyComponentUVE>(
        [&children, root](EntityUVE entity, HierarchyComponentUVE& hierarchy) {
            if (hierarchy.parent == root) {
                children.push_back(entity);
            }
        });
    for (EntityUVE child : children) {
        CollectSubtreeUVE(entityManager, child, outEntities);
    }
}

} // namespace

bool SceneSerializerUVE::SaveUVE(IEntityManagerUVE& entityManager, const std::vector<EntityUVE>& rootEntities,
                                  const std::filesystem::path& path, SceneAssetTypeUVE assetType) {
    std::vector<EntityUVE> allEntities;
    for (EntityUVE root : rootEntities) {
        CollectSubtreeUVE(entityManager, root, allEntities);
    }

    std::unordered_map<EntityUVE, std::uint32_t> entityToLocalId;
    entityToLocalId.reserve(allEntities.size());
    for (std::uint32_t index = 0; index < allEntities.size(); ++index) {
        entityToLocalId.emplace(allEntities[index], index);
    }

    nlohmann::json entitiesJson = nlohmann::json::array();
    for (EntityUVE entity : allEntities) {
        nlohmann::json componentsJson = nlohmann::json::object();

        for (std::type_index type : entityManager.GetComponentTypesUVE(entity)) {
            if (type == std::type_index(typeid(WorldTransformComponentUVE))) {
                continue; // derived/cached, never serialized
            }
            if (type == std::type_index(typeid(HierarchyComponentUVE))) {
                const HierarchyComponentUVE& hierarchy = entityManager.GetComponentUVE<HierarchyComponentUVE>(entity);
                std::int64_t parentLocalId = -1;
                if (hierarchy.parent != kInvalidEntityUVE) {
                    const auto parentIt = entityToLocalId.find(hierarchy.parent);
                    if (parentIt != entityToLocalId.end()) {
                        parentLocalId = static_cast<std::int64_t>(parentIt->second);
                    }
                    // else: parent lies outside the saved subtree - treat this entity as a root.
                }
                componentsJson["HierarchyComponentUVE"] = {{"parentLocalId", parentLocalId}};
                continue;
            }

            const std::string* const name = FindNameForTypeIndexUVE(type);
            if (name == nullptr) {
                UVE_ERROR("SceneSerializerUVE: no registered serializer for a component type on "
                          "entity index {} - aborting save of \"{}\"",
                          entity.index, path.string());
                return false;
            }
            componentsJson[*name] = GetRegistrationsByNameUVE().at(*name).toJson(entityManager, entity);
        }

        entitiesJson.push_back({{"localId", entityToLocalId.at(entity)}, {"components", std::move(componentsJson)}});
    }

    nlohmann::json payload;
    payload["entities"] = std::move(entitiesJson);
    const std::string payloadText = payload.dump();
    const std::byte* const payloadBytes = reinterpret_cast<const std::byte*>(payloadText.data());
    const std::vector<std::byte> payloadBuffer(payloadBytes, payloadBytes + payloadText.size());

    return Asset::WriteUveFileUVE(path, assetType, payloadBuffer);
}

std::vector<EntityUVE> SceneSerializerUVE::LoadUVE(IEntityManagerUVE& entityManager,
                                                    const std::filesystem::path& path) {
    std::optional<std::pair<Asset::UveFileHeaderUVE, std::vector<std::byte>>> file =
        Asset::ReadUveFileUVE(path);
    if (!file.has_value()) {
        return {}; // ReadUveFileUVE already logged the specific reason.
    }
    const auto& [header, payloadBuffer] = file.value();
    if (header.assetType != SceneAssetTypeUVE::Scene && header.assetType != SceneAssetTypeUVE::Prefab) {
        UVE_ERROR("SceneSerializerUVE: \"{}\" has unexpected asset type {}", path.string(),
                  static_cast<std::uint32_t>(header.assetType));
        return {};
    }

    const std::string payloadText(reinterpret_cast<const char*>(payloadBuffer.data()), payloadBuffer.size());
    nlohmann::json payload;
    try {
        payload = nlohmann::json::parse(payloadText);
    } catch (const nlohmann::json::parse_error& parseError) {
        UVE_ERROR("SceneSerializerUVE: failed to parse \"{}\": {}", path.string(), parseError.what());
        return {};
    }

    std::vector<std::pair<std::uint32_t, nlohmann::json>> orderedEntities;
    std::unordered_map<std::uint32_t, EntityUVE> localIdToEntity;
    try {
        for (const nlohmann::json& entityJson : payload.at("entities")) {
            const std::uint32_t localId = entityJson.at("localId").get<std::uint32_t>();
            const EntityUVE entity = entityManager.CreateEntityUVE();
            localIdToEntity.emplace(localId, entity);
            orderedEntities.emplace_back(localId, entityJson);
        }
    } catch (const nlohmann::json::exception& jsonError) {
        UVE_ERROR("SceneSerializerUVE: malformed entity list in \"{}\": {}", path.string(), jsonError.what());
        return {};
    }

    std::vector<EntityUVE> roots;
    for (const auto& [localId, entityJson] : orderedEntities) {
        const EntityUVE entity = localIdToEntity.at(localId);
        bool isRoot = true;
        bool hasTransform = false;

        try {
            for (const auto& [componentName, componentJson] : entityJson.at("components").items()) {
                if (componentName == "HierarchyComponentUVE") {
                    const std::int64_t parentLocalId = componentJson.at("parentLocalId").get<std::int64_t>();
                    EntityUVE parent = kInvalidEntityUVE;
                    if (parentLocalId >= 0) {
                        const auto parentIt = localIdToEntity.find(static_cast<std::uint32_t>(parentLocalId));
                        if (parentIt != localIdToEntity.end()) {
                            parent = parentIt->second;
                            isRoot = false;
                        }
                    }
                    entityManager.AddComponentUVE<HierarchyComponentUVE>(entity, HierarchyComponentUVE{parent});
                    continue;
                }

                const auto registrationIt = GetRegistrationsByNameUVE().find(componentName);
                if (registrationIt == GetRegistrationsByNameUVE().end()) {
                    UVE_ERROR("SceneSerializerUVE: \"{}\" references unknown component type \"{}\"",
                              path.string(), componentName);
                    return {};
                }
                registrationIt->second.fromJson(entityManager, entity, componentJson);
                if (componentName == "TransformComponentUVE") {
                    hasTransform = true;
                }
            }
        } catch (const nlohmann::json::exception& jsonError) {
            UVE_ERROR("SceneSerializerUVE: malformed component data in \"{}\": {}", path.string(),
                      jsonError.what());
            return {};
        }

        // AttachTransformUVE always adds Transform+World Transform+Hierarchy together; since
        // WorldTransformComponentUVE is never serialized, restore it here so SceneGraphUVE::
        // UpdateUVE() picks this entity back up correctly after load.
        if (hasTransform && !entityManager.HasComponentUVE<WorldTransformComponentUVE>(entity)) {
            entityManager.AddComponentUVE<WorldTransformComponentUVE>(entity);
        }

        if (isRoot) {
            roots.push_back(entity);
        }
    }

    return roots;
}

} // namespace UVE::Scene
