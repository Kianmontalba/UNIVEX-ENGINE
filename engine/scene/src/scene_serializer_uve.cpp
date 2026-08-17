// Copyright (c) 2026 UniVex Studios. All Rights Reserved.


#include "uve/scene/scene_serializer_uve.h"

#include <cstddef>
#include <cstdint>
#include <functional>
#include <limits>
#include <optional>
#include <stdexcept>
#include <string>
#include <string_view>
#include <typeindex>
#include <unordered_map>
#include <unordered_set>
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
#include "uve/scene/components/name_component_uve.h"
#include "uve/scene/components/particle_emitter_component_uve.h"
#include "uve/scene/components/primitive_mesh_component_uve.h"
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

[[nodiscard]] nlohmann::json ToJsonUVE(const PrimitiveMeshComponentUVE& component) {
    return {{"kind", static_cast<std::uint8_t>(component.kind)}, {"baseColor", ToJsonUVE(component.baseColor)}};
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

[[nodiscard]] nlohmann::json ToJsonUVE(const NameComponentUVE& component) {
    return {{"name", component.name}};
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
        table.emplace("PrimitiveMeshComponentUVE",
                      MakeRegistrationUVE<PrimitiveMeshComponentUVE>([](const nlohmann::json& json) {
                          PrimitiveMeshComponentUVE primitive;
                          primitive.kind = static_cast<PrimitiveMeshKindUVE>(json.at("kind").get<std::uint8_t>());
                          primitive.baseColor = Vector3FromJsonUVE(json.at("baseColor"));
                          if (!IsPrimitiveMeshComponentValidUVE(primitive)) {
                              throw std::runtime_error("Invalid PrimitiveMeshComponentUVE payload");
                          }
                          return primitive;
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
                          const CameraComponentUVE camera{json.at("fieldOfViewDegrees").get<float>(),
                                                          json.at("nearPlane").get<float>(),
                                                          json.at("farPlane").get<float>()};
                          if (!IsCameraComponentValidUVE(camera)) {
                              throw std::runtime_error("Invalid CameraComponentUVE payload");
                          }
                          return camera;
                      }));
        table.emplace("NameComponentUVE", MakeRegistrationUVE<NameComponentUVE>([](const nlohmann::json& json) {
                          return NameComponentUVE{json.at("name").get<std::string>()};
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
/// `outEntities`, depth-first. The visited set deduplicates overlapping requested roots and makes
/// malformed hierarchy cycles fail closed rather than recursing indefinitely.
[[nodiscard]] bool CollectSubtreeUVE(IEntityManagerUVE& entityManager, const EntityUVE root,
                                     std::unordered_set<EntityUVE>& visited,
                                     std::vector<EntityUVE>& outEntities) {
    if (!entityManager.IsAliveUVE(root)) {
        return false;
    }
    if (!visited.emplace(root).second) {
        return true;
    }

    outEntities.push_back(root);
    std::vector<EntityUVE> children;
    entityManager.ForEachUVE<HierarchyComponentUVE>(
        [&children, root](const EntityUVE entity, HierarchyComponentUVE& hierarchy) {
            if (hierarchy.parent == root) {
                children.push_back(entity);
            }
        });
    for (const EntityUVE child : children) {
        if (!CollectSubtreeUVE(entityManager, child, visited, outEntities)) {
            return false;
        }
    }
    return true;
}

[[nodiscard]] bool IsSceneAssetTypeUVE(const SceneAssetTypeUVE assetType) noexcept {
    return assetType == SceneAssetTypeUVE::Scene || assetType == SceneAssetTypeUVE::Prefab;
}

[[nodiscard]] std::optional<std::vector<std::byte>> EncodeScenePayloadUVE(
    IEntityManagerUVE& entityManager, const std::vector<EntityUVE>& rootEntities,
    const std::string_view sourceDescription) {
    std::vector<EntityUVE> allEntities;
    std::unordered_set<EntityUVE> visited;
    for (const EntityUVE root : rootEntities) {
        if (!CollectSubtreeUVE(entityManager, root, visited, allEntities)) {
            UVE_ERROR("SceneSerializerUVE: \"{}\" includes an invalid root entity", sourceDescription);
            return std::nullopt;
        }
    }
    if (allEntities.size() > std::numeric_limits<std::uint32_t>::max()) {
        UVE_ERROR("SceneSerializerUVE: \"{}\" contains too many entities to serialize", sourceDescription);
        return std::nullopt;
    }

    std::unordered_map<EntityUVE, std::uint32_t> entityToLocalId;
    entityToLocalId.reserve(allEntities.size());
    for (std::uint32_t index = 0; index < allEntities.size(); ++index) {
        entityToLocalId.emplace(allEntities[index], index);
    }

    nlohmann::json entitiesJson = nlohmann::json::array();
    for (const EntityUVE entity : allEntities) {
        nlohmann::json componentsJson = nlohmann::json::object();
        for (const std::type_index type : entityManager.GetComponentTypesUVE(entity)) {
            if (type == std::type_index(typeid(WorldTransformComponentUVE))) {
                continue; // Derived/cached state is rebuilt after restore.
            }
            if (type == std::type_index(typeid(HierarchyComponentUVE))) {
                const HierarchyComponentUVE& hierarchy = entityManager.GetComponentUVE<HierarchyComponentUVE>(entity);
                std::int64_t parentLocalId = -1;
                if (hierarchy.parent != kInvalidEntityUVE) {
                    const auto parentIt = entityToLocalId.find(hierarchy.parent);
                    if (parentIt != entityToLocalId.end()) {
                        parentLocalId = static_cast<std::int64_t>(parentIt->second);
                    }
                    // A parent outside the captured subtree intentionally becomes a restored root.
                }
                componentsJson["HierarchyComponentUVE"] = {{"parentLocalId", parentLocalId}};
                continue;
            }

            const std::string* const name = FindNameForTypeIndexUVE(type);
            if (name == nullptr) {
                UVE_ERROR("SceneSerializerUVE: no registered serializer for a component type on entity index {} "
                          "while encoding \"{}\"",
                          entity.index, sourceDescription);
                return std::nullopt;
            }
            componentsJson[*name] = GetRegistrationsByNameUVE().at(*name).toJson(entityManager, entity);
        }
        entitiesJson.push_back({{"localId", entityToLocalId.at(entity)}, {"components", std::move(componentsJson)}});
    }

    nlohmann::json payload;
    payload["entities"] = std::move(entitiesJson);
    const std::string payloadText = payload.dump();
    const auto* const payloadBytes = reinterpret_cast<const std::byte*>(payloadText.data());
    return std::vector<std::byte>{payloadBytes, payloadBytes + payloadText.size()};
}

void RollbackRestoredEntitiesUVE(IEntityManagerUVE& entityManager, std::vector<EntityUVE>& createdEntities) {
    for (auto entity = createdEntities.rbegin(); entity != createdEntities.rend(); ++entity) {
        if (entityManager.IsAliveUVE(*entity)) {
            entityManager.DestroyEntityUVE(*entity);
        }
    }
}

[[nodiscard]] std::optional<std::vector<EntityUVE>> DecodeScenePayloadUVE(
    IEntityManagerUVE& entityManager, const std::vector<std::byte>& payloadBuffer,
    const std::string_view sourceDescription) {
    const std::string payloadText(reinterpret_cast<const char*>(payloadBuffer.data()), payloadBuffer.size());
    nlohmann::json payload;
    try {
        payload = nlohmann::json::parse(payloadText);
    } catch (const nlohmann::json::parse_error& parseError) {
        UVE_ERROR("SceneSerializerUVE: failed to parse \"{}\": {}", sourceDescription, parseError.what());
        return std::nullopt;
    }

    std::vector<std::pair<std::uint32_t, nlohmann::json>> orderedEntities;
    std::unordered_set<std::uint32_t> localIds;
    try {
        for (const nlohmann::json& entityJson : payload.at("entities")) {
            const std::uint32_t localId = entityJson.at("localId").get<std::uint32_t>();
            const nlohmann::json& components = entityJson.at("components");
            if (!components.is_object() || !localIds.emplace(localId).second) {
                UVE_ERROR("SceneSerializerUVE: malformed entity list in \"{}\"", sourceDescription);
                return std::nullopt;
            }
            for (const auto& [componentName, componentJson] : components.items()) {
                if (componentName == "HierarchyComponentUVE") {
                    static_cast<void>(componentJson.at("parentLocalId").get<std::int64_t>());
                    continue;
                }
                if (GetRegistrationsByNameUVE().find(componentName) == GetRegistrationsByNameUVE().end()) {
                    UVE_ERROR("SceneSerializerUVE: \"{}\" references unknown component type \"{}\"",
                              sourceDescription, componentName);
                    return std::nullopt;
                }
            }
            orderedEntities.emplace_back(localId, entityJson);
        }
    } catch (const nlohmann::json::exception& jsonError) {
        UVE_ERROR("SceneSerializerUVE: malformed entity list in \"{}\": {}", sourceDescription, jsonError.what());
        return std::nullopt;
    }

    std::unordered_map<std::uint32_t, EntityUVE> localIdToEntity;
    localIdToEntity.reserve(orderedEntities.size());
    std::vector<EntityUVE> createdEntities;
    createdEntities.reserve(orderedEntities.size());
    for (const auto& [localId, unusedEntityJson] : orderedEntities) {
        static_cast<void>(unusedEntityJson);
        const EntityUVE entity = entityManager.CreateEntityUVE();
        localIdToEntity.emplace(localId, entity);
        createdEntities.push_back(entity);
    }

    std::vector<EntityUVE> roots;
    try {
        for (const auto& [localId, entityJson] : orderedEntities) {
            const EntityUVE entity = localIdToEntity.at(localId);
            bool isRoot = true;
            bool hasTransform = false;
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
                registrationIt->second.fromJson(entityManager, entity, componentJson);
                hasTransform = hasTransform || componentName == "TransformComponentUVE";
            }

            // AttachTransformUVE creates Transform+WorldTransform+Hierarchy together. Recreate the
            // derived component here so the next SceneGraphUVE update sees restored transforms.
            if (hasTransform && !entityManager.HasComponentUVE<WorldTransformComponentUVE>(entity)) {
                entityManager.AddComponentUVE<WorldTransformComponentUVE>(entity);
            }
            if (isRoot) {
                roots.push_back(entity);
            }
        }
    } catch (const nlohmann::json::exception& jsonError) {
        UVE_ERROR("SceneSerializerUVE: malformed component data in \"{}\": {}", sourceDescription,
                  jsonError.what());
        RollbackRestoredEntitiesUVE(entityManager, createdEntities);
        return std::nullopt;
    } catch (const std::exception& validationError) {
        UVE_ERROR("SceneSerializerUVE: invalid component data in \"{}\": {}", sourceDescription,
                  validationError.what());
        RollbackRestoredEntitiesUVE(entityManager, createdEntities);
        return std::nullopt;
    }

    return roots;
}

[[nodiscard]] bool ValidateSceneAssetTypeUVE(const SceneAssetTypeUVE assetType,
                                             const std::string_view sourceDescription) {
    if (IsSceneAssetTypeUVE(assetType)) {
        return true;
    }
    UVE_ERROR("SceneSerializerUVE: \"{}\" has unexpected asset type {}", sourceDescription,
              static_cast<std::uint32_t>(assetType));
    return false;
}

} // namespace

std::optional<SceneSnapshotUVE> SceneSerializerUVE::CaptureUVE(
    IEntityManagerUVE& entityManager, const std::vector<EntityUVE>& rootEntities,
    const SceneAssetTypeUVE assetType) const {
    if (!ValidateSceneAssetTypeUVE(assetType, "scene snapshot")) {
        return std::nullopt;
    }
    const std::optional<std::vector<std::byte>> payload =
        EncodeScenePayloadUVE(entityManager, rootEntities, "scene snapshot");
    if (!payload.has_value()) {
        return std::nullopt;
    }
    return SceneSnapshotUVE{Asset::EncodeUveFileEnvelopeUVE(assetType, *payload), assetType};
}

std::vector<EntityUVE> SceneSerializerUVE::RestoreUVE(IEntityManagerUVE& entityManager,
                                                       const SceneSnapshotUVE& snapshot) const {
    const auto envelope = Asset::DecodeUveFileEnvelopeUVE(snapshot.bytes, "scene snapshot");
    if (!envelope.has_value()) {
        return {};
    }
    const auto& [header, payload] = *envelope;
    if (!ValidateSceneAssetTypeUVE(header.assetType, "scene snapshot") || header.assetType != snapshot.assetType) {
        if (header.assetType != snapshot.assetType) {
            UVE_ERROR("SceneSerializerUVE: scene snapshot asset type metadata does not match its envelope");
        }
        return {};
    }
    const std::optional<std::vector<EntityUVE>> restored =
        DecodeScenePayloadUVE(entityManager, payload, "scene snapshot");
    return restored.value_or(std::vector<EntityUVE>{});
}

bool SceneSerializerUVE::SaveUVE(IEntityManagerUVE& entityManager, const std::vector<EntityUVE>& rootEntities,
                                  const std::filesystem::path& path, const SceneAssetTypeUVE assetType) {
    if (!ValidateSceneAssetTypeUVE(assetType, path.string())) {
        return false;
    }
    const std::optional<std::vector<std::byte>> payload = EncodeScenePayloadUVE(entityManager, rootEntities, path.string());
    return payload.has_value() && Asset::WriteUveFileUVE(path, assetType, *payload);
}

std::vector<EntityUVE> SceneSerializerUVE::LoadUVE(IEntityManagerUVE& entityManager,
                                                    const std::filesystem::path& path) {
    const auto file = Asset::ReadUveFileUVE(path);
    if (!file.has_value()) {
        return {};
    }
    const auto& [header, payload] = *file;
    if (!ValidateSceneAssetTypeUVE(header.assetType, path.string())) {
        return {};
    }
    const std::optional<std::vector<EntityUVE>> restored = DecodeScenePayloadUVE(entityManager, payload, path.string());
    return restored.value_or(std::vector<EntityUVE>{});
}

} // namespace UVE::Scene
