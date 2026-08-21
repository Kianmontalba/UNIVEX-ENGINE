// Copyright (c) 2026 UniVex Studios. All Rights Reserved.
#include "uve/scripting/script_builtin_nodes_uve.h"
#include "uve/scripting/script_bytecode_uve.h"
#include "uve/scripting/script_runtime_uve.h"
#include "uve/scripting/script_vm_uve.h"
#include "uve/scripting/script_rotation_value_uve.h"
#include "uve/scripting/script_entity_query_adapter_uve.h"
#include "uve/scripting/script_compiler_ir_uve.h"
#include "uve/scripting/script_debugger_uve.h"
#include "uve/scripting/script_graph_editor_backend_uve.h"
#include "uve/scripting/script_graph_canvas_uve.h"
#include "uve/scripting/script_graph_canvas_persistence_uve.h"
#include "uve/scripting/script_graph_persistence_uve.h"
#include "uve/scripting/script_graph_uve.h"
#include "uve/scripting/script_graph_runtime_binding_uve.h"
#include "uve/scripting/script_component_runtime_ownership_uve.h"
#include "uve/scripting/script_hot_reload_uve.h"

#include <gtest/gtest.h>

#include <array>
#include <cmath>
#include <cstdint>
#include <limits>
#include <optional>
#include <vector>

#include "uve/events/event_system_uve.h"
#include "uve/memory/memory_manager_uve.h"
#include "uve/scene/components/name_component_uve.h"
#include "uve/scene/entity_manager_uve.h"

namespace UVE::Scripting {
namespace {

ScriptNodeTypeDescriptorUVE MakeSourceNodeUVE() {
    return ScriptNodeTypeDescriptorUVE{
        "test.source",
        "Test Source",
        {{"Out", ScriptPinDirectionUVE::Output, ScriptValueTypeUVE::Number},
         {"Exec", ScriptPinDirectionUVE::Output, ScriptValueTypeUVE::Execution}},
    };
}

ScriptNodeTypeDescriptorUVE MakeSinkNodeUVE() {
    return ScriptNodeTypeDescriptorUVE{
        "test.sink",
        "Test Sink",
        {{"In", ScriptPinDirectionUVE::Input, ScriptValueTypeUVE::Number},
         {"Exec", ScriptPinDirectionUVE::Input, ScriptValueTypeUVE::Execution}},
    };
}

ScriptNodeTypeDescriptorUVE MakeBooleanSourceNodeUVE() {
    return ScriptNodeTypeDescriptorUVE{
        "test.boolean-source",
        "Boolean Source",
        {{"Out", ScriptPinDirectionUVE::Output, ScriptValueTypeUVE::Boolean}},
    };
}

struct EngineLogCaptureUVE final {
    std::size_t callCount = 0U;
    float lastValue = 0.0F;
    bool accept = true;
};

bool CaptureEngineLogUVE(void* userData, const float value) noexcept {
    auto* capture = static_cast<EngineLogCaptureUVE*>(userData);
    if (capture == nullptr) {
        return false;
    }
    ++capture->callCount;
    capture->lastValue = value;
    return capture->accept;
}

struct EngineTimeCaptureUVE final {
    std::size_t callCount = 0U;
    float value = 0.0F;
    bool accept = true;
    bool finite = true;
};

bool CaptureEngineTimeUVE(void* userData, float* outSeconds) noexcept {
    auto* capture = static_cast<EngineTimeCaptureUVE*>(userData);
    if (capture == nullptr || outSeconds == nullptr) {
        return false;
    }
    ++capture->callCount;
    if (!capture->accept) {
        return false;
    }
    *outSeconds = capture->finite ? capture->value : std::numeric_limits<float>::quiet_NaN();
    return true;
}

struct EntityLifecycleCaptureUVE final {
    std::size_t spawnCount = 0U;
    std::size_t destroyCount = 0U;
    std::size_t findCount = 0U;
    std::size_t getCount = 0U;
    std::size_t addCount = 0U;
    std::size_t removeCount = 0U;
    Scene::EntityUVE entity{42U, 3U};
    std::string lastComponentType;
    float lastHandle = 0.0F;
};

bool CaptureEntitySpawnUVE(void* userData, Scene::EntityUVE* outEntity) noexcept {
    auto* capture = static_cast<EntityLifecycleCaptureUVE*>(userData);
    if (capture == nullptr || outEntity == nullptr) {
        return false;
    }
    ++capture->spawnCount;
    *outEntity = capture->entity;
    return true;
}

bool CaptureEntityDestroyUVE(void* userData, const Scene::EntityUVE entity) noexcept {
    auto* capture = static_cast<EntityLifecycleCaptureUVE*>(userData);
    if (capture == nullptr || entity != capture->entity) {
        return false;
    }
    ++capture->destroyCount;
    return true;
}

bool CaptureEntityFindUVE(void* userData, const ScriptComponentValueUVE& component,
                         Scene::EntityUVE* outEntity) noexcept {
    auto* capture = static_cast<EntityLifecycleCaptureUVE*>(userData);
    if (capture == nullptr || outEntity == nullptr) {
        return false;
    }
    ++capture->findCount;
    capture->lastComponentType = component.componentTypeId;
    *outEntity = capture->entity;
    return true;
}

bool CaptureEntityGetUVE(void* userData, const float handle, Scene::EntityUVE* outEntity) noexcept {
    auto* capture = static_cast<EntityLifecycleCaptureUVE*>(userData);
    if (capture == nullptr || outEntity == nullptr) {
        return false;
    }
    ++capture->getCount;
    capture->lastHandle = handle;
    *outEntity = capture->entity;
    return true;
}

bool CaptureEntityMutationUVE(void* userData, const Scene::EntityUVE entity,
                             const ScriptComponentValueUVE& component, const bool isAdd) noexcept {
    auto* capture = static_cast<EntityLifecycleCaptureUVE*>(userData);
    if (capture == nullptr || entity != capture->entity) {
        return false;
    }
    if (isAdd) {
        ++capture->addCount;
    } else {
        ++capture->removeCount;
    }
    capture->lastComponentType = component.componentTypeId;
    return true;
}

bool CaptureEntityAddUVE(void* userData, const Scene::EntityUVE entity,
                         const ScriptComponentValueUVE& component) noexcept {
    return CaptureEntityMutationUVE(userData, entity, component, true);
}

bool CaptureEntityRemoveUVE(void* userData, const Scene::EntityUVE entity,
                            const ScriptComponentValueUVE& component) noexcept {
    return CaptureEntityMutationUVE(userData, entity, component, false);
}

void RegisterTestNodesUVE(ScriptNodeRegistryUVE& registry) {
    ASSERT_TRUE(registry.RegisterNodeTypeUVE(MakeSourceNodeUVE()));
    ASSERT_TRUE(registry.RegisterNodeTypeUVE(MakeSinkNodeUVE()));
}

} // namespace

TEST(ScriptNodeRegistryUVETest, RegisterNodeTypeUVE_RejectsDuplicateAndMalformedDescriptors) {
    ScriptNodeRegistryUVE registry;
    EXPECT_FALSE(registry.RegisterNodeTypeUVE({"", "Empty", {}}));
    EXPECT_FALSE(registry.RegisterNodeTypeUVE({"test.empty-display", "", {}}));
    EXPECT_FALSE(registry.RegisterNodeTypeUVE({"test.empty-pin", "Invalid", {{"", ScriptPinDirectionUVE::Input,
                                                                                ScriptValueTypeUVE::Number}}}));
    EXPECT_FALSE(registry.RegisterNodeTypeUVE({"test.duplicate-pin", "Invalid",
                                                {{"Value", ScriptPinDirectionUVE::Input, ScriptValueTypeUVE::Number},
                                                 {"Value", ScriptPinDirectionUVE::Output, ScriptValueTypeUVE::Number}}}));
    EXPECT_TRUE(registry.RegisterNodeTypeUVE(MakeSourceNodeUVE()));
    EXPECT_FALSE(registry.RegisterNodeTypeUVE(MakeSourceNodeUVE()));
    EXPECT_EQ(registry.GetNodeTypeCountUVE(), 1U);
}

TEST(ScriptNodeRegistryUVETest, BuiltInVector3Catalog_RegistersDeterministicDescriptorContracts) {
    ScriptNodeRegistryUVE registry;

    ASSERT_TRUE(RegisterBuiltInScriptNodesUVE(registry));
    EXPECT_FALSE(RegisterBuiltInScriptNodesUVE(registry));
    EXPECT_EQ(registry.GetNodeTypeCountUVE(), 167U);

    const std::vector<ScriptNodeTypeDescriptorUVE> descriptors = registry.GetNodeTypeDescriptorsUVE();
    ASSERT_EQ(descriptors.size(), 167U);
    const std::vector<std::string> expectedIds{
        "flow.sequence", "flow.branch", "flow.return", "flow.do_once", "flow.gate", "flow.switch",
        "flow.event", "flow.loop", "flow.for_loop", "flow.while_loop", "flow.delay",
        "convert.number_to_boolean", "convert.boolean_to_number", "convert.vector2_to_vector3", "convert.vector3_to_vector2",
        "math.float.add", "math.float.subtract", "math.float.multiply", "math.float.divide",
        "math.float.modulo", "math.float.abs", "math.float.min", "math.float.max", "math.float.clamp", "math.float.power",
        "math.float.lerp", "math.float.remap", "math.float.sin", "math.float.cos", "math.float.tan", "math.float.sqrt",
        "math.float.random", "math.float.random_range",
        "math.vector2.make", "math.vector2.add", "math.vector2.subtract", "math.vector2.multiply",
        "math.vector2.length", "math.vector2.normalize", "math.vector2.dot", "math.vector2.distance",
        "math.vector2.direction", "math.vector2.lerp",
        "math.vector3.make", "math.vector3.add", "math.vector3.subtract", "math.vector3.multiply",
        "math.vector3.dot", "math.vector3.cross", "math.vector3.length", "math.vector3.normalize",
        "math.vector3.distance", "math.vector3.direction", "math.vector3.lerp",
        "math.rotation.make", "math.rotation.break", "math.rotation.degrees", "math.rotation.radians",
        "math.rotation.euler", "math.rotation.quaternion", "math.rotation.look_at", "math.rotation.slerp", "math.rotation.rotate",
        "logic.boolean.not", "logic.boolean.and", "logic.boolean.or", "logic.boolean.xor",
        "logic.boolean.equal", "logic.boolean.not_equal", "logic.boolean.greater", "logic.boolean.less",
        "logic.boolean.greater_equal", "logic.boolean.less_equal",
        "math.transform.make", "math.transform.break", "math.transform.get_position", "math.transform.set_position",
        "math.transform.get_rotation", "math.transform.set_rotation", "math.transform.get_scale", "math.transform.set_scale",
        "math.transform.translate", "math.transform.rotate", "math.transform.transform_point",
        "query.entity.has_component", "query.entity.get_component", "engine.log", "engine.get_time",
        "variable.make_number", "variable.get_number", "variable.set_number",
        "variable.make_boolean", "variable.get_boolean", "variable.set_boolean",
        "variable.make_vector3", "variable.get_vector3", "variable.set_vector3",
        "variable.make_array", "variable.get_array", "variable.set_array",
        "variable.make_map", "variable.get_map", "variable.set_map",
        "variable.make_set", "variable.get_set", "variable.set_set",
        "variable.make_struct", "variable.get_struct", "variable.set_struct",
        "entity.spawn", "entity.destroy", "entity.find", "entity.get_entity", "entity.add_component", "entity.remove_component",
        "input.key_pressed", "input.key_released", "input.key_down", "input.mouse_position", "input.mouse_button",
        "input.gamepad_button", "input.get_axis", "input.get_action",
        "camera.get_camera", "camera.set_position", "camera.set_rotation", "camera.look_at", "camera.set_fov",
        "camera.shake", "camera.set_active",
        "animation.play", "animation.stop", "animation.pause", "animation.blend", "animation.blend_space",
        "animation.set_speed", "animation.set_weight", "animation.montage", "animation.get_current_animation",
        "animation.is_playing", "motion.query.build", "motion.query.search", "motion.query.get_best_match",
        "motion.query.set_trajectory", "motion.query.set_pose", "motion.query.set_velocity", "motion.query.set_facing",
        "motion.query.set_yaw", "motion.query.transition", "motion.query.motion_warp",
        "physics.raycast", "physics.sphere_cast", "physics.box_cast", "physics.capsule_cast", "physics.overlap",
        "physics.apply_force", "physics.apply_impulse", "physics.set_velocity", "physics.get_velocity",
        "physics.enable_gravity", "physics.is_colliding", "audio.set_volume", "audio.set_pitch",
        "audio.set_3d_position", "audio.play_sound", "audio.stop_sound", "audio.is_playing"};
    ASSERT_EQ(expectedIds.size(), descriptors.size());
    for (std::size_t index = 0U; index < expectedIds.size(); ++index) {
        EXPECT_EQ(descriptors[index].typeId, expectedIds[index]);
    }
    for (std::size_t index = 0U; index < 11U; ++index) {
        EXPECT_EQ(descriptors[index].category, "Flow");
        EXPECT_EQ(descriptors[index].iconId, "node.flow");
    }
    for (std::size_t index = 11U; index < 15U; ++index) {
        EXPECT_EQ(descriptors[index].category, "Conversion");
        EXPECT_EQ(descriptors[index].iconId, "node.conversion");
    }
    for (std::size_t index = 15U; index < 33U; ++index) {
        EXPECT_EQ(descriptors[index].category, "Math");
        EXPECT_EQ(descriptors[index].iconId, "node.math.float");
    }
    for (std::size_t index = 33U; index < 43U; ++index) {
        EXPECT_EQ(descriptors[index].category, "Math");
        EXPECT_EQ(descriptors[index].iconId, "node.math.vector2");
    }
    for (std::size_t index = 43U; index < 54U; ++index) {
        EXPECT_EQ(descriptors[index].category, "Math");
        EXPECT_EQ(descriptors[index].iconId, "node.math.vector3");
    }
    for (std::size_t index = 54U; index < 63U; ++index) {
        EXPECT_EQ(descriptors[index].category, "Rotation");
        EXPECT_EQ(descriptors[index].iconId, "node.math.rotation");
    }
    for (std::size_t index = 63U; index < 73U; ++index) {
        EXPECT_EQ(descriptors[index].category, "Logic");
        EXPECT_EQ(descriptors[index].iconId, "node.logic.boolean");
    }
    for (std::size_t index = 73U; index < 84U; ++index) {
        EXPECT_EQ(descriptors[index].category, "Transform");
        EXPECT_EQ(descriptors[index].iconId, "node.math.transform");
    }
    for (std::size_t index = 84U; index < 86U; ++index) {
        EXPECT_EQ(descriptors[index].category, "Entity Query");
        EXPECT_EQ(descriptors[index].iconId, "node.entity.query");
    }
    for (std::size_t index = 86U; index < 88U; ++index) {
        EXPECT_EQ(descriptors[index].category, "Engine");
        EXPECT_EQ(descriptors[index].iconId, "node.engine");
    }
    for (std::size_t index = 88U; index < 109U; ++index) {
        EXPECT_EQ(descriptors[index].category, "Variable");
        EXPECT_EQ(descriptors[index].iconId, "node.variable");
    }
    for (std::size_t index = 109U; index < 115U; ++index) {
        EXPECT_EQ(descriptors[index].category, "Entity");
        EXPECT_EQ(descriptors[index].iconId, "node.entity");
    }
    for (std::size_t index = 115U; index < 123U; ++index) {
        EXPECT_EQ(descriptors[index].category, "Input");
        EXPECT_EQ(descriptors[index].iconId, "node.input");
    }
    for (std::size_t index = 123U; index < 130U; ++index) {
        EXPECT_EQ(descriptors[index].category, "Camera");
        EXPECT_EQ(descriptors[index].iconId, "node.camera");
    }
    for (std::size_t index = 130U; index < 140U; ++index) {
        EXPECT_EQ(descriptors[index].category, "Animation");
        EXPECT_EQ(descriptors[index].iconId, "node.animation");
    }
    for (std::size_t index = 140U; index < 150U; ++index) {
        EXPECT_EQ(descriptors[index].category, "Motion Query");
        EXPECT_EQ(descriptors[index].iconId, "node.motion_query");
    }
    for (std::size_t index = 150U; index < 161U; ++index) {
        EXPECT_EQ(descriptors[index].category, "Physics");
        EXPECT_EQ(descriptors[index].iconId, "node.physics");
    }
    for (std::size_t index = 161U; index < 167U; ++index) {
        EXPECT_EQ(descriptors[index].category, "Audio");
        EXPECT_EQ(descriptors[index].iconId, "node.audio");
    }

    const ScriptNodeTypeDescriptorUVE* lerp = registry.FindNodeTypeUVE("math.float.lerp");
    ASSERT_NE(lerp, nullptr);
    ASSERT_EQ(lerp->pins.size(), 4U);
    EXPECT_EQ(lerp->pins[2].name, "Alpha");
    EXPECT_EQ(lerp->pins[2].type, ScriptValueTypeUVE::Number);
    const ScriptNodeTypeDescriptorUVE* remap = registry.FindNodeTypeUVE("math.float.remap");
    ASSERT_NE(remap, nullptr);
    ASSERT_EQ(remap->pins.size(), 6U);
    EXPECT_EQ(remap->pins[1].name, "FromMin");
    const ScriptNodeTypeDescriptorUVE* randomRange = registry.FindNodeTypeUVE("math.float.random_range");
    ASSERT_NE(randomRange, nullptr);
    ASSERT_EQ(randomRange->pins.size(), 4U);
    EXPECT_EQ(randomRange->pins[0].name, "Seed");

    const ScriptNodeTypeDescriptorUVE* sequence = registry.FindNodeTypeUVE("flow.sequence");
    ASSERT_NE(sequence, nullptr);
    ASSERT_EQ(sequence->pins.size(), 3U);
    EXPECT_EQ(sequence->pins[0].role, ScriptPinRoleUVE::Execution);
    EXPECT_EQ(sequence->pins[1].role, ScriptPinRoleUVE::Execution);
    EXPECT_EQ(sequence->pins[2].role, ScriptPinRoleUVE::Execution);
    EXPECT_EQ(sequence->pins[1].name, "Then");
    EXPECT_EQ(sequence->pins[2].name, "Then2");

    const ScriptNodeTypeDescriptorUVE* branch = registry.FindNodeTypeUVE("flow.branch");
    ASSERT_NE(branch, nullptr);
    ASSERT_EQ(branch->pins.size(), 4U);
    EXPECT_EQ(branch->pins[0].role, ScriptPinRoleUVE::Execution);
    EXPECT_EQ(branch->pins[1].type, ScriptValueTypeUVE::Boolean);
    EXPECT_EQ(branch->pins[2].role, ScriptPinRoleUVE::Execution);
    EXPECT_EQ(branch->pins[3].role, ScriptPinRoleUVE::Execution);

    const ScriptNodeTypeDescriptorUVE* event = registry.FindNodeTypeUVE("flow.event");
    ASSERT_NE(event, nullptr);
    ASSERT_EQ(event->pins.size(), 1U);
    EXPECT_EQ(event->pins[0].name, "Then");
    EXPECT_EQ(event->pins[0].role, ScriptPinRoleUVE::Execution);
    EXPECT_EQ(event->pins[0].direction, ScriptPinDirectionUVE::Output);

    const ScriptNodeTypeDescriptorUVE* loop = registry.FindNodeTypeUVE("flow.loop");
    ASSERT_NE(loop, nullptr);
    ASSERT_EQ(loop->pins.size(), 4U);
    EXPECT_EQ(loop->pins[1].name, "Count");
    EXPECT_EQ(loop->pins[1].type, ScriptValueTypeUVE::Number);
    EXPECT_EQ(loop->pins[2].name, "Body");
    EXPECT_EQ(loop->pins[3].name, "Completed");
    EXPECT_EQ(loop->pins[2].role, ScriptPinRoleUVE::Execution);
    EXPECT_EQ(loop->pins[3].role, ScriptPinRoleUVE::Execution);

    const ScriptNodeTypeDescriptorUVE* forLoop = registry.FindNodeTypeUVE("flow.for_loop");
    ASSERT_NE(forLoop, nullptr);
    ASSERT_EQ(forLoop->pins.size(), 5U);
    EXPECT_EQ(forLoop->pins[1].type, ScriptValueTypeUVE::Number);
    EXPECT_EQ(forLoop->pins[4].name, "Index");
    EXPECT_EQ(forLoop->pins[4].type, ScriptValueTypeUVE::Number);
    EXPECT_EQ(forLoop->pins[4].direction, ScriptPinDirectionUVE::Output);

    const ScriptNodeTypeDescriptorUVE* whileLoop = registry.FindNodeTypeUVE("flow.while_loop");
    ASSERT_NE(whileLoop, nullptr);
    ASSERT_EQ(whileLoop->pins.size(), 4U);
    EXPECT_EQ(whileLoop->pins[1].name, "Condition");
    EXPECT_EQ(whileLoop->pins[1].type, ScriptValueTypeUVE::Boolean);
    EXPECT_EQ(whileLoop->pins[2].role, ScriptPinRoleUVE::Execution);
    EXPECT_EQ(whileLoop->pins[3].role, ScriptPinRoleUVE::Execution);

    const ScriptNodeTypeDescriptorUVE* delay = registry.FindNodeTypeUVE("flow.delay");
    ASSERT_NE(delay, nullptr);
    ASSERT_EQ(delay->pins.size(), 3U);
    EXPECT_EQ(delay->pins[1].name, "Frames");
    EXPECT_EQ(delay->pins[1].type, ScriptValueTypeUVE::Number);
    EXPECT_EQ(delay->pins[2].name, "Then");
    EXPECT_EQ(delay->pins[2].role, ScriptPinRoleUVE::Execution);

    const auto assertCollectionDescriptors = [&](const char* makeId, const char* getId, const char* setId,
                                                   const ScriptValueTypeUVE type) {
        const ScriptNodeTypeDescriptorUVE* make = registry.FindNodeTypeUVE(makeId);
        const ScriptNodeTypeDescriptorUVE* get = registry.FindNodeTypeUVE(getId);
        const ScriptNodeTypeDescriptorUVE* set = registry.FindNodeTypeUVE(setId);
        ASSERT_NE(make, nullptr);
        ASSERT_NE(get, nullptr);
        ASSERT_NE(set, nullptr);
        ASSERT_EQ(make->pins.size(), 3U);
        ASSERT_EQ(get->pins.size(), 2U);
        ASSERT_EQ(set->pins.size(), 3U);
        EXPECT_EQ(make->pins[1].type, type);
        EXPECT_EQ(make->pins[2].type, type);
        EXPECT_EQ(get->pins[1].type, type);
        EXPECT_EQ(set->pins[1].type, type);
        EXPECT_EQ(set->pins[2].type, type);
        EXPECT_EQ(make->pins[0].type, ScriptValueTypeUVE::Number);
        EXPECT_EQ(get->pins[0].type, ScriptValueTypeUVE::Number);
        EXPECT_EQ(set->pins[0].type, ScriptValueTypeUVE::Number);
    };
    assertCollectionDescriptors("variable.make_array", "variable.get_array", "variable.set_array", ScriptValueTypeUVE::Array);
    assertCollectionDescriptors("variable.make_map", "variable.get_map", "variable.set_map", ScriptValueTypeUVE::Map);
    assertCollectionDescriptors("variable.make_set", "variable.get_set", "variable.set_set", ScriptValueTypeUVE::Set);
    assertCollectionDescriptors("variable.make_struct", "variable.get_struct", "variable.set_struct", ScriptValueTypeUVE::Struct);

    const ScriptNodeTypeDescriptorUVE* numberToBoolean = registry.FindNodeTypeUVE("convert.number_to_boolean");
    ASSERT_NE(numberToBoolean, nullptr);
    ASSERT_EQ(numberToBoolean->pins.size(), 2U);
    EXPECT_EQ(numberToBoolean->pins[0].type, ScriptValueTypeUVE::Number);
    EXPECT_EQ(numberToBoolean->pins[1].type, ScriptValueTypeUVE::Boolean);

    const ScriptNodeTypeDescriptorUVE* vector2ToVector3 = registry.FindNodeTypeUVE("convert.vector2_to_vector3");
    ASSERT_NE(vector2ToVector3, nullptr);
    ASSERT_EQ(vector2ToVector3->pins.size(), 3U);
    EXPECT_EQ(vector2ToVector3->pins[0].type, ScriptValueTypeUVE::Vector2);
    EXPECT_EQ(vector2ToVector3->pins[1].name, "Z");
    EXPECT_EQ(vector2ToVector3->pins[1].type, ScriptValueTypeUVE::Number);
    ASSERT_TRUE(vector2ToVector3->pins[1].defaultValue.has_value());
    EXPECT_EQ(*vector2ToVector3->pins[1].defaultValue, "0");
    EXPECT_EQ(vector2ToVector3->pins[2].type, ScriptValueTypeUVE::Vector3);

    const ScriptNodeTypeDescriptorUVE* abs = registry.FindNodeTypeUVE("math.float.abs");
    ASSERT_NE(abs, nullptr);
    ASSERT_EQ(abs->pins.size(), 2U);
    EXPECT_EQ(abs->pins[0].name, "Value");
    EXPECT_EQ(abs->pins[0].type, ScriptValueTypeUVE::Number);
    EXPECT_EQ(abs->pins[1].name, "Result");
    EXPECT_EQ(abs->pins[1].type, ScriptValueTypeUVE::Number);

    const ScriptNodeTypeDescriptorUVE* clamp = registry.FindNodeTypeUVE("math.float.clamp");
    ASSERT_NE(clamp, nullptr);
    ASSERT_EQ(clamp->pins.size(), 4U);
    EXPECT_EQ(clamp->pins[0].name, "Value");
    EXPECT_EQ(clamp->pins[1].name, "Min");
    EXPECT_EQ(clamp->pins[2].name, "Max");
    EXPECT_EQ(clamp->pins[3].name, "Result");
    for (const ScriptPinDescriptorUVE& pin : clamp->pins) {
        EXPECT_EQ(pin.type, ScriptValueTypeUVE::Number);
    }

    const ScriptNodeTypeDescriptorUVE* hasComponent = registry.FindNodeTypeUVE("query.entity.has_component");
    ASSERT_NE(hasComponent, nullptr);
    ASSERT_EQ(hasComponent->pins.size(), 3U);
    EXPECT_EQ(hasComponent->pins[0].type, ScriptValueTypeUVE::Entity);
    EXPECT_EQ(hasComponent->pins[1].type, ScriptValueTypeUVE::Component);
    EXPECT_EQ(hasComponent->pins[2].type, ScriptValueTypeUVE::Boolean);

    const ScriptNodeTypeDescriptorUVE* getComponent = registry.FindNodeTypeUVE("query.entity.get_component");
    ASSERT_NE(getComponent, nullptr);
    ASSERT_EQ(getComponent->pins.size(), 3U);
    EXPECT_EQ(getComponent->pins[0].type, ScriptValueTypeUVE::Entity);
    EXPECT_EQ(getComponent->pins[1].type, ScriptValueTypeUVE::Component);
    EXPECT_EQ(getComponent->pins[2].type, ScriptValueTypeUVE::Component);

    const ScriptNodeTypeDescriptorUVE* entitySpawn = registry.FindNodeTypeUVE("entity.spawn");
    ASSERT_NE(entitySpawn, nullptr);
    ASSERT_EQ(entitySpawn->pins.size(), 1U);
    EXPECT_EQ(entitySpawn->category, "Entity");
    EXPECT_EQ(entitySpawn->iconId, "node.entity");
    EXPECT_EQ(entitySpawn->pins[0].name, "Result");
    EXPECT_EQ(entitySpawn->pins[0].direction, ScriptPinDirectionUVE::Output);
    EXPECT_EQ(entitySpawn->pins[0].type, ScriptValueTypeUVE::Entity);

    const ScriptNodeTypeDescriptorUVE* entityDestroy = registry.FindNodeTypeUVE("entity.destroy");
    ASSERT_NE(entityDestroy, nullptr);
    ASSERT_EQ(entityDestroy->pins.size(), 2U);
    EXPECT_EQ(entityDestroy->pins[0].name, "Entity");
    EXPECT_EQ(entityDestroy->pins[0].direction, ScriptPinDirectionUVE::Input);
    EXPECT_EQ(entityDestroy->pins[0].type, ScriptValueTypeUVE::Entity);
    EXPECT_EQ(entityDestroy->pins[1].name, "Result");
    EXPECT_EQ(entityDestroy->pins[1].direction, ScriptPinDirectionUVE::Output);
    EXPECT_EQ(entityDestroy->pins[1].type, ScriptValueTypeUVE::Boolean);

    const ScriptNodeTypeDescriptorUVE* entityFind = registry.FindNodeTypeUVE("entity.find");
    ASSERT_NE(entityFind, nullptr);
    ASSERT_EQ(entityFind->pins.size(), 2U);
    EXPECT_EQ(entityFind->pins[0].name, "Component");
    EXPECT_EQ(entityFind->pins[0].type, ScriptValueTypeUVE::Component);
    EXPECT_EQ(entityFind->pins[1].name, "Result");
    EXPECT_EQ(entityFind->pins[1].type, ScriptValueTypeUVE::Entity);

    const ScriptNodeTypeDescriptorUVE* entityGet = registry.FindNodeTypeUVE("entity.get_entity");
    ASSERT_NE(entityGet, nullptr);
    ASSERT_EQ(entityGet->pins.size(), 2U);
    EXPECT_EQ(entityGet->pins[0].name, "Handle");
    EXPECT_EQ(entityGet->pins[0].type, ScriptValueTypeUVE::Number);
    EXPECT_EQ(entityGet->pins[1].name, "Result");
    EXPECT_EQ(entityGet->pins[1].type, ScriptValueTypeUVE::Entity);

    for (const char* typeId : {"entity.add_component", "entity.remove_component"}) {
        const ScriptNodeTypeDescriptorUVE* mutation = registry.FindNodeTypeUVE(typeId);
        ASSERT_NE(mutation, nullptr);
        ASSERT_EQ(mutation->pins.size(), 3U);
        EXPECT_EQ(mutation->category, "Entity");
        EXPECT_EQ(mutation->iconId, "node.entity");
        EXPECT_EQ(mutation->pins[0].name, "Entity");
        EXPECT_EQ(mutation->pins[0].type, ScriptValueTypeUVE::Entity);
        EXPECT_EQ(mutation->pins[1].name, "Component");
        EXPECT_EQ(mutation->pins[1].type, ScriptValueTypeUVE::Component);
        EXPECT_EQ(mutation->pins[2].name, "Result");
        EXPECT_EQ(mutation->pins[2].type, ScriptValueTypeUVE::Boolean);
    }

    const ScriptNodeTypeDescriptorUVE* keyPressed = registry.FindNodeTypeUVE("input.key_pressed");
    ASSERT_NE(keyPressed, nullptr);
    ASSERT_EQ(keyPressed->pins.size(), 2U);
    EXPECT_EQ(keyPressed->pins[0].name, "Key");
    EXPECT_EQ(keyPressed->pins[0].type, ScriptValueTypeUVE::Number);
    EXPECT_EQ(keyPressed->pins[1].name, "Result");
    EXPECT_EQ(keyPressed->pins[1].type, ScriptValueTypeUVE::Boolean);

    const ScriptNodeTypeDescriptorUVE* mousePosition = registry.FindNodeTypeUVE("input.mouse_position");
    ASSERT_NE(mousePosition, nullptr);
    ASSERT_EQ(mousePosition->pins.size(), 1U);
    EXPECT_EQ(mousePosition->pins[0].name, "Position");
    EXPECT_EQ(mousePosition->pins[0].type, ScriptValueTypeUVE::Vector2);

    const ScriptNodeTypeDescriptorUVE* gamepadButton = registry.FindNodeTypeUVE("input.gamepad_button");
    ASSERT_NE(gamepadButton, nullptr);
    ASSERT_EQ(gamepadButton->pins.size(), 3U);
    EXPECT_EQ(gamepadButton->pins[0].type, ScriptValueTypeUVE::Number);
    EXPECT_EQ(gamepadButton->pins[1].type, ScriptValueTypeUVE::Number);
    EXPECT_EQ(gamepadButton->pins[2].type, ScriptValueTypeUVE::Boolean);

    const ScriptNodeTypeDescriptorUVE* cameraGet = registry.FindNodeTypeUVE("camera.get_camera");
    ASSERT_NE(cameraGet, nullptr);
    ASSERT_EQ(cameraGet->pins.size(), 1U);
    EXPECT_EQ(cameraGet->pins[0].name, "Result");
    EXPECT_EQ(cameraGet->pins[0].type, ScriptValueTypeUVE::Entity);

    for (const char* typeId : {"camera.set_position", "camera.set_rotation", "camera.look_at", "camera.set_fov",
                               "camera.shake", "camera.set_active"}) {
        const ScriptNodeTypeDescriptorUVE* camera = registry.FindNodeTypeUVE(typeId);
        ASSERT_NE(camera, nullptr);
        EXPECT_EQ(camera->category, "Camera");
        EXPECT_EQ(camera->iconId, "node.camera");
        ASSERT_GE(camera->pins.size(), 3U);
        EXPECT_EQ(camera->pins.front().name, "Camera");
        EXPECT_EQ(camera->pins.front().type, ScriptValueTypeUVE::Entity);
        EXPECT_EQ(camera->pins.back().name, "Result");
        EXPECT_EQ(camera->pins.back().type, ScriptValueTypeUVE::Boolean);
    }

    const ScriptNodeTypeDescriptorUVE* animationPlay = registry.FindNodeTypeUVE("animation.play");
    ASSERT_NE(animationPlay, nullptr);
    ASSERT_EQ(animationPlay->pins.size(), 4U);
    EXPECT_EQ(animationPlay->pins[0].type, ScriptValueTypeUVE::Entity);
    EXPECT_EQ(animationPlay->pins[1].name, "Clip");
    EXPECT_EQ(animationPlay->pins[1].type, ScriptValueTypeUVE::Number);
    EXPECT_EQ(animationPlay->pins[3].type, ScriptValueTypeUVE::Boolean);
    for (const char* typeId : {"animation.stop", "animation.pause", "animation.blend", "animation.blend_space",
                               "animation.set_speed", "animation.set_weight", "animation.montage",
                               "animation.get_current_animation", "animation.is_playing"}) {
        const ScriptNodeTypeDescriptorUVE* animation = registry.FindNodeTypeUVE(typeId);
        ASSERT_NE(animation, nullptr);
        EXPECT_EQ(animation->category, "Animation");
        EXPECT_EQ(animation->iconId, "node.animation");
        EXPECT_EQ(animation->pins.front().type, ScriptValueTypeUVE::Entity);
    }
    const ScriptNodeTypeDescriptorUVE* motionBuild = registry.FindNodeTypeUVE("motion.query.build");
    ASSERT_NE(motionBuild, nullptr);
    ASSERT_EQ(motionBuild->pins.size(), 5U);
    EXPECT_EQ(motionBuild->pins[0].type, ScriptValueTypeUVE::Entity);
    EXPECT_EQ(motionBuild->pins[1].type, ScriptValueTypeUVE::Vector3);
    EXPECT_EQ(motionBuild->pins[2].type, ScriptValueTypeUVE::Vector3);
    EXPECT_EQ(motionBuild->pins[4].type, ScriptValueTypeUVE::Boolean);
    for (const char* typeId : {"motion.query.search", "motion.query.get_best_match", "motion.query.set_trajectory",
                               "motion.query.set_pose", "motion.query.set_velocity", "motion.query.set_facing",
                               "motion.query.set_yaw", "motion.query.transition", "motion.query.motion_warp"}) {
        const ScriptNodeTypeDescriptorUVE* motion = registry.FindNodeTypeUVE(typeId);
        ASSERT_NE(motion, nullptr);
        EXPECT_EQ(motion->category, "Motion Query");
        EXPECT_EQ(motion->iconId, "node.motion_query");
        EXPECT_EQ(motion->pins.front().type, ScriptValueTypeUVE::Entity);
    }

    const ScriptNodeTypeDescriptorUVE* physicsRaycast = registry.FindNodeTypeUVE("physics.raycast");
    ASSERT_NE(physicsRaycast, nullptr);
    ASSERT_EQ(physicsRaycast->pins.size(), 10U);
    EXPECT_EQ(physicsRaycast->pins[0].type, ScriptValueTypeUVE::Vector3);
    EXPECT_EQ(physicsRaycast->pins[4].type, ScriptValueTypeUVE::Entity);
    EXPECT_EQ(physicsRaycast->pins[5].type, ScriptValueTypeUVE::Boolean);
    EXPECT_EQ(physicsRaycast->pins[6].type, ScriptValueTypeUVE::Entity);
    EXPECT_EQ(physicsRaycast->pins[8].type, ScriptValueTypeUVE::Vector3);
    EXPECT_EQ(physicsRaycast->pins[9].type, ScriptValueTypeUVE::Number);
    const ScriptNodeTypeDescriptorUVE* physicsOverlap = registry.FindNodeTypeUVE("physics.overlap");
    ASSERT_NE(physicsOverlap, nullptr);
    ASSERT_EQ(physicsOverlap->pins.size(), 4U);
    EXPECT_EQ(physicsOverlap->pins[0].type, ScriptValueTypeUVE::Vector3);
    EXPECT_EQ(physicsOverlap->pins[1].type, ScriptValueTypeUVE::Vector3);
    EXPECT_EQ(physicsOverlap->pins[3].type, ScriptValueTypeUVE::Number);
    const ScriptNodeTypeDescriptorUVE* physicsApplyForce = registry.FindNodeTypeUVE("physics.apply_force");
    ASSERT_NE(physicsApplyForce, nullptr);
    ASSERT_EQ(physicsApplyForce->pins.size(), 3U);
    EXPECT_EQ(physicsApplyForce->pins[0].type, ScriptValueTypeUVE::Entity);
    EXPECT_EQ(physicsApplyForce->pins[1].type, ScriptValueTypeUVE::Vector3);
    EXPECT_EQ(physicsApplyForce->pins[2].type, ScriptValueTypeUVE::Boolean);
    const ScriptNodeTypeDescriptorUVE* physicsGetVelocity = registry.FindNodeTypeUVE("physics.get_velocity");
    ASSERT_NE(physicsGetVelocity, nullptr);
    ASSERT_EQ(physicsGetVelocity->pins.size(), 2U);
    EXPECT_EQ(physicsGetVelocity->pins[0].type, ScriptValueTypeUVE::Entity);
    EXPECT_EQ(physicsGetVelocity->pins[1].direction, ScriptPinDirectionUVE::Output);
    EXPECT_EQ(physicsGetVelocity->pins[1].type, ScriptValueTypeUVE::Vector3);

    const ScriptNodeTypeDescriptorUVE* engineLog = registry.FindNodeTypeUVE("engine.log");
    ASSERT_NE(engineLog, nullptr);
    ASSERT_EQ(engineLog->pins.size(), 1U);
    EXPECT_EQ(engineLog->category, "Engine");
    EXPECT_EQ(engineLog->iconId, "node.engine");
    EXPECT_EQ(engineLog->pins[0].name, "Value");
    EXPECT_EQ(engineLog->pins[0].type, ScriptValueTypeUVE::Number);

    const ScriptNodeTypeDescriptorUVE* engineGetTime = registry.FindNodeTypeUVE("engine.get_time");
    ASSERT_NE(engineGetTime, nullptr);
    ASSERT_EQ(engineGetTime->pins.size(), 1U);
    EXPECT_EQ(engineGetTime->pins[0].name, "Value");
    EXPECT_EQ(engineGetTime->pins[0].direction, ScriptPinDirectionUVE::Output);
    EXPECT_EQ(engineGetTime->pins[0].type, ScriptValueTypeUVE::Number);

    const ScriptNodeTypeDescriptorUVE* makeNumber = registry.FindNodeTypeUVE("variable.make_number");
    ASSERT_NE(makeNumber, nullptr);
    ASSERT_EQ(makeNumber->pins.size(), 3U);
    EXPECT_EQ(makeNumber->pins[0].name, "Slot");
    EXPECT_EQ(makeNumber->pins[0].type, ScriptValueTypeUVE::Number);
    EXPECT_EQ(makeNumber->pins[1].name, "Value");
    EXPECT_EQ(makeNumber->pins[2].name, "Result");
    EXPECT_EQ(makeNumber->pins[2].direction, ScriptPinDirectionUVE::Output);

    const ScriptNodeTypeDescriptorUVE* getBoolean = registry.FindNodeTypeUVE("variable.get_boolean");
    ASSERT_NE(getBoolean, nullptr);
    ASSERT_EQ(getBoolean->pins.size(), 2U);
    EXPECT_EQ(getBoolean->pins[0].type, ScriptValueTypeUVE::Number);
    EXPECT_EQ(getBoolean->pins[1].type, ScriptValueTypeUVE::Boolean);
    EXPECT_EQ(getBoolean->pins[1].direction, ScriptPinDirectionUVE::Output);

    const ScriptNodeTypeDescriptorUVE* setVector3 = registry.FindNodeTypeUVE("variable.set_vector3");
    ASSERT_NE(setVector3, nullptr);
    ASSERT_EQ(setVector3->pins.size(), 3U);
    EXPECT_EQ(setVector3->pins[0].type, ScriptValueTypeUVE::Number);
    EXPECT_EQ(setVector3->pins[1].type, ScriptValueTypeUVE::Vector3);
    EXPECT_EQ(setVector3->pins[2].type, ScriptValueTypeUVE::Vector3);
    EXPECT_EQ(setVector3->pins[2].direction, ScriptPinDirectionUVE::Output);

    const ScriptNodeTypeDescriptorUVE* makeVector2 = registry.FindNodeTypeUVE("math.vector2.make");
    ASSERT_NE(makeVector2, nullptr);
    ASSERT_EQ(makeVector2->pins.size(), 3U);
    EXPECT_EQ(makeVector2->pins[0].type, ScriptValueTypeUVE::Number);
    EXPECT_EQ(makeVector2->pins[1].type, ScriptValueTypeUVE::Number);
    EXPECT_EQ(makeVector2->pins[2].direction, ScriptPinDirectionUVE::Output);
    EXPECT_EQ(makeVector2->pins[2].type, ScriptValueTypeUVE::Vector2);

    const ScriptNodeTypeDescriptorUVE* multiplyVector2 = registry.FindNodeTypeUVE("math.vector2.multiply");
    ASSERT_NE(multiplyVector2, nullptr);
    ASSERT_EQ(multiplyVector2->pins.size(), 3U);
    EXPECT_EQ(multiplyVector2->pins[0].type, ScriptValueTypeUVE::Vector2);
    EXPECT_EQ(multiplyVector2->pins[1].type, ScriptValueTypeUVE::Number);
    EXPECT_EQ(multiplyVector2->pins[2].type, ScriptValueTypeUVE::Vector2);

    const ScriptNodeTypeDescriptorUVE* lengthVector2 = registry.FindNodeTypeUVE("math.vector2.length");
    ASSERT_NE(lengthVector2, nullptr);
    EXPECT_EQ(lengthVector2->pins[0].type, ScriptValueTypeUVE::Vector2);
    EXPECT_EQ(lengthVector2->pins[1].type, ScriptValueTypeUVE::Number);

    const ScriptNodeTypeDescriptorUVE* normalizeVector2 = registry.FindNodeTypeUVE("math.vector2.normalize");
    ASSERT_NE(normalizeVector2, nullptr);
    EXPECT_EQ(normalizeVector2->pins[0].type, ScriptValueTypeUVE::Vector2);
    EXPECT_EQ(normalizeVector2->pins[1].type, ScriptValueTypeUVE::Vector2);

    const ScriptNodeTypeDescriptorUVE* dotVector2 = registry.FindNodeTypeUVE("math.vector2.dot");
    ASSERT_NE(dotVector2, nullptr);
    ASSERT_EQ(dotVector2->pins.size(), 3U);
    EXPECT_EQ(dotVector2->pins[0].type, ScriptValueTypeUVE::Vector2);
    EXPECT_EQ(dotVector2->pins[1].type, ScriptValueTypeUVE::Vector2);
    EXPECT_EQ(dotVector2->pins[2].type, ScriptValueTypeUVE::Number);

    const ScriptNodeTypeDescriptorUVE* distanceVector2 = registry.FindNodeTypeUVE("math.vector2.distance");
    ASSERT_NE(distanceVector2, nullptr);
    EXPECT_EQ(distanceVector2->pins[2].name, "Distance");
    EXPECT_EQ(distanceVector2->pins[2].type, ScriptValueTypeUVE::Number);

    const ScriptNodeTypeDescriptorUVE* directionVector2 = registry.FindNodeTypeUVE("math.vector2.direction");
    ASSERT_NE(directionVector2, nullptr);
    EXPECT_EQ(directionVector2->pins[0].name, "From");
    EXPECT_EQ(directionVector2->pins[1].name, "To");
    EXPECT_EQ(directionVector2->pins[2].type, ScriptValueTypeUVE::Vector2);

    const ScriptNodeTypeDescriptorUVE* lerpVector2 = registry.FindNodeTypeUVE("math.vector2.lerp");
    ASSERT_NE(lerpVector2, nullptr);
    ASSERT_EQ(lerpVector2->pins.size(), 4U);
    EXPECT_EQ(lerpVector2->pins[2].name, "Alpha");
    EXPECT_EQ(lerpVector2->pins[2].type, ScriptValueTypeUVE::Number);

    const ScriptNodeTypeDescriptorUVE* make = registry.FindNodeTypeUVE("math.vector3.make");
    ASSERT_NE(make, nullptr);
    ASSERT_EQ(make->pins.size(), 4U);
    EXPECT_EQ(make->pins[0].type, ScriptValueTypeUVE::Number);
    EXPECT_EQ(make->pins[3].direction, ScriptPinDirectionUVE::Output);
    EXPECT_EQ(make->pins[3].type, ScriptValueTypeUVE::Vector3);

    const ScriptNodeTypeDescriptorUVE* dot = registry.FindNodeTypeUVE("math.vector3.dot");
    ASSERT_NE(dot, nullptr);
    ASSERT_EQ(dot->pins.size(), 3U);
    EXPECT_EQ(dot->pins[0].type, ScriptValueTypeUVE::Vector3);
    EXPECT_EQ(dot->pins[2].type, ScriptValueTypeUVE::Number);

    const ScriptNodeTypeDescriptorUVE* distanceVector3 = registry.FindNodeTypeUVE("math.vector3.distance");
    ASSERT_NE(distanceVector3, nullptr);
    EXPECT_EQ(distanceVector3->pins[2].name, "Distance");
    EXPECT_EQ(distanceVector3->pins[2].type, ScriptValueTypeUVE::Number);

    const ScriptNodeTypeDescriptorUVE* directionVector3 = registry.FindNodeTypeUVE("math.vector3.direction");
    ASSERT_NE(directionVector3, nullptr);
    EXPECT_EQ(directionVector3->pins[0].name, "From");
    EXPECT_EQ(directionVector3->pins[1].name, "To");
    EXPECT_EQ(directionVector3->pins[2].type, ScriptValueTypeUVE::Vector3);

    const ScriptNodeTypeDescriptorUVE* lerpVector3 = registry.FindNodeTypeUVE("math.vector3.lerp");
    ASSERT_NE(lerpVector3, nullptr);
    ASSERT_EQ(lerpVector3->pins.size(), 4U);
    EXPECT_EQ(lerpVector3->pins[2].name, "Alpha");
    EXPECT_EQ(lerpVector3->pins[2].type, ScriptValueTypeUVE::Number);

    const ScriptNodeTypeDescriptorUVE* multiply = registry.FindNodeTypeUVE("math.vector3.multiply");
    ASSERT_NE(multiply, nullptr);
    ASSERT_EQ(multiply->pins.size(), 3U);
    EXPECT_EQ(multiply->pins[0].type, ScriptValueTypeUVE::Vector3);
    EXPECT_EQ(multiply->pins[1].type, ScriptValueTypeUVE::Number);
    EXPECT_EQ(multiply->pins[2].type, ScriptValueTypeUVE::Vector3);

    const ScriptNodeTypeDescriptorUVE* makeRotation = registry.FindNodeTypeUVE("math.rotation.make");
    ASSERT_NE(makeRotation, nullptr);
    ASSERT_EQ(makeRotation->pins.size(), 3U);
    EXPECT_EQ(makeRotation->pins[0].type, ScriptValueTypeUVE::Vector3);
    EXPECT_EQ(makeRotation->pins[1].type, ScriptValueTypeUVE::Number);
    EXPECT_EQ(makeRotation->pins[2].type, ScriptValueTypeUVE::Rotation);
    const ScriptNodeTypeDescriptorUVE* slerpRotation = registry.FindNodeTypeUVE("math.rotation.slerp");
    ASSERT_NE(slerpRotation, nullptr);
    ASSERT_EQ(slerpRotation->pins.size(), 4U);
    EXPECT_EQ(slerpRotation->pins[0].type, ScriptValueTypeUVE::Rotation);
    EXPECT_EQ(slerpRotation->pins[1].type, ScriptValueTypeUVE::Rotation);
    EXPECT_EQ(slerpRotation->pins[3].type, ScriptValueTypeUVE::Rotation);
    const ScriptNodeTypeDescriptorUVE* rotateRotation = registry.FindNodeTypeUVE("math.rotation.rotate");
    ASSERT_NE(rotateRotation, nullptr);
    EXPECT_EQ(rotateRotation->pins[0].type, ScriptValueTypeUVE::Rotation);
    EXPECT_EQ(rotateRotation->pins[1].type, ScriptValueTypeUVE::Vector3);
    EXPECT_EQ(rotateRotation->pins[2].type, ScriptValueTypeUVE::Vector3);

    const ScriptNodeTypeDescriptorUVE* makeTransform = registry.FindNodeTypeUVE("math.transform.make");
    ASSERT_NE(makeTransform, nullptr);
    ASSERT_EQ(makeTransform->pins.size(), 4U);
    EXPECT_EQ(makeTransform->pins[0].type, ScriptValueTypeUVE::Vector3);
    EXPECT_EQ(makeTransform->pins[1].type, ScriptValueTypeUVE::Rotation);
    EXPECT_EQ(makeTransform->pins[2].type, ScriptValueTypeUVE::Vector3);
    EXPECT_EQ(makeTransform->pins[3].type, ScriptValueTypeUVE::Transform);
    const ScriptNodeTypeDescriptorUVE* breakTransform = registry.FindNodeTypeUVE("math.transform.break");
    ASSERT_NE(breakTransform, nullptr);
    ASSERT_EQ(breakTransform->pins.size(), 4U);
    EXPECT_EQ(breakTransform->pins[0].type, ScriptValueTypeUVE::Transform);
    EXPECT_EQ(breakTransform->pins[1].type, ScriptValueTypeUVE::Vector3);
    EXPECT_EQ(breakTransform->pins[2].type, ScriptValueTypeUVE::Rotation);
    EXPECT_EQ(breakTransform->pins[3].type, ScriptValueTypeUVE::Vector3);
    const ScriptNodeTypeDescriptorUVE* transformPoint = registry.FindNodeTypeUVE("math.transform.transform_point");
    ASSERT_NE(transformPoint, nullptr);
    ASSERT_EQ(transformPoint->pins.size(), 3U);
    EXPECT_EQ(transformPoint->pins[0].type, ScriptValueTypeUVE::Transform);
    EXPECT_EQ(transformPoint->pins[1].type, ScriptValueTypeUVE::Vector3);
    EXPECT_EQ(transformPoint->pins[2].type, ScriptValueTypeUVE::Vector3);
}

TEST(ScriptVectorMathUVETest, Vector2V2FunctionsAreFiniteAndDeterministic) {
    const ScriptVector2ValueUVE lhs{{3.0F, 4.0F}};
    const ScriptVector2ValueUVE rhs{{1.0F, 2.0F}};
    const ScriptVector2NumberResultUVE dot = EvaluateScriptVector2DotUVE(lhs, rhs);
    ASSERT_TRUE(dot.IsAppliedUVE());
    EXPECT_FLOAT_EQ(dot.value, 11.0F);

    const ScriptVector2NumberResultUVE distance = EvaluateScriptVector2DistanceUVE(
        ScriptVector2ValueUVE{{0.0F, 0.0F}}, lhs);
    ASSERT_TRUE(distance.IsAppliedUVE());
    EXPECT_FLOAT_EQ(distance.value, 5.0F);

    const ScriptVector2ValueResultUVE direction = EvaluateScriptVector2DirectionUVE(
        ScriptVector2ValueUVE{{1.0F, 2.0F}}, ScriptVector2ValueUVE{{4.0F, 6.0F}});
    ASSERT_TRUE(direction.IsAppliedUVE());
    EXPECT_EQ(direction.value, (ScriptVector2ValueUVE{{0.6F, 0.8F}}));

    const ScriptVector2ValueResultUVE lerp = EvaluateScriptVector2LerpUVE(
        ScriptVector2ValueUVE{{0.0F, 0.0F}}, ScriptVector2ValueUVE{{4.0F, 6.0F}}, 0.5F);
    ASSERT_TRUE(lerp.IsAppliedUVE());
    EXPECT_EQ(lerp.value, (ScriptVector2ValueUVE{{2.0F, 3.0F}}));

    EXPECT_EQ(EvaluateScriptVector2DirectionUVE(lhs, lhs).code,
              ScriptVector2EvaluationCodeUVE::ZeroLengthNormalize);
    EXPECT_EQ(EvaluateScriptVector2DotUVE(
                  ScriptVector2ValueUVE{{std::numeric_limits<float>::infinity(), 0.0F}}, rhs)
                  .code,
              ScriptVector2EvaluationCodeUVE::NonFiniteInput);
}

TEST(ScriptVectorMathUVETest, Vector3V2FunctionsReuseFiniteDirectionAndLerpPolicy) {
    const ScriptVector3ValueUVE lhs{{3.0F, 4.0F, 0.0F}};
    const ScriptVector3NumberResultUVE distance = EvaluateScriptVector3DistanceUVE(
        ScriptVector3ValueUVE{{0.0F, 0.0F, 0.0F}}, lhs);
    ASSERT_TRUE(distance.IsAppliedUVE());
    EXPECT_FLOAT_EQ(distance.value, 5.0F);

    const ScriptVector3ValueResultUVE direction = EvaluateScriptVector3DirectionUVE(
        ScriptVector3ValueUVE{{0.0F, 0.0F, 0.0F}}, ScriptVector3ValueUVE{{0.0F, 3.0F, 4.0F}});
    ASSERT_TRUE(direction.IsAppliedUVE());
    EXPECT_EQ(direction.value, (ScriptVector3ValueUVE{{0.0F, 0.6F, 0.8F}}));

    const ScriptVector3ValueResultUVE lerp = EvaluateScriptVector3LerpUVE(
        ScriptVector3ValueUVE{{0.0F, 0.0F, 0.0F}}, ScriptVector3ValueUVE{{4.0F, 6.0F, 8.0F}}, 0.5F);
    ASSERT_TRUE(lerp.IsAppliedUVE());
    EXPECT_EQ(lerp.value, (ScriptVector3ValueUVE{{2.0F, 3.0F, 4.0F}}));

    EXPECT_EQ(EvaluateScriptVector3DirectionUVE(lhs, lhs).code,
              ScriptVector3EvaluationCodeUVE::ZeroLengthNormalize);
}

TEST(ScriptGraphUVETest, ValidateUVE_EnforcesExecutionLinkCardinality) {
    ScriptNodeRegistryUVE registry;
    ASSERT_TRUE(RegisterBuiltInScriptNodesUVE(registry));
    ScriptGraphUVE graph;
    ASSERT_TRUE(graph.AddNodeUVE({1U, "flow.sequence"}));
    ASSERT_TRUE(graph.AddNodeUVE({2U, "flow.sequence"}));
    ASSERT_TRUE(graph.AddNodeUVE({3U, "flow.branch"}));
    ASSERT_TRUE(graph.AddLinkUVE(ScriptLinkUVE{{1U, "Then"}, {3U, "In"}}));
    ASSERT_TRUE(graph.AddLinkUVE(ScriptLinkUVE{{1U, "Then"}, {2U, "In"}}));
    ASSERT_TRUE(graph.AddLinkUVE(ScriptLinkUVE{{2U, "Then"}, {3U, "In"}}));

    const std::vector<ScriptValidationDiagnosticUVE> diagnostics = graph.ValidateUVE(registry);
    ASSERT_EQ(diagnostics.size(), 2U);
    EXPECT_EQ(diagnostics[0].code, ScriptValidationCodeUVE::ExecutionLinkCardinality);
    EXPECT_EQ(diagnostics[0].nodeId, 1U);
    EXPECT_EQ(diagnostics[1].code, ScriptValidationCodeUVE::ExecutionLinkCardinality);
    EXPECT_EQ(diagnostics[1].nodeId, 3U);
}

TEST(ScriptGraphUVETest, ValidateUVE_EnforcesDataInputCardinality) {
    ScriptNodeRegistryUVE registry;
    ASSERT_TRUE(RegisterBuiltInScriptNodesUVE(registry));
    ScriptGraphUVE graph;
    ASSERT_TRUE(graph.AddNodeUVE({10U, "engine.get_time"}));
    ASSERT_TRUE(graph.AddNodeUVE({11U, "engine.get_time"}));
    ASSERT_TRUE(graph.AddNodeUVE({20U, "math.float.add"}));
    ASSERT_TRUE(graph.AddLinkUVE(ScriptLinkUVE{{10U, "Value"}, {20U, "A"}}));
    ASSERT_TRUE(graph.AddLinkUVE(ScriptLinkUVE{{11U, "Value"}, {20U, "A"}}));

    const std::vector<ScriptValidationDiagnosticUVE> diagnostics = graph.ValidateUVE(registry);
    ASSERT_EQ(diagnostics.size(), 1U);
    EXPECT_EQ(diagnostics.front().code, ScriptValidationCodeUVE::DataLinkCardinality);
    EXPECT_EQ(diagnostics.front().nodeId, 20U);
    EXPECT_EQ(diagnostics.front().pinName, "A");
    ASSERT_TRUE(diagnostics.front().relatedEndpoint.has_value());
    EXPECT_EQ(diagnostics.front().relatedEndpoint->nodeId, 11U);
    EXPECT_EQ(diagnostics.front().relatedEndpoint->pinName, "Value");
}

TEST(ScriptGraphUVETest, ValidateUVE_AllowsDistinctDataInputPins) {
    ScriptNodeRegistryUVE registry;
    ASSERT_TRUE(RegisterBuiltInScriptNodesUVE(registry));
    ScriptGraphUVE graph;
    ASSERT_TRUE(graph.AddNodeUVE({10U, "engine.get_time"}));
    ASSERT_TRUE(graph.AddNodeUVE({20U, "math.float.add"}));
    ASSERT_TRUE(graph.AddLinkUVE(ScriptLinkUVE{{10U, "Value"}, {20U, "A"}}));
    ASSERT_TRUE(graph.AddLinkUVE(ScriptLinkUVE{{10U, "Value"}, {20U, "B"}}));

    EXPECT_TRUE(graph.ValidateUVE(registry).empty());
}

TEST(ScriptGraphUVETest, ValidateUVE_AllowsMaximumNodeCount) {
    ScriptNodeRegistryUVE registry;
    ASSERT_TRUE(RegisterBuiltInScriptNodesUVE(registry));
    ScriptGraphUVE graph;
    for (std::uint32_t nodeId = 1U; nodeId <= kMaximumScriptGraphNodesUVE; ++nodeId) {
        ASSERT_TRUE(graph.AddNodeUVE({nodeId, "engine.get_time"}));
    }

    EXPECT_TRUE(graph.ValidateUVE(registry).empty());
}

TEST(ScriptGraphUVETest, ValidateUVE_RejectsNodeCountAboveMaximum) {
    ScriptNodeRegistryUVE registry;
    ASSERT_TRUE(RegisterBuiltInScriptNodesUVE(registry));
    ScriptGraphUVE graph;
    for (std::uint32_t nodeId = 1U; nodeId <= kMaximumScriptGraphNodesUVE + 1U; ++nodeId) {
        ASSERT_TRUE(graph.AddNodeUVE({nodeId, "engine.get_time"}));
    }

    const std::vector<ScriptValidationDiagnosticUVE> diagnostics = graph.ValidateUVE(registry);
    ASSERT_EQ(diagnostics.size(), 1U);
    EXPECT_EQ(diagnostics.front().code, ScriptValidationCodeUVE::NodeCountExceeded);
    EXPECT_EQ(diagnostics.front().nodeId, 0U);
    EXPECT_TRUE(diagnostics.front().pinName.empty());
    EXPECT_EQ(diagnostics.front().sourceContext, "Graph");
    EXPECT_EQ(diagnostics.front().message, "Graph node count exceeds the maximum of 64.");
}

TEST(ScriptGraphUVETest, ValidateUVE_AllowsMaximumLinkCount) {
    ScriptNodeRegistryUVE registry;
    std::vector<ScriptPinDescriptorUVE> sourcePins;
    std::vector<ScriptPinDescriptorUVE> sinkPins;
    for (std::size_t index = 0U; index < kMaximumScriptGraphLinksUVE; ++index) {
        sourcePins.emplace_back("Out" + std::to_string(index), ScriptPinDirectionUVE::Output,
                                ScriptValueTypeUVE::Number);
        sinkPins.emplace_back("In" + std::to_string(index), ScriptPinDirectionUVE::Input,
                              ScriptValueTypeUVE::Number);
    }
    ASSERT_TRUE(registry.RegisterNodeTypeUVE({"test.many_outputs", "Many Outputs", std::move(sourcePins)}));
    ASSERT_TRUE(registry.RegisterNodeTypeUVE({"test.many_inputs", "Many Inputs", std::move(sinkPins)}));
    ScriptGraphUVE graph;
    ASSERT_TRUE(graph.AddNodeUVE({1U, "test.many_outputs"}));
    ASSERT_TRUE(graph.AddNodeUVE({2U, "test.many_inputs"}));
    for (std::size_t index = 0U; index < kMaximumScriptGraphLinksUVE; ++index) {
        ASSERT_TRUE(graph.AddLinkUVE({{1U, "Out" + std::to_string(index)},
                                      {2U, "In" + std::to_string(index)}}));
    }

    EXPECT_TRUE(graph.ValidateUVE(registry).empty());
}

TEST(ScriptGraphUVETest, ValidateUVE_RejectsLinkCountAboveMaximum) {
    ScriptNodeRegistryUVE registry;
    std::vector<ScriptPinDescriptorUVE> sourcePins;
    std::vector<ScriptPinDescriptorUVE> sinkPins;
    for (std::size_t index = 0U; index <= kMaximumScriptGraphLinksUVE; ++index) {
        sourcePins.emplace_back("Out" + std::to_string(index), ScriptPinDirectionUVE::Output,
                                ScriptValueTypeUVE::Number);
        sinkPins.emplace_back("In" + std::to_string(index), ScriptPinDirectionUVE::Input,
                              ScriptValueTypeUVE::Number);
    }
    ASSERT_TRUE(registry.RegisterNodeTypeUVE({"test.many_outputs", "Many Outputs", std::move(sourcePins)}));
    ASSERT_TRUE(registry.RegisterNodeTypeUVE({"test.many_inputs", "Many Inputs", std::move(sinkPins)}));
    ScriptGraphUVE graph;
    ASSERT_TRUE(graph.AddNodeUVE({1U, "test.many_outputs"}));
    ASSERT_TRUE(graph.AddNodeUVE({2U, "test.many_inputs"}));
    for (std::size_t index = 0U; index <= kMaximumScriptGraphLinksUVE; ++index) {
        ASSERT_TRUE(graph.AddLinkUVE({{1U, "Out" + std::to_string(index)},
                                      {2U, "In" + std::to_string(index)}}));
    }

    const std::vector<ScriptValidationDiagnosticUVE> diagnostics = graph.ValidateUVE(registry);
    ASSERT_EQ(diagnostics.size(), 1U);
    EXPECT_EQ(diagnostics.front().code, ScriptValidationCodeUVE::LinkCountExceeded);
    EXPECT_EQ(diagnostics.front().nodeId, 0U);
    EXPECT_TRUE(diagnostics.front().pinName.empty());
    EXPECT_EQ(diagnostics.front().sourceContext, "Graph");
    EXPECT_EQ(diagnostics.front().message, "Graph link count exceeds the maximum of 256.");
}

TEST(ScriptCompilerIRUVETest, CompileScriptGraphToIrUVE_PreservesEngineLogBindingNode) {
    ScriptNodeRegistryUVE registry;
    ASSERT_TRUE(RegisterBuiltInScriptNodesUVE(registry));
    ScriptGraphUVE graph;
    ASSERT_TRUE(graph.AddNodeUVE({70U, "engine.log"}));

    const ScriptIrCompileResultUVE result = CompileScriptGraphToIrUVE(graph, registry);
    ASSERT_TRUE(result.IsSuccessUVE());
    ASSERT_EQ(result.program->instructions.size(), 1U);
    EXPECT_EQ(result.program->instructions.front().kind, ScriptIrInstructionKindUVE::ExecuteNode);
    EXPECT_EQ(result.program->instructions.front().sourceNodeId, 70U);
    EXPECT_EQ(result.program->instructions.front().nodeTypeId, "engine.log");
}

TEST(ScriptCompilerIRUVETest, CompileScriptGraphToIrUVE_PreservesEngineGetTimeBindingNode) {
    ScriptNodeRegistryUVE registry;
    ASSERT_TRUE(RegisterBuiltInScriptNodesUVE(registry));
    ScriptGraphUVE graph;
    ASSERT_TRUE(graph.AddNodeUVE({72U, "engine.get_time"}));

    const ScriptIrCompileResultUVE result = CompileScriptGraphToIrUVE(graph, registry);
    ASSERT_TRUE(result.IsSuccessUVE());
    ASSERT_EQ(result.program->instructions.size(), 1U);
    EXPECT_EQ(result.program->instructions.front().kind, ScriptIrInstructionKindUVE::ExecuteNode);
    EXPECT_EQ(result.program->instructions.front().sourceNodeId, 72U);
    EXPECT_EQ(result.program->instructions.front().nodeTypeId, "engine.get_time");
}

TEST(ScriptCompilerIRUVETest, CompileScriptGraphToIrUVE_LowersFlowSequenceDirectDispatch) {
    ScriptNodeRegistryUVE registry;
    ASSERT_TRUE(RegisterBuiltInScriptNodesUVE(registry));
    ScriptGraphUVE graph;
    ASSERT_TRUE(graph.AddNodeUVE({1U, "flow.sequence"}));

    const ScriptIrCompileResultUVE result = CompileScriptGraphToIrUVE(graph, registry);
    ASSERT_TRUE(result.IsSuccessUVE());
    ASSERT_EQ(result.program->instructions.size(), 1U);
    EXPECT_EQ(result.program->instructions.front().kind, ScriptIrInstructionKindUVE::SequenceDispatch);
    EXPECT_EQ(result.program->instructions.front().firstTargetInstructionIndex, 1U);
    EXPECT_EQ(result.program->instructions.front().secondTargetInstructionIndex, 1U);
}

TEST(ScriptCompilerIRUVETest, CompileScriptGraphToIrUVE_LowersDirectSequenceExecutionLinks) {
    ScriptNodeRegistryUVE registry;
    ASSERT_TRUE(RegisterBuiltInScriptNodesUVE(registry));
    ASSERT_TRUE(registry.RegisterNodeTypeUVE(MakeSinkNodeUVE()));
    ScriptGraphUVE graph;
    ASSERT_TRUE(graph.AddNodeUVE({1U, "flow.sequence"}));
    ASSERT_TRUE(graph.AddNodeUVE({2U, "test.sink"}));
    ASSERT_TRUE(graph.AddNodeUVE({3U, "test.sink"}));
    ASSERT_TRUE(graph.AddLinkUVE(ScriptLinkUVE{{1U, "Then"}, {2U, "Exec"}}));
    ASSERT_TRUE(graph.AddLinkUVE(ScriptLinkUVE{{1U, "Then2"}, {3U, "Exec"}}));

    const ScriptIrCompileResultUVE result = CompileScriptGraphToIrUVE(graph, registry);
    ASSERT_TRUE(result.IsSuccessUVE());
    ASSERT_EQ(result.program->instructions.size(), 3U);
    EXPECT_EQ(result.program->instructions[0].kind, ScriptIrInstructionKindUVE::SequenceDispatch);
    EXPECT_EQ(result.program->instructions[0].firstTargetInstructionIndex, 1U);
    EXPECT_EQ(result.program->instructions[0].secondTargetInstructionIndex, 2U);
    EXPECT_EQ(result.program->instructions[1].kind, ScriptIrInstructionKindUVE::ExecuteNode);
    EXPECT_EQ(result.program->instructions[2].kind, ScriptIrInstructionKindUVE::ExecuteNode);
}

TEST(ScriptCompilerIRUVETest, CompileScriptGraphToIrUVE_LowersFlowBranchToConditionalJump) {
    ScriptNodeRegistryUVE registry;
    ASSERT_TRUE(RegisterBuiltInScriptNodesUVE(registry));
    ASSERT_TRUE(registry.RegisterNodeTypeUVE(MakeSinkNodeUVE()));
    ScriptGraphUVE graph;
    ASSERT_TRUE(graph.AddNodeUVE({1U, "flow.branch"}));
    ASSERT_TRUE(graph.AddNodeUVE({2U, "test.sink"}));
    ASSERT_TRUE(graph.AddNodeUVE({3U, "test.sink"}));
    ASSERT_TRUE(graph.AddLinkUVE(ScriptLinkUVE{{1U, "True"}, {2U, "Exec"}}));
    ASSERT_TRUE(graph.AddLinkUVE(ScriptLinkUVE{{1U, "False"}, {3U, "Exec"}}));

    const ScriptIrCompileResultUVE result = CompileScriptGraphToIrUVE(graph, registry);
    ASSERT_TRUE(result.IsSuccessUVE());
    ASSERT_EQ(result.program->instructions.size(), 3U);
    const ScriptIrInstructionUVE& branch = result.program->instructions.front();
    EXPECT_EQ(branch.kind, ScriptIrInstructionKindUVE::ConditionalJump);
    EXPECT_EQ(branch.sourceNodeId, 1U);
    EXPECT_EQ(branch.nodeTypeId, "flow.branch");
    EXPECT_EQ(branch.sourcePinName, "Condition");
    EXPECT_TRUE(branch.targetPinName.empty());
    EXPECT_EQ(branch.trueTargetInstructionIndex, 1U);
    EXPECT_EQ(branch.falseTargetInstructionIndex, 2U);
    EXPECT_EQ(result.program->instructions[1].kind, ScriptIrInstructionKindUVE::ExecuteNode);
    EXPECT_EQ(result.program->instructions[2].kind, ScriptIrInstructionKindUVE::ExecuteNode);
}

TEST(ScriptCompilerIRUVETest, CompileScriptGraphToIrUVE_LowersFlowControlDispatchNodes) {
    ScriptNodeRegistryUVE registry;
    ASSERT_TRUE(RegisterBuiltInScriptNodesUVE(registry));
    ScriptGraphUVE graph;
    ASSERT_TRUE(graph.AddNodeUVE({1U, "flow.return"}));
    ASSERT_TRUE(graph.AddNodeUVE({2U, "flow.do_once"}));
    ASSERT_TRUE(graph.AddNodeUVE({3U, "flow.gate"}));
    ASSERT_TRUE(graph.AddNodeUVE({4U, "flow.switch"}));

    const ScriptIrCompileResultUVE result = CompileScriptGraphToIrUVE(graph, registry);
    ASSERT_TRUE(result.IsSuccessUVE());
    ASSERT_EQ(result.program->version, 5U);
    ASSERT_EQ(result.program->instructions.size(), 7U);
    EXPECT_EQ(result.program->instructions[0].nodeTypeId, "flow.return");
    EXPECT_EQ(result.program->instructions[0].sourcePinName, "In");
    EXPECT_EQ(result.program->instructions[0].kind, ScriptIrInstructionKindUVE::FlowControlDispatch);
    EXPECT_EQ(result.program->instructions[1].nodeTypeId, "flow.do_once");
    EXPECT_EQ(result.program->instructions[1].sourcePinName, "In");
    EXPECT_EQ(result.program->instructions[2].sourcePinName, "Reset");
    EXPECT_EQ(result.program->instructions[3].nodeTypeId, "flow.gate");
    EXPECT_EQ(result.program->instructions[3].sourcePinName, "In");
    EXPECT_EQ(result.program->instructions[4].sourcePinName, "Open");
    EXPECT_EQ(result.program->instructions[5].sourcePinName, "Close");
    EXPECT_EQ(result.program->instructions[6].nodeTypeId, "flow.switch");
    EXPECT_EQ(result.program->instructions[6].sourcePinName, "In");
    EXPECT_EQ(result.program->instructions[6].kind, ScriptIrInstructionKindUVE::FlowControlDispatch);
}

TEST(ScriptCompilerIRUVETest, CompileScriptGraphToIrUVE_StagesExplicitConversionInput) {
    ScriptNodeRegistryUVE registry;
    ASSERT_TRUE(RegisterBuiltInScriptNodesUVE(registry));
    ScriptGraphUVE graph;
    ASSERT_TRUE(graph.AddNodeUVE({1U, "convert.vector2_to_vector3"}));
    ASSERT_TRUE(graph.AddNodeUVE({10U, "math.vector2.make"}));
    ASSERT_TRUE(graph.AddLinkUVE({{10U, "Vector"}, {1U, "Vector"}}));

    const ScriptIrCompileResultUVE result = CompileScriptGraphToIrUVE(graph, registry);
    ASSERT_TRUE(result.IsSuccessUVE());
    ASSERT_EQ(result.program->instructions.size(), 3U);
    EXPECT_EQ(result.program->instructions[0].nodeTypeId, "math.vector2.make");
    EXPECT_EQ(result.program->instructions[1].kind, ScriptIrInstructionKindUVE::TransferValue);
    EXPECT_EQ(result.program->instructions[1].sourcePinName, "Vector");
    EXPECT_EQ(result.program->instructions[1].targetNodeId, 1U);
    EXPECT_EQ(result.program->instructions[1].targetPinName, "Vector");
    EXPECT_EQ(result.program->instructions[2].nodeTypeId, "convert.vector2_to_vector3");

    ScriptGraphUVE invalidGraph;
    ASSERT_TRUE(invalidGraph.AddNodeUVE({1U, "convert.vector2_to_vector3"}));
    ASSERT_TRUE(invalidGraph.AddNodeUVE({10U, "math.vector3.make"}));
    ASSERT_TRUE(invalidGraph.AddLinkUVE({{10U, "Result"}, {1U, "Vector"}}));
    const ScriptIrCompileResultUVE invalidResult = CompileScriptGraphToIrUVE(invalidGraph, registry);
    EXPECT_FALSE(invalidResult.IsSuccessUVE());
}

TEST(ScriptCompilerIRUVETest, CompileScriptGraphToIrUVE_StagesBooleanConditionBeforeFlowBranch) {
    ScriptNodeRegistryUVE registry;
    ASSERT_TRUE(RegisterBuiltInScriptNodesUVE(registry));
    ASSERT_TRUE(registry.RegisterNodeTypeUVE(MakeSinkNodeUVE()));
    ScriptGraphUVE graph;
    ASSERT_TRUE(graph.AddNodeUVE({1U, "flow.branch"}));
    ASSERT_TRUE(graph.AddNodeUVE({2U, "logic.boolean.not"}));
    ASSERT_TRUE(graph.AddNodeUVE({3U, "test.sink"}));
    ASSERT_TRUE(graph.AddLinkUVE(ScriptLinkUVE{{2U, "Result"}, {1U, "Condition"}}));
    ASSERT_TRUE(graph.AddLinkUVE(ScriptLinkUVE{{1U, "True"}, {3U, "Exec"}}));

    const ScriptIrCompileResultUVE result = CompileScriptGraphToIrUVE(graph, registry);
    ASSERT_TRUE(result.IsSuccessUVE());
    ASSERT_EQ(result.program->instructions.size(), 4U);
    EXPECT_EQ(result.program->instructions[0].kind, ScriptIrInstructionKindUVE::ExecuteNode);
    EXPECT_EQ(result.program->instructions[0].sourceNodeId, 2U);
    EXPECT_EQ(result.program->instructions[1].kind, ScriptIrInstructionKindUVE::TransferValue);
    EXPECT_EQ(result.program->instructions[1].sourceNodeId, 2U);
    EXPECT_EQ(result.program->instructions[1].targetNodeId, 1U);
    EXPECT_EQ(result.program->instructions[1].sourcePinName, "Result");
    EXPECT_EQ(result.program->instructions[1].targetPinName, "Condition");
    EXPECT_EQ(result.program->instructions[2].kind, ScriptIrInstructionKindUVE::ConditionalJump);
    EXPECT_EQ(result.program->instructions[2].sourceNodeId, 1U);
    EXPECT_EQ(result.program->instructions[2].trueTargetInstructionIndex, 3U);
    EXPECT_EQ(result.program->instructions[2].falseTargetInstructionIndex, 4U);
    EXPECT_EQ(result.program->instructions[3].kind, ScriptIrInstructionKindUVE::ExecuteNode);
}

TEST(ScriptCompilerIRUVETest, CompileScriptGraphToIrUVE_StagesNumberComparisonConditionBeforeFlowBranch) {
    ScriptNodeRegistryUVE registry;
    ASSERT_TRUE(RegisterBuiltInScriptNodesUVE(registry));
    ASSERT_TRUE(registry.RegisterNodeTypeUVE(MakeSinkNodeUVE()));
    ScriptGraphUVE graph;
    ASSERT_TRUE(graph.AddNodeUVE({1U, "flow.branch"}));
    ASSERT_TRUE(graph.AddNodeUVE({2U, "logic.boolean.greater"}));
    ASSERT_TRUE(graph.AddNodeUVE({3U, "test.sink"}));
    ASSERT_TRUE(graph.AddLinkUVE(ScriptLinkUVE{{2U, "Result"}, {1U, "Condition"}}));
    ASSERT_TRUE(graph.AddLinkUVE(ScriptLinkUVE{{1U, "True"}, {3U, "Exec"}}));

    const ScriptIrCompileResultUVE result = CompileScriptGraphToIrUVE(graph, registry);
    ASSERT_TRUE(result.IsSuccessUVE());
    ASSERT_EQ(result.program->instructions.size(), 4U);
    EXPECT_EQ(result.program->instructions[0].sourceNodeId, 2U);
    EXPECT_EQ(result.program->instructions[0].kind, ScriptIrInstructionKindUVE::ExecuteNode);
    EXPECT_EQ(result.program->instructions[1].kind, ScriptIrInstructionKindUVE::TransferValue);
    EXPECT_TRUE(result.program->instructions[1].isStagedTransfer);
    EXPECT_EQ(result.program->instructions[1].sourceNodeId, 2U);
    EXPECT_EQ(result.program->instructions[1].targetNodeId, 1U);
    EXPECT_EQ(result.program->instructions[1].sourcePinName, "Result");
    EXPECT_EQ(result.program->instructions[1].targetPinName, "Condition");
    EXPECT_EQ(result.program->instructions[2].kind, ScriptIrInstructionKindUVE::ConditionalJump);
    EXPECT_EQ(result.program->instructions[2].trueTargetInstructionIndex, 3U);
    EXPECT_EQ(result.program->instructions[2].falseTargetInstructionIndex, 4U);
    EXPECT_EQ(result.program->instructions[3].sourceNodeId, 3U);
}

TEST(ScriptVmUVETest, ExecuteScriptBytecodeUVE_RunsNumberComparisonConditionBranch) {
    ScriptNodeRegistryUVE registry;
    ASSERT_TRUE(RegisterBuiltInScriptNodesUVE(registry));
    ASSERT_TRUE(registry.RegisterNodeTypeUVE(MakeSinkNodeUVE()));
    ScriptGraphUVE graph;
    ASSERT_TRUE(graph.AddNodeUVE({1U, "flow.branch"}));
    ASSERT_TRUE(graph.AddNodeUVE({2U, "logic.boolean.greater"}));
    ASSERT_TRUE(graph.AddNodeUVE({3U, "test.sink"}));
    ASSERT_TRUE(graph.AddLinkUVE(ScriptLinkUVE{{2U, "Result"}, {1U, "Condition"}}));
    ASSERT_TRUE(graph.AddLinkUVE(ScriptLinkUVE{{1U, "True"}, {3U, "Exec"}}));

    const ScriptIrCompileResultUVE compiled = CompileScriptGraphToIrUVE(graph, registry);
    ASSERT_TRUE(compiled.IsSuccessUVE());
    std::vector<ScriptBytecodeDiagnosticUVE> diagnostics;
    const std::optional<ScriptBytecodeProgramUVE> bytecode =
        LowerIrToBytecodeUVE(*compiled.program, diagnostics);
    ASSERT_TRUE(bytecode.has_value());
    ASSERT_TRUE(diagnostics.empty());

    ScriptVmExecutionContextUVE trueContext;
    ASSERT_TRUE(trueContext.SetInputUVE(2U, "A", 3.0F));
    ASSERT_TRUE(trueContext.SetInputUVE(2U, "B", 2.0F));
    const ScriptVmExecutionResultUVE trueResult = ExecuteScriptBytecodeUVE(*bytecode, trueContext);
    EXPECT_TRUE(trueResult.IsSuccessUVE());
    EXPECT_EQ(trueResult.instructionsExecuted, 4U);
    EXPECT_TRUE(std::get<bool>(*trueContext.FindOutputUVE(2U, "Result")));

    ScriptVmExecutionContextUVE falseContext;
    ASSERT_TRUE(falseContext.SetInputUVE(2U, "A", 2.0F));
    ASSERT_TRUE(falseContext.SetInputUVE(2U, "B", 3.0F));
    const ScriptVmExecutionResultUVE falseResult = ExecuteScriptBytecodeUVE(*bytecode, falseContext);
    EXPECT_TRUE(falseResult.IsSuccessUVE());
    EXPECT_EQ(falseResult.instructionsExecuted, 3U);
    EXPECT_FALSE(std::get<bool>(*falseContext.FindOutputUVE(2U, "Result")));
}

TEST(ScriptCompilerIRUVETest, CompileScriptGraphToIrUVE_StagesEntityQueryConditionBeforeFlowBranch) {
    ScriptNodeRegistryUVE registry;
    ASSERT_TRUE(RegisterBuiltInScriptNodesUVE(registry));
    ASSERT_TRUE(registry.RegisterNodeTypeUVE(MakeSinkNodeUVE()));
    ScriptGraphUVE graph;
    ASSERT_TRUE(graph.AddNodeUVE({1U, "flow.branch"}));
    ASSERT_TRUE(graph.AddNodeUVE({2U, "query.entity.has_component"}));
    ASSERT_TRUE(graph.AddNodeUVE({3U, "test.sink"}));
    ASSERT_TRUE(graph.AddLinkUVE(ScriptLinkUVE{{2U, "Result"}, {1U, "Condition"}}));
    ASSERT_TRUE(graph.AddLinkUVE(ScriptLinkUVE{{1U, "True"}, {3U, "Exec"}}));

    const ScriptIrCompileResultUVE result = CompileScriptGraphToIrUVE(graph, registry);
    ASSERT_TRUE(result.IsSuccessUVE());
    ASSERT_EQ(result.program->instructions.size(), 4U);
    EXPECT_EQ(result.program->instructions[0].sourceNodeId, 2U);
    EXPECT_EQ(result.program->instructions[0].kind, ScriptIrInstructionKindUVE::ExecuteNode);
    EXPECT_EQ(result.program->instructions[1].kind, ScriptIrInstructionKindUVE::TransferValue);
    EXPECT_EQ(result.program->instructions[1].sourceNodeId, 2U);
    EXPECT_EQ(result.program->instructions[1].targetNodeId, 1U);
    EXPECT_EQ(result.program->instructions[2].kind, ScriptIrInstructionKindUVE::ConditionalJump);
    EXPECT_EQ(result.program->instructions[2].sourceNodeId, 1U);
    EXPECT_EQ(result.program->instructions[3].sourceNodeId, 3U);
}

TEST(ScriptVmUVETest, ExecuteScriptBytecodeUVE_RunsEntityQueryConditionBranch) {
    ScriptNodeRegistryUVE registry;
    ASSERT_TRUE(RegisterBuiltInScriptNodesUVE(registry));
    ASSERT_TRUE(registry.RegisterNodeTypeUVE(MakeSinkNodeUVE()));
    ScriptGraphUVE graph;
    ASSERT_TRUE(graph.AddNodeUVE({1U, "flow.branch"}));
    ASSERT_TRUE(graph.AddNodeUVE({2U, "query.entity.has_component"}));
    ASSERT_TRUE(graph.AddNodeUVE({3U, "test.sink"}));
    ASSERT_TRUE(graph.AddLinkUVE(ScriptLinkUVE{{2U, "Result"}, {1U, "Condition"}}));
    ASSERT_TRUE(graph.AddLinkUVE(ScriptLinkUVE{{1U, "True"}, {3U, "Exec"}}));

    const ScriptIrCompileResultUVE compiled = CompileScriptGraphToIrUVE(graph, registry);
    ASSERT_TRUE(compiled.IsSuccessUVE());
    std::vector<ScriptBytecodeDiagnosticUVE> diagnostics;
    const std::optional<ScriptBytecodeProgramUVE> bytecode =
        LowerIrToBytecodeUVE(*compiled.program, diagnostics);
    ASSERT_TRUE(bytecode.has_value());
    ASSERT_TRUE(diagnostics.empty());

    const Scene::EntityUVE entity{42U, 3U};
    ScriptVmExecutionContextUVE context;
    ASSERT_TRUE(context.SetInputUVE(2U, "Entity", ScriptEntityValueUVE{entity}));
    ASSERT_TRUE(context.SetInputUVE(
        2U, "Component", ScriptComponentValueUVE{Scene::kInvalidEntityUVE, "MeshComponentUVE", false}));
    ASSERT_TRUE(context.SetComponentFactUVE(entity, "MeshComponentUVE", true));
    const ScriptVmExecutionResultUVE result = ExecuteScriptBytecodeUVE(*bytecode, context);

    EXPECT_TRUE(result.IsSuccessUVE());
    EXPECT_EQ(result.instructionsExecuted, 4U);
    EXPECT_TRUE(std::get<bool>(*context.FindOutputUVE(2U, "Result")));
}

TEST(ScriptCompilerIRUVETest, CompileScriptGraphToIrUVE_RejectsFlowBranchTooDeepConditionDependency) {
    ScriptNodeRegistryUVE registry;
    ASSERT_TRUE(RegisterBuiltInScriptNodesUVE(registry));
    ScriptGraphUVE graph;
    ASSERT_TRUE(graph.AddNodeUVE({1U, "flow.branch"}));
    ASSERT_TRUE(graph.AddNodeUVE({2U, "logic.boolean.not"}));
    ASSERT_TRUE(graph.AddNodeUVE({3U, "logic.boolean.not"}));
    ASSERT_TRUE(graph.AddNodeUVE({4U, "logic.boolean.not"}));
    ASSERT_TRUE(graph.AddLinkUVE(ScriptLinkUVE{{4U, "Result"}, {3U, "Value"}}));
    ASSERT_TRUE(graph.AddLinkUVE(ScriptLinkUVE{{3U, "Result"}, {2U, "Value"}}));
    ASSERT_TRUE(graph.AddLinkUVE(ScriptLinkUVE{{2U, "Result"}, {1U, "Condition"}}));

    const ScriptIrCompileResultUVE result = CompileScriptGraphToIrUVE(graph, registry);
    EXPECT_FALSE(result.IsSuccessUVE());
    ASSERT_EQ(result.diagnostics.size(), 1U);
    EXPECT_EQ(result.diagnostics.front().code, ScriptValidationCodeUVE::UnsupportedRuntimeNode);
    EXPECT_EQ(result.diagnostics.front().nodeId, 3U);
    EXPECT_EQ(result.diagnostics.front().pinName, "Value");
}

TEST(ScriptCompilerIRUVETest, CompileScriptGraphToIrUVE_RejectsFlowBranchConditionCycle) {
    ScriptNodeRegistryUVE registry;
    ASSERT_TRUE(RegisterBuiltInScriptNodesUVE(registry));
    ScriptGraphUVE graph;
    ASSERT_TRUE(graph.AddNodeUVE({1U, "flow.branch"}));
    ASSERT_TRUE(graph.AddNodeUVE({2U, "logic.boolean.not"}));
    ASSERT_TRUE(graph.AddNodeUVE({3U, "logic.boolean.not"}));
    ASSERT_TRUE(graph.AddLinkUVE(ScriptLinkUVE{{3U, "Result"}, {2U, "Value"}}));
    ASSERT_TRUE(graph.AddLinkUVE(ScriptLinkUVE{{2U, "Result"}, {3U, "Value"}}));
    ASSERT_TRUE(graph.AddLinkUVE(ScriptLinkUVE{{2U, "Result"}, {1U, "Condition"}}));

    const ScriptIrCompileResultUVE result = CompileScriptGraphToIrUVE(graph, registry);
    EXPECT_FALSE(result.IsSuccessUVE());
    ASSERT_EQ(result.diagnostics.size(), 1U);
    EXPECT_EQ(result.diagnostics.front().code, ScriptValidationCodeUVE::UnsupportedRuntimeNode);
    EXPECT_EQ(result.diagnostics.front().nodeId, 3U);
    EXPECT_EQ(result.diagnostics.front().pinName, "Value");
}

TEST(ScriptCompilerIRUVETest, CompileScriptGraphToIrUVE_RejectsNonBuiltinConditionProducer) {
    ScriptNodeRegistryUVE registry;
    ASSERT_TRUE(RegisterBuiltInScriptNodesUVE(registry));
    ASSERT_TRUE(registry.RegisterNodeTypeUVE(MakeBooleanSourceNodeUVE()));
    ScriptGraphUVE graph;
    ASSERT_TRUE(graph.AddNodeUVE({1U, "flow.branch"}));
    ASSERT_TRUE(graph.AddNodeUVE({2U, "test.boolean-source"}));
    ASSERT_TRUE(graph.AddLinkUVE(ScriptLinkUVE{{2U, "Out"}, {1U, "Condition"}}));

    const ScriptIrCompileResultUVE result = CompileScriptGraphToIrUVE(graph, registry);
    EXPECT_FALSE(result.IsSuccessUVE());
    ASSERT_EQ(result.diagnostics.size(), 1U);
    EXPECT_EQ(result.diagnostics.front().code, ScriptValidationCodeUVE::UnsupportedRuntimeNode);
    EXPECT_EQ(result.diagnostics.front().nodeId, 1U);
    EXPECT_EQ(result.diagnostics.front().pinName, "Condition");
}

TEST(ScriptVmUVETest, ExecuteScriptBytecodeUVE_RunsCompiledStagedBooleanConditionDependency) {
    ScriptNodeRegistryUVE registry;
    ASSERT_TRUE(RegisterBuiltInScriptNodesUVE(registry));
    ASSERT_TRUE(registry.RegisterNodeTypeUVE(MakeSinkNodeUVE()));
    ScriptGraphUVE graph;
    ASSERT_TRUE(graph.AddNodeUVE({1U, "flow.branch"}));
    ASSERT_TRUE(graph.AddNodeUVE({2U, "logic.boolean.not"}));
    ASSERT_TRUE(graph.AddNodeUVE({3U, "logic.boolean.not"}));
    ASSERT_TRUE(graph.AddNodeUVE({4U, "test.sink"}));
    ASSERT_TRUE(graph.AddLinkUVE(ScriptLinkUVE{{3U, "Result"}, {2U, "Value"}}));
    ASSERT_TRUE(graph.AddLinkUVE(ScriptLinkUVE{{2U, "Result"}, {1U, "Condition"}}));
    ASSERT_TRUE(graph.AddLinkUVE(ScriptLinkUVE{{1U, "True"}, {4U, "Exec"}}));

    const ScriptIrCompileResultUVE compiled = CompileScriptGraphToIrUVE(graph, registry);
    ASSERT_TRUE(compiled.IsSuccessUVE());
    std::vector<ScriptBytecodeDiagnosticUVE> diagnostics;
    const std::optional<ScriptBytecodeProgramUVE> bytecode =
        LowerIrToBytecodeUVE(*compiled.program, diagnostics);
    ASSERT_TRUE(bytecode.has_value());
    ASSERT_TRUE(diagnostics.empty());
    ASSERT_EQ(bytecode->instructions.size(), 6U);

    ScriptVmExecutionContextUVE context;
    ASSERT_TRUE(context.SetInputUVE(3U, "Value", true));
    const ScriptVmExecutionResultUVE result = ExecuteScriptBytecodeUVE(*bytecode, context);
    EXPECT_TRUE(result.IsSuccessUVE());
    EXPECT_EQ(result.instructionsExecuted, 6U);
    EXPECT_FALSE(std::get<bool>(*context.FindOutputUVE(3U, "Result")));
    EXPECT_TRUE(std::get<bool>(*context.FindOutputUVE(2U, "Result")));
}

TEST(ScriptCompilerIRUVETest, CompileScriptGraphToIrUVE_StagesBooleanProducerBeforeConsumer) {
    ScriptNodeRegistryUVE registry;
    ASSERT_TRUE(RegisterBuiltInScriptNodesUVE(registry));
    ScriptGraphUVE graph;
    ASSERT_TRUE(graph.AddNodeUVE({30U, "logic.boolean.not"}));
    ASSERT_TRUE(graph.AddNodeUVE({20U, "logic.boolean.and"}));
    ASSERT_TRUE(graph.AddLinkUVE(ScriptLinkUVE{{30U, "Result"}, {20U, "A"}}));

    const ScriptIrCompileResultUVE result = CompileScriptGraphToIrUVE(graph, registry);
    ASSERT_TRUE(result.IsSuccessUVE());
    ASSERT_EQ(result.program->instructions.size(), 3U);
    EXPECT_EQ(result.program->instructions[0].nodeTypeId, "logic.boolean.not");
    EXPECT_EQ(result.program->instructions[1].kind, ScriptIrInstructionKindUVE::TransferValue);
    EXPECT_EQ(result.program->instructions[1].sourceNodeId, 30U);
    EXPECT_EQ(result.program->instructions[1].targetNodeId, 20U);
    EXPECT_EQ(result.program->instructions[1].sourcePinName, "Result");
    EXPECT_EQ(result.program->instructions[1].targetPinName, "A");
    EXPECT_EQ(result.program->instructions[2].nodeTypeId, "logic.boolean.and");
}

TEST(ScriptCompilerIRUVETest, CompileScriptGraphToIrUVE_StagesEntityQueryBooleanProducerBeforeConsumer) {
    ScriptNodeRegistryUVE registry;
    ASSERT_TRUE(RegisterBuiltInScriptNodesUVE(registry));
    ScriptGraphUVE graph;
    ASSERT_TRUE(graph.AddNodeUVE({30U, "query.entity.has_component"}));
    ASSERT_TRUE(graph.AddNodeUVE({20U, "logic.boolean.and"}));
    ASSERT_TRUE(graph.AddLinkUVE(ScriptLinkUVE{{30U, "Result"}, {20U, "A"}}));

    const ScriptIrCompileResultUVE result = CompileScriptGraphToIrUVE(graph, registry);
    ASSERT_TRUE(result.IsSuccessUVE());
    ASSERT_EQ(result.program->instructions.size(), 3U);
    EXPECT_EQ(result.program->instructions[0].nodeTypeId, "query.entity.has_component");
    EXPECT_EQ(result.program->instructions[1].kind, ScriptIrInstructionKindUVE::TransferValue);
    EXPECT_EQ(result.program->instructions[1].sourceNodeId, 30U);
    EXPECT_EQ(result.program->instructions[1].targetNodeId, 20U);
    EXPECT_EQ(result.program->instructions[2].nodeTypeId, "logic.boolean.and");
}

TEST(ScriptCompilerIRUVETest, CompileScriptGraphToIrUVE_RejectsSecondEntityQueryBooleanConsumer) {
    ScriptNodeRegistryUVE registry;
    ASSERT_TRUE(RegisterBuiltInScriptNodesUVE(registry));
    ScriptGraphUVE graph;
    ASSERT_TRUE(graph.AddNodeUVE({10U, "query.entity.has_component"}));
    ASSERT_TRUE(graph.AddNodeUVE({20U, "logic.boolean.and"}));
    ASSERT_TRUE(graph.AddNodeUVE({30U, "logic.boolean.or"}));
    ASSERT_TRUE(graph.AddLinkUVE(ScriptLinkUVE{{10U, "Result"}, {20U, "A"}}));
    ASSERT_TRUE(graph.AddLinkUVE(ScriptLinkUVE{{10U, "Result"}, {30U, "B"}}));

    const ScriptIrCompileResultUVE result = CompileScriptGraphToIrUVE(graph, registry);
    EXPECT_FALSE(result.IsSuccessUVE());
    ASSERT_EQ(result.diagnostics.size(), 1U);
    EXPECT_EQ(result.diagnostics.front().code, ScriptValidationCodeUVE::UnsupportedRuntimeNode);
    EXPECT_EQ(result.diagnostics.front().nodeId, 30U);
    EXPECT_EQ(result.diagnostics.front().pinName, "B");
}

TEST(ScriptVmUVETest, ExecuteScriptBytecodeUVE_RunsStagedEntityQueryBooleanChain) {
    ScriptNodeRegistryUVE registry;
    ASSERT_TRUE(RegisterBuiltInScriptNodesUVE(registry));
    ScriptGraphUVE graph;
    ASSERT_TRUE(graph.AddNodeUVE({30U, "query.entity.has_component"}));
    ASSERT_TRUE(graph.AddNodeUVE({20U, "logic.boolean.and"}));
    ASSERT_TRUE(graph.AddLinkUVE(ScriptLinkUVE{{30U, "Result"}, {20U, "A"}}));
    const ScriptIrCompileResultUVE compiled = CompileScriptGraphToIrUVE(graph, registry);
    ASSERT_TRUE(compiled.IsSuccessUVE());
    std::vector<ScriptBytecodeDiagnosticUVE> diagnostics;
    const std::optional<ScriptBytecodeProgramUVE> bytecode =
        LowerIrToBytecodeUVE(*compiled.program, diagnostics);
    ASSERT_TRUE(bytecode.has_value());
    ASSERT_TRUE(diagnostics.empty());

    const Scene::EntityUVE entity{42U, 3U};
    ScriptVmExecutionContextUVE context;
    ASSERT_TRUE(context.SetInputUVE(30U, "Entity", ScriptEntityValueUVE{entity}));
    ASSERT_TRUE(context.SetInputUVE(
        30U, "Component", ScriptComponentValueUVE{Scene::kInvalidEntityUVE, "MeshComponentUVE", false}));
    ASSERT_TRUE(context.SetComponentFactUVE(entity, "MeshComponentUVE", true));
    ASSERT_TRUE(context.SetInputUVE(20U, "B", true));
    const ScriptVmExecutionResultUVE result = ExecuteScriptBytecodeUVE(*bytecode, context);

    EXPECT_TRUE(result.IsSuccessUVE());
    EXPECT_EQ(result.instructionsExecuted, 3U);
    EXPECT_TRUE(std::get<bool>(*context.FindOutputUVE(30U, "Result")));
    EXPECT_TRUE(std::get<bool>(*context.FindOutputUVE(20U, "Result")));
}

TEST(ScriptCompilerIRUVETest, CompileScriptGraphToIrUVE_StagesQueryComponentTokenBeforeHasComponent) {
    ScriptNodeRegistryUVE registry;
    ASSERT_TRUE(RegisterBuiltInScriptNodesUVE(registry));
    ScriptGraphUVE graph;
    ASSERT_TRUE(graph.AddNodeUVE({30U, "query.entity.get_component"}));
    ASSERT_TRUE(graph.AddNodeUVE({20U, "query.entity.has_component"}));
    ASSERT_TRUE(graph.AddLinkUVE(ScriptLinkUVE{{30U, "Result"}, {20U, "Component"}}));

    const ScriptIrCompileResultUVE result = CompileScriptGraphToIrUVE(graph, registry);
    ASSERT_TRUE(result.IsSuccessUVE());
    ASSERT_EQ(result.program->instructions.size(), 3U);
    EXPECT_EQ(result.program->instructions[0].nodeTypeId, "query.entity.get_component");
    EXPECT_EQ(result.program->instructions[1].kind, ScriptIrInstructionKindUVE::TransferValue);
    EXPECT_EQ(result.program->instructions[1].sourceNodeId, 30U);
    EXPECT_EQ(result.program->instructions[1].targetNodeId, 20U);
    EXPECT_EQ(result.program->instructions[1].sourcePinName, "Result");
    EXPECT_EQ(result.program->instructions[1].targetPinName, "Component");
    EXPECT_TRUE(result.program->instructions[1].isStagedTransfer);
    EXPECT_EQ(result.program->instructions[2].nodeTypeId, "query.entity.has_component");
}

TEST(ScriptCompilerIRUVETest, CompileScriptGraphToIrUVE_RejectsSecondQueryComponentConsumer) {
    ScriptNodeRegistryUVE registry;
    ASSERT_TRUE(RegisterBuiltInScriptNodesUVE(registry));
    ScriptGraphUVE graph;
    ASSERT_TRUE(graph.AddNodeUVE({10U, "query.entity.get_component"}));
    ASSERT_TRUE(graph.AddNodeUVE({20U, "query.entity.has_component"}));
    ASSERT_TRUE(graph.AddNodeUVE({30U, "query.entity.has_component"}));
    ASSERT_TRUE(graph.AddLinkUVE(ScriptLinkUVE{{10U, "Result"}, {20U, "Component"}}));
    ASSERT_TRUE(graph.AddLinkUVE(ScriptLinkUVE{{10U, "Result"}, {30U, "Component"}}));

    const ScriptIrCompileResultUVE result = CompileScriptGraphToIrUVE(graph, registry);
    EXPECT_FALSE(result.IsSuccessUVE());
    ASSERT_EQ(result.diagnostics.size(), 1U);
    EXPECT_EQ(result.diagnostics.front().code, ScriptValidationCodeUVE::UnsupportedRuntimeNode);
    EXPECT_EQ(result.diagnostics.front().nodeId, 30U);
    EXPECT_EQ(result.diagnostics.front().pinName, "Component");
}

TEST(ScriptVmUVETest, ExecuteScriptBytecodeUVE_RunsStagedQueryComponentToken) {
    ScriptNodeRegistryUVE registry;
    ASSERT_TRUE(RegisterBuiltInScriptNodesUVE(registry));
    ScriptGraphUVE graph;
    ASSERT_TRUE(graph.AddNodeUVE({30U, "query.entity.get_component"}));
    ASSERT_TRUE(graph.AddNodeUVE({20U, "query.entity.has_component"}));
    ASSERT_TRUE(graph.AddLinkUVE(ScriptLinkUVE{{30U, "Result"}, {20U, "Component"}}));
    const ScriptIrCompileResultUVE compiled = CompileScriptGraphToIrUVE(graph, registry);
    ASSERT_TRUE(compiled.IsSuccessUVE());
    std::vector<ScriptBytecodeDiagnosticUVE> diagnostics;
    const std::optional<ScriptBytecodeProgramUVE> bytecode =
        LowerIrToBytecodeUVE(*compiled.program, diagnostics);
    ASSERT_TRUE(bytecode.has_value());
    ASSERT_TRUE(diagnostics.empty());

    const Scene::EntityUVE entity{42U, 3U};
    const ScriptComponentValueUVE componentToken{Scene::kInvalidEntityUVE, "MeshComponentUVE", false};
    ScriptVmExecutionContextUVE context;
    ASSERT_TRUE(context.SetInputUVE(30U, "Entity", ScriptEntityValueUVE{entity}));
    ASSERT_TRUE(context.SetInputUVE(30U, "Component", componentToken));
    ASSERT_TRUE(context.SetInputUVE(20U, "Entity", ScriptEntityValueUVE{entity}));
    ASSERT_TRUE(context.SetComponentFactUVE(entity, "MeshComponentUVE", true));
    const ScriptVmExecutionResultUVE result = ExecuteScriptBytecodeUVE(*bytecode, context);

    EXPECT_TRUE(result.IsSuccessUVE());
    EXPECT_EQ(result.instructionsExecuted, 3U);
    const auto copied = context.FindOutputUVE(30U, "Result");
    ASSERT_TRUE(copied.has_value());
    ASSERT_TRUE(std::holds_alternative<ScriptComponentValueUVE>(*copied));
    EXPECT_TRUE(std::get<ScriptComponentValueUVE>(*copied).present);
    EXPECT_TRUE(std::get<bool>(*context.FindOutputUVE(20U, "Result")));
}

TEST(ScriptBytecodeUVETest, EncodeDecodeScriptBytecodeUVE_RoundTripsStagedTransferMarker) {
    ScriptBytecodeProgramUVE program;
    program.instructions.push_back({ScriptIrInstructionKindUVE::TransferValue, 4U, 9U, {}, "Out", "In",
                                    0U, 0U, 0U, 0U, true});
    std::vector<ScriptBytecodeDiagnosticUVE> diagnostics;
    const std::vector<std::uint8_t> bytes = EncodeScriptBytecodeUVE(program, diagnostics);
    ASSERT_TRUE(diagnostics.empty());
    const ScriptBytecodeDecodeResultUVE decoded = DecodeScriptBytecodeUVE(bytes);
    ASSERT_TRUE(decoded.IsSuccessUVE());
    ASSERT_EQ(decoded.program->version, ScriptBytecodeProgramUVE::kCurrentVersionUVE);
    ASSERT_EQ(decoded.program->instructions.size(), 1U);
    EXPECT_TRUE(decoded.program->instructions.front().isStagedTransfer);
}

TEST(ScriptVmUVETest, ExecuteScriptBytecodeUVE_EmitsStagedValueTransferTrace) {
    ScriptBytecodeProgramUVE program;
    program.instructions.push_back({ScriptIrInstructionKindUVE::TransferValue, 4U, 9U, {}, "Out", "In",
                                    0U, 0U, 0U, 0U, true});
    ScriptVmExecutionContextUVE context;
    ASSERT_TRUE(context.SetOutputUVE(4U, "Out", 7.0F));
    const ScriptVmExecutionResultUVE result = ExecuteScriptBytecodeUVE(program, context);
    ASSERT_TRUE(result.IsSuccessUVE());
    ASSERT_FALSE(result.trace.empty());
    EXPECT_EQ(result.trace.front().kind, ScriptVmTraceEventKindUVE::StagedValueTransferred);
    EXPECT_EQ(result.trace.front().sourceNodeId, 4U);
    EXPECT_EQ(result.trace.front().targetNodeId, 9U);
    const auto copied = context.FindInputUVE(9U, "In");
    ASSERT_TRUE(copied.has_value());
    EXPECT_FLOAT_EQ(std::get<float>(*copied), 7.0F);
}

TEST(ScriptDebuggerUVETest, StepUVE_ReportsStagedValueTransferTrace) {
    ScriptBytecodeProgramUVE program;
    program.instructions.push_back({ScriptIrInstructionKindUVE::TransferValue, 4U, 9U, {}, "Out", "In",
                                    0U, 0U, 0U, 0U, true});
    ScriptDebuggerUVE debugger;
    ASSERT_TRUE(debugger.AttachUVE(program));
    const ScriptDebuggerSnapshotUVE stepped = debugger.StepUVE();
    ASSERT_FALSE(stepped.trace.empty());
    EXPECT_EQ(stepped.trace.front().kind, ScriptVmTraceEventKindUVE::StagedValueTransferred);
    EXPECT_EQ(stepped.trace.front().sourceNodeId, 4U);
    EXPECT_EQ(stepped.trace.front().targetNodeId, 9U);
}

TEST(ScriptCompilerIRUVETest, CompileScriptGraphToIrUVE_RejectsSecondBooleanStagedConsumer) {
    ScriptNodeRegistryUVE registry;
    ASSERT_TRUE(RegisterBuiltInScriptNodesUVE(registry));
    ScriptGraphUVE graph;
    ASSERT_TRUE(graph.AddNodeUVE({10U, "logic.boolean.not"}));
    ASSERT_TRUE(graph.AddNodeUVE({20U, "logic.boolean.and"}));
    ASSERT_TRUE(graph.AddNodeUVE({30U, "logic.boolean.or"}));
    ASSERT_TRUE(graph.AddLinkUVE(ScriptLinkUVE{{10U, "Result"}, {20U, "A"}}));
    ASSERT_TRUE(graph.AddLinkUVE(ScriptLinkUVE{{20U, "Result"}, {30U, "B"}}));

    const ScriptIrCompileResultUVE result = CompileScriptGraphToIrUVE(graph, registry);
    EXPECT_FALSE(result.IsSuccessUVE());
    ASSERT_EQ(result.diagnostics.size(), 1U);
    EXPECT_EQ(result.diagnostics.front().code, ScriptValidationCodeUVE::UnsupportedRuntimeNode);
    EXPECT_EQ(result.diagnostics.front().nodeId, 30U);
    EXPECT_EQ(result.diagnostics.front().pinName, "B");
}

TEST(ScriptVmUVETest, ExecuteScriptBytecodeUVE_RunsStagedBooleanChain) {
    ScriptNodeRegistryUVE registry;
    ASSERT_TRUE(RegisterBuiltInScriptNodesUVE(registry));
    ScriptGraphUVE graph;
    ASSERT_TRUE(graph.AddNodeUVE({30U, "logic.boolean.not"}));
    ASSERT_TRUE(graph.AddNodeUVE({20U, "logic.boolean.and"}));
    ASSERT_TRUE(graph.AddLinkUVE(ScriptLinkUVE{{30U, "Result"}, {20U, "A"}}));
    const ScriptIrCompileResultUVE compiled = CompileScriptGraphToIrUVE(graph, registry);
    ASSERT_TRUE(compiled.IsSuccessUVE());
    std::vector<ScriptBytecodeDiagnosticUVE> diagnostics;
    const std::optional<ScriptBytecodeProgramUVE> bytecode =
        LowerIrToBytecodeUVE(*compiled.program, diagnostics);
    ASSERT_TRUE(bytecode.has_value());
    ASSERT_TRUE(diagnostics.empty());

    ScriptVmExecutionContextUVE context;
    ASSERT_TRUE(context.SetInputUVE(30U, "Value", true));
    ASSERT_TRUE(context.SetInputUVE(20U, "B", true));
    const ScriptVmExecutionResultUVE result = ExecuteScriptBytecodeUVE(*bytecode, context);

    EXPECT_TRUE(result.IsSuccessUVE());
    EXPECT_EQ(result.instructionsExecuted, 3U);
    ASSERT_TRUE(context.FindOutputUVE(30U, "Result").has_value());
    ASSERT_TRUE(context.FindOutputUVE(20U, "Result").has_value());
    EXPECT_FALSE(std::get<bool>(*context.FindOutputUVE(30U, "Result")));
    EXPECT_FALSE(std::get<bool>(*context.FindOutputUVE(20U, "Result")));
}

TEST(ScriptCompilerIRUVETest, CompileScriptGraphToIrUVE_StagesEngineTimeBeforeFloatConsumer) {
    ScriptNodeRegistryUVE registry;
    ASSERT_TRUE(RegisterBuiltInScriptNodesUVE(registry));
    ScriptGraphUVE graph;
    ASSERT_TRUE(graph.AddNodeUVE({10U, "engine.get_time"}));
    ASSERT_TRUE(graph.AddNodeUVE({20U, "math.float.add"}));
    ASSERT_TRUE(graph.AddLinkUVE(ScriptLinkUVE{{10U, "Value"}, {20U, "A"}}));

    const ScriptIrCompileResultUVE result = CompileScriptGraphToIrUVE(graph, registry);
    ASSERT_TRUE(result.IsSuccessUVE());
    ASSERT_EQ(result.program->instructions.size(), 3U);
    EXPECT_EQ(result.program->instructions[0].nodeTypeId, "engine.get_time");
    EXPECT_EQ(result.program->instructions[1].kind, ScriptIrInstructionKindUVE::TransferValue);
    EXPECT_EQ(result.program->instructions[1].sourceNodeId, 10U);
    EXPECT_EQ(result.program->instructions[1].targetNodeId, 20U);
    EXPECT_EQ(result.program->instructions[1].sourcePinName, "Value");
    EXPECT_EQ(result.program->instructions[1].targetPinName, "A");
    EXPECT_EQ(result.program->instructions[2].nodeTypeId, "math.float.add");
}

TEST(ScriptCompilerIRUVETest, CompileScriptGraphToIrUVE_StagesTwoNumbersBeforeComparison) {
    ScriptNodeRegistryUVE registry;
    ASSERT_TRUE(RegisterBuiltInScriptNodesUVE(registry));
    ScriptGraphUVE graph;
    ASSERT_TRUE(graph.AddNodeUVE({10U, "engine.get_time"}));
    ASSERT_TRUE(graph.AddNodeUVE({11U, "engine.get_time"}));
    ASSERT_TRUE(graph.AddNodeUVE({20U, "logic.boolean.greater_equal"}));
    ASSERT_TRUE(graph.AddLinkUVE(ScriptLinkUVE{{10U, "Value"}, {20U, "A"}}));
    ASSERT_TRUE(graph.AddLinkUVE(ScriptLinkUVE{{11U, "Value"}, {20U, "B"}}));

    const ScriptIrCompileResultUVE result = CompileScriptGraphToIrUVE(graph, registry);
    ASSERT_TRUE(result.IsSuccessUVE());
    ASSERT_EQ(result.program->instructions.size(), 5U);
    EXPECT_EQ(result.program->instructions[0].nodeTypeId, "engine.get_time");
    EXPECT_EQ(result.program->instructions[1].kind, ScriptIrInstructionKindUVE::TransferValue);
    EXPECT_TRUE(result.program->instructions[1].isStagedTransfer);
    EXPECT_EQ(result.program->instructions[1].sourceNodeId, 10U);
    EXPECT_EQ(result.program->instructions[1].targetNodeId, 20U);
    EXPECT_EQ(result.program->instructions[1].targetPinName, "A");
    EXPECT_EQ(result.program->instructions[2].nodeTypeId, "engine.get_time");
    EXPECT_EQ(result.program->instructions[3].kind, ScriptIrInstructionKindUVE::TransferValue);
    EXPECT_TRUE(result.program->instructions[3].isStagedTransfer);
    EXPECT_EQ(result.program->instructions[3].sourceNodeId, 11U);
    EXPECT_EQ(result.program->instructions[3].targetNodeId, 20U);
    EXPECT_EQ(result.program->instructions[3].targetPinName, "B");
    EXPECT_EQ(result.program->instructions[4].nodeTypeId, "logic.boolean.greater_equal");
}

TEST(ScriptCompilerIRUVETest, CompileScriptGraphToIrUVE_RejectsSecondComparisonConsumer) {
    ScriptNodeRegistryUVE registry;
    ASSERT_TRUE(RegisterBuiltInScriptNodesUVE(registry));
    ScriptGraphUVE graph;
    ASSERT_TRUE(graph.AddNodeUVE({10U, "engine.get_time"}));
    ASSERT_TRUE(graph.AddNodeUVE({11U, "engine.get_time"}));
    ASSERT_TRUE(graph.AddNodeUVE({20U, "logic.boolean.greater"}));
    ASSERT_TRUE(graph.AddNodeUVE({30U, "logic.boolean.less"}));
    ASSERT_TRUE(graph.AddLinkUVE(ScriptLinkUVE{{10U, "Value"}, {20U, "A"}}));
    ASSERT_TRUE(graph.AddLinkUVE(ScriptLinkUVE{{11U, "Value"}, {20U, "B"}}));
    ASSERT_TRUE(graph.AddLinkUVE(ScriptLinkUVE{{10U, "Value"}, {30U, "A"}}));

    const ScriptIrCompileResultUVE result = CompileScriptGraphToIrUVE(graph, registry);
    EXPECT_FALSE(result.IsSuccessUVE());
    ASSERT_EQ(result.diagnostics.size(), 1U);
    EXPECT_EQ(result.diagnostics.front().code, ScriptValidationCodeUVE::UnsupportedRuntimeNode);
    EXPECT_EQ(result.diagnostics.front().nodeId, 30U);
    EXPECT_EQ(result.diagnostics.front().pinName, "A");
}

TEST(ScriptVmUVETest, ExecuteScriptBytecodeUVE_RunsStagedNumbersIntoComparison) {
    ScriptNodeRegistryUVE registry;
    ASSERT_TRUE(RegisterBuiltInScriptNodesUVE(registry));
    ScriptGraphUVE graph;
    ASSERT_TRUE(graph.AddNodeUVE({10U, "engine.get_time"}));
    ASSERT_TRUE(graph.AddNodeUVE({11U, "engine.get_time"}));
    ASSERT_TRUE(graph.AddNodeUVE({20U, "logic.boolean.greater_equal"}));
    ASSERT_TRUE(graph.AddLinkUVE(ScriptLinkUVE{{10U, "Value"}, {20U, "A"}}));
    ASSERT_TRUE(graph.AddLinkUVE(ScriptLinkUVE{{11U, "Value"}, {20U, "B"}}));
    const ScriptIrCompileResultUVE compiled = CompileScriptGraphToIrUVE(graph, registry);
    ASSERT_TRUE(compiled.IsSuccessUVE());
    std::vector<ScriptBytecodeDiagnosticUVE> diagnostics;
    const std::optional<ScriptBytecodeProgramUVE> bytecode =
        LowerIrToBytecodeUVE(*compiled.program, diagnostics);
    ASSERT_TRUE(bytecode.has_value());
    ASSERT_TRUE(diagnostics.empty());

    EngineTimeCaptureUVE capture;
    capture.value = 2.5F;
    const ScriptEngineCallBindingsUVE bindings{nullptr, &capture, CaptureEngineTimeUVE};
    ScriptVmExecutionOptionsUVE options;
    options.engineCallBindings = &bindings;
    ScriptVmExecutionContextUVE context;
    const ScriptVmExecutionResultUVE result = ExecuteScriptBytecodeUVE(*bytecode, context, options);

    EXPECT_TRUE(result.IsSuccessUVE());
    EXPECT_EQ(result.instructionsExecuted, 5U);
    EXPECT_EQ(capture.callCount, 2U);
    ASSERT_TRUE(context.FindOutputUVE(20U, "Result").has_value());
    EXPECT_TRUE(std::get<bool>(*context.FindOutputUVE(20U, "Result")));
}

TEST(ScriptCompilerIRUVETest, CompileScriptGraphToIrUVE_RejectsComposedFloatDependencyScheduling) {
    ScriptNodeRegistryUVE registry;
    ASSERT_TRUE(RegisterBuiltInScriptNodesUVE(registry));
    ScriptGraphUVE graph;
    ASSERT_TRUE(graph.AddNodeUVE({10U, "engine.get_time"}));
    ASSERT_TRUE(graph.AddNodeUVE({20U, "math.float.add"}));
    ASSERT_TRUE(graph.AddNodeUVE({30U, "math.float.multiply"}));
    ASSERT_TRUE(graph.AddLinkUVE(ScriptLinkUVE{{10U, "Value"}, {20U, "A"}}));
    ASSERT_TRUE(graph.AddLinkUVE(ScriptLinkUVE{{20U, "Result"}, {30U, "A"}}));

    const ScriptIrCompileResultUVE result = CompileScriptGraphToIrUVE(graph, registry);
    EXPECT_FALSE(result.IsSuccessUVE());
    ASSERT_EQ(result.diagnostics.size(), 1U);
    EXPECT_EQ(result.diagnostics.front().code, ScriptValidationCodeUVE::UnsupportedRuntimeNode);
    EXPECT_EQ(result.diagnostics.front().nodeId, 30U);
    EXPECT_EQ(result.diagnostics.front().pinName, "A");
}

TEST(ScriptVmUVETest, ExecuteScriptBytecodeUVE_RunsStagedEngineTimeFloatLink) {
    ScriptNodeRegistryUVE registry;
    ASSERT_TRUE(RegisterBuiltInScriptNodesUVE(registry));
    ScriptGraphUVE graph;
    ASSERT_TRUE(graph.AddNodeUVE({10U, "engine.get_time"}));
    ASSERT_TRUE(graph.AddNodeUVE({20U, "math.float.add"}));
    ASSERT_TRUE(graph.AddLinkUVE(ScriptLinkUVE{{10U, "Value"}, {20U, "A"}}));
    const ScriptIrCompileResultUVE compiled = CompileScriptGraphToIrUVE(graph, registry);
    ASSERT_TRUE(compiled.IsSuccessUVE());
    std::vector<ScriptBytecodeDiagnosticUVE> diagnostics;
    const std::optional<ScriptBytecodeProgramUVE> bytecode =
        LowerIrToBytecodeUVE(*compiled.program, diagnostics);
    ASSERT_TRUE(bytecode.has_value());
    ASSERT_TRUE(diagnostics.empty());

    ScriptVmExecutionContextUVE context;
    ASSERT_TRUE(context.SetInputUVE(20U, "B", 3.0F));
    EngineTimeCaptureUVE capture;
    capture.value = 2.5F;
    const ScriptEngineCallBindingsUVE bindings{nullptr, &capture, CaptureEngineTimeUVE};
    ScriptVmExecutionOptionsUVE options;
    options.engineCallBindings = &bindings;
    const ScriptVmExecutionResultUVE result = ExecuteScriptBytecodeUVE(*bytecode, context, options);

    EXPECT_TRUE(result.IsSuccessUVE());
    EXPECT_EQ(result.instructionsExecuted, 3U);
    EXPECT_EQ(capture.callCount, 1U);
    ASSERT_TRUE(context.FindOutputUVE(20U, "Result").has_value());
    EXPECT_FLOAT_EQ(std::get<float>(*context.FindOutputUVE(20U, "Result")), 5.5F);
}

TEST(ScriptCompilerIRUVETest, CompileScriptGraphToIrUVE_StagesNumberBeforeVector3ScaleConsumer) {
    ScriptNodeRegistryUVE registry;
    ASSERT_TRUE(RegisterBuiltInScriptNodesUVE(registry));
    ScriptGraphUVE graph;
    ASSERT_TRUE(graph.AddNodeUVE({30U, "math.float.multiply"}));
    ASSERT_TRUE(graph.AddNodeUVE({20U, "math.vector3.multiply"}));
    ASSERT_TRUE(graph.AddLinkUVE(ScriptLinkUVE{{30U, "Result"}, {20U, "Scale"}}));

    const ScriptIrCompileResultUVE result = CompileScriptGraphToIrUVE(graph, registry);
    ASSERT_TRUE(result.IsSuccessUVE());
    ASSERT_EQ(result.program->instructions.size(), 3U);
    EXPECT_EQ(result.program->instructions[0].nodeTypeId, "math.float.multiply");
    EXPECT_EQ(result.program->instructions[1].kind, ScriptIrInstructionKindUVE::TransferValue);
    EXPECT_EQ(result.program->instructions[1].sourceNodeId, 30U);
    EXPECT_EQ(result.program->instructions[1].targetNodeId, 20U);
    EXPECT_EQ(result.program->instructions[1].sourcePinName, "Result");
    EXPECT_EQ(result.program->instructions[1].targetPinName, "Scale");
    EXPECT_EQ(result.program->instructions[2].nodeTypeId, "math.vector3.multiply");
}

TEST(ScriptCompilerIRUVETest, CompileScriptGraphToIrUVE_RejectsSecondVector3ScaleConsumer) {
    ScriptNodeRegistryUVE registry;
    ASSERT_TRUE(RegisterBuiltInScriptNodesUVE(registry));
    ScriptGraphUVE graph;
    ASSERT_TRUE(graph.AddNodeUVE({10U, "engine.get_time"}));
    ASSERT_TRUE(graph.AddNodeUVE({20U, "math.vector3.multiply"}));
    ASSERT_TRUE(graph.AddNodeUVE({30U, "math.float.add"}));
    ASSERT_TRUE(graph.AddNodeUVE({40U, "math.vector3.multiply"}));
    ASSERT_TRUE(graph.AddLinkUVE(ScriptLinkUVE{{10U, "Value"}, {20U, "Scale"}}));
    ASSERT_TRUE(graph.AddLinkUVE(ScriptLinkUVE{{30U, "Result"}, {40U, "Scale"}}));

    const ScriptIrCompileResultUVE result = CompileScriptGraphToIrUVE(graph, registry);
    EXPECT_FALSE(result.IsSuccessUVE());
    ASSERT_EQ(result.diagnostics.size(), 1U);
    EXPECT_EQ(result.diagnostics.front().code, ScriptValidationCodeUVE::UnsupportedRuntimeNode);
    EXPECT_EQ(result.diagnostics.front().nodeId, 40U);
    EXPECT_EQ(result.diagnostics.front().pinName, "Scale");
}

TEST(ScriptCompilerIRUVETest, CompileScriptGraphToIrUVE_RejectsComposedNumberBeforeVector3Scale) {
    ScriptNodeRegistryUVE registry;
    ASSERT_TRUE(RegisterBuiltInScriptNodesUVE(registry));
    ScriptGraphUVE graph;
    ASSERT_TRUE(graph.AddNodeUVE({10U, "engine.get_time"}));
    ASSERT_TRUE(graph.AddNodeUVE({30U, "math.float.add"}));
    ASSERT_TRUE(graph.AddNodeUVE({20U, "math.vector3.multiply"}));
    ASSERT_TRUE(graph.AddLinkUVE(ScriptLinkUVE{{10U, "Value"}, {30U, "A"}}));
    ASSERT_TRUE(graph.AddLinkUVE(ScriptLinkUVE{{30U, "Result"}, {20U, "Scale"}}));

    const ScriptIrCompileResultUVE result = CompileScriptGraphToIrUVE(graph, registry);
    EXPECT_FALSE(result.IsSuccessUVE());
    ASSERT_EQ(result.diagnostics.size(), 1U);
    EXPECT_EQ(result.diagnostics.front().code, ScriptValidationCodeUVE::UnsupportedRuntimeNode);
    EXPECT_EQ(result.diagnostics.front().nodeId, 20U);
    EXPECT_EQ(result.diagnostics.front().pinName, "Scale");
}

TEST(ScriptVmUVETest, ExecuteScriptBytecodeUVE_RunsFloatProducerIntoVector3Scale) {
    ScriptNodeRegistryUVE registry;
    ASSERT_TRUE(RegisterBuiltInScriptNodesUVE(registry));
    ScriptGraphUVE graph;
    ASSERT_TRUE(graph.AddNodeUVE({30U, "math.float.multiply"}));
    ASSERT_TRUE(graph.AddNodeUVE({20U, "math.vector3.multiply"}));
    ASSERT_TRUE(graph.AddLinkUVE(ScriptLinkUVE{{30U, "Result"}, {20U, "Scale"}}));
    const ScriptIrCompileResultUVE compiled = CompileScriptGraphToIrUVE(graph, registry);
    ASSERT_TRUE(compiled.IsSuccessUVE());
    std::vector<ScriptBytecodeDiagnosticUVE> diagnostics;
    const std::optional<ScriptBytecodeProgramUVE> bytecode =
        LowerIrToBytecodeUVE(*compiled.program, diagnostics);
    ASSERT_TRUE(bytecode.has_value());
    ASSERT_TRUE(diagnostics.empty());

    ScriptVmExecutionContextUVE context;
    ASSERT_TRUE(context.SetInputUVE(30U, "A", 2.0F));
    ASSERT_TRUE(context.SetInputUVE(30U, "B", 3.0F));
    ASSERT_TRUE(context.SetInputUVE(20U, "Vector", ScriptVector3ValueUVE{{1.0F, -2.0F, 3.0F}}));
    const ScriptVmExecutionResultUVE result = ExecuteScriptBytecodeUVE(*bytecode, context);

    EXPECT_TRUE(result.IsSuccessUVE());
    EXPECT_EQ(result.instructionsExecuted, 3U);
    ASSERT_TRUE(context.FindOutputUVE(20U, "Result").has_value());
    EXPECT_EQ(std::get<ScriptVector3ValueUVE>(*context.FindOutputUVE(20U, "Result")),
              (ScriptVector3ValueUVE{{6.0F, -12.0F, 18.0F}}));
}

TEST(ScriptCompilerIRUVETest, CompileScriptGraphToIrUVE_StagesVector3ProducerBeforeConsumer) {
    ScriptNodeRegistryUVE registry;
    ASSERT_TRUE(RegisterBuiltInScriptNodesUVE(registry));
    ScriptGraphUVE graph;
    ASSERT_TRUE(graph.AddNodeUVE({30U, "math.vector3.make"}));
    ASSERT_TRUE(graph.AddNodeUVE({20U, "math.vector3.add"}));
    ASSERT_TRUE(graph.AddLinkUVE(ScriptLinkUVE{{30U, "Vector"}, {20U, "A"}}));

    const ScriptIrCompileResultUVE result = CompileScriptGraphToIrUVE(graph, registry);
    ASSERT_TRUE(result.IsSuccessUVE());
    ASSERT_EQ(result.program->instructions.size(), 3U);
    EXPECT_EQ(result.program->instructions[0].nodeTypeId, "math.vector3.make");
    EXPECT_EQ(result.program->instructions[1].kind, ScriptIrInstructionKindUVE::TransferValue);
    EXPECT_EQ(result.program->instructions[1].sourceNodeId, 30U);
    EXPECT_EQ(result.program->instructions[1].targetNodeId, 20U);
    EXPECT_EQ(result.program->instructions[1].sourcePinName, "Vector");
    EXPECT_EQ(result.program->instructions[1].targetPinName, "A");
    EXPECT_EQ(result.program->instructions[2].nodeTypeId, "math.vector3.add");
}

TEST(ScriptCompilerIRUVETest, CompileScriptGraphToIrUVE_RejectsComposedVector3DependencyScheduling) {
    ScriptNodeRegistryUVE registry;
    ASSERT_TRUE(RegisterBuiltInScriptNodesUVE(registry));
    ScriptGraphUVE graph;
    ASSERT_TRUE(graph.AddNodeUVE({10U, "math.vector3.make"}));
    ASSERT_TRUE(graph.AddNodeUVE({20U, "math.vector3.add"}));
    ASSERT_TRUE(graph.AddNodeUVE({30U, "math.vector3.cross"}));
    ASSERT_TRUE(graph.AddLinkUVE(ScriptLinkUVE{{10U, "Vector"}, {20U, "A"}}));
    ASSERT_TRUE(graph.AddLinkUVE(ScriptLinkUVE{{20U, "Result"}, {30U, "A"}}));

    const ScriptIrCompileResultUVE result = CompileScriptGraphToIrUVE(graph, registry);
    EXPECT_FALSE(result.IsSuccessUVE());
    ASSERT_EQ(result.diagnostics.size(), 1U);
    EXPECT_EQ(result.diagnostics.front().code, ScriptValidationCodeUVE::UnsupportedRuntimeNode);
    EXPECT_EQ(result.diagnostics.front().nodeId, 30U);
    EXPECT_EQ(result.diagnostics.front().pinName, "A");
}

TEST(ScriptVmUVETest, ExecuteScriptBytecodeUVE_RunsStagedVector3Link) {
    ScriptNodeRegistryUVE registry;
    ASSERT_TRUE(RegisterBuiltInScriptNodesUVE(registry));
    ScriptGraphUVE graph;
    ASSERT_TRUE(graph.AddNodeUVE({30U, "math.vector3.make"}));
    ASSERT_TRUE(graph.AddNodeUVE({20U, "math.vector3.add"}));
    ASSERT_TRUE(graph.AddLinkUVE(ScriptLinkUVE{{30U, "Vector"}, {20U, "A"}}));
    const ScriptIrCompileResultUVE compiled = CompileScriptGraphToIrUVE(graph, registry);
    ASSERT_TRUE(compiled.IsSuccessUVE());
    std::vector<ScriptBytecodeDiagnosticUVE> diagnostics;
    const std::optional<ScriptBytecodeProgramUVE> bytecode =
        LowerIrToBytecodeUVE(*compiled.program, diagnostics);
    ASSERT_TRUE(bytecode.has_value());
    ASSERT_TRUE(diagnostics.empty());

    ScriptVmExecutionContextUVE context;
    ASSERT_TRUE(context.SetInputUVE(30U, "X", 1.0F));
    ASSERT_TRUE(context.SetInputUVE(30U, "Y", 2.0F));
    ASSERT_TRUE(context.SetInputUVE(30U, "Z", 3.0F));
    ASSERT_TRUE(context.SetInputUVE(20U, "B", ScriptVector3ValueUVE{{4.0F, 5.0F, 6.0F}}));
    const ScriptVmExecutionResultUVE result = ExecuteScriptBytecodeUVE(*bytecode, context);

    EXPECT_TRUE(result.IsSuccessUVE());
    EXPECT_EQ(result.instructionsExecuted, 3U);
    ASSERT_TRUE(context.FindOutputUVE(20U, "Result").has_value());
    EXPECT_EQ(std::get<ScriptVector3ValueUVE>(*context.FindOutputUVE(20U, "Result")),
              (ScriptVector3ValueUVE{{5.0F, 7.0F, 9.0F}}));
}

TEST(ScriptCompilerIRUVETest, CompileScriptGraphToIrUVE_StagesVector3DotNumberBeforeFloatConsumer) {
    ScriptNodeRegistryUVE registry;
    ASSERT_TRUE(RegisterBuiltInScriptNodesUVE(registry));
    ScriptGraphUVE graph;
    ASSERT_TRUE(graph.AddNodeUVE({30U, "math.vector3.dot"}));
    ASSERT_TRUE(graph.AddNodeUVE({20U, "math.float.add"}));
    ASSERT_TRUE(graph.AddLinkUVE(ScriptLinkUVE{{30U, "Result"}, {20U, "A"}}));

    const ScriptIrCompileResultUVE result = CompileScriptGraphToIrUVE(graph, registry);
    ASSERT_TRUE(result.IsSuccessUVE());
    ASSERT_EQ(result.program->instructions.size(), 3U);
    EXPECT_EQ(result.program->instructions[0].nodeTypeId, "math.vector3.dot");
    EXPECT_EQ(result.program->instructions[1].kind, ScriptIrInstructionKindUVE::TransferValue);
    EXPECT_EQ(result.program->instructions[1].sourceNodeId, 30U);
    EXPECT_EQ(result.program->instructions[1].targetNodeId, 20U);
    EXPECT_EQ(result.program->instructions[1].sourcePinName, "Result");
    EXPECT_EQ(result.program->instructions[1].targetPinName, "A");
    EXPECT_EQ(result.program->instructions[2].nodeTypeId, "math.float.add");
}

TEST(ScriptCompilerIRUVETest, CompileScriptGraphToIrUVE_StagesVector2DotNumberBeforeFloatConsumer) {
    ScriptNodeRegistryUVE registry;
    ASSERT_TRUE(RegisterBuiltInScriptNodesUVE(registry));
    ScriptGraphUVE graph;
    ASSERT_TRUE(graph.AddNodeUVE({30U, "math.vector2.dot"}));
    ASSERT_TRUE(graph.AddNodeUVE({20U, "math.float.add"}));
    ASSERT_TRUE(graph.AddLinkUVE(ScriptLinkUVE{{30U, "Result"}, {20U, "A"}}));

    const ScriptIrCompileResultUVE result = CompileScriptGraphToIrUVE(graph, registry);
    ASSERT_TRUE(result.IsSuccessUVE());
    ASSERT_EQ(result.program->instructions.size(), 3U);
    EXPECT_EQ(result.program->instructions[0].nodeTypeId, "math.vector2.dot");
    EXPECT_EQ(result.program->instructions[1].kind, ScriptIrInstructionKindUVE::TransferValue);
    EXPECT_EQ(result.program->instructions[1].sourceNodeId, 30U);
    EXPECT_EQ(result.program->instructions[1].targetNodeId, 20U);
    EXPECT_EQ(result.program->instructions[1].sourcePinName, "Result");
    EXPECT_EQ(result.program->instructions[1].targetPinName, "A");
    EXPECT_EQ(result.program->instructions[2].nodeTypeId, "math.float.add");
}

TEST(ScriptCompilerIRUVETest, CompileScriptGraphToIrUVE_StagesVector3DirectionBeforeConsumer) {
    ScriptNodeRegistryUVE registry;
    ASSERT_TRUE(RegisterBuiltInScriptNodesUVE(registry));
    ScriptGraphUVE graph;
    ASSERT_TRUE(graph.AddNodeUVE({30U, "math.vector3.direction"}));
    ASSERT_TRUE(graph.AddNodeUVE({20U, "math.vector3.add"}));
    ASSERT_TRUE(graph.AddLinkUVE(ScriptLinkUVE{{30U, "Result"}, {20U, "A"}}));

    const ScriptIrCompileResultUVE result = CompileScriptGraphToIrUVE(graph, registry);
    ASSERT_TRUE(result.IsSuccessUVE());
    ASSERT_EQ(result.program->instructions.size(), 3U);
    EXPECT_EQ(result.program->instructions[0].nodeTypeId, "math.vector3.direction");
    EXPECT_EQ(result.program->instructions[1].kind, ScriptIrInstructionKindUVE::TransferValue);
    EXPECT_EQ(result.program->instructions[1].sourceNodeId, 30U);
    EXPECT_EQ(result.program->instructions[1].targetNodeId, 20U);
    EXPECT_EQ(result.program->instructions[1].sourcePinName, "Result");
    EXPECT_EQ(result.program->instructions[1].targetPinName, "A");
    EXPECT_EQ(result.program->instructions[2].nodeTypeId, "math.vector3.add");
}

TEST(ScriptCompilerIRUVETest, CompileScriptGraphToIrUVE_StagesVector3LengthNumberBeforeFloatConsumer) {
    ScriptNodeRegistryUVE registry;
    ASSERT_TRUE(RegisterBuiltInScriptNodesUVE(registry));
    ScriptGraphUVE graph;
    ASSERT_TRUE(graph.AddNodeUVE({10U, "math.vector3.length"}));
    ASSERT_TRUE(graph.AddNodeUVE({20U, "math.float.multiply"}));
    ASSERT_TRUE(graph.AddLinkUVE(ScriptLinkUVE{{10U, "Length"}, {20U, "B"}}));

    const ScriptIrCompileResultUVE result = CompileScriptGraphToIrUVE(graph, registry);
    ASSERT_TRUE(result.IsSuccessUVE());
    ASSERT_EQ(result.program->instructions.size(), 3U);
    EXPECT_EQ(result.program->instructions[0].nodeTypeId, "math.vector3.length");
    EXPECT_EQ(result.program->instructions[1].kind, ScriptIrInstructionKindUVE::TransferValue);
    EXPECT_EQ(result.program->instructions[1].targetPinName, "B");
    EXPECT_EQ(result.program->instructions[2].nodeTypeId, "math.float.multiply");
}

TEST(ScriptCompilerIRUVETest, CompileScriptGraphToIrUVE_RejectsComposedNumberDependencyScheduling) {
    ScriptNodeRegistryUVE registry;
    ASSERT_TRUE(RegisterBuiltInScriptNodesUVE(registry));
    ScriptGraphUVE graph;
    ASSERT_TRUE(graph.AddNodeUVE({10U, "math.vector3.dot"}));
    ASSERT_TRUE(graph.AddNodeUVE({20U, "math.float.add"}));
    ASSERT_TRUE(graph.AddNodeUVE({30U, "math.float.multiply"}));
    ASSERT_TRUE(graph.AddLinkUVE(ScriptLinkUVE{{10U, "Result"}, {20U, "A"}}));
    ASSERT_TRUE(graph.AddLinkUVE(ScriptLinkUVE{{20U, "Result"}, {30U, "A"}}));

    const ScriptIrCompileResultUVE result = CompileScriptGraphToIrUVE(graph, registry);
    EXPECT_FALSE(result.IsSuccessUVE());
    ASSERT_EQ(result.diagnostics.size(), 1U);
    EXPECT_EQ(result.diagnostics.front().code, ScriptValidationCodeUVE::UnsupportedRuntimeNode);
    EXPECT_EQ(result.diagnostics.front().nodeId, 30U);
    EXPECT_EQ(result.diagnostics.front().pinName, "A");
}

TEST(ScriptVmUVETest, ExecuteScriptBytecodeUVE_RunsStagedVector3DotNumberIntoFloat) {
    ScriptNodeRegistryUVE registry;
    ASSERT_TRUE(RegisterBuiltInScriptNodesUVE(registry));
    ScriptGraphUVE graph;
    ASSERT_TRUE(graph.AddNodeUVE({30U, "math.vector3.dot"}));
    ASSERT_TRUE(graph.AddNodeUVE({20U, "math.float.add"}));
    ASSERT_TRUE(graph.AddLinkUVE(ScriptLinkUVE{{30U, "Result"}, {20U, "A"}}));
    const ScriptIrCompileResultUVE compiled = CompileScriptGraphToIrUVE(graph, registry);
    ASSERT_TRUE(compiled.IsSuccessUVE());
    std::vector<ScriptBytecodeDiagnosticUVE> diagnostics;
    const std::optional<ScriptBytecodeProgramUVE> bytecode =
        LowerIrToBytecodeUVE(*compiled.program, diagnostics);
    ASSERT_TRUE(bytecode.has_value());
    ASSERT_TRUE(diagnostics.empty());

    ScriptVmExecutionContextUVE context;
    ASSERT_TRUE(context.SetInputUVE(30U, "A", ScriptVector3ValueUVE{{1.0F, 2.0F, 3.0F}}));
    ASSERT_TRUE(context.SetInputUVE(30U, "B", ScriptVector3ValueUVE{{4.0F, 5.0F, 6.0F}}));
    ASSERT_TRUE(context.SetInputUVE(20U, "B", 8.0F));
    const ScriptVmExecutionResultUVE result = ExecuteScriptBytecodeUVE(*bytecode, context);

    EXPECT_TRUE(result.IsSuccessUVE());
    EXPECT_EQ(result.instructionsExecuted, 3U);
    ASSERT_TRUE(context.FindOutputUVE(20U, "Result").has_value());
    EXPECT_FLOAT_EQ(std::get<float>(*context.FindOutputUVE(20U, "Result")), 40.0F);
}

TEST(ScriptCompilerIRUVETest, CompileScriptGraphToIrUVE_StagesFloatProducerBeforeConsumer) {
    ScriptNodeRegistryUVE registry;
    ASSERT_TRUE(RegisterBuiltInScriptNodesUVE(registry));
    ScriptGraphUVE graph;
    ASSERT_TRUE(graph.AddNodeUVE({30U, "math.float.multiply"}));
    ASSERT_TRUE(graph.AddNodeUVE({20U, "math.float.add"}));
    ASSERT_TRUE(graph.AddLinkUVE(ScriptLinkUVE{{30U, "Result"}, {20U, "A"}}));

    const ScriptIrCompileResultUVE result = CompileScriptGraphToIrUVE(graph, registry);
    ASSERT_TRUE(result.IsSuccessUVE());
    ASSERT_EQ(result.program->instructions.size(), 3U);
    EXPECT_EQ(result.program->instructions[0].nodeTypeId, "math.float.multiply");
    EXPECT_EQ(result.program->instructions[1].kind, ScriptIrInstructionKindUVE::TransferValue);
    EXPECT_EQ(result.program->instructions[1].sourceNodeId, 30U);
    EXPECT_EQ(result.program->instructions[1].targetNodeId, 20U);
    EXPECT_EQ(result.program->instructions[1].sourcePinName, "Result");
    EXPECT_EQ(result.program->instructions[1].targetPinName, "A");
    EXPECT_EQ(result.program->instructions[2].nodeTypeId, "math.float.add");
}

TEST(ScriptCompilerIRUVETest, CompileScriptGraphToIrUVE_RejectsSecondFloatStagedConsumer) {
    ScriptNodeRegistryUVE registry;
    ASSERT_TRUE(RegisterBuiltInScriptNodesUVE(registry));
    ScriptGraphUVE graph;
    ASSERT_TRUE(graph.AddNodeUVE({10U, "math.float.multiply"}));
    ASSERT_TRUE(graph.AddNodeUVE({20U, "math.float.add"}));
    ASSERT_TRUE(graph.AddNodeUVE({30U, "math.float.subtract"}));
    ASSERT_TRUE(graph.AddLinkUVE(ScriptLinkUVE{{10U, "Result"}, {20U, "A"}}));
    ASSERT_TRUE(graph.AddLinkUVE(ScriptLinkUVE{{20U, "Result"}, {30U, "B"}}));

    const ScriptIrCompileResultUVE result = CompileScriptGraphToIrUVE(graph, registry);
    EXPECT_FALSE(result.IsSuccessUVE());
    ASSERT_EQ(result.diagnostics.size(), 1U);
    EXPECT_EQ(result.diagnostics.front().code, ScriptValidationCodeUVE::UnsupportedRuntimeNode);
    EXPECT_EQ(result.diagnostics.front().nodeId, 30U);
    EXPECT_EQ(result.diagnostics.front().pinName, "B");
}

TEST(ScriptVmUVETest, ExecuteScriptBytecodeUVE_RunsStagedFloatChain) {
    ScriptNodeRegistryUVE registry;
    ASSERT_TRUE(RegisterBuiltInScriptNodesUVE(registry));
    ScriptGraphUVE graph;
    ASSERT_TRUE(graph.AddNodeUVE({30U, "math.float.multiply"}));
    ASSERT_TRUE(graph.AddNodeUVE({20U, "math.float.add"}));
    ASSERT_TRUE(graph.AddLinkUVE(ScriptLinkUVE{{30U, "Result"}, {20U, "A"}}));
    const ScriptIrCompileResultUVE compiled = CompileScriptGraphToIrUVE(graph, registry);
    ASSERT_TRUE(compiled.IsSuccessUVE());
    std::vector<ScriptBytecodeDiagnosticUVE> diagnostics;
    const std::optional<ScriptBytecodeProgramUVE> bytecode =
        LowerIrToBytecodeUVE(*compiled.program, diagnostics);
    ASSERT_TRUE(bytecode.has_value());
    ASSERT_TRUE(diagnostics.empty());

    ScriptVmExecutionContextUVE context;
    ASSERT_TRUE(context.SetInputUVE(30U, "A", 2.0F));
    ASSERT_TRUE(context.SetInputUVE(30U, "B", 4.0F));
    ASSERT_TRUE(context.SetInputUVE(20U, "B", 5.0F));
    const ScriptVmExecutionResultUVE result = ExecuteScriptBytecodeUVE(*bytecode, context);

    EXPECT_TRUE(result.IsSuccessUVE());
    EXPECT_EQ(result.instructionsExecuted, 3U);
    ASSERT_TRUE(context.FindOutputUVE(20U, "Result").has_value());
    EXPECT_FLOAT_EQ(std::get<float>(*context.FindOutputUVE(20U, "Result")), 13.0F);
}

TEST(ScriptVmUVETest, ExecuteScriptBytecodeUVE_EngineLogUsesCallerOwnedBinding) {
    ScriptBytecodeProgramUVE program;
    program.instructions.push_back({ScriptIrInstructionKindUVE::ExecuteNode, 70U, 0U, "engine.log", {}, {}});
    ScriptVmExecutionContextUVE context;
    ASSERT_TRUE(context.SetInputUVE(70U, "Value", 42.5F));
    EngineLogCaptureUVE capture;
    const ScriptEngineCallBindingsUVE bindings{CaptureEngineLogUVE, &capture};
    ScriptVmExecutionOptionsUVE options;
    options.engineCallBindings = &bindings;

    const ScriptVmExecutionResultUVE result = ExecuteScriptBytecodeUVE(program, context, options);
    EXPECT_TRUE(result.IsSuccessUVE());
    EXPECT_EQ(capture.callCount, 1U);
    EXPECT_FLOAT_EQ(capture.lastValue, 42.5F);
    ASSERT_EQ(result.trace.size(), 2U);
    EXPECT_EQ(result.trace.front().nodeTypeId, "engine.log");
}

TEST(ScriptVmUVETest, ExecuteScriptBytecodeUVE_EngineGetTimeWritesFiniteCopiedOutput) {
    ScriptBytecodeProgramUVE program;
    program.instructions.push_back({ScriptIrInstructionKindUVE::ExecuteNode, 72U, 0U, "engine.get_time", {}, {}});
    ScriptVmExecutionContextUVE context;
    EngineTimeCaptureUVE capture;
    capture.value = 12.75F;
    const ScriptEngineCallBindingsUVE bindings{nullptr, &capture, CaptureEngineTimeUVE};
    ScriptVmExecutionOptionsUVE options;
    options.engineCallBindings = &bindings;

    const ScriptVmExecutionResultUVE result = ExecuteScriptBytecodeUVE(program, context, options);
    EXPECT_TRUE(result.IsSuccessUVE());
    EXPECT_EQ(capture.callCount, 1U);
    ASSERT_TRUE(context.FindOutputUVE(72U, "Value").has_value());
    EXPECT_FLOAT_EQ(std::get<float>(*context.FindOutputUVE(72U, "Value")), 12.75F);
}

TEST(ScriptVmUVETest, ExecuteScriptBytecodeUVE_EngineGetTimeRejectsUnboundRejectedOrNonFiniteOutput) {
    ScriptBytecodeProgramUVE program;
    program.instructions.push_back({ScriptIrInstructionKindUVE::ExecuteNode, 73U, 0U, "engine.get_time", {}, {}});
    ScriptVmExecutionContextUVE context;

    const ScriptVmExecutionResultUVE unbound = ExecuteScriptBytecodeUVE(program, context);
    EXPECT_EQ(unbound.status, ScriptVmStatusUVE::NodeExecutionFailed);

    EngineTimeCaptureUVE capture;
    const ScriptEngineCallBindingsUVE bindings{nullptr, &capture, CaptureEngineTimeUVE};
    ScriptVmExecutionOptionsUVE options;
    options.engineCallBindings = &bindings;
    capture.accept = false;
    EXPECT_EQ(ExecuteScriptBytecodeUVE(program, context, options).status,
              ScriptVmStatusUVE::NodeExecutionFailed);
    capture.accept = true;
    capture.finite = false;
    EXPECT_EQ(ExecuteScriptBytecodeUVE(program, context, options).status,
              ScriptVmStatusUVE::NodeExecutionFailed);
    EXPECT_EQ(capture.callCount, 2U);
}

TEST(ScriptVmUVETest, ExecuteScriptBytecodeUVE_EngineLogRejectsUnboundOrRejectedCall) {
    ScriptBytecodeProgramUVE program;
    program.instructions.push_back({ScriptIrInstructionKindUVE::ExecuteNode, 71U, 0U, "engine.log", {}, {}});
    ScriptVmExecutionContextUVE context;
    ASSERT_TRUE(context.SetInputUVE(71U, "Value", 1.0F));

    const ScriptVmExecutionResultUVE unbound = ExecuteScriptBytecodeUVE(program, context);
    EXPECT_EQ(unbound.status, ScriptVmStatusUVE::NodeExecutionFailed);
    ASSERT_EQ(unbound.diagnostics.size(), 1U);

    EngineLogCaptureUVE capture;
    capture.accept = false;
    const ScriptEngineCallBindingsUVE bindings{CaptureEngineLogUVE, &capture};
    ScriptVmExecutionOptionsUVE options;
    options.engineCallBindings = &bindings;
    const ScriptVmExecutionResultUVE rejected = ExecuteScriptBytecodeUVE(program, context, options);
    EXPECT_EQ(rejected.status, ScriptVmStatusUVE::NodeExecutionFailed);
    EXPECT_EQ(capture.callCount, 1U);
}

TEST(ScriptNodeRegistryUVETest, FindNodeTypeUVE_ReturnsCopiedStableDescriptorView) {
    ScriptNodeRegistryUVE registry;
    ASSERT_TRUE(registry.RegisterNodeTypeUVE(MakeSourceNodeUVE()));
    const ScriptNodeTypeDescriptorUVE* descriptor = registry.FindNodeTypeUVE("test.source");
    ASSERT_NE(descriptor, nullptr);
    EXPECT_EQ(descriptor->displayName, "Test Source");
    EXPECT_EQ(descriptor->pins.size(), 2U);
    EXPECT_EQ(registry.FindNodeTypeUVE("missing"), nullptr);
}

TEST(ScriptNodeRegistryUVETest, DescriptorV2_PreservesPresentationMetadataAndOrdersDescriptorsDeterministically) {
    ScriptNodeRegistryUVE registry;
    ScriptNodeTypeDescriptorUVE late{
        "test.late", "Late", {{"Value", ScriptPinDirectionUVE::Input, ScriptValueTypeUVE::Number,
                                  ScriptPinRoleUVE::Data, std::string("0.016")}},
        "FLOW", "node.branch", 20U, kScriptNodePresentationFlagCollapsibleUVE};
    ScriptNodeTypeDescriptorUVE early{
        "test.early", "Early", {{"Exec", ScriptPinDirectionUVE::Output, ScriptValueTypeUVE::Execution}},
        "EVENT", "node.event", 10U, kScriptNodePresentationFlagCompactUVE};
    ASSERT_TRUE(registry.RegisterNodeTypeUVE(late));
    ASSERT_TRUE(registry.RegisterNodeTypeUVE(early));

    const ScriptNodeTypeDescriptorUVE* found = registry.FindNodeTypeUVE("test.late");
    ASSERT_NE(found, nullptr);
    EXPECT_EQ(found->category, "FLOW");
    EXPECT_EQ(found->iconId, "node.branch");
    EXPECT_EQ(found->displayOrder, 20U);
    EXPECT_EQ(found->presentationFlags, kScriptNodePresentationFlagCollapsibleUVE);
    ASSERT_EQ(found->pins.size(), 1U);
    EXPECT_EQ(found->pins[0].defaultValue, std::optional<std::string>("0.016"));

    const std::vector<ScriptNodeTypeDescriptorUVE> ordered = registry.GetNodeTypeDescriptorsUVE();
    ASSERT_EQ(ordered.size(), 2U);
    EXPECT_EQ(ordered[0].typeId, "test.early");
    EXPECT_EQ(ordered[1].typeId, "test.late");
    EXPECT_EQ(ordered[0].pins[0].role, ScriptPinRoleUVE::Execution);
}

TEST(ScriptNodeRegistryUVETest, DescriptorV2_RejectsExecutionDefaultValues) {
    ScriptNodeRegistryUVE registry;
    EXPECT_FALSE(registry.RegisterNodeTypeUVE({
        "test.invalid-exec-default", "Invalid", {{"Exec", ScriptPinDirectionUVE::Output,
                                                     ScriptValueTypeUVE::Execution, ScriptPinRoleUVE::Execution,
                                                     std::string("true")}}}));
}

TEST(ScriptGraphUVETest, AddNodeUVE_RejectsEmptyTypeAndDuplicateIdsWithoutMutation) {
    ScriptGraphUVE graph;
    EXPECT_FALSE(graph.AddNodeUVE({1U, ""}));
    EXPECT_TRUE(graph.AddNodeUVE({1U, "test.source"}));
    EXPECT_FALSE(graph.AddNodeUVE({1U, "test.sink"}));
    EXPECT_EQ(graph.GetNodesUVE().size(), 1U);
}

TEST(ScriptGraphUVETest, AddLinkUVE_RejectsEmptyEndpointsAndDuplicateLinks) {
    ScriptGraphUVE graph;
    EXPECT_FALSE(graph.AddLinkUVE({{0U, "Out"}, {2U, "In"}}));
    EXPECT_FALSE(graph.AddLinkUVE({{1U, ""}, {2U, "In"}}));
    const ScriptLinkUVE link{{1U, "Out"}, {2U, "In"}};
    EXPECT_TRUE(graph.AddLinkUVE(link));
    EXPECT_FALSE(graph.AddLinkUVE(link));
    EXPECT_EQ(graph.GetLinksUVE().size(), 1U);
}

TEST(ScriptGraphUVETest, ValidateUVE_ValidTypedOutputToInputHasNoDiagnostics) {
    ScriptNodeRegistryUVE registry;
    RegisterTestNodesUVE(registry);
    ScriptGraphUVE graph;
    ASSERT_TRUE(graph.AddNodeUVE({1U, "test.source"}));
    ASSERT_TRUE(graph.AddNodeUVE({2U, "test.sink"}));
    ASSERT_TRUE(graph.AddLinkUVE({{1U, "Out"}, {2U, "In"}}));
    EXPECT_TRUE(graph.ValidateUVE(registry).empty());
}

TEST(ScriptGraphUVETest, ValidateUVE_ReportsUnknownNodeTypeAndUnknownPins) {
    ScriptNodeRegistryUVE registry;
    RegisterTestNodesUVE(registry);
    ScriptGraphUVE graph;
    ASSERT_TRUE(graph.AddNodeUVE({1U, "missing.node"}));
    ASSERT_TRUE(graph.AddNodeUVE({2U, "test.sink"}));
    ASSERT_TRUE(graph.AddLinkUVE({{1U, "Out"}, {2U, "Missing"}}));
    const auto diagnostics = graph.ValidateUVE(registry);
    ASSERT_EQ(diagnostics.size(), 1U);
    EXPECT_EQ(diagnostics[0].code, ScriptValidationCodeUVE::UnknownNodeType);
}

TEST(ScriptGraphUVETest, ValidateUVE_ReportsWrongDirectionsAndIncompatibleTypes) {
    ScriptNodeRegistryUVE registry;
    RegisterTestNodesUVE(registry);
    ASSERT_TRUE(registry.RegisterNodeTypeUVE({"test.boolean-sink", "Boolean Sink",
                                              {{"In", ScriptPinDirectionUVE::Input, ScriptValueTypeUVE::Boolean}}}));
    ScriptGraphUVE graph;
    ASSERT_TRUE(graph.AddNodeUVE({1U, "test.source"}));
    ASSERT_TRUE(graph.AddNodeUVE({2U, "test.boolean-sink"}));
    ASSERT_TRUE(graph.AddNodeUVE({3U, "test.boolean-sink"}));
    ASSERT_TRUE(graph.AddLinkUVE({{2U, "In"}, {3U, "In"}}));
    ASSERT_TRUE(graph.AddLinkUVE({{1U, "Out"}, {2U, "In"}}));
    const auto diagnostics = graph.ValidateUVE(registry);
    ASSERT_EQ(diagnostics.size(), 2U);
    EXPECT_EQ(diagnostics[0].code, ScriptValidationCodeUVE::WrongPinDirection);
    EXPECT_EQ(diagnostics[0].sourceContext, "Node #2 / pin In");
    ASSERT_TRUE(diagnostics[0].relatedEndpoint.has_value());
    EXPECT_EQ(*diagnostics[0].relatedEndpoint, (ScriptPinEndpointUVE{3U, "In"}));
    EXPECT_EQ(diagnostics[1].code, ScriptValidationCodeUVE::IncompatiblePinTypes);
    EXPECT_EQ(diagnostics[1].sourceContext, "Node #2 / pin In");
    ASSERT_TRUE(diagnostics[1].relatedEndpoint.has_value());
    EXPECT_EQ(*diagnostics[1].relatedEndpoint, (ScriptPinEndpointUVE{1U, "Out"}));
}

TEST(ScriptGraphUVETest, ValidateUVE_ReportsSelfLinkAndMissingNodeDeterministically) {
    ScriptNodeRegistryUVE registry;
    RegisterTestNodesUVE(registry);
    ScriptGraphUVE graph;
    ASSERT_TRUE(graph.AddNodeUVE({1U, "test.source"}));
    ASSERT_TRUE(graph.AddLinkUVE({{1U, "Out"}, {1U, "Exec"}}));
    ASSERT_TRUE(graph.AddLinkUVE({{1U, "Out"}, {9U, "In"}}));
    const auto diagnostics = graph.ValidateUVE(registry);
    ASSERT_EQ(diagnostics.size(), 3U);
    EXPECT_EQ(diagnostics[0].code, ScriptValidationCodeUVE::SelfLink);
    ASSERT_TRUE(diagnostics[0].relatedEndpoint.has_value());
    EXPECT_EQ(*diagnostics[0].relatedEndpoint, (ScriptPinEndpointUVE{1U, "Exec"}));
    EXPECT_EQ(diagnostics[1].code, ScriptValidationCodeUVE::WrongPinDirection);
    ASSERT_TRUE(diagnostics[1].relatedEndpoint.has_value());
    EXPECT_EQ(*diagnostics[1].relatedEndpoint, (ScriptPinEndpointUVE{1U, "Out"}));
    EXPECT_EQ(diagnostics[2].code, ScriptValidationCodeUVE::EmptyLinkEndpoint);
    ASSERT_TRUE(diagnostics[2].relatedEndpoint.has_value());
    EXPECT_EQ(*diagnostics[2].relatedEndpoint, (ScriptPinEndpointUVE{1U, "Out"}));
}

TEST(ScriptPinCompatibilityUVETest, AreScriptPinTypesCompatibleUVE_RequiresExactTypes) {
    EXPECT_TRUE(AreScriptPinTypesCompatibleUVE(ScriptValueTypeUVE::Execution, ScriptValueTypeUVE::Execution));
    EXPECT_TRUE(AreScriptPinTypesCompatibleUVE(ScriptValueTypeUVE::Vector3, ScriptValueTypeUVE::Vector3));
    EXPECT_FALSE(AreScriptPinTypesCompatibleUVE(ScriptValueTypeUVE::Number, ScriptValueTypeUVE::Boolean));
    EXPECT_FALSE(AreScriptPinTypesCompatibleUVE(ScriptValueTypeUVE::Entity, ScriptValueTypeUVE::Asset));
}

} // namespace UVE::Scripting

namespace UVE::Scripting {

TEST(ScriptCompilerIRUVETest, CompileScriptGraphToIrUVE_SortsNodesAndLinksDeterministically) {
    ScriptNodeRegistryUVE registry;
    RegisterTestNodesUVE(registry);
    ScriptGraphUVE graph;
    ASSERT_TRUE(graph.AddNodeUVE({20U, "test.sink"}));
    ASSERT_TRUE(graph.AddNodeUVE({10U, "test.source"}));
    ASSERT_TRUE(graph.AddLinkUVE({{10U, "Out"}, {20U, "In"}}));

    const ScriptIrCompileResultUVE result = CompileScriptGraphToIrUVE(graph, registry);
    ASSERT_TRUE(result.IsSuccessUVE());
    ASSERT_TRUE(result.program.has_value());
    EXPECT_EQ(result.program->version, ScriptIrProgramUVE::kCurrentVersionUVE);
    ASSERT_EQ(result.program->instructions.size(), 3U);
    EXPECT_EQ(result.program->instructions[0].kind, ScriptIrInstructionKindUVE::ExecuteNode);
    EXPECT_EQ(result.program->instructions[0].sourceNodeId, 10U);
    EXPECT_EQ(result.program->instructions[1].sourceNodeId, 20U);
    EXPECT_EQ(result.program->instructions[2].kind, ScriptIrInstructionKindUVE::TransferValue);
    EXPECT_EQ(result.program->instructions[2].sourceNodeId, 10U);
    EXPECT_EQ(result.program->instructions[2].targetNodeId, 20U);
    EXPECT_EQ(result.program->sourceNodeIds, (std::vector<std::uint32_t>{10U, 20U, 10U}));
}

TEST(ScriptCompilerIRUVETest, CompileScriptGraphToIrUVE_BoundsDiagnosticPresentationFields) {
    ScriptNodeRegistryUVE registry;
    RegisterTestNodesUVE(registry);
    ScriptGraphUVE graph;
    const std::string longTypeId(900U, 'x');
    ASSERT_TRUE(graph.AddNodeUVE({1U, longTypeId}));

    const ScriptIrCompileResultUVE result = CompileScriptGraphToIrUVE(graph, registry);
    ASSERT_FALSE(result.IsSuccessUVE());
    ASSERT_EQ(result.diagnostics.size(), 1U);
    EXPECT_EQ(result.diagnostics[0].severity, ScriptDiagnosticSeverityUVE::Error);
    EXPECT_LE(result.diagnostics[0].message.size(), kMaximumScriptDiagnosticMessageBytesUVE);
    EXPECT_LE(result.diagnostics[0].sourceContext.size(), kMaximumScriptDiagnosticSourceContextBytesUVE);
}

TEST(ScriptCompilerIRUVETest, CompileScriptGraphToIrUVE_RejectsInvalidGraphWithoutPartialProgram) {
    ScriptNodeRegistryUVE registry;
    RegisterTestNodesUVE(registry);
    ScriptGraphUVE graph;
    ASSERT_TRUE(graph.AddNodeUVE({1U, "missing.node"}));

    const ScriptIrCompileResultUVE result = CompileScriptGraphToIrUVE(graph, registry);
    EXPECT_FALSE(result.IsSuccessUVE());
    EXPECT_FALSE(result.program.has_value());
    ASSERT_EQ(result.diagnostics.size(), 1U);
    EXPECT_EQ(result.diagnostics[0].code, ScriptValidationCodeUVE::UnknownNodeType);
    EXPECT_EQ(result.diagnostics[0].severity, ScriptDiagnosticSeverityUVE::Error);
    EXPECT_EQ(result.diagnostics[0].sourceContext, "Node #1");
    EXPECT_EQ(result.diagnostics[0].message, "Node type is not registered: missing.node");
}

} // namespace UVE::Scripting


namespace UVE::Scripting {

TEST(ScriptGraphRuntimeBindingUVETest, BindUVE_CompilesLowersAndAttachesValidatedGraph) {
    ScriptNodeRegistryUVE registry;
    RegisterTestNodesUVE(registry);
    ScriptGraphUVE graph;
    ASSERT_TRUE(graph.AddNodeUVE({10U, "test.source"}));
    ASSERT_TRUE(graph.AddNodeUVE({20U, "test.sink"}));
    ASSERT_TRUE(graph.AddLinkUVE({{10U, "Out"}, {20U, "In"}}));
    ScriptRuntimeUVE runtime;

    const ScriptGraphRuntimeBindingResultUVE result =
        ScriptGraphRuntimeBindingUVE::BindUVE(graph, registry, runtime, {7U, 3U});

    ASSERT_TRUE(result.IsAcceptedUVE());
    EXPECT_EQ(result.runtimeCode, ScriptRuntimeAttachCodeUVE::Accepted);
    EXPECT_TRUE(result.compileDiagnostics.empty());
    EXPECT_TRUE(result.bytecodeDiagnostics.empty());
    EXPECT_TRUE(runtime.HasInstanceUVE({7U, 3U}));
    const auto snapshots = runtime.GetSnapshotUVE();
    ASSERT_EQ(snapshots.size(), 1U);
    EXPECT_EQ(snapshots[0].instructionCount, 3U);
}

TEST(ScriptGraphRuntimeBindingUVETest, BindUVE_RejectsBeforeRuntimeMutationWhenGraphCompileFails) {
    ScriptNodeRegistryUVE registry;
    RegisterTestNodesUVE(registry);
    ScriptGraphUVE graph;
    ASSERT_TRUE(graph.AddNodeUVE({1U, "missing.node"}));
    ScriptRuntimeUVE runtime;

    const ScriptGraphRuntimeBindingResultUVE result =
        ScriptGraphRuntimeBindingUVE::BindUVE(graph, registry, runtime, {8U, 1U});

    EXPECT_EQ(result.code, ScriptGraphRuntimeBindingCodeUVE::CompileRejected);
    ASSERT_EQ(result.compileDiagnostics.size(), 1U);
    EXPECT_EQ(result.compileDiagnostics[0].code, ScriptValidationCodeUVE::UnknownNodeType);
    EXPECT_EQ(runtime.GetInstanceCountUVE(), 0U);
}

TEST(ScriptGraphRuntimeBindingUVETest, BindUVE_RejectsDuplicateEntityWithoutOverwritingRuntimeState) {
    ScriptNodeRegistryUVE registry;
    RegisterTestNodesUVE(registry);
    ScriptGraphUVE graph;
    ASSERT_TRUE(graph.AddNodeUVE({1U, "test.source"}));
    ScriptRuntimeUVE runtime;
    ASSERT_TRUE(runtime.AttachUVE({9U, 2U}, ScriptBytecodeProgramUVE{}));

    const ScriptGraphRuntimeBindingResultUVE result =
        ScriptGraphRuntimeBindingUVE::BindUVE(graph, registry, runtime, {9U, 2U});

    EXPECT_EQ(result.code, ScriptGraphRuntimeBindingCodeUVE::RuntimeRejected);
    EXPECT_EQ(result.runtimeCode, ScriptRuntimeAttachCodeUVE::DuplicateInstance);
    EXPECT_EQ(runtime.GetInstanceCountUVE(), 1U);
}

TEST(ScriptGraphRuntimeBindingUVETest, BindUVE_RejectsInvalidEntityWithoutCompilationOrRuntimeMutation) {
    ScriptNodeRegistryUVE registry;
    RegisterTestNodesUVE(registry);
    ScriptGraphUVE graph;
    ASSERT_TRUE(graph.AddNodeUVE({1U, "test.source"}));
    ScriptRuntimeUVE runtime;

    const ScriptGraphRuntimeBindingResultUVE result =
        ScriptGraphRuntimeBindingUVE::BindUVE(graph, registry, runtime, Scene::kInvalidEntityUVE);

    EXPECT_EQ(result.code, ScriptGraphRuntimeBindingCodeUVE::InvalidEntity);
    EXPECT_TRUE(result.compileDiagnostics.empty());
    EXPECT_EQ(runtime.GetInstanceCountUVE(), 0U);
}

TEST(ScriptComponentRuntimeOwnershipUVETest, ReconcileUVE_AttachesValidatedPathThroughGraphBinding) {
    ScriptNodeRegistryUVE registry;
    RegisterTestNodesUVE(registry);
    ScriptGraphUVE graph;
    ASSERT_TRUE(graph.AddNodeUVE({1U, "test.source"}));
    ScriptRuntimeUVE runtime;

    const ScriptComponentRuntimeOwnershipResultUVE result =
        ScriptComponentRuntimeOwnershipUVE::ReconcileUVE(
            Scene::ScriptComponentUVE{"scripts/player.uvescript"}, graph, registry, runtime, {8U, 1U});

    EXPECT_EQ(result.code, ScriptComponentRuntimeOwnershipCodeUVE::Attached);
    EXPECT_TRUE(result.IsAcceptedUVE());
    EXPECT_EQ(runtime.GetInstanceCountUVE(), 1U);
}

TEST(ScriptComponentRuntimeOwnershipUVETest, ReconcileUVE_EmptyPathDetachesIdempotently) {
    ScriptRuntimeUVE runtime;
    ASSERT_TRUE(runtime.AttachUVE({9U, 1U}, ScriptBytecodeProgramUVE{}));
    ScriptNodeRegistryUVE registry;
    ScriptGraphUVE graph;

    const ScriptComponentRuntimeOwnershipResultUVE detached =
        ScriptComponentRuntimeOwnershipUVE::ReconcileUVE(
            Scene::ScriptComponentUVE{}, graph, registry, runtime, {9U, 1U});
    EXPECT_EQ(detached.code, ScriptComponentRuntimeOwnershipCodeUVE::Detached);
    EXPECT_EQ(runtime.GetInstanceCountUVE(), 0U);

    const ScriptComponentRuntimeOwnershipResultUVE alreadyDetached =
        ScriptComponentRuntimeOwnershipUVE::ReconcileUVE(
            Scene::ScriptComponentUVE{}, graph, registry, runtime, {9U, 1U});
    EXPECT_EQ(alreadyDetached.code, ScriptComponentRuntimeOwnershipCodeUVE::Detached);
    EXPECT_EQ(runtime.GetInstanceCountUVE(), 0U);
}

TEST(ScriptComponentRuntimeOwnershipUVETest, ReconcileUVE_RejectsInvalidPathAndGraphWithoutRuntimeMutation) {
    ScriptNodeRegistryUVE registry;
    RegisterTestNodesUVE(registry);
    ScriptGraphUVE invalidGraph;
    ASSERT_TRUE(invalidGraph.AddNodeUVE({1U, "unknown.node"}));
    ScriptRuntimeUVE runtime;
    const Scene::EntityUVE entity{10U, 1U};

    const ScriptComponentRuntimeOwnershipResultUVE invalidPath =
        ScriptComponentRuntimeOwnershipUVE::ReconcileUVE(
            Scene::ScriptComponentUVE{"../outside.uvescript"}, invalidGraph, registry, runtime, entity);
    EXPECT_EQ(invalidPath.code, ScriptComponentRuntimeOwnershipCodeUVE::InvalidComponent);
    EXPECT_EQ(runtime.GetInstanceCountUVE(), 0U);

    const ScriptComponentRuntimeOwnershipResultUVE rejectedGraph =
        ScriptComponentRuntimeOwnershipUVE::ReconcileUVE(
            Scene::ScriptComponentUVE{"scripts/player.uvescript"}, invalidGraph, registry, runtime, entity);
    EXPECT_EQ(rejectedGraph.code, ScriptComponentRuntimeOwnershipCodeUVE::GraphRejected);
    EXPECT_EQ(runtime.GetInstanceCountUVE(), 0U);
}

TEST(ScriptComponentRuntimeOwnershipUVETest, ReconcileUVE_RejectsReplacementWhileRuntimeIsActive) {
    ScriptNodeRegistryUVE registry;
    RegisterTestNodesUVE(registry);
    ScriptGraphUVE graph;
    ASSERT_TRUE(graph.AddNodeUVE({1U, "test.source"}));
    ScriptRuntimeUVE runtime;
    const Scene::EntityUVE entity{11U, 1U};
    ASSERT_TRUE(runtime.AttachUVE(entity, ScriptBytecodeProgramUVE{}));

    const ScriptComponentRuntimeOwnershipResultUVE result =
        ScriptComponentRuntimeOwnershipUVE::ReconcileUVE(
            Scene::ScriptComponentUVE{"scripts/replacement.uvescript"}, graph, registry, runtime, entity);

    EXPECT_EQ(result.code, ScriptComponentRuntimeOwnershipCodeUVE::DuplicateRuntime);
    EXPECT_EQ(runtime.GetInstanceCountUVE(), 1U);
}

TEST(ScriptBytecodeUVETest, LowerIrToBytecodeUVE_EnforcesInstructionCountCap) {
    ScriptIrProgramUVE boundary;
    boundary.instructions.resize(ScriptIrProgramUVE::kMaximumInstructionsUVE);
    std::vector<ScriptBytecodeDiagnosticUVE> boundaryDiagnostics;
    const std::optional<ScriptBytecodeProgramUVE> accepted =
        LowerIrToBytecodeUVE(boundary, boundaryDiagnostics);
    ASSERT_TRUE(accepted.has_value());
    EXPECT_TRUE(boundaryDiagnostics.empty());

    ScriptIrProgramUVE oversized;
    oversized.instructions.resize(ScriptIrProgramUVE::kMaximumInstructionsUVE + 1U);
    std::vector<ScriptBytecodeDiagnosticUVE> oversizedDiagnostics;
    const std::optional<ScriptBytecodeProgramUVE> rejected =
        LowerIrToBytecodeUVE(oversized, oversizedDiagnostics);
    EXPECT_FALSE(rejected.has_value());
    ASSERT_EQ(oversizedDiagnostics.size(), 1U);
    EXPECT_EQ(oversizedDiagnostics.front().code, ScriptBytecodeDiagnosticCodeUVE::InstructionLimitExceeded);
    EXPECT_EQ(oversizedDiagnostics.front().offset, 0U);
    EXPECT_EQ(oversizedDiagnostics.front().message, "IR instruction count exceeds the maximum of 256.");
}

TEST(ScriptBytecodeUVETest, EncodeDecodeScriptBytecodeUVE_RoundTripsVersionedProgram) {
    ScriptBytecodeProgramUVE program;
    program.instructions.push_back({ScriptIrInstructionKindUVE::ExecuteNode, 4U, 0U, "test.source", {}, {}});
    program.instructions.push_back({ScriptIrInstructionKindUVE::TransferValue, 4U, 9U, {}, "Out", "In"});
    std::vector<ScriptBytecodeDiagnosticUVE> diagnostics;
    const std::vector<std::uint8_t> bytes = EncodeScriptBytecodeUVE(program, diagnostics);
    ASSERT_TRUE(diagnostics.empty());
    const ScriptBytecodeDecodeResultUVE decoded = DecodeScriptBytecodeUVE(bytes);
    ASSERT_TRUE(decoded.IsSuccessUVE());
    ASSERT_EQ(decoded.program->instructions.size(), 2U);
    EXPECT_EQ(decoded.program->version, ScriptBytecodeProgramUVE::kCurrentVersionUVE);
    EXPECT_EQ(decoded.program->instructions[0].nodeTypeId, "test.source");
    EXPECT_EQ(decoded.program->instructions[1].sourcePinName, "Out");
}

TEST(ScriptBytecodeUVETest, LegacyV1DataOnlyBytecode_DecodesAndExecutes) {
    ScriptBytecodeProgramUVE program;
    program.instructions.push_back({ScriptIrInstructionKindUVE::ExecuteNode, 4U, 0U, "test.source", {}, {}});
    std::vector<ScriptBytecodeDiagnosticUVE> diagnostics;
    std::vector<std::uint8_t> bytes = EncodeScriptBytecodeUVE(program, diagnostics);
    ASSERT_TRUE(diagnostics.empty());
    bytes[4U] = static_cast<std::uint8_t>(ScriptBytecodeProgramUVE::kLegacyVersionUVE);
    bytes[5U] = 0U;
    bytes[6U] = 0U;
    bytes[7U] = 0U;
    bytes.pop_back(); // v1 has no per-instruction staged-transfer metadata.

    const ScriptBytecodeDecodeResultUVE decoded = DecodeScriptBytecodeUVE(bytes);
    ASSERT_TRUE(decoded.IsSuccessUVE());
    EXPECT_EQ(decoded.program->version, ScriptBytecodeProgramUVE::kLegacyVersionUVE);
    const ScriptVmExecutionResultUVE execution = ExecuteScriptBytecodeUVE(*decoded.program);
    EXPECT_TRUE(execution.IsSuccessUVE());
    EXPECT_EQ(execution.instructionsExecuted, 1U);
}

TEST(ScriptBytecodeUVETest, ConditionalJumpV2_RoundTripsTargetsAndRejectsLegacyEncoding) {
    ScriptBytecodeProgramUVE program;
    program.instructions.push_back({ScriptIrInstructionKindUVE::ConditionalJump, 7U, 0U, "flow.branch",
                                    "Condition", {}, 1U, 0U});
    std::vector<ScriptBytecodeDiagnosticUVE> diagnostics;
    const std::vector<std::uint8_t> bytes = EncodeScriptBytecodeUVE(program, diagnostics);
    ASSERT_TRUE(diagnostics.empty());
    const ScriptBytecodeDecodeResultUVE decoded = DecodeScriptBytecodeUVE(bytes);
    ASSERT_TRUE(decoded.IsSuccessUVE());
    ASSERT_EQ(decoded.program->instructions.size(), 1U);
    EXPECT_EQ(decoded.program->instructions.front().kind, ScriptIrInstructionKindUVE::ConditionalJump);
    EXPECT_EQ(decoded.program->instructions.front().trueTargetInstructionIndex, 1U);
    EXPECT_EQ(decoded.program->instructions.front().falseTargetInstructionIndex, 0U);

    std::vector<std::uint8_t> legacyBytes = bytes;
    legacyBytes[4U] = static_cast<std::uint8_t>(ScriptBytecodeProgramUVE::kLegacyVersionUVE);
    legacyBytes[5U] = 0U;
    legacyBytes[6U] = 0U;
    legacyBytes[7U] = 0U;
    const ScriptBytecodeDecodeResultUVE legacy = DecodeScriptBytecodeUVE(legacyBytes);
    EXPECT_FALSE(legacy.IsSuccessUVE());
    ASSERT_EQ(legacy.diagnostics.size(), 1U);
    EXPECT_EQ(legacy.diagnostics.front().code, ScriptBytecodeDiagnosticCodeUVE::InvalidInstruction);
}

TEST(ScriptBytecodeUVETest, SequenceDispatchV3_RoundTripsOrderedTargets) {
    ScriptBytecodeProgramUVE program;
    program.instructions.push_back({ScriptIrInstructionKindUVE::SequenceDispatch, 7U, 0U, "flow.sequence",
                                    "Then", "Then2", 0U, 0U, 1U, 0U});
    std::vector<ScriptBytecodeDiagnosticUVE> diagnostics;
    const std::vector<std::uint8_t> bytes = EncodeScriptBytecodeUVE(program, diagnostics);
    ASSERT_TRUE(diagnostics.empty());
    const ScriptBytecodeDecodeResultUVE decoded = DecodeScriptBytecodeUVE(bytes);
    ASSERT_TRUE(decoded.IsSuccessUVE());
    ASSERT_EQ(decoded.program->instructions.size(), 1U);
    EXPECT_EQ(decoded.program->version, ScriptBytecodeProgramUVE::kCurrentVersionUVE);
    EXPECT_EQ(decoded.program->instructions.front().kind, ScriptIrInstructionKindUVE::SequenceDispatch);
    EXPECT_EQ(decoded.program->instructions.front().firstTargetInstructionIndex, 1U);
    EXPECT_EQ(decoded.program->instructions.front().secondTargetInstructionIndex, 0U);
}

TEST(ScriptBytecodeUVETest, FlowControlDispatchV5_RoundTripsTargetsAndRejectsLegacyEncoding) {
    ScriptBytecodeProgramUVE program;
    program.instructions.push_back({ScriptIrInstructionKindUVE::FlowControlDispatch, 7U, 0U, "flow.switch",
                                    "In", {}, 1U, 2U, 0U, 0U, false, 3U});
    program.instructions.push_back({ScriptIrInstructionKindUVE::ExecuteNode, 8U, 0U, "test.case0", {}, {}});
    program.instructions.push_back({ScriptIrInstructionKindUVE::ExecuteNode, 9U, 0U, "test.case1", {}, {}});
    program.instructions.push_back({ScriptIrInstructionKindUVE::ExecuteNode, 10U, 0U, "test.default", {}, {}});
    std::vector<ScriptBytecodeDiagnosticUVE> diagnostics;
    const std::vector<std::uint8_t> bytes = EncodeScriptBytecodeUVE(program, diagnostics);
    ASSERT_TRUE(diagnostics.empty());
    const ScriptBytecodeDecodeResultUVE decoded = DecodeScriptBytecodeUVE(bytes);
    ASSERT_TRUE(decoded.IsSuccessUVE());
    ASSERT_EQ(decoded.program->version, ScriptBytecodeProgramUVE::kFlowControlDispatchVersionUVE);
    ASSERT_EQ(decoded.program->instructions.size(), 4U);
    EXPECT_EQ(decoded.program->instructions.front().kind, ScriptIrInstructionKindUVE::FlowControlDispatch);
    EXPECT_EQ(decoded.program->instructions.front().trueTargetInstructionIndex, 1U);
    EXPECT_EQ(decoded.program->instructions.front().falseTargetInstructionIndex, 2U);
    EXPECT_EQ(decoded.program->instructions.front().defaultTargetInstructionIndex, 3U);

    std::vector<std::uint8_t> legacyBytes = bytes;
    legacyBytes[4U] = static_cast<std::uint8_t>(ScriptBytecodeProgramUVE::kStagedTransferVersionUVE);
    legacyBytes[5U] = 0U;
    legacyBytes[6U] = 0U;
    legacyBytes[7U] = 0U;
    const ScriptBytecodeDecodeResultUVE legacy = DecodeScriptBytecodeUVE(legacyBytes);
    EXPECT_FALSE(legacy.IsSuccessUVE());
    ASSERT_EQ(legacy.diagnostics.size(), 1U);
    EXPECT_EQ(legacy.diagnostics.front().code, ScriptBytecodeDiagnosticCodeUVE::InvalidInstruction);
}

TEST(ScriptBytecodeUVETest, EncodeScriptBytecodeUVE_RejectsOutOfRangeConditionalJumpTarget) {
    ScriptBytecodeProgramUVE program;
    program.instructions.push_back({ScriptIrInstructionKindUVE::ConditionalJump, 1U, 0U, "flow.branch",
                                    "Condition", {}, 2U, 0U});
    std::vector<ScriptBytecodeDiagnosticUVE> diagnostics;
    EXPECT_TRUE(EncodeScriptBytecodeUVE(program, diagnostics).empty());
    ASSERT_EQ(diagnostics.size(), 1U);
    EXPECT_EQ(diagnostics.front().code, ScriptBytecodeDiagnosticCodeUVE::InvalidInstruction);
}

TEST(ScriptBytecodeUVETest, DecodeScriptBytecodeUVE_RejectsCorruptHeadersAndTruncation) {
    const ScriptBytecodeDecodeResultUVE badMagic = DecodeScriptBytecodeUVE({0U, 1U, 2U, 3U});
    ASSERT_FALSE(badMagic.IsSuccessUVE());
    ASSERT_EQ(badMagic.diagnostics.size(), 1U);
    EXPECT_EQ(badMagic.diagnostics[0].code, ScriptBytecodeDiagnosticCodeUVE::InvalidMagic);
    const ScriptBytecodeDecodeResultUVE truncated = DecodeScriptBytecodeUVE({'U', 'V', 'E', 'S', 1U, 0U, 0U, 0U});
    ASSERT_FALSE(truncated.IsSuccessUVE());
    EXPECT_EQ(truncated.diagnostics[0].code, ScriptBytecodeDiagnosticCodeUVE::Truncated);
}

TEST(ScriptBytecodeUVETest, EncodeScriptBytecodeUVE_RejectsInstructionLimit) {
    ScriptBytecodeProgramUVE program;
    program.instructions.resize(ScriptBytecodeProgramUVE::kMaximumInstructionsUVE + 1U);
    std::vector<ScriptBytecodeDiagnosticUVE> diagnostics;
    EXPECT_TRUE(EncodeScriptBytecodeUVE(program, diagnostics).empty());
    ASSERT_EQ(diagnostics.size(), 1U);
    EXPECT_EQ(diagnostics[0].code, ScriptBytecodeDiagnosticCodeUVE::InstructionLimitExceeded);
}

} // namespace UVE::Scripting


namespace UVE::Scripting {

TEST(ScriptVmUVETest, ExecuteScriptBytecodeUVE_CompletesValidProgramWithinBudget) {
    ScriptBytecodeProgramUVE program;
    program.instructions.push_back({ScriptIrInstructionKindUVE::ExecuteNode, 1U, 0U, "test.source", {}, {}});
    program.instructions.push_back({ScriptIrInstructionKindUVE::TransferValue, 1U, 2U, {}, "Out", "In"});
    const ScriptVmExecutionResultUVE result = ExecuteScriptBytecodeUVE(program, {2U});
    EXPECT_TRUE(result.IsSuccessUVE());
    EXPECT_EQ(result.instructionsExecuted, 2U);
}

TEST(ScriptVmUVETest, ExecuteScriptBytecodeUVE_DispatchesReturnDoOnceGateAndSwitch) {
    ScriptBytecodeProgramUVE returnProgram;
    returnProgram.instructions.push_back({ScriptIrInstructionKindUVE::FlowControlDispatch, 1U, 0U, "flow.return",
                                          "In", {}, 1U, 1U, 0U, 0U, false, 1U});
    returnProgram.instructions.push_back({ScriptIrInstructionKindUVE::ExecuteNode, 2U, 0U, "test.source", {}, {}});
    const ScriptVmExecutionResultUVE returnResult = ExecuteScriptBytecodeUVE(returnProgram);
    ASSERT_TRUE(returnResult.IsSuccessUVE());
    EXPECT_EQ(returnResult.instructionsExecuted, 1U);
    ASSERT_EQ(returnResult.trace.size(), 2U);
    EXPECT_EQ(returnResult.trace[0].message, "Return terminated execution.");
    EXPECT_EQ(returnResult.trace[1].kind, ScriptVmTraceEventKindUVE::Completed);

    ScriptBytecodeProgramUVE doOnceProgram;
    doOnceProgram.instructions.push_back({ScriptIrInstructionKindUVE::FlowControlDispatch, 10U, 0U, "flow.do_once",
                                          "In", {}, 1U, 3U, 0U, 0U, false, 2U});
    doOnceProgram.instructions.push_back({ScriptIrInstructionKindUVE::FlowControlDispatch, 11U, 0U, "flow.return",
                                          "In", {}, 3U, 3U, 0U, 0U, false, 3U});
    doOnceProgram.instructions.push_back({ScriptIrInstructionKindUVE::FlowControlDispatch, 12U, 0U, "flow.return",
                                          "In", {}, 3U, 3U, 0U, 0U, false, 3U});
    ScriptVmExecutionContextUVE doOnceContext;
    const ScriptVmExecutionResultUVE firstDoOnce = ExecuteScriptBytecodeUVE(doOnceProgram, doOnceContext);
    ASSERT_TRUE(firstDoOnce.IsSuccessUVE());
    EXPECT_EQ(firstDoOnce.instructionsExecuted, 2U);
    EXPECT_EQ(doOnceContext.FindDoOnceLatchUVE(10U), std::optional<bool>(true));
    const ScriptVmExecutionResultUVE repeatedDoOnce = ExecuteScriptBytecodeUVE(doOnceProgram, doOnceContext);
    ASSERT_TRUE(repeatedDoOnce.IsSuccessUVE());
    EXPECT_EQ(repeatedDoOnce.instructionsExecuted, 2U);
    EXPECT_EQ(repeatedDoOnce.trace[0].message, "Do Once suppressed a repeated execution.");
    ASSERT_TRUE(doOnceContext.ResetDoOnceLatchUVE(10U));
    const ScriptVmExecutionResultUVE resetDoOnce = ExecuteScriptBytecodeUVE(doOnceProgram, doOnceContext);
    ASSERT_TRUE(resetDoOnce.IsSuccessUVE());
    EXPECT_EQ(resetDoOnce.trace[0].message, "Do Once fired Then.");

    ScriptBytecodeProgramUVE gateProgram;
    gateProgram.instructions.push_back({ScriptIrInstructionKindUVE::FlowControlDispatch, 20U, 0U, "flow.gate",
                                        "In", {}, 1U, 2U, 0U, 0U, false, 2U});
    gateProgram.instructions.push_back({ScriptIrInstructionKindUVE::FlowControlDispatch, 21U, 0U, "flow.return",
                                        "In", {}, 3U, 3U, 0U, 0U, false, 3U});
    gateProgram.instructions.push_back({ScriptIrInstructionKindUVE::FlowControlDispatch, 22U, 0U, "flow.return",
                                        "In", {}, 3U, 3U, 0U, 0U, false, 3U});
    ScriptVmExecutionContextUVE gateContext;
    const ScriptVmExecutionResultUVE closedGate = ExecuteScriptBytecodeUVE(gateProgram, gateContext);
    ASSERT_TRUE(closedGate.IsSuccessUVE());
    EXPECT_EQ(closedGate.trace[0].message, "Gate suppressed a closed input.");
    ASSERT_TRUE(gateContext.SetGateStateUVE(20U, true));
    const ScriptVmExecutionResultUVE openGate = ExecuteScriptBytecodeUVE(gateProgram, gateContext);
    ASSERT_TRUE(openGate.IsSuccessUVE());
    EXPECT_EQ(openGate.trace[0].message, "Gate routed through Exit.");
    EXPECT_EQ(gateContext.FindGateStateUVE(20U), std::optional<bool>(true));

    ScriptBytecodeProgramUVE switchProgram;
    switchProgram.instructions.push_back({ScriptIrInstructionKindUVE::FlowControlDispatch, 30U, 0U, "flow.switch",
                                          "In", {}, 1U, 2U, 0U, 0U, false, 3U});
    switchProgram.instructions.push_back({ScriptIrInstructionKindUVE::FlowControlDispatch, 31U, 0U, "flow.return",
                                          "In", {}, 4U, 4U, 0U, 0U, false, 4U});
    switchProgram.instructions.push_back({ScriptIrInstructionKindUVE::FlowControlDispatch, 32U, 0U, "flow.return",
                                          "In", {}, 4U, 4U, 0U, 0U, false, 4U});
    switchProgram.instructions.push_back({ScriptIrInstructionKindUVE::FlowControlDispatch, 33U, 0U, "flow.return",
                                          "In", {}, 4U, 4U, 0U, 0U, false, 4U});
    ScriptVmExecutionContextUVE switchContext;
    ASSERT_TRUE(switchContext.SetInputUVE(30U, "Value", 0.0F));
    const ScriptVmExecutionResultUVE case0 = ExecuteScriptBytecodeUVE(switchProgram, switchContext);
    ASSERT_TRUE(case0.IsSuccessUVE());
    EXPECT_EQ(case0.trace[0].message, "Switch selected Case0.");
    ASSERT_TRUE(switchContext.SetInputUVE(30U, "Value", 1.0F));
    const ScriptVmExecutionResultUVE case1 = ExecuteScriptBytecodeUVE(switchProgram, switchContext);
    ASSERT_TRUE(case1.IsSuccessUVE());
    EXPECT_EQ(case1.trace[0].message, "Switch selected Case1.");
    switchContext.inputs.clear();
    const ScriptVmExecutionResultUVE defaultCase = ExecuteScriptBytecodeUVE(switchProgram, switchContext);
    ASSERT_TRUE(defaultCase.IsSuccessUVE());
    EXPECT_EQ(defaultCase.trace[0].message, "Switch selected Default because Value was unavailable or non-finite.");
}

TEST(ScriptCompilerIRUVETest, CompileScriptGraphToIrUVE_LowersRemainingFlowLoopFamily) {
    ScriptNodeRegistryUVE registry;
    ASSERT_TRUE(RegisterBuiltInScriptNodesUVE(registry));
    ScriptGraphUVE graph;
    ASSERT_TRUE(graph.AddNodeUVE({1U, "flow.event"}));
    ASSERT_TRUE(graph.AddNodeUVE({2U, "flow.loop"}));
    ASSERT_TRUE(graph.AddNodeUVE({3U, "flow.for_loop"}));
    ASSERT_TRUE(graph.AddNodeUVE({4U, "flow.while_loop"}));
    ASSERT_TRUE(graph.AddNodeUVE({5U, "flow.delay"}));

    const ScriptIrCompileResultUVE result = CompileScriptGraphToIrUVE(graph, registry);
    ASSERT_TRUE(result.IsSuccessUVE());
    ASSERT_EQ(result.program->instructions.size(), 5U);
    EXPECT_EQ(result.program->instructions[0].nodeTypeId, "flow.event");
    EXPECT_EQ(result.program->instructions[0].sourcePinName, "Event");
    EXPECT_EQ(result.program->instructions[1].nodeTypeId, "flow.loop");
    EXPECT_EQ(result.program->instructions[2].nodeTypeId, "flow.for_loop");
    EXPECT_EQ(result.program->instructions[3].nodeTypeId, "flow.while_loop");
    EXPECT_EQ(result.program->instructions[4].nodeTypeId, "flow.delay");
    for (const ScriptIrInstructionUVE& instruction : result.program->instructions) {
        EXPECT_EQ(instruction.kind, ScriptIrInstructionKindUVE::FlowControlDispatch);
    }
}

TEST(ScriptVmUVETest, ExecuteScriptBytecodeUVE_DispatchesEventLoopsAndFrameDelay) {
    ScriptBytecodeProgramUVE eventProgram;
    eventProgram.instructions.push_back({ScriptIrInstructionKindUVE::FlowControlDispatch, 1U, 0U, "flow.event",
                                          "Event", {}, 1U, 2U, 0U, 0U, false, 2U});
    eventProgram.instructions.push_back({ScriptIrInstructionKindUVE::FlowControlDispatch, 2U, 0U, "flow.return",
                                          "In", {}, 2U, 2U, 0U, 0U, false, 2U});
    ScriptVmExecutionContextUVE eventContext;
    const ScriptVmExecutionResultUVE eventResult = ExecuteScriptBytecodeUVE(eventProgram, eventContext);
    ASSERT_TRUE(eventResult.IsSuccessUVE());
    ASSERT_GE(eventResult.trace.size(), 2U);
    EXPECT_EQ(eventResult.trace[0].message, "Event fired Then.");

    ScriptBytecodeProgramUVE forProgram;
    forProgram.instructions.push_back({ScriptIrInstructionKindUVE::FlowControlDispatch, 10U, 0U, "flow.for_loop",
                                        "In", {}, 1U, 2U, 0U, 0U, false, 2U});
    forProgram.instructions.push_back({ScriptIrInstructionKindUVE::FlowControlDispatch, 11U, 0U, "flow.return",
                                        "In", {}, 2U, 2U, 0U, 0U, false, 2U});
    ScriptVmExecutionContextUVE forContext;
    ASSERT_TRUE(forContext.SetInputUVE(10U, "Count", 2.0F));
    const ScriptVmExecutionResultUVE firstFor = ExecuteScriptBytecodeUVE(forProgram, forContext);
    ASSERT_TRUE(firstFor.IsSuccessUVE());
    EXPECT_EQ(firstFor.trace[0].message, "For Loop dispatched Body.");
    EXPECT_EQ(forContext.FindOutputUVE(10U, "Index"), std::optional<ScriptVmValueUVE>(0.0F));
    const ScriptVmExecutionResultUVE secondFor = ExecuteScriptBytecodeUVE(forProgram, forContext);
    ASSERT_TRUE(secondFor.IsSuccessUVE());
    EXPECT_EQ(forContext.FindOutputUVE(10U, "Index"), std::optional<ScriptVmValueUVE>(1.0F));
    const ScriptVmExecutionResultUVE completedFor = ExecuteScriptBytecodeUVE(forProgram, forContext);
    ASSERT_TRUE(completedFor.IsSuccessUVE());
    EXPECT_EQ(completedFor.trace[0].message, "Loop completed.");
    EXPECT_EQ(forContext.FindLoopStateUVE(10U), std::optional<ScriptVmLoopStateUVE>(ScriptVmLoopStateUVE{10U, 0U, false}));

    ScriptBytecodeProgramUVE whileProgram;
    whileProgram.instructions.push_back({ScriptIrInstructionKindUVE::FlowControlDispatch, 20U, 0U, "flow.while_loop",
                                          "In", {}, 1U, 2U, 0U, 0U, false, 2U});
    whileProgram.instructions.push_back({ScriptIrInstructionKindUVE::FlowControlDispatch, 21U, 0U, "flow.return",
                                          "In", {}, 2U, 2U, 0U, 0U, false, 2U});
    ScriptVmExecutionContextUVE whileContext;
    ASSERT_TRUE(whileContext.SetInputUVE(20U, "Condition", false));
    const ScriptVmExecutionResultUVE whileResult = ExecuteScriptBytecodeUVE(whileProgram, whileContext);
    ASSERT_TRUE(whileResult.IsSuccessUVE());
    EXPECT_EQ(whileResult.trace[0].message, "While Loop completed because Condition was false.");

    ScriptBytecodeProgramUVE delayProgram;
    delayProgram.instructions.push_back({ScriptIrInstructionKindUVE::FlowControlDispatch, 30U, 0U, "flow.delay",
                                          "In", {}, 1U, 1U, 0U, 0U, false, 1U});
    delayProgram.instructions.push_back({ScriptIrInstructionKindUVE::FlowControlDispatch, 31U, 0U, "flow.return",
                                          "In", {}, 2U, 2U, 0U, 0U, false, 2U});
    ScriptVmExecutionContextUVE delayContext;
    ASSERT_TRUE(delayContext.SetInputUVE(30U, "Frames", 2.0F));
    const ScriptVmExecutionResultUVE firstDelay = ExecuteScriptBytecodeUVE(delayProgram, delayContext);
    ASSERT_TRUE(firstDelay.IsSuccessUVE());
    EXPECT_EQ(firstDelay.trace[0].message, "Delay yielded until the next runtime tick.");
    EXPECT_EQ(delayContext.FindDelayStateUVE(30U), std::optional<ScriptVmDelayStateUVE>(ScriptVmDelayStateUVE{30U, 2U, true}));
    const ScriptVmExecutionResultUVE secondDelay = ExecuteScriptBytecodeUVE(delayProgram, delayContext);
    ASSERT_TRUE(secondDelay.IsSuccessUVE());
    EXPECT_EQ(secondDelay.trace[0].message, "Delay yielded while its bounded frame state remained active.");
    const ScriptVmExecutionResultUVE thirdDelay = ExecuteScriptBytecodeUVE(delayProgram, delayContext);
    ASSERT_TRUE(thirdDelay.IsSuccessUVE());
    EXPECT_EQ(thirdDelay.trace[0].message, "Delay dispatched Then.");
    EXPECT_EQ(delayContext.FindDelayStateUVE(30U), std::optional<ScriptVmDelayStateUVE>(ScriptVmDelayStateUVE{30U, 0U, false}));
}

TEST(ScriptVmUVETest, ExecuteScriptBytecodeUVE_ConditionalJumpSelectsTrueAndFalseTargets) {
    ScriptBytecodeProgramUVE program;
    program.instructions.push_back({ScriptIrInstructionKindUVE::ConditionalJump, 1U, 0U, "flow.branch",
                                    "Condition", {}, 1U, 2U});
    program.instructions.push_back({ScriptIrInstructionKindUVE::ExecuteNode, 20U, 0U, "math.float.add", {}, {}});
    program.instructions.push_back({ScriptIrInstructionKindUVE::ExecuteNode, 30U, 0U, "math.float.subtract", {}, {}});

    ScriptVmExecutionContextUVE trueContext;
    ASSERT_TRUE(trueContext.SetInputUVE(1U, "Condition", true));
    ASSERT_TRUE(trueContext.SetInputUVE(20U, "A", 5.0F));
    ASSERT_TRUE(trueContext.SetInputUVE(20U, "B", 2.0F));
    ASSERT_TRUE(trueContext.SetInputUVE(30U, "A", 5.0F));
    ASSERT_TRUE(trueContext.SetInputUVE(30U, "B", 2.0F));
    const ScriptVmExecutionResultUVE trueResult = ExecuteScriptBytecodeUVE(program, trueContext);
    ASSERT_TRUE(trueResult.IsSuccessUVE());
    ASSERT_EQ(trueResult.trace.size(), 4U);
    EXPECT_EQ(trueResult.trace[0].message, "ConditionalJump evaluated true.");
    EXPECT_EQ(trueResult.trace[1].sourceNodeId, 20U);
    EXPECT_EQ(trueResult.trace[2].sourceNodeId, 30U);
    EXPECT_EQ(trueResult.trace[3].kind, ScriptVmTraceEventKindUVE::Completed);

    ScriptVmExecutionContextUVE falseContext = trueContext;
    ASSERT_TRUE(falseContext.SetInputUVE(1U, "Condition", false));
    const ScriptVmExecutionResultUVE falseResult = ExecuteScriptBytecodeUVE(program, falseContext);
    ASSERT_TRUE(falseResult.IsSuccessUVE());
    ASSERT_EQ(falseResult.trace.size(), 3U);
    EXPECT_EQ(falseResult.trace[0].message, "ConditionalJump evaluated false.");
    EXPECT_EQ(falseResult.trace[1].sourceNodeId, 30U);
    EXPECT_EQ(falseResult.trace[2].kind, ScriptVmTraceEventKindUVE::Completed);
}

TEST(ScriptVmUVETest, ExecuteScriptBytecodeUVE_SequenceDispatchExecutesOrderedDirectTargets) {
    ScriptBytecodeProgramUVE program;
    program.instructions.push_back({ScriptIrInstructionKindUVE::SequenceDispatch, 1U, 0U, "flow.sequence",
                                    "Then", "Then2", 0U, 0U, 1U, 2U});
    program.instructions.push_back({ScriptIrInstructionKindUVE::ExecuteNode, 20U, 0U, "math.float.add", {}, {}});
    program.instructions.push_back({ScriptIrInstructionKindUVE::ExecuteNode, 30U, 0U, "math.float.subtract", {}, {}});
    ScriptVmExecutionContextUVE context;
    ASSERT_TRUE(context.SetInputUVE(20U, "A", 5.0F));
    ASSERT_TRUE(context.SetInputUVE(20U, "B", 2.0F));
    ASSERT_TRUE(context.SetInputUVE(30U, "A", 5.0F));
    ASSERT_TRUE(context.SetInputUVE(30U, "B", 2.0F));

    const ScriptVmExecutionResultUVE result = ExecuteScriptBytecodeUVE(program, context);
    ASSERT_TRUE(result.IsSuccessUVE());
    EXPECT_EQ(result.instructionsExecuted, 3U);
    ASSERT_EQ(result.trace.size(), 4U);
    EXPECT_EQ(result.trace[0].message, "SequenceDispatch selected ordered execution targets.");
    EXPECT_EQ(result.trace[1].sourceNodeId, 20U);
    EXPECT_EQ(result.trace[2].sourceNodeId, 30U);
    EXPECT_EQ(result.trace[3].kind, ScriptVmTraceEventKindUVE::Completed);
}

TEST(ScriptVmUVETest, ExecuteScriptBytecodeUVE_SequenceDispatchSkipsMissingFirstOutput) {
    ScriptBytecodeProgramUVE program;
    program.instructions.push_back({ScriptIrInstructionKindUVE::SequenceDispatch, 1U, 0U, "flow.sequence",
                                    "Then", "Then2", 0U, 0U, 2U, 1U});
    program.instructions.push_back({ScriptIrInstructionKindUVE::ExecuteNode, 30U, 0U, "math.float.add", {}, {}});
    ScriptVmExecutionContextUVE context;
    ASSERT_TRUE(context.SetInputUVE(30U, "A", 5.0F));
    ASSERT_TRUE(context.SetInputUVE(30U, "B", 2.0F));

    const ScriptVmExecutionResultUVE result = ExecuteScriptBytecodeUVE(program, context);
    ASSERT_TRUE(result.IsSuccessUVE());
    EXPECT_EQ(result.instructionsExecuted, 2U);
    ASSERT_EQ(result.trace.size(), 3U);
    EXPECT_EQ(result.trace[1].sourceNodeId, 30U);
    EXPECT_EQ(result.trace[2].kind, ScriptVmTraceEventKindUVE::Completed);
}

TEST(ScriptVmUVETest, ExecuteScriptBytecodeUVE_SequenceDispatchSelfLoopStopsAtBudget) {
    ScriptBytecodeProgramUVE program;
    program.instructions.push_back({ScriptIrInstructionKindUVE::SequenceDispatch, 1U, 0U, "flow.sequence",
                                    "Then", "Then2", 0U, 0U, 0U, 0U});
    ScriptVmExecutionContextUVE context;
    const ScriptVmExecutionResultUVE result = ExecuteScriptBytecodeUVE(program, context, {2U});
    EXPECT_EQ(result.status, ScriptVmStatusUVE::InstructionBudgetExceeded);
    EXPECT_EQ(result.instructionsExecuted, 2U);
    ASSERT_EQ(result.trace.size(), 3U);
    EXPECT_EQ(result.trace.back().kind, ScriptVmTraceEventKindUVE::Failed);
}

TEST(ScriptVmUVETest, ExecuteScriptBytecodeUVE_ConditionalJumpRequiresBooleanCondition) {
    ScriptBytecodeProgramUVE program;
    program.instructions.push_back({ScriptIrInstructionKindUVE::ConditionalJump, 1U, 0U, "flow.branch",
                                    "Condition", {}, 1U, 1U});
    ScriptVmExecutionContextUVE context;
    const ScriptVmExecutionResultUVE result = ExecuteScriptBytecodeUVE(program, context);
    EXPECT_EQ(result.status, ScriptVmStatusUVE::NodeExecutionFailed);
    ASSERT_EQ(result.trace.size(), 1U);
    EXPECT_EQ(result.trace.front().kind, ScriptVmTraceEventKindUVE::Failed);
    EXPECT_EQ(result.trace.front().message, "ConditionalJump requires a Boolean condition input.");
}

TEST(ScriptVmUVETest, ExecuteScriptBytecodeUVE_ConditionalJumpSelfLoopStopsAtBudget) {
    ScriptBytecodeProgramUVE program;
    program.instructions.push_back({ScriptIrInstructionKindUVE::ConditionalJump, 1U, 0U, "flow.branch",
                                    "Condition", {}, 0U, 0U});
    ScriptVmExecutionContextUVE context;
    ASSERT_TRUE(context.SetInputUVE(1U, "Condition", true));
    const ScriptVmExecutionResultUVE result = ExecuteScriptBytecodeUVE(program, context, {3U});
    EXPECT_EQ(result.status, ScriptVmStatusUVE::InstructionBudgetExceeded);
    EXPECT_EQ(result.instructionsExecuted, 3U);
    ASSERT_EQ(result.trace.size(), 4U);
    EXPECT_EQ(result.trace.back().kind, ScriptVmTraceEventKindUVE::Failed);
    EXPECT_TRUE(result.trace.back().message.find("Instruction budget") != std::string::npos);
}

TEST(ScriptVmUVETest, ExecuteScriptBytecodeUVE_ConditionalJumpRejectsInvalidTarget) {
    ScriptBytecodeProgramUVE program;
    program.instructions.push_back({ScriptIrInstructionKindUVE::ConditionalJump, 1U, 0U, "flow.branch",
                                    "Condition", {}, 2U, 0U});
    ScriptVmExecutionContextUVE context;
    ASSERT_TRUE(context.SetInputUVE(1U, "Condition", true));
    const ScriptVmExecutionResultUVE result = ExecuteScriptBytecodeUVE(program, context);
    EXPECT_EQ(result.status, ScriptVmStatusUVE::NodeExecutionFailed);
    ASSERT_EQ(result.trace.size(), 1U);
    EXPECT_EQ(result.trace.front().kind, ScriptVmTraceEventKindUVE::Failed);
    EXPECT_TRUE(result.trace.front().message.find("outside") != std::string::npos);
}

TEST(ScriptVmUVETest, ExecuteScriptBytecodeUVE_CapturesNodeAndCompletionTraceInOrder) {
    ScriptBytecodeProgramUVE program;
    program.instructions.push_back(
        {ScriptIrInstructionKindUVE::ExecuteNode, 1U, 0U, "math.float.add", {}, {}});
    ScriptVmExecutionContextUVE context;
    ASSERT_TRUE(context.SetInputUVE(1U, "A", 2.0F));
    ASSERT_TRUE(context.SetInputUVE(1U, "B", 3.0F));

    const ScriptVmExecutionResultUVE result = ExecuteScriptBytecodeUVE(program, context);
    ASSERT_TRUE(result.IsSuccessUVE());
    ASSERT_EQ(result.trace.size(), 2U);
    EXPECT_EQ(result.trace[0].kind, ScriptVmTraceEventKindUVE::NodeExecuted);
    EXPECT_EQ(result.trace[0].instructionIndex, 0U);
    EXPECT_EQ(result.trace[0].sourceNodeId, 1U);
    EXPECT_EQ(result.trace[0].nodeTypeId, "math.float.add");
    EXPECT_EQ(result.trace[1].kind, ScriptVmTraceEventKindUVE::Completed);
    EXPECT_FALSE(result.traceTruncated);
}

TEST(ScriptVmUVETest, ExecuteScriptBytecodeUVE_CapturesTypedValueTransferTrace) {
    ScriptBytecodeProgramUVE program;
    program.instructions.push_back(
        {ScriptIrInstructionKindUVE::TransferValue, 4U, 9U, {}, "Result", "A"});
    ScriptVmExecutionContextUVE context;
    ASSERT_TRUE(context.SetOutputUVE(4U, "Result", ScriptVector3ValueUVE{{1.0F, 2.0F, 3.0F}}));

    const ScriptVmExecutionResultUVE result = ExecuteScriptBytecodeUVE(program, context);
    ASSERT_TRUE(result.IsSuccessUVE());
    ASSERT_EQ(result.trace.size(), 2U);
    EXPECT_EQ(result.trace[0].kind, ScriptVmTraceEventKindUVE::ValueTransferred);
    EXPECT_EQ(result.trace[0].sourceNodeId, 4U);
    EXPECT_EQ(result.trace[0].targetNodeId, 9U);
    EXPECT_EQ(result.trace[1].kind, ScriptVmTraceEventKindUVE::Completed);
}

TEST(ScriptVmUVETest, ExecuteScriptBytecodeUVE_CapturesFailureTraceWithDiagnosticMessage) {
    ScriptBytecodeProgramUVE program;
    program.instructions.push_back(
        {ScriptIrInstructionKindUVE::ExecuteNode, 12U, 0U, "math.vector3.normalize", {}, {}});
    ScriptVmExecutionContextUVE context;
    ASSERT_TRUE(context.SetInputUVE(12U, "Vector", ScriptVector3ValueUVE{{0.0F, 0.0F, 0.0F}}));

    const ScriptVmExecutionResultUVE result = ExecuteScriptBytecodeUVE(program, context);
    ASSERT_FALSE(result.IsSuccessUVE());
    ASSERT_EQ(result.trace.size(), 1U);
    EXPECT_EQ(result.trace.front().kind, ScriptVmTraceEventKindUVE::Failed);
    EXPECT_EQ(result.trace.front().instructionIndex, 0U);
    ASSERT_EQ(result.diagnostics.size(), 1U);
    EXPECT_EQ(result.trace.front().message, result.diagnostics.front().message);
}

TEST(ScriptVmUVETest, ExecuteScriptBytecodeUVE_BoundsTraceEventCount) {
    ScriptBytecodeProgramUVE program;
    program.instructions.resize(ScriptVmExecutionResultUVE::kMaximumTraceEventsUVE + 3U);

    const ScriptVmExecutionResultUVE result = ExecuteScriptBytecodeUVE(program);
    EXPECT_TRUE(result.IsSuccessUVE());
    EXPECT_EQ(result.trace.size(), ScriptVmExecutionResultUVE::kMaximumTraceEventsUVE);
    EXPECT_TRUE(result.traceTruncated);
}

TEST(ScriptVmUVETest, ExecuteScriptBytecodeUVE_DispatchesAllBuiltInVector3Nodes) {
    const auto makeProgram = [](const std::uint32_t nodeId, const char* nodeTypeId) {
        ScriptBytecodeProgramUVE program;
        program.instructions.push_back({ScriptIrInstructionKindUVE::ExecuteNode, nodeId, 0U, nodeTypeId, {}, {}});
        return program;
    };

    ScriptVmExecutionContextUVE makeContext;
    ASSERT_TRUE(makeContext.SetInputUVE(1U, "X", 1.0F));
    ASSERT_TRUE(makeContext.SetInputUVE(1U, "Y", -2.0F));
    ASSERT_TRUE(makeContext.SetInputUVE(1U, "Z", 3.0F));
    EXPECT_TRUE(ExecuteScriptBytecodeUVE(makeProgram(1U, "math.vector3.make"), makeContext).IsSuccessUVE());
    const auto makeOutput = makeContext.FindOutputUVE(1U, "Vector");
    ASSERT_TRUE(makeOutput.has_value());
    ASSERT_TRUE(std::holds_alternative<ScriptVector3ValueUVE>(*makeOutput));
    EXPECT_EQ(std::get<ScriptVector3ValueUVE>(*makeOutput), (ScriptVector3ValueUVE{{1.0F, -2.0F, 3.0F}}));

    ScriptVmExecutionContextUVE addContext;
    ASSERT_TRUE(addContext.SetInputUVE(2U, "A", ScriptVector3ValueUVE{{1.0F, 2.0F, 3.0F}}));
    ASSERT_TRUE(addContext.SetInputUVE(2U, "B", ScriptVector3ValueUVE{{4.0F, 5.0F, 6.0F}}));
    EXPECT_TRUE(ExecuteScriptBytecodeUVE(makeProgram(2U, "math.vector3.add"), addContext).IsSuccessUVE());
    EXPECT_EQ(std::get<ScriptVector3ValueUVE>(*addContext.FindOutputUVE(2U, "Result")),
              (ScriptVector3ValueUVE{{5.0F, 7.0F, 9.0F}}));

    ScriptVmExecutionContextUVE subtractContext;
    ASSERT_TRUE(subtractContext.SetInputUVE(3U, "A", ScriptVector3ValueUVE{{4.0F, 5.0F, 6.0F}}));
    ASSERT_TRUE(subtractContext.SetInputUVE(3U, "B", ScriptVector3ValueUVE{{1.0F, 2.0F, 3.0F}}));
    EXPECT_TRUE(ExecuteScriptBytecodeUVE(makeProgram(3U, "math.vector3.subtract"), subtractContext).IsSuccessUVE());
    EXPECT_EQ(std::get<ScriptVector3ValueUVE>(*subtractContext.FindOutputUVE(3U, "Result")),
              (ScriptVector3ValueUVE{{3.0F, 3.0F, 3.0F}}));

    ScriptVmExecutionContextUVE multiplyContext;
    ASSERT_TRUE(multiplyContext.SetInputUVE(4U, "Vector", ScriptVector3ValueUVE{{1.0F, -2.0F, 3.0F}}));
    ASSERT_TRUE(multiplyContext.SetInputUVE(4U, "Scale", 2.0F));
    EXPECT_TRUE(ExecuteScriptBytecodeUVE(makeProgram(4U, "math.vector3.multiply"), multiplyContext).IsSuccessUVE());
    EXPECT_EQ(std::get<ScriptVector3ValueUVE>(*multiplyContext.FindOutputUVE(4U, "Result")),
              (ScriptVector3ValueUVE{{2.0F, -4.0F, 6.0F}}));

    ScriptVmExecutionContextUVE dotContext;
    ASSERT_TRUE(dotContext.SetInputUVE(5U, "A", ScriptVector3ValueUVE{{1.0F, 2.0F, 3.0F}}));
    ASSERT_TRUE(dotContext.SetInputUVE(5U, "B", ScriptVector3ValueUVE{{4.0F, 5.0F, 6.0F}}));
    EXPECT_TRUE(ExecuteScriptBytecodeUVE(makeProgram(5U, "math.vector3.dot"), dotContext).IsSuccessUVE());
    EXPECT_FLOAT_EQ(std::get<float>(*dotContext.FindOutputUVE(5U, "Result")), 32.0F);

    ScriptVmExecutionContextUVE crossContext;
    ASSERT_TRUE(crossContext.SetInputUVE(6U, "A", ScriptVector3ValueUVE{{1.0F, 0.0F, 0.0F}}));
    ASSERT_TRUE(crossContext.SetInputUVE(6U, "B", ScriptVector3ValueUVE{{0.0F, 1.0F, 0.0F}}));
    EXPECT_TRUE(ExecuteScriptBytecodeUVE(makeProgram(6U, "math.vector3.cross"), crossContext).IsSuccessUVE());
    EXPECT_EQ(std::get<ScriptVector3ValueUVE>(*crossContext.FindOutputUVE(6U, "Result")),
              (ScriptVector3ValueUVE{{0.0F, 0.0F, 1.0F}}));

    ScriptVmExecutionContextUVE lengthContext;
    ASSERT_TRUE(lengthContext.SetInputUVE(7U, "Vector", ScriptVector3ValueUVE{{3.0F, 4.0F, 0.0F}}));
    EXPECT_TRUE(ExecuteScriptBytecodeUVE(makeProgram(7U, "math.vector3.length"), lengthContext).IsSuccessUVE());
    EXPECT_FLOAT_EQ(std::get<float>(*lengthContext.FindOutputUVE(7U, "Length")), 5.0F);

    ScriptVmExecutionContextUVE normalizeContext;
    ASSERT_TRUE(normalizeContext.SetInputUVE(8U, "Vector", ScriptVector3ValueUVE{{0.0F, 3.0F, 4.0F}}));
    EXPECT_TRUE(ExecuteScriptBytecodeUVE(makeProgram(8U, "math.vector3.normalize"), normalizeContext).IsSuccessUVE());
    EXPECT_EQ(std::get<ScriptVector3ValueUVE>(*normalizeContext.FindOutputUVE(8U, "Result")),
              (ScriptVector3ValueUVE{{0.0F, 0.6F, 0.8F}}));
}

TEST(ScriptVmUVETest, ExecuteScriptBytecodeUVE_DispatchesFloatAndBooleanNodes) {
    const auto makeProgram = [](const std::uint32_t nodeId, const char* nodeTypeId) {
        ScriptBytecodeProgramUVE program;
        program.instructions.push_back({ScriptIrInstructionKindUVE::ExecuteNode, nodeId, 0U, nodeTypeId, {}, {}});
        return program;
    };

    const auto runFloat = [&](const char* nodeTypeId, float lhs, float rhs) {
        ScriptVmExecutionContextUVE context;
        EXPECT_TRUE(context.SetInputUVE(10U, "A", lhs));
        EXPECT_TRUE(context.SetInputUVE(10U, "B", rhs));
        EXPECT_TRUE(ExecuteScriptBytecodeUVE(makeProgram(10U, nodeTypeId), context).IsSuccessUVE());
        const auto output = context.FindOutputUVE(10U, "Result");
        EXPECT_TRUE(output.has_value());
        if (!output.has_value() || !std::holds_alternative<float>(*output)) {
            return 0.0F;
        }
        return std::get<float>(*output);
    };
    EXPECT_FLOAT_EQ(runFloat("math.float.add", 2.0F, 3.0F), 5.0F);
    EXPECT_FLOAT_EQ(runFloat("math.float.subtract", 2.0F, 3.0F), -1.0F);
    EXPECT_FLOAT_EQ(runFloat("math.float.multiply", 2.0F, 3.0F), 6.0F);
    EXPECT_FLOAT_EQ(runFloat("math.float.divide", 6.0F, 3.0F), 2.0F);

    ScriptVmExecutionContextUVE divideByZeroContext;
    ASSERT_TRUE(divideByZeroContext.SetInputUVE(11U, "A", 1.0F));
    ASSERT_TRUE(divideByZeroContext.SetInputUVE(11U, "B", 0.0F));
    const ScriptVmExecutionResultUVE divideByZero =
        ExecuteScriptBytecodeUVE(makeProgram(11U, "math.float.divide"), divideByZeroContext);
    EXPECT_EQ(divideByZero.status, ScriptVmStatusUVE::NodeExecutionFailed);

    const auto runBoolean = [&](const char* nodeTypeId, bool lhs, bool rhs) {
        ScriptVmExecutionContextUVE context;
        EXPECT_TRUE(context.SetInputUVE(20U, "A", lhs));
        EXPECT_TRUE(context.SetInputUVE(20U, "B", rhs));
        EXPECT_TRUE(ExecuteScriptBytecodeUVE(makeProgram(20U, nodeTypeId), context).IsSuccessUVE());
        const auto output = context.FindOutputUVE(20U, "Result");
        EXPECT_TRUE(output.has_value());
        if (!output.has_value() || !std::holds_alternative<bool>(*output)) {
            return false;
        }
        return std::get<bool>(*output);
    };
    EXPECT_FALSE(runBoolean("logic.boolean.and", true, false));
    EXPECT_TRUE(runBoolean("logic.boolean.or", true, false));
    EXPECT_TRUE(runBoolean("logic.boolean.xor", true, false));

    ScriptVmExecutionContextUVE notContext;
    ASSERT_TRUE(notContext.SetInputUVE(21U, "Value", false));
    ASSERT_TRUE(ExecuteScriptBytecodeUVE(makeProgram(21U, "logic.boolean.not"), notContext).IsSuccessUVE());
    EXPECT_TRUE(std::get<bool>(*notContext.FindOutputUVE(21U, "Result")));
}

TEST(ScriptVmUVETest, ExecuteScriptBytecodeUVE_DispatchesScalarMathUtilityNodes) {
    const auto makeProgram = [](const std::uint32_t nodeId, const char* nodeTypeId) {
        ScriptBytecodeProgramUVE program;
        program.instructions.push_back({ScriptIrInstructionKindUVE::ExecuteNode, nodeId, 0U, nodeTypeId, {}, {}});
        return program;
    };
    const auto runBinary = [&](const char* nodeTypeId, const float lhs, const float rhs) {
        ScriptVmExecutionContextUVE context;
        EXPECT_TRUE(context.SetInputUVE(30U, "A", lhs));
        EXPECT_TRUE(context.SetInputUVE(30U, "B", rhs));
        const ScriptVmExecutionResultUVE result =
            ExecuteScriptBytecodeUVE(makeProgram(30U, nodeTypeId), context);
        EXPECT_TRUE(result.IsSuccessUVE());
        const auto output = context.FindOutputUVE(30U, "Result");
        EXPECT_TRUE(output.has_value());
        EXPECT_TRUE(output.has_value() && std::holds_alternative<float>(*output));
        return output.has_value() && std::holds_alternative<float>(*output) ?
                   std::get<float>(*output) : 0.0F;
    };
    EXPECT_FLOAT_EQ(runBinary("math.float.modulo", 7.0F, 3.0F), 1.0F);
    EXPECT_FLOAT_EQ(runBinary("math.float.modulo", -7.0F, 3.0F), -1.0F);
    EXPECT_FLOAT_EQ(runBinary("math.float.min", 2.0F, 5.0F), 2.0F);
    EXPECT_FLOAT_EQ(runBinary("math.float.max", 2.0F, 5.0F), 5.0F);
    EXPECT_FLOAT_EQ(runBinary("math.float.power", 2.0F, 3.0F), 8.0F);

    ScriptVmExecutionContextUVE absContext;
    ASSERT_TRUE(absContext.SetInputUVE(31U, "Value", -3.5F));
    ASSERT_TRUE(ExecuteScriptBytecodeUVE(makeProgram(31U, "math.float.abs"), absContext).IsSuccessUVE());
    EXPECT_FLOAT_EQ(std::get<float>(*absContext.FindOutputUVE(31U, "Result")), 3.5F);

    ScriptVmExecutionContextUVE clampContext;
    ASSERT_TRUE(clampContext.SetInputUVE(32U, "Value", 7.0F));
    ASSERT_TRUE(clampContext.SetInputUVE(32U, "Min", 0.0F));
    ASSERT_TRUE(clampContext.SetInputUVE(32U, "Max", 5.0F));
    ASSERT_TRUE(ExecuteScriptBytecodeUVE(makeProgram(32U, "math.float.clamp"), clampContext).IsSuccessUVE());
    EXPECT_FLOAT_EQ(std::get<float>(*clampContext.FindOutputUVE(32U, "Result")), 5.0F);

    ScriptVmExecutionContextUVE moduloByZeroContext;
    ASSERT_TRUE(moduloByZeroContext.SetInputUVE(33U, "A", 1.0F));
    ASSERT_TRUE(moduloByZeroContext.SetInputUVE(33U, "B", 0.0F));
    EXPECT_EQ(ExecuteScriptBytecodeUVE(makeProgram(33U, "math.float.modulo"), moduloByZeroContext).status,
              ScriptVmStatusUVE::NodeExecutionFailed);

    ScriptVmExecutionContextUVE reversedClampContext;
    ASSERT_TRUE(reversedClampContext.SetInputUVE(34U, "Value", 1.0F));
    ASSERT_TRUE(reversedClampContext.SetInputUVE(34U, "Min", 5.0F));
    ASSERT_TRUE(reversedClampContext.SetInputUVE(34U, "Max", 0.0F));
    EXPECT_EQ(ExecuteScriptBytecodeUVE(makeProgram(34U, "math.float.clamp"), reversedClampContext).status,
              ScriptVmStatusUVE::NodeExecutionFailed);

    ScriptVmExecutionContextUVE nonFinitePowerContext;
    ASSERT_TRUE(nonFinitePowerContext.SetInputUVE(35U, "A", -2.0F));
    ASSERT_TRUE(nonFinitePowerContext.SetInputUVE(35U, "B", 0.5F));
    EXPECT_EQ(ExecuteScriptBytecodeUVE(makeProgram(35U, "math.float.power"), nonFinitePowerContext).status,
              ScriptVmStatusUVE::NodeExecutionFailed);

    ScriptVmExecutionContextUVE lerpContext;
    ASSERT_TRUE(lerpContext.SetInputUVE(36U, "A", 10.0F));
    ASSERT_TRUE(lerpContext.SetInputUVE(36U, "B", 20.0F));
    ASSERT_TRUE(lerpContext.SetInputUVE(36U, "Alpha", 0.25F));
    ASSERT_TRUE(ExecuteScriptBytecodeUVE(makeProgram(36U, "math.float.lerp"), lerpContext).IsSuccessUVE());
    EXPECT_FLOAT_EQ(std::get<float>(*lerpContext.FindOutputUVE(36U, "Result")), 12.5F);

    ScriptVmExecutionContextUVE remapContext;
    ASSERT_TRUE(remapContext.SetInputUVE(37U, "Value", 5.0F));
    ASSERT_TRUE(remapContext.SetInputUVE(37U, "FromMin", 0.0F));
    ASSERT_TRUE(remapContext.SetInputUVE(37U, "FromMax", 10.0F));
    ASSERT_TRUE(remapContext.SetInputUVE(37U, "ToMin", 100.0F));
    ASSERT_TRUE(remapContext.SetInputUVE(37U, "ToMax", 200.0F));
    ASSERT_TRUE(ExecuteScriptBytecodeUVE(makeProgram(37U, "math.float.remap"), remapContext).IsSuccessUVE());
    EXPECT_FLOAT_EQ(std::get<float>(*remapContext.FindOutputUVE(37U, "Result")), 150.0F);

    const auto runUnary = [&](const std::uint32_t nodeId, const char* nodeTypeId, const float value) {
        ScriptVmExecutionContextUVE context;
        EXPECT_TRUE(context.SetInputUVE(nodeId, "Value", value));
        EXPECT_TRUE(ExecuteScriptBytecodeUVE(makeProgram(nodeId, nodeTypeId), context).IsSuccessUVE());
        const auto output = context.FindOutputUVE(nodeId, "Result");
        EXPECT_TRUE(output.has_value() && std::holds_alternative<float>(*output));
        return output.has_value() && std::holds_alternative<float>(*output) ?
                   std::get<float>(*output) : 0.0F;
    };
    EXPECT_NEAR(runUnary(38U, "math.float.sin", 0.5F), std::sin(0.5F), 1.0e-6F);
    EXPECT_NEAR(runUnary(39U, "math.float.cos", 0.5F), std::cos(0.5F), 1.0e-6F);
    EXPECT_NEAR(runUnary(40U, "math.float.tan", 0.5F), std::tan(0.5F), 1.0e-6F);
    EXPECT_FLOAT_EQ(runUnary(41U, "math.float.sqrt", 9.0F), 3.0F);

    ScriptVmExecutionContextUVE randomContextA;
    ScriptVmExecutionContextUVE randomContextB;
    ASSERT_TRUE(randomContextA.SetInputUVE(42U, "Seed", 123.0F));
    ASSERT_TRUE(randomContextB.SetInputUVE(42U, "Seed", 123.0F));
    ASSERT_TRUE(ExecuteScriptBytecodeUVE(makeProgram(42U, "math.float.random"), randomContextA).IsSuccessUVE());
    ASSERT_TRUE(ExecuteScriptBytecodeUVE(makeProgram(42U, "math.float.random"), randomContextB).IsSuccessUVE());
    const float randomA = std::get<float>(*randomContextA.FindOutputUVE(42U, "Result"));
    const float randomB = std::get<float>(*randomContextB.FindOutputUVE(42U, "Result"));
    EXPECT_FLOAT_EQ(randomA, randomB);
    EXPECT_GE(randomA, 0.0F);
    EXPECT_LT(randomA, 1.0F);

    ScriptVmExecutionContextUVE randomRangeContext;
    ASSERT_TRUE(randomRangeContext.SetInputUVE(43U, "Seed", 123.0F));
    ASSERT_TRUE(randomRangeContext.SetInputUVE(43U, "Min", -4.0F));
    ASSERT_TRUE(randomRangeContext.SetInputUVE(43U, "Max", 6.0F));
    ASSERT_TRUE(ExecuteScriptBytecodeUVE(makeProgram(43U, "math.float.random_range"), randomRangeContext).IsSuccessUVE());
    const float randomRange = std::get<float>(*randomRangeContext.FindOutputUVE(43U, "Result"));
    EXPECT_GE(randomRange, -4.0F);
    EXPECT_LE(randomRange, 6.0F);

    ScriptVmExecutionContextUVE negativeSqrtContext;
    ASSERT_TRUE(negativeSqrtContext.SetInputUVE(44U, "Value", -1.0F));
    EXPECT_EQ(ExecuteScriptBytecodeUVE(makeProgram(44U, "math.float.sqrt"), negativeSqrtContext).status,
              ScriptVmStatusUVE::NodeExecutionFailed);

    ScriptVmExecutionContextUVE flatRemapContext;
    ASSERT_TRUE(flatRemapContext.SetInputUVE(45U, "Value", 1.0F));
    ASSERT_TRUE(flatRemapContext.SetInputUVE(45U, "FromMin", 2.0F));
    ASSERT_TRUE(flatRemapContext.SetInputUVE(45U, "FromMax", 2.0F));
    ASSERT_TRUE(flatRemapContext.SetInputUVE(45U, "ToMin", 0.0F));
    ASSERT_TRUE(flatRemapContext.SetInputUVE(45U, "ToMax", 1.0F));
    EXPECT_EQ(ExecuteScriptBytecodeUVE(makeProgram(45U, "math.float.remap"), flatRemapContext).status,
              ScriptVmStatusUVE::NodeExecutionFailed);

    ScriptVmExecutionContextUVE reversedRandomRangeContext;
    ASSERT_TRUE(reversedRandomRangeContext.SetInputUVE(46U, "Seed", 1.0F));
    ASSERT_TRUE(reversedRandomRangeContext.SetInputUVE(46U, "Min", 2.0F));
    ASSERT_TRUE(reversedRandomRangeContext.SetInputUVE(46U, "Max", 1.0F));
    EXPECT_EQ(ExecuteScriptBytecodeUVE(makeProgram(46U, "math.float.random_range"), reversedRandomRangeContext).status,
              ScriptVmStatusUVE::NodeExecutionFailed);
}

TEST(ScriptVmUVETest, ExecuteScriptBytecodeUVE_DispatchesNumberComparisonNodes) {
    const auto makeProgram = [](const char* nodeTypeId) {
        ScriptBytecodeProgramUVE program;
        program.instructions.push_back({ScriptIrInstructionKindUVE::ExecuteNode, 40U, 0U, nodeTypeId, {}, {}});
        return program;
    };
    const auto runComparison = [&](const char* nodeTypeId, const float lhs, const float rhs) {
        ScriptVmExecutionContextUVE context;
        EXPECT_TRUE(context.SetInputUVE(40U, "A", lhs));
        EXPECT_TRUE(context.SetInputUVE(40U, "B", rhs));
        const ScriptVmExecutionResultUVE result =
            ExecuteScriptBytecodeUVE(makeProgram(nodeTypeId), context);
        EXPECT_TRUE(result.IsSuccessUVE());
        const auto output = context.FindOutputUVE(40U, "Result");
        EXPECT_TRUE(output.has_value());
        EXPECT_TRUE(output.has_value() && std::holds_alternative<bool>(*output));
        return output.has_value() && std::holds_alternative<bool>(*output) && std::get<bool>(*output);
    };
    EXPECT_TRUE(runComparison("logic.boolean.equal", 2.0F, 2.0F));
    EXPECT_FALSE(runComparison("logic.boolean.equal", 2.0F, 3.0F));
    EXPECT_TRUE(runComparison("logic.boolean.not_equal", 2.0F, 3.0F));
    EXPECT_TRUE(runComparison("logic.boolean.greater", 3.0F, 2.0F));
    EXPECT_TRUE(runComparison("logic.boolean.less", 2.0F, 3.0F));
    EXPECT_TRUE(runComparison("logic.boolean.greater_equal", 3.0F, 3.0F));
    EXPECT_TRUE(runComparison("logic.boolean.less_equal", 2.0F, 2.0F));

    ScriptVmExecutionContextUVE nonFiniteContext;
    ASSERT_TRUE(nonFiniteContext.SetInputUVE(40U, "A", std::numeric_limits<float>::quiet_NaN()));
    ASSERT_TRUE(nonFiniteContext.SetInputUVE(40U, "B", 1.0F));
    const ScriptVmExecutionResultUVE nonFiniteResult =
        ExecuteScriptBytecodeUVE(makeProgram("logic.boolean.equal"), nonFiniteContext);
    EXPECT_EQ(nonFiniteResult.status, ScriptVmStatusUVE::NodeExecutionFailed);
}

TEST(ScriptEntityQueryAdapterUVETest, PopulateComponentFactsUVE_StagesEcsPresenceInBindingOrder) {
    Memory::MemoryManagerUVE memoryManager;
    Events::EventSystemUVE eventSystem;
    Scene::EntityManagerUVE entityManager{memoryManager.GetDefaultAllocatorUVE(), eventSystem};
    const Scene::EntityUVE entity = entityManager.CreateEntityUVE();
    entityManager.AddComponentUVE<Scene::NameComponentUVE>(entity, Scene::NameComponentUVE{"Hero"});

    struct MissingComponentUVE final {};
    const std::vector<ScriptEntityComponentTypeBindingUVE> bindings{
        {"NameComponentUVE", std::type_index(typeid(Scene::NameComponentUVE))},
        {"MissingComponentUVE", std::type_index(typeid(MissingComponentUVE))},
    };
    ScriptVmExecutionContextUVE context;
    const ScriptEntityQueryAdapterResultUVE populated =
        ScriptEntityQueryAdapterUVE::PopulateComponentFactsUVE(entityManager, entity, bindings, context);
    ASSERT_TRUE(populated.IsAppliedUVE());
    EXPECT_EQ(populated.factsWritten, 2U);
    const auto present = context.FindComponentFactUVE(entity, "NameComponentUVE");
    ASSERT_TRUE(present.has_value());
    EXPECT_TRUE(present->present);
    const auto absent = context.FindComponentFactUVE(entity, "MissingComponentUVE");
    ASSERT_TRUE(absent.has_value());
    EXPECT_FALSE(absent->present);

    ASSERT_TRUE(context.SetInputUVE(50U, "Entity", ScriptEntityValueUVE{entity}));
    ASSERT_TRUE(context.SetInputUVE(
        50U, "Component", ScriptComponentValueUVE{Scene::kInvalidEntityUVE, "NameComponentUVE", false}));
    ScriptBytecodeProgramUVE hasProgram;
    hasProgram.instructions.push_back(
        {ScriptIrInstructionKindUVE::ExecuteNode, 50U, 0U, "query.entity.has_component", {}, {}});
    ASSERT_TRUE(ExecuteScriptBytecodeUVE(hasProgram, context).IsSuccessUVE());
    EXPECT_TRUE(std::get<bool>(*context.FindOutputUVE(50U, "Result")));

    ASSERT_TRUE(context.SetInputUVE(51U, "Entity", ScriptEntityValueUVE{entity}));
    ASSERT_TRUE(context.SetInputUVE(
        51U, "Component", ScriptComponentValueUVE{Scene::kInvalidEntityUVE, "MissingComponentUVE", false}));
    ScriptBytecodeProgramUVE getProgram;
    getProgram.instructions.push_back(
        {ScriptIrInstructionKindUVE::ExecuteNode, 51U, 0U, "query.entity.get_component", {}, {}});
    ASSERT_TRUE(ExecuteScriptBytecodeUVE(getProgram, context).IsSuccessUVE());
    const auto copied = context.FindOutputUVE(51U, "Result");
    ASSERT_TRUE(copied.has_value());
    ASSERT_TRUE(std::holds_alternative<ScriptComponentValueUVE>(*copied));
    EXPECT_FALSE(std::get<ScriptComponentValueUVE>(*copied).present);

    const std::size_t factCountBeforeInvalid = context.componentFacts.size();
    const auto rejected = ScriptEntityQueryAdapterUVE::PopulateComponentFactsUVE(
        entityManager, Scene::kInvalidEntityUVE, bindings, context);
    EXPECT_EQ(rejected.code, ScriptEntityQueryAdapterCodeUVE::InvalidEntity);
    EXPECT_EQ(context.componentFacts.size(), factCountBeforeInvalid);
}

TEST(ScriptVmUVETest, ExecuteScriptBytecodeUVE_DispatchesEntityQueryNodesFromCopiedFacts) {
    const auto makeProgram = [](const std::uint32_t nodeId, const char* nodeTypeId) {
        ScriptBytecodeProgramUVE program;
        program.instructions.push_back({ScriptIrInstructionKindUVE::ExecuteNode, nodeId, 0U, nodeTypeId, {}, {}});
        return program;
    };
    const Scene::EntityUVE entity{42U, 3U};
    const ScriptComponentValueUVE componentToken{Scene::kInvalidEntityUVE, "MeshComponentUVE", false};
    ScriptVmExecutionContextUVE hasContext;
    ASSERT_TRUE(hasContext.SetInputUVE(40U, "Entity", ScriptEntityValueUVE{entity}));
    ASSERT_TRUE(hasContext.SetInputUVE(40U, "Component", componentToken));
    ASSERT_TRUE(hasContext.SetComponentFactUVE(entity, "MeshComponentUVE", true));
    ASSERT_TRUE(ExecuteScriptBytecodeUVE(makeProgram(40U, "query.entity.has_component"), hasContext).IsSuccessUVE());
    EXPECT_EQ(std::get<bool>(*hasContext.FindOutputUVE(40U, "Result")), true);

    ScriptVmExecutionContextUVE getContext;
    ASSERT_TRUE(getContext.SetInputUVE(41U, "Entity", ScriptEntityValueUVE{entity}));
    ASSERT_TRUE(getContext.SetInputUVE(41U, "Component", componentToken));
    ASSERT_TRUE(getContext.SetComponentFactUVE(entity, "MeshComponentUVE", false));
    ASSERT_TRUE(ExecuteScriptBytecodeUVE(makeProgram(41U, "query.entity.get_component"), getContext).IsSuccessUVE());
    const auto componentOutput = getContext.FindOutputUVE(41U, "Result");
    ASSERT_TRUE(componentOutput.has_value());
    ASSERT_TRUE(std::holds_alternative<ScriptComponentValueUVE>(*componentOutput));
    EXPECT_FALSE(std::get<ScriptComponentValueUVE>(*componentOutput).present);

    ScriptVmExecutionContextUVE missingFactContext;
    ASSERT_TRUE(missingFactContext.SetInputUVE(42U, "Entity", ScriptEntityValueUVE{entity}));
    ASSERT_TRUE(missingFactContext.SetInputUVE(42U, "Component", componentToken));
    const ScriptVmExecutionResultUVE missingFact =
        ExecuteScriptBytecodeUVE(makeProgram(42U, "query.entity.has_component"), missingFactContext);
    EXPECT_EQ(missingFact.status, ScriptVmStatusUVE::NodeExecutionFailed);
}

TEST(ScriptVmUVETest, ExecuteScriptBytecodeUVE_ResolvesCompilerStyleNodeThenTransferOrdering) {
    ScriptBytecodeProgramUVE program;
    program.instructions.push_back(
        {ScriptIrInstructionKindUVE::ExecuteNode, 20U, 0U, "math.vector3.make", {}, {}});
    program.instructions.push_back(
        {ScriptIrInstructionKindUVE::ExecuteNode, 30U, 0U, "math.vector3.add", {}, {}});
    program.instructions.push_back(
        {ScriptIrInstructionKindUVE::TransferValue, 20U, 30U, {}, "Vector", "A"});

    ScriptVmExecutionContextUVE context;
    ASSERT_TRUE(context.SetInputUVE(20U, "X", 1.0F));
    ASSERT_TRUE(context.SetInputUVE(20U, "Y", 2.0F));
    ASSERT_TRUE(context.SetInputUVE(20U, "Z", 3.0F));
    ASSERT_TRUE(context.SetInputUVE(30U, "B", ScriptVector3ValueUVE{{4.0F, 5.0F, 6.0F}}));

    const ScriptVmExecutionResultUVE result = ExecuteScriptBytecodeUVE(program, context);
    EXPECT_TRUE(result.IsSuccessUVE());
    EXPECT_EQ(result.instructionsExecuted, 3U);
    EXPECT_EQ(std::get<ScriptVector3ValueUVE>(*context.FindOutputUVE(30U, "Result")),
              (ScriptVector3ValueUVE{{5.0F, 7.0F, 9.0F}}));
}

TEST(ScriptVmUVETest, ExecuteScriptBytecodeUVE_TransfersTypedValuesAndRejectsNodeFailure) {
    ScriptBytecodeProgramUVE transferProgram;
    transferProgram.instructions.push_back(
        {ScriptIrInstructionKindUVE::TransferValue, 10U, 11U, {}, "Result", "Vector"});
    ScriptVmExecutionContextUVE transferContext;
    ASSERT_TRUE(transferContext.SetOutputUVE(10U, "Result", ScriptVector3ValueUVE{{7.0F, 8.0F, 9.0F}}));
    const ScriptVmExecutionResultUVE transferred = ExecuteScriptBytecodeUVE(transferProgram, transferContext);
    EXPECT_TRUE(transferred.IsSuccessUVE());
    EXPECT_EQ(std::get<ScriptVector3ValueUVE>(*transferContext.FindInputUVE(11U, "Vector")),
              (ScriptVector3ValueUVE{{7.0F, 8.0F, 9.0F}}));

    ScriptBytecodeProgramUVE failingProgram;
    failingProgram.instructions.push_back(
        {ScriptIrInstructionKindUVE::ExecuteNode, 12U, 0U, "math.vector3.normalize", {}, {}});
    ScriptVmExecutionContextUVE failingContext;
    ASSERT_TRUE(failingContext.SetInputUVE(12U, "Vector", ScriptVector3ValueUVE{{0.0F, 0.0F, 0.0F}}));
    const ScriptVmExecutionResultUVE failed = ExecuteScriptBytecodeUVE(failingProgram, failingContext);
    EXPECT_FALSE(failed.IsSuccessUVE());
    EXPECT_EQ(failed.status, ScriptVmStatusUVE::NodeExecutionFailed);
    ASSERT_EQ(failed.diagnostics.size(), 1U);
    EXPECT_EQ(failed.diagnostics[0].instructionIndex, 0U);
}

TEST(ScriptVmUVETest, ExecuteScriptBytecodeUVE_StopsAtInstructionBudget) {
    ScriptBytecodeProgramUVE program;
    program.instructions.resize(3U);
    const ScriptVmExecutionResultUVE result = ExecuteScriptBytecodeUVE(program, {2U});
    EXPECT_FALSE(result.IsSuccessUVE());
    EXPECT_EQ(result.status, ScriptVmStatusUVE::InstructionBudgetExceeded);
    EXPECT_EQ(result.instructionsExecuted, 2U);
    ASSERT_EQ(result.diagnostics.size(), 1U);
    EXPECT_EQ(result.diagnostics[0].instructionIndex, 2U);
}

TEST(ScriptVmUVETest, ExecuteScriptBytecodeUVE_RejectsUnsupportedVersion) {
    ScriptBytecodeProgramUVE program;
    program.version = 99U;
    const ScriptVmExecutionResultUVE result = ExecuteScriptBytecodeUVE(program);
    EXPECT_FALSE(result.IsSuccessUVE());
    EXPECT_EQ(result.status, ScriptVmStatusUVE::InvalidInstruction);
    EXPECT_EQ(result.instructionsExecuted, 0U);
}

} // namespace UVE::Scripting


namespace UVE::Scripting {

TEST(ScriptRuntimeUVETest, AttachUVE_RejectsInvalidDuplicateAndAcceptsGenerationalIdentity) {
    ScriptRuntimeUVE runtime;
    ScriptBytecodeProgramUVE program;
    EXPECT_FALSE(runtime.AttachUVE(Scene::kInvalidEntityUVE, program));
    const Scene::EntityUVE first{3U, 1U};
    const Scene::EntityUVE replacement{3U, 2U};
    EXPECT_TRUE(runtime.AttachUVE(first, program));
    EXPECT_FALSE(runtime.AttachUVE(first, program));
    EXPECT_TRUE(runtime.AttachUVE(replacement, program));
    EXPECT_EQ(runtime.GetInstanceCountUVE(), 2U);
}

TEST(ScriptRuntimeUVETest, AttachDetailedUVEReturnsStructuredDiagnosticsForValidationAndCapacity) {
    ScriptRuntimeUVE runtime;
    ScriptBytecodeProgramUVE valid;

    const ScriptRuntimeAttachResultUVE invalidEntity = runtime.AttachDetailedUVE(Scene::kInvalidEntityUVE, valid);
    EXPECT_EQ(invalidEntity.code, ScriptRuntimeAttachCodeUVE::InvalidEntity);
    EXPECT_FALSE(invalidEntity.IsAcceptedUVE());
    EXPECT_FALSE(invalidEntity.message.empty());

    ScriptBytecodeProgramUVE invalidProgram;
    invalidProgram.version = 99U;
    const ScriptRuntimeAttachResultUVE invalid = runtime.AttachDetailedUVE({1U, 1U}, invalidProgram);
    EXPECT_EQ(invalid.code, ScriptRuntimeAttachCodeUVE::InvalidProgram);
    EXPECT_FALSE(invalid.IsAcceptedUVE());
    EXPECT_FALSE(invalid.diagnostics.empty());
    EXPECT_FALSE(invalid.message.empty());

    const Scene::EntityUVE entity{1U, 1U};
    const ScriptRuntimeAttachResultUVE accepted = runtime.AttachDetailedUVE(entity, valid);
    EXPECT_EQ(accepted.code, ScriptRuntimeAttachCodeUVE::Accepted);
    EXPECT_TRUE(accepted.IsAcceptedUVE());
    EXPECT_FALSE(accepted.message.empty());
    EXPECT_TRUE(runtime.AttachUVE({2U, 1U}, valid));

    const ScriptRuntimeAttachResultUVE duplicate = runtime.AttachDetailedUVE(entity, valid);
    EXPECT_EQ(duplicate.code, ScriptRuntimeAttachCodeUVE::DuplicateInstance);
    EXPECT_FALSE(duplicate.IsAcceptedUVE());
    EXPECT_FALSE(duplicate.message.empty());

    ScriptRuntimeUVE capacityRuntime;
    for (std::size_t index = 0U; index < ScriptRuntimeUVE::kMaximumInstancesUVE; ++index) {
        ASSERT_TRUE(capacityRuntime.AttachUVE(
            {static_cast<std::uint32_t>(1000U + index), 1U}, valid));
    }
    const ScriptRuntimeAttachResultUVE capacity = capacityRuntime.AttachDetailedUVE({9999U, 1U}, valid);
    EXPECT_EQ(capacity.code, ScriptRuntimeAttachCodeUVE::CapacityExceeded);
    EXPECT_FALSE(capacity.IsAcceptedUVE());
    EXPECT_FALSE(capacity.message.empty());
}

TEST(ScriptRuntimeUVETest, TickDetailedUVE_UsesBorrowedEngineTimeBinding) {
    ScriptRuntimeUVE runtime;
    ScriptBytecodeProgramUVE program;
    program.instructions.push_back({ScriptIrInstructionKindUVE::ExecuteNode, 72U, 0U, "engine.get_time", {}, {}});
    const Scene::EntityUVE entity{72U, 1U};
    ASSERT_TRUE(runtime.AttachUVE(entity, program));

    EngineTimeCaptureUVE capture;
    capture.value = 4.5F;
    const ScriptEngineCallBindingsUVE bindings{nullptr, &capture, CaptureEngineTimeUVE};
    ScriptVmExecutionOptionsUVE options;
    options.engineCallBindings = &bindings;
    const ScriptRuntimeTickBatchResultUVE tick = runtime.TickDetailedUVE(options);
    ASSERT_TRUE(tick.IsSuccessUVE());
    ASSERT_EQ(tick.results.size(), 1U);
    EXPECT_EQ(capture.callCount, 1U);
    const auto state = runtime.GetStateUVE(entity);
    ASSERT_TRUE(state.has_value());
    ASSERT_TRUE(state->executionContext.FindOutputUVE(72U, "Value").has_value());
    EXPECT_FLOAT_EQ(std::get<float>(*state->executionContext.FindOutputUVE(72U, "Value")), 4.5F);
}

TEST(ScriptRuntimeUVETest, TickDetailedUVE_UsesBorrowedEngineLogBinding) {
    ScriptRuntimeUVE runtime;
    ScriptBytecodeProgramUVE program;
    program.instructions.push_back({ScriptIrInstructionKindUVE::ExecuteNode, 70U, 0U, "engine.log", {}, {}});
    const Scene::EntityUVE entity{70U, 1U};
    ASSERT_TRUE(runtime.AttachUVE(entity, program));
    std::optional<ScriptRuntimeStateUVE> state = runtime.GetStateUVE(entity);
    ASSERT_TRUE(state.has_value());
    ASSERT_TRUE(state->executionContext.SetInputUVE(70U, "Value", 9.25F));
    ASSERT_TRUE(runtime.SetStateUVE(entity, *state));

    EngineLogCaptureUVE capture;
    const ScriptEngineCallBindingsUVE bindings{CaptureEngineLogUVE, &capture};
    ScriptVmExecutionOptionsUVE options;
    options.engineCallBindings = &bindings;
    const ScriptRuntimeTickBatchResultUVE tick = runtime.TickDetailedUVE(options);
    ASSERT_TRUE(tick.IsSuccessUVE());
    ASSERT_EQ(tick.results.size(), 1U);
    EXPECT_EQ(tick.results.front().execution.status, ScriptVmStatusUVE::Completed);
    EXPECT_EQ(capture.callCount, 1U);
    EXPECT_FLOAT_EQ(capture.lastValue, 9.25F);
}

TEST(ScriptRuntimeUVETest, TickUVE_IsDeterministicAndSkipsDisabledInstances) {
    ScriptRuntimeUVE runtime;
    ScriptBytecodeProgramUVE program;
    program.instructions.resize(2U);
    ASSERT_TRUE(runtime.AttachUVE({9U, 1U}, program));
    ASSERT_TRUE(runtime.AttachUVE({2U, 4U}, program));
    ASSERT_TRUE(runtime.AttachUVE({5U, 1U}, program));
    ASSERT_TRUE(runtime.SetEnabledUVE({5U, 1U}, false));
    const auto results = runtime.TickUVE({2U});
    ASSERT_EQ(results.size(), 2U);
    EXPECT_EQ(results[0].entity, (Scene::EntityUVE{2U, 4U}));
    EXPECT_EQ(results[1].entity, (Scene::EntityUVE{9U, 1U}));
    EXPECT_TRUE(results[0].execution.IsSuccessUVE());
    EXPECT_EQ(results[0].execution.instructionsExecuted, 2U);
}

TEST(ScriptRuntimeUVETest, TickDetailedUVE_SummarizesEnabledCompletedAndDisabledInstances) {
    ScriptRuntimeUVE runtime;
    ScriptBytecodeProgramUVE program;
    program.instructions.resize(2U);
    ASSERT_TRUE(runtime.AttachUVE({9U, 1U}, program));
    ASSERT_TRUE(runtime.AttachUVE({2U, 4U}, program));
    ASSERT_TRUE(runtime.SetEnabledUVE({2U, 4U}, false));

    const ScriptRuntimeTickBatchResultUVE detailed = runtime.TickDetailedUVE({4U});
    ASSERT_EQ(detailed.results.size(), 1U);
    EXPECT_EQ(detailed.results[0].entity, (Scene::EntityUVE{9U, 1U}));
    EXPECT_EQ(detailed.summary.enabledInstanceCount, 1U);
    EXPECT_EQ(detailed.summary.completedCount, 1U);
    EXPECT_EQ(detailed.summary.instructionBudgetExceededCount, 0U);
    EXPECT_EQ(detailed.summary.invalidInstructionCount, 0U);
    EXPECT_EQ(detailed.summary.diagnosticCount, 0U);
    EXPECT_TRUE(detailed.IsSuccessUVE());

    const auto legacyResults = runtime.TickUVE({4U});
    ASSERT_EQ(legacyResults.size(), detailed.results.size());
    EXPECT_EQ(legacyResults[0].entity, detailed.results[0].entity);
    EXPECT_EQ(legacyResults[0].execution.instructionsExecuted,
              detailed.results[0].execution.instructionsExecuted);
}

TEST(ScriptRuntimeUVETest, TickWithEntityQueryDetailedUVE_RefreshesFactsAndAccountsAdapterFailures) {
    Memory::MemoryManagerUVE memoryManager;
    Events::EventSystemUVE eventSystem;
    Scene::EntityManagerUVE entityManager{memoryManager.GetDefaultAllocatorUVE(), eventSystem};
    const Scene::EntityUVE aliveEntity = entityManager.CreateEntityUVE();
    entityManager.AddComponentUVE<Scene::NameComponentUVE>(aliveEntity, Scene::NameComponentUVE{"Hero"});
    const Scene::EntityUVE notAliveEntity{99U, 1U};

    ScriptBytecodeProgramUVE program;
    program.instructions.push_back(
        {ScriptIrInstructionKindUVE::ExecuteNode, 70U, 0U, "query.entity.has_component", {}, {}});
    ScriptRuntimeUVE runtime;
    ASSERT_TRUE(runtime.AttachUVE(aliveEntity, program));
    ASSERT_TRUE(runtime.AttachUVE(notAliveEntity, program));

    ScriptRuntimeStateUVE aliveState;
    ASSERT_TRUE(aliveState.executionContext.SetInputUVE(70U, "Entity", ScriptEntityValueUVE{aliveEntity}));
    ASSERT_TRUE(aliveState.executionContext.SetInputUVE(
        70U, "Component", ScriptComponentValueUVE{Scene::kInvalidEntityUVE, "NameComponentUVE", false}));
    ASSERT_TRUE(runtime.SetStateUVE(aliveEntity, aliveState));
    const std::vector<ScriptEntityComponentTypeBindingUVE> bindings{
        {"NameComponentUVE", std::type_index(typeid(Scene::NameComponentUVE))},
    };

    const ScriptRuntimeTickBatchResultUVE detailed =
        runtime.TickWithEntityQueryDetailedUVE(entityManager, bindings);
    ASSERT_EQ(detailed.results.size(), 2U);
    EXPECT_EQ(detailed.results[0].entity, aliveEntity);
    EXPECT_EQ(detailed.results[1].entity, notAliveEntity);
    EXPECT_EQ(detailed.summary.enabledInstanceCount, 2U);
    EXPECT_EQ(detailed.summary.completedCount, 1U);
    EXPECT_EQ(detailed.summary.nodeExecutionFailedCount, 1U);
    EXPECT_EQ(detailed.summary.diagnosticCount, 1U);
    EXPECT_FALSE(detailed.IsSuccessUVE());
    ASSERT_TRUE(detailed.results[0].execution.IsSuccessUVE());
    ASSERT_GE(detailed.results[0].execution.trace.size(), 3U);
    EXPECT_EQ(detailed.results[0].execution.trace[0].kind,
              ScriptVmTraceEventKindUVE::QueryFactsRefreshed);
    EXPECT_EQ(detailed.results[0].execution.trace[0].entity, aliveEntity);
    EXPECT_EQ(detailed.results[1].execution.status, ScriptVmStatusUVE::NodeExecutionFailed);
    ASSERT_EQ(detailed.results[1].execution.trace.size(), 1U);
    EXPECT_EQ(detailed.results[1].execution.trace.front().kind, ScriptVmTraceEventKindUVE::Failed);
    EXPECT_EQ(detailed.results[1].execution.trace.front().entity, notAliveEntity);
    EXPECT_FALSE(detailed.results[1].execution.trace.front().message.empty());

    const auto refreshedState = runtime.GetStateUVE(aliveEntity);
    ASSERT_TRUE(refreshedState.has_value());
    const auto fact = refreshedState->executionContext.FindComponentFactUVE(aliveEntity, "NameComponentUVE");
    ASSERT_TRUE(fact.has_value());
    EXPECT_TRUE(fact->present);
    EXPECT_TRUE(runtime.SetEnabledUVE(notAliveEntity, false));
    const ScriptRuntimeTickBatchResultUVE disabled =
        runtime.TickWithEntityQueryDetailedUVE(entityManager, bindings);
    ASSERT_EQ(disabled.results.size(), 1U);
    EXPECT_EQ(disabled.summary.enabledInstanceCount, 1U);
    EXPECT_EQ(disabled.summary.completedCount, 1U);
    EXPECT_EQ(disabled.summary.nodeExecutionFailedCount, 0U);
    EXPECT_TRUE(disabled.IsSuccessUVE());
}

TEST(ScriptRuntimeUVETest, TickDetailedUVE_ExecutesTypedVector3ThroughPerEntityContext) {
    ScriptRuntimeUVE runtime;
    ScriptBytecodeProgramUVE program;
    program.instructions.push_back(
        {ScriptIrInstructionKindUVE::ExecuteNode, 21U, 0U, "math.vector3.make", {}, {}});
    const Scene::EntityUVE entity{21U, 1U};
    ASSERT_TRUE(runtime.AttachUVE(entity, program));

    ScriptRuntimeStateUVE state;
    ASSERT_TRUE(state.executionContext.SetInputUVE(21U, "X", 2.0F));
    ASSERT_TRUE(state.executionContext.SetInputUVE(21U, "Y", -4.0F));
    ASSERT_TRUE(state.executionContext.SetInputUVE(21U, "Z", 8.0F));
    ASSERT_TRUE(runtime.SetStateUVE(entity, state));

    const ScriptRuntimeTickBatchResultUVE detailed = runtime.TickDetailedUVE();
    ASSERT_TRUE(detailed.IsSuccessUVE());
    ASSERT_EQ(detailed.results.size(), 1U);
    EXPECT_EQ(detailed.summary.completedCount, 1U);
    EXPECT_EQ(detailed.results.front().execution.instructionsExecuted, 1U);

    const auto stored = runtime.GetStateUVE(entity);
    ASSERT_TRUE(stored.has_value());
    const auto output = stored->executionContext.FindOutputUVE(21U, "Vector");
    ASSERT_TRUE(output.has_value());
    EXPECT_EQ(std::get<ScriptVector3ValueUVE>(*output), (ScriptVector3ValueUVE{{2.0F, -4.0F, 8.0F}}));
}

TEST(ScriptRuntimeUVETest, SetStateDetailedUVE_RejectsInvalidVmBindingWithoutMutation) {
    ScriptRuntimeUVE runtime;
    ScriptBytecodeProgramUVE program;
    const Scene::EntityUVE entity{22U, 1U};
    ASSERT_TRUE(runtime.AttachUVE(entity, program));

    ScriptRuntimeStateUVE valid;
    ASSERT_TRUE(valid.executionContext.SetInputUVE(22U, "Value", 3.0F));
    ASSERT_TRUE(runtime.SetStateUVE(entity, valid));

    ScriptRuntimeStateUVE invalid = valid;
    invalid.executionContext.inputs.front().value = std::numeric_limits<float>::infinity();
    const ScriptRuntimeStateUpdateResultUVE rejected = runtime.SetStateDetailedUVE(entity, invalid);
    EXPECT_EQ(rejected.code, ScriptRuntimeStateUpdateCodeUVE::InvalidVmBinding);
    EXPECT_FALSE(rejected.IsAcceptedUVE());
    EXPECT_EQ(runtime.GetStateUVE(entity), std::optional<ScriptRuntimeStateUVE>(valid));
}

TEST(ScriptRuntimeUVETest, TickDetailedUVE_SummarizesInstructionBudgetDiagnostics) {
    ScriptRuntimeUVE runtime;
    ScriptBytecodeProgramUVE program;
    program.instructions.resize(3U);
    ASSERT_TRUE(runtime.AttachUVE({4U, 2U}, program));

    const ScriptRuntimeTickBatchResultUVE detailed = runtime.TickDetailedUVE({2U});
    ASSERT_EQ(detailed.results.size(), 1U);
    EXPECT_EQ(detailed.summary.enabledInstanceCount, 1U);
    EXPECT_EQ(detailed.summary.completedCount, 0U);
    EXPECT_EQ(detailed.summary.instructionBudgetExceededCount, 1U);
    EXPECT_EQ(detailed.summary.invalidInstructionCount, 0U);
    EXPECT_EQ(detailed.summary.diagnosticCount, 1U);
    EXPECT_FALSE(detailed.IsSuccessUVE());
    EXPECT_EQ(detailed.results[0].execution.status, ScriptVmStatusUVE::InstructionBudgetExceeded);
}

TEST(ScriptRuntimeUVETest, GetSnapshotUVE_ReturnsDeterministicCopiedInstanceMetadata) {
    ScriptRuntimeUVE runtime;
    ScriptBytecodeProgramUVE firstProgram;
    firstProgram.instructions.resize(2U);
    ScriptBytecodeProgramUVE secondProgram;
    secondProgram.instructions.resize(1U);
    const Scene::EntityUVE firstEntity{9U, 1U};
    const Scene::EntityUVE secondEntity{2U, 4U};
    ASSERT_TRUE(runtime.AttachUVE(firstEntity, firstProgram));
    ASSERT_TRUE(runtime.AttachUVE(secondEntity, secondProgram));

    ScriptRuntimeStateUVE firstState;
    firstState.values = {3, 5};
    ASSERT_TRUE(runtime.SetStateUVE(firstEntity, firstState));
    ASSERT_TRUE(runtime.SetEnabledUVE(firstEntity, false));

    const auto initialSnapshot = runtime.GetSnapshotUVE();
    ASSERT_EQ(initialSnapshot.size(), 2U);
    EXPECT_EQ(initialSnapshot[0].entity, secondEntity);
    EXPECT_EQ(initialSnapshot[0].generation, 1U);
    EXPECT_EQ(initialSnapshot[0].programVersion, ScriptBytecodeProgramUVE::kCurrentVersionUVE);
    EXPECT_EQ(initialSnapshot[0].instructionCount, 1U);
    EXPECT_EQ(initialSnapshot[0].stateValueCount, 0U);
    EXPECT_TRUE(initialSnapshot[0].enabled);
    EXPECT_EQ(initialSnapshot[1].entity, firstEntity);
    EXPECT_EQ(initialSnapshot[1].instructionCount, 2U);
    EXPECT_EQ(initialSnapshot[1].stateValueCount, 2U);
    EXPECT_FALSE(initialSnapshot[1].enabled);

    const ScriptRuntimeReloadResultUVE reload = runtime.ReloadUVE(firstEntity, firstProgram);
    ASSERT_TRUE(reload.IsAcceptedUVE());
    const auto reloadedSnapshot = runtime.GetSnapshotUVE();
    ASSERT_EQ(reloadedSnapshot.size(), 2U);
    EXPECT_EQ(reloadedSnapshot[1].generation, 2U);
    EXPECT_EQ(reloadedSnapshot[1].stateValueCount, 2U);
    EXPECT_FALSE(reloadedSnapshot[1].enabled);

    ASSERT_TRUE(runtime.DetachUVE(secondEntity));
    const auto detachedSnapshot = runtime.GetSnapshotUVE();
    ASSERT_EQ(detachedSnapshot.size(), 1U);
    EXPECT_EQ(detachedSnapshot.front().entity, firstEntity);
}

TEST(ScriptRuntimeUVETest, SetEnabledDetailedUVEReturnsStructuredDiagnosticsAndControlsTicking) {
    ScriptRuntimeUVE runtime;
    ScriptBytecodeProgramUVE program;
    const Scene::EntityUVE entity{6U, 1U};
    ASSERT_TRUE(runtime.AttachUVE(entity, program));

    const ScriptRuntimeEnabledUpdateResultUVE unchanged = runtime.SetEnabledDetailedUVE(entity, true);
    EXPECT_EQ(unchanged.code, ScriptRuntimeEnabledUpdateCodeUVE::Unchanged);
    EXPECT_TRUE(unchanged.IsAcceptedUVE());
    EXPECT_FALSE(unchanged.message.empty());

    const ScriptRuntimeEnabledUpdateResultUVE disabled = runtime.SetEnabledDetailedUVE(entity, false);
    EXPECT_EQ(disabled.code, ScriptRuntimeEnabledUpdateCodeUVE::Applied);
    EXPECT_TRUE(disabled.IsAcceptedUVE());
    EXPECT_TRUE(runtime.TickUVE().empty());
    EXPECT_TRUE(runtime.SetEnabledUVE(entity, false));

    const ScriptRuntimeEnabledUpdateResultUVE enabled = runtime.SetEnabledDetailedUVE(entity, true);
    EXPECT_EQ(enabled.code, ScriptRuntimeEnabledUpdateCodeUVE::Applied);
    EXPECT_TRUE(enabled.IsAcceptedUVE());
    EXPECT_EQ(runtime.TickUVE().size(), 1U);

    const ScriptRuntimeEnabledUpdateResultUVE missing = runtime.SetEnabledDetailedUVE({9U, 1U}, true);
    EXPECT_EQ(missing.code, ScriptRuntimeEnabledUpdateCodeUVE::NoActiveInstance);
    EXPECT_FALSE(missing.IsAcceptedUVE());
    EXPECT_FALSE(missing.message.empty());
}

TEST(ScriptRuntimeUVETest, DetachDetailedUVEReturnsStructuredLifecycleDiagnostics) {
    ScriptRuntimeUVE runtime;
    ScriptBytecodeProgramUVE program;
    const Scene::EntityUVE entity{4U, 1U};
    ASSERT_TRUE(runtime.AttachUVE(entity, program));

    const ScriptRuntimeDetachResultUVE wrongGeneration = runtime.DetachDetailedUVE({4U, 2U});
    EXPECT_EQ(wrongGeneration.code, ScriptRuntimeDetachCodeUVE::NoActiveInstance);
    EXPECT_FALSE(wrongGeneration.IsAcceptedUVE());
    EXPECT_FALSE(wrongGeneration.message.empty());
    EXPECT_TRUE(runtime.HasInstanceUVE(entity));

    const ScriptRuntimeDetachResultUVE applied = runtime.DetachDetailedUVE(entity);
    EXPECT_EQ(applied.code, ScriptRuntimeDetachCodeUVE::Applied);
    EXPECT_TRUE(applied.IsAcceptedUVE());
    EXPECT_FALSE(applied.message.empty());
    EXPECT_FALSE(runtime.HasInstanceUVE(entity));
    EXPECT_FALSE(runtime.DetachUVE(entity));
}

TEST(ScriptRuntimeUVETest, ReloadUVE_RejectsInvalidCandidateAndRetainsLastKnownGoodProgram) {
    ScriptRuntimeUVE runtime;
    ScriptBytecodeProgramUVE initial;
    initial.instructions.resize(1U);
    ASSERT_TRUE(runtime.AttachUVE({7U, 1U}, initial));

    ScriptBytecodeProgramUVE invalid;
    invalid.version = 99U;
    const ScriptRuntimeReloadResultUVE rejected = runtime.ReloadUVE({7U, 1U}, invalid);
    EXPECT_EQ(rejected.code, ScriptRuntimeReloadCodeUVE::RejectedInvalidProgram);
    EXPECT_EQ(rejected.activeGeneration, 1U);
    EXPECT_TRUE(rejected.lastKnownGoodRetained);
    ASSERT_EQ(rejected.diagnostics.size(), 1U);
    EXPECT_EQ(rejected.diagnostics[0].code, ScriptBytecodeDiagnosticCodeUVE::UnsupportedVersion);
    EXPECT_EQ(runtime.TickUVE({8U}).front().execution.instructionsExecuted, 1U);
}

TEST(ScriptRuntimeUVETest, StateUVE_IsBoundedAndPreservedAcrossCompatibleReload) {
    ScriptRuntimeUVE runtime;
    ScriptBytecodeProgramUVE initial;
    initial.instructions.resize(1U);
    const Scene::EntityUVE entity{8U, 1U};
    ASSERT_TRUE(runtime.AttachUVE(entity, initial));

    ScriptRuntimeStateUVE state;
    state.values = {11, -7, 42};
    const ScriptRuntimeStateUpdateResultUVE appliedState = runtime.SetStateDetailedUVE(entity, state);
    EXPECT_EQ(appliedState.code, ScriptRuntimeStateUpdateCodeUVE::Applied);
    EXPECT_TRUE(appliedState.IsAcceptedUVE());
    EXPECT_FALSE(appliedState.message.empty());
    ASSERT_EQ(runtime.GetStateUVE(entity), std::optional<ScriptRuntimeStateUVE>(state));

    const ScriptRuntimeStateUpdateResultUVE unchangedState = runtime.SetStateDetailedUVE(entity, state);
    EXPECT_EQ(unchangedState.code, ScriptRuntimeStateUpdateCodeUVE::Unchanged);
    EXPECT_TRUE(unchangedState.IsAcceptedUVE());
    EXPECT_TRUE(runtime.SetStateUVE(entity, state));

    ScriptBytecodeProgramUVE replacement;
    replacement.instructions.resize(2U);
    const ScriptRuntimeReloadResultUVE accepted = runtime.ReloadUVE(entity, replacement);
    EXPECT_TRUE(accepted.IsAcceptedUVE());
    EXPECT_TRUE(accepted.compatibleStatePreserved);
    EXPECT_EQ(accepted.activeGeneration, 2U);
    EXPECT_EQ(runtime.GetStateUVE(entity), std::optional<ScriptRuntimeStateUVE>(state));

    ScriptBytecodeProgramUVE invalid;
    invalid.version = 99U;
    const ScriptRuntimeReloadResultUVE rejected = runtime.ReloadUVE(entity, invalid);
    EXPECT_EQ(rejected.code, ScriptRuntimeReloadCodeUVE::RejectedInvalidProgram);
    EXPECT_FALSE(rejected.compatibleStatePreserved);
    EXPECT_TRUE(rejected.lastKnownGoodRetained);
    EXPECT_EQ(runtime.GetStateUVE(entity), std::optional<ScriptRuntimeStateUVE>(state));

    ScriptRuntimeStateUVE oversized;
    oversized.values.resize(ScriptRuntimeUVE::kMaximumStateValuesUVE + 1U);
    const ScriptRuntimeStateUpdateResultUVE capacity = runtime.SetStateDetailedUVE(entity, std::move(oversized));
    EXPECT_EQ(capacity.code, ScriptRuntimeStateUpdateCodeUVE::CapacityExceeded);
    EXPECT_FALSE(capacity.IsAcceptedUVE());
    EXPECT_FALSE(capacity.message.empty());

    const ScriptRuntimeStateUpdateResultUVE missing = runtime.SetStateDetailedUVE({9U, 1U}, state);
    EXPECT_EQ(missing.code, ScriptRuntimeStateUpdateCodeUVE::NoActiveInstance);
    EXPECT_FALSE(missing.IsAcceptedUVE());
    EXPECT_FALSE(missing.message.empty());
    EXPECT_FALSE(runtime.SetStateUVE({9U, 1U}, state));
}

TEST(ScriptRuntimeUVETest, StateUVE_SupportsTypedVector3ValuesWithoutChangingScalarSlots) {
    ScriptRuntimeUVE runtime;
    ScriptBytecodeProgramUVE program;
    const Scene::EntityUVE entity{10U, 1U};
    ASSERT_TRUE(runtime.AttachUVE(entity, program));

    const ScriptVector3ValueUVE position{{1.0F, -2.0F, 3.5F}};
    const ScriptVector3ValueUVE direction{{0.0F, 1.0F, 0.0F}};
    ScriptRuntimeStateUVE state;
    state.values = {17, -4};
    state.vector3Values = {position, direction};

    const ScriptRuntimeStateUpdateResultUVE applied = runtime.SetStateDetailedUVE(entity, state);
    EXPECT_EQ(applied.code, ScriptRuntimeStateUpdateCodeUVE::Applied);
    EXPECT_TRUE(applied.IsAcceptedUVE());

    const auto stored = runtime.GetStateUVE(entity);
    ASSERT_EQ(stored, std::optional<ScriptRuntimeStateUVE>(state));
    ASSERT_EQ(stored->values, (std::vector<std::int64_t>{17, -4}));
    ASSERT_EQ(stored->vector3Values.size(), 2U);
    EXPECT_EQ(stored->vector3Values[0], position);
    EXPECT_EQ(stored->vector3Values[1], direction);
}

TEST(ScriptRuntimeUVETest, StateUVE_RejectsNonFiniteTypedVector3WithoutMutation) {
    ScriptRuntimeUVE runtime;
    ScriptBytecodeProgramUVE program;
    const Scene::EntityUVE entity{11U, 1U};
    ASSERT_TRUE(runtime.AttachUVE(entity, program));

    ScriptRuntimeStateUVE validState;
    validState.vector3Values = {ScriptVector3ValueUVE{{1.0F, 2.0F, 3.0F}}};
    ASSERT_TRUE(runtime.SetStateUVE(entity, validState));

    ScriptRuntimeStateUVE invalidState = validState;
    invalidState.vector3Values.front().value.y = std::numeric_limits<float>::quiet_NaN();
    const ScriptRuntimeStateUpdateResultUVE rejected = runtime.SetStateDetailedUVE(entity, invalidState);
    EXPECT_EQ(rejected.code, ScriptRuntimeStateUpdateCodeUVE::NonFiniteVector3);
    EXPECT_FALSE(rejected.IsAcceptedUVE());
    EXPECT_FALSE(rejected.message.empty());
    EXPECT_EQ(runtime.GetStateUVE(entity), std::optional<ScriptRuntimeStateUVE>(validState));
}

TEST(ScriptRuntimeUVETest, StateUVE_BoundsTypedVector3ValuesIndependentlyFromScalarSlots) {
    ScriptRuntimeUVE runtime;
    ScriptBytecodeProgramUVE program;
    const Scene::EntityUVE entity{12U, 1U};
    ASSERT_TRUE(runtime.AttachUVE(entity, program));

    ScriptRuntimeStateUVE oversized;
    oversized.values = {1};
    oversized.vector3Values.resize(ScriptRuntimeUVE::kMaximumStateVector3ValuesUVE + 1U);
    const ScriptRuntimeStateUpdateResultUVE rejected = runtime.SetStateDetailedUVE(entity, oversized);
    EXPECT_EQ(rejected.code, ScriptRuntimeStateUpdateCodeUVE::CapacityExceeded);
    EXPECT_FALSE(rejected.IsAcceptedUVE());
    EXPECT_FALSE(rejected.message.empty());
    EXPECT_EQ(runtime.GetStateUVE(entity), std::optional<ScriptRuntimeStateUVE>(ScriptRuntimeStateUVE{}));

    ScriptRuntimeStateUVE valid;
    valid.values.resize(ScriptRuntimeUVE::kMaximumStateValuesUVE);
    valid.vector3Values = {ScriptVector3ValueUVE{{4.0F, 5.0F, 6.0F}}};
    const ScriptRuntimeStateUpdateResultUVE applied = runtime.SetStateDetailedUVE(entity, valid);
    EXPECT_EQ(applied.code, ScriptRuntimeStateUpdateCodeUVE::Applied);
    EXPECT_EQ(runtime.GetStateUVE(entity), std::optional<ScriptRuntimeStateUVE>(valid));
}

TEST(ScriptRuntimeUVETest, StateUVE_PreservesTypedValuesAcrossCompatibleReload) {
    ScriptRuntimeUVE runtime;
    ScriptBytecodeProgramUVE initial;
    const Scene::EntityUVE entity{13U, 1U};
    ASSERT_TRUE(runtime.AttachUVE(entity, initial));

    ScriptRuntimeStateUVE state;
    state.values = {8};
    state.vector3Values = {ScriptVector3ValueUVE{{-1.0F, 0.5F, 9.0F}}};
    ASSERT_TRUE(runtime.SetStateUVE(entity, state));

    ScriptBytecodeProgramUVE replacement;
    replacement.instructions.resize(2U);
    const ScriptRuntimeReloadResultUVE reloaded = runtime.ReloadUVE(entity, replacement);
    ASSERT_TRUE(reloaded.IsAcceptedUVE());
    EXPECT_TRUE(reloaded.compatibleStatePreserved);
    EXPECT_EQ(runtime.GetStateUVE(entity), std::optional<ScriptRuntimeStateUVE>(state));
}

TEST(ScriptRuntimeUVETest, ReloadUVE_AcceptsValidReplacementAndRejectsMissingInstance) {
    ScriptRuntimeUVE runtime;
    ScriptBytecodeProgramUVE initial;
    ASSERT_TRUE(runtime.AttachUVE({8U, 1U}, initial));

    ScriptBytecodeProgramUVE replacement;
    replacement.instructions.resize(2U);
    const ScriptRuntimeReloadResultUVE accepted = runtime.ReloadUVE({8U, 1U}, replacement);
    EXPECT_TRUE(accepted.IsAcceptedUVE());
    EXPECT_EQ(accepted.activeGeneration, 2U);
    EXPECT_FALSE(accepted.lastKnownGoodRetained);
    ASSERT_EQ(runtime.TickUVE({8U}).size(), 1U);
    EXPECT_EQ(runtime.TickUVE({8U}).front().execution.instructionsExecuted, 2U);

    const ScriptRuntimeReloadResultUVE missing = runtime.ReloadUVE({9U, 1U}, replacement);
    EXPECT_EQ(missing.code, ScriptRuntimeReloadCodeUVE::NoActiveInstance);
    EXPECT_EQ(missing.activeGeneration, 0U);
}

} // namespace UVE::Scripting


namespace UVE::Scripting {

TEST(ScriptGraphPersistenceUVETest, EncodeDecodeSchema_RoundTripsNodesLinksLayoutMetadataDeterministically) {
    ScriptGraphSchemaUVE schema{};
    ASSERT_TRUE(schema.graph.AddNodeUVE({2U, "test.sink"}));
    ASSERT_TRUE(schema.graph.AddNodeUVE({1U, "test.source"}));
    ASSERT_TRUE(schema.graph.AddLinkUVE({{2U, "Out"}, {1U, "In"}}));
    ASSERT_TRUE(schema.graph.AddLinkUVE({{1U, "Out"}, {2U, "In"}}));
    schema.layout = {{2U, 30.0F, 40.0F}, {1U, 10.0F, 20.0F}};
    schema.metadata.emplace("zeta", "last");
    schema.metadata.emplace("alpha", "first");

    std::vector<ScriptPersistenceDiagnosticUVE> diagnostics;
    const std::string encoded = EncodeScriptGraphSchemaUVE(schema, diagnostics);
    ASSERT_TRUE(diagnostics.empty());
    EXPECT_EQ(encoded,
              R"({"schemaVersion":1,"nodes":[{"id":1,"typeId":"test.source"},{"id":2,"typeId":"test.sink"}],"links":[{"output":{"nodeId":1,"pinName":"Out"},"input":{"nodeId":2,"pinName":"In"}},{"output":{"nodeId":2,"pinName":"Out"},"input":{"nodeId":1,"pinName":"In"}}],"layout":[{"nodeId":1,"x":10.0,"y":20.0},{"nodeId":2,"x":30.0,"y":40.0}],"metadata":{"alpha":"first","zeta":"last"}})");

    const ScriptGraphSchemaDecodeResultUVE decoded = DecodeScriptGraphSchemaUVE(encoded);
    ASSERT_TRUE(decoded.IsSuccessUVE());
    EXPECT_EQ(decoded.schema->schemaVersion, kScriptGraphSchemaVersionUVE);
    EXPECT_EQ(decoded.schema->graph.GetNodesUVE().size(), 2U);
    EXPECT_EQ(decoded.schema->graph.GetLinksUVE().size(), 2U);
    EXPECT_EQ(decoded.schema->layout, (std::vector<ScriptGraphLayoutEntryUVE>{{1U, 10.0F, 20.0F}, {2U, 30.0F, 40.0F}}));
    EXPECT_EQ(decoded.schema->metadata.at("alpha"), "first");

    std::vector<ScriptPersistenceDiagnosticUVE> secondDiagnostics;
    EXPECT_EQ(EncodeScriptGraphSchemaUVE(*decoded.schema, secondDiagnostics), encoded);
    EXPECT_TRUE(secondDiagnostics.empty());
}

TEST(ScriptGraphPersistenceUVETest, DecodeSchema_RejectsUnknownFieldsMalformedInputAndFutureVersion) {
    const ScriptGraphSchemaDecodeResultUVE unknown = DecodeScriptGraphSchemaUVE(
        R"({"schemaVersion":1,"nodes":[],"links":[],"layout":[],"metadata":{},"future":true})");
    ASSERT_FALSE(unknown.IsSuccessUVE());
    EXPECT_EQ(unknown.diagnostics[0].code, ScriptPersistenceDiagnosticCodeUVE::UnknownField);

    const ScriptGraphSchemaDecodeResultUVE malformed = DecodeScriptGraphSchemaUVE(
        R"({"schemaVersion":1,"nodes":[{"id":1,"typeId":"test"}],"links":[],"layout":[],"metadata":{}})");
    ASSERT_TRUE(malformed.IsSuccessUVE());

    const ScriptGraphSchemaDecodeResultUVE missing = DecodeScriptGraphSchemaUVE(
        R"({"schemaVersion":1,"nodes":[],"links":[],"metadata":{}})");
    ASSERT_FALSE(missing.IsSuccessUVE());
    EXPECT_EQ(missing.diagnostics[0].code, ScriptPersistenceDiagnosticCodeUVE::MissingField);

    const ScriptGraphSchemaDecodeResultUVE future = DecodeScriptGraphSchemaUVE(
        R"({"schemaVersion":2,"nodes":[],"links":[],"layout":[],"metadata":{}})");
    ASSERT_FALSE(future.IsSuccessUVE());
    EXPECT_EQ(future.diagnostics[0].code, ScriptPersistenceDiagnosticCodeUVE::UnsupportedVersion);

    const ScriptGraphSchemaDecodeResultUVE duplicateLayout = DecodeScriptGraphSchemaUVE(
        R"({"schemaVersion":1,"nodes":[{"id":1,"typeId":"test"}],"links":[],"layout":[{"nodeId":1,"x":0,"y":0},{"nodeId":1,"x":1,"y":1}],"metadata":{}})");
    ASSERT_FALSE(duplicateLayout.IsSuccessUVE());
    EXPECT_EQ(duplicateLayout.diagnostics[0].code, ScriptPersistenceDiagnosticCodeUVE::DuplicateEntry);
}

TEST(ScriptGraphPersistenceUVETest, EncodeDecodeScriptGraphUVE_RoundTripsDeterministically) {
    ScriptGraphUVE graph;
    ASSERT_TRUE(graph.AddNodeUVE({2U, "test.sink"}));
    ASSERT_TRUE(graph.AddNodeUVE({1U, "test.source"}));
    ASSERT_TRUE(graph.AddLinkUVE({{1U, "Out"}, {2U, "In"}}));
    std::vector<ScriptPersistenceDiagnosticUVE> diagnostics;
    const std::string encoded = EncodeScriptGraphUVE(graph, diagnostics);
    ASSERT_TRUE(diagnostics.empty());
    const ScriptGraphDecodeResultUVE decoded = DecodeScriptGraphUVE(encoded);
    ASSERT_TRUE(decoded.IsSuccessUVE());
    EXPECT_EQ(decoded.graph->GetNodesUVE().size(), 2U);
    EXPECT_EQ(decoded.graph->GetLinksUVE().size(), 1U);
    std::vector<ScriptPersistenceDiagnosticUVE> secondDiagnostics;
    EXPECT_EQ(EncodeScriptGraphUVE(*decoded.graph, secondDiagnostics), encoded);
    EXPECT_TRUE(secondDiagnostics.empty());
}

TEST(ScriptGraphPersistenceUVETest, DecodeScriptGraphUVE_RejectsMalformedVersionAndDuplicates) {
    const ScriptGraphDecodeResultUVE malformed = DecodeScriptGraphUVE("{not-json");
    ASSERT_FALSE(malformed.IsSuccessUVE());
    EXPECT_EQ(malformed.diagnostics[0].code, ScriptPersistenceDiagnosticCodeUVE::InvalidJson);
    const ScriptGraphDecodeResultUVE unsupported = DecodeScriptGraphUVE(
        R"({"schemaVersion":99,"nodes":[],"links":[]})");
    ASSERT_FALSE(unsupported.IsSuccessUVE());
    EXPECT_EQ(unsupported.diagnostics[0].code, ScriptPersistenceDiagnosticCodeUVE::UnsupportedVersion);
    const ScriptGraphDecodeResultUVE duplicate = DecodeScriptGraphUVE(
        R"({"schemaVersion":1,"nodes":[{"id":1,"typeId":"test"},{"id":1,"typeId":"test"}],"links":[],"layout":[],"metadata":{}})");
    ASSERT_FALSE(duplicate.IsSuccessUVE());
    EXPECT_EQ(duplicate.diagnostics[0].code, ScriptPersistenceDiagnosticCodeUVE::DuplicateEntry);
}

TEST(ScriptGraphPersistenceUVETest, EncodeScriptGraphUVE_EnforcesTextLimitWithoutOutput) {
    ScriptGraphUVE graph;
    ASSERT_TRUE(graph.AddNodeUVE({1U, "test.source"}));
    std::vector<ScriptPersistenceDiagnosticUVE> diagnostics;
    EXPECT_TRUE(EncodeScriptGraphUVE(graph, diagnostics, {4096U, 8192U, 4096U, 128U, 4096U, 4U}).empty());
    ASSERT_EQ(diagnostics.size(), 1U);
    EXPECT_EQ(diagnostics[0].code, ScriptPersistenceDiagnosticCodeUVE::LimitExceeded);
}

} // namespace UVE::Scripting


namespace UVE::Scripting {

TEST(ScriptGraphEditorBackendUVETest, CommandsAreTransactionalAndNodeRemovalCleansIncidentLinks) {
    ScriptGraphEditorBackendUVE backend;
    ASSERT_TRUE(backend.AddNodeUVE({1U, "test.source"}).IsAppliedUVE());
    ASSERT_TRUE(backend.AddNodeUVE({2U, "test.sink"}).IsAppliedUVE());
    ASSERT_TRUE(backend.AddLinkUVE({{1U, "Out"}, {2U, "In"}}).IsAppliedUVE());
    EXPECT_EQ(backend.GetGraphUVE().GetLinksUVE().size(), 1U);
    EXPECT_FALSE(backend.AddLinkUVE({{1U, "Out"}, {2U, "In"}}).IsAppliedUVE());
    EXPECT_EQ(backend.GetGraphUVE().GetLinksUVE().size(), 1U);
    ASSERT_TRUE(backend.RemoveNodeUVE(1U).IsAppliedUVE());
    EXPECT_TRUE(backend.GetGraphUVE().GetNodesUVE().size() == 1U);
    EXPECT_TRUE(backend.GetGraphUVE().GetLinksUVE().empty());
}

TEST(ScriptGraphEditorBackendUVETest, UndoRedoRestoresSnapshotsAndNewEditClearsRedo) {
    ScriptGraphEditorBackendUVE backend;
    ASSERT_TRUE(backend.AddNodeUVE({1U, "test.source"}).IsAppliedUVE());
    ASSERT_TRUE(backend.AddNodeUVE({2U, "test.sink"}).IsAppliedUVE());
    EXPECT_EQ(backend.GetUndoCountUVE(), 2U);
    ASSERT_TRUE(backend.UndoUVE().IsAppliedUVE());
    EXPECT_EQ(backend.GetGraphUVE().GetNodesUVE().size(), 1U);
    EXPECT_EQ(backend.GetRedoCountUVE(), 1U);
    ASSERT_TRUE(backend.RedoUVE().IsAppliedUVE());
    EXPECT_EQ(backend.GetGraphUVE().GetNodesUVE().size(), 2U);
    ASSERT_TRUE(backend.UndoUVE().IsAppliedUVE());
    ASSERT_TRUE(backend.AddNodeUVE({3U, "test.source"}).IsAppliedUVE());
    EXPECT_EQ(backend.GetRedoCountUVE(), 0U);
    EXPECT_EQ(backend.RedoUVE().code, ScriptGraphCommandCodeUVE::NoHistory);
}

TEST(ScriptGraphCanvasUVETest, ApplyGraphSchemaReplacesAuthoritativeGraphAndLayoutWithUndo) {
    ScriptNodeRegistryUVE registry;
    RegisterTestNodesUVE(registry);
    ScriptGraphCanvasUVE canvas(registry);
    ASSERT_TRUE(canvas.AddNodeUVE({1U, "test.source"}, {1.0F, 2.0F}).IsAppliedUVE());

    ScriptGraphSchemaUVE schema{};
    ASSERT_TRUE(schema.graph.AddNodeUVE({10U, "test.source"}));
    ASSERT_TRUE(schema.graph.AddNodeUVE({20U, "test.sink"}));
    ASSERT_TRUE(schema.graph.AddLinkUVE({{10U, "Out"}, {20U, "In"}}));
    schema.layout = {{20U, 40.0F, 50.0F}, {10U, 10.0F, 20.0F}};
    const ScriptGraphCanvasCommandResultUVE applied = canvas.ApplyGraphSchemaUVE(std::move(schema));
    ASSERT_TRUE(applied.IsAppliedUVE());
    EXPECT_TRUE(canvas.GetSnapshotUVE().dirty);
    EXPECT_EQ(canvas.GetGraphUVE().GetNodesUVE().front().id, 10U);
    EXPECT_EQ(canvas.GetLayoutSnapshotUVE().entries.size(), 2U);
    EXPECT_TRUE(canvas.UndoUVE().IsAppliedUVE());
    EXPECT_EQ(canvas.GetGraphUVE().GetNodesUVE().front().id, 1U);
}

TEST(ScriptGraphCanvasUVETest, RemoveNodeUndoRestoresExactGraphLayoutSelectionAndLinkOrder) {
    ScriptNodeRegistryUVE registry;
    RegisterTestNodesUVE(registry);
    ScriptGraphCanvasUVE canvas(registry);
    ASSERT_TRUE(canvas.AddNodeUVE({1U, "test.source"}, {10.0F, 20.0F}).IsAppliedUVE());
    ASSERT_TRUE(canvas.AddNodeUVE({2U, "test.sink"}, {30.0F, 40.0F}).IsAppliedUVE());
    ASSERT_TRUE(canvas.AddNodeUVE({3U, "test.sink"}, {50.0F, 60.0F}).IsAppliedUVE());
    ASSERT_TRUE(canvas.AddLinkUVE({{1U, "Out"}, {2U, "In"}}).IsAppliedUVE());
    ASSERT_TRUE(canvas.AddLinkUVE({{1U, "Exec"}, {3U, "Exec"}}).IsAppliedUVE());
    ASSERT_TRUE(canvas.SetSelectionUVE({1U, 2U, 3U}).IsAppliedUVE());
    const ScriptGraphCanvasSnapshotUVE before = canvas.GetSnapshotUVE();

    ASSERT_TRUE(canvas.RemoveNodeUVE(2U).IsAppliedUVE());
    EXPECT_EQ(canvas.GetSnapshotUVE().selectedNodeIds, (std::vector<std::uint32_t>{1U, 3U}));
    ASSERT_TRUE(canvas.UndoUVE().IsAppliedUVE());
    const ScriptGraphCanvasSnapshotUVE restored = canvas.GetSnapshotUVE();
    EXPECT_EQ(restored.nodes, before.nodes);
    EXPECT_EQ(restored.links, before.links);
    EXPECT_EQ(restored.selectedNodeIds, before.selectedNodeIds);
    ASSERT_TRUE(canvas.RedoUVE().IsAppliedUVE());
    EXPECT_EQ(canvas.GetSnapshotUVE().nodes.size(), 2U);
    EXPECT_EQ(canvas.GetSnapshotUVE().links.size(), 1U);
}

TEST(ScriptGraphCanvasUVETest, ViewChangesAreNotUndoableAndRejectInvalidValues) {
    ScriptNodeRegistryUVE registry;
    RegisterTestNodesUVE(registry);
    ScriptGraphCanvasUVE canvas(registry);
    const std::uint64_t initialRevision = canvas.GetSnapshotUVE().revision;
    ASSERT_TRUE(canvas.SetViewUVE({{12.0F, -4.0F}, 2.0F}).IsAppliedUVE());
    EXPECT_EQ(canvas.GetUndoCountUVE(), 0U);
    EXPECT_EQ(canvas.GetSnapshotUVE().view.pan, (ScriptGraphCanvasPointUVE{12.0F, -4.0F}));
    EXPECT_EQ(canvas.GetSnapshotUVE().view.zoom, 2.0F);
    EXPECT_GT(canvas.GetSnapshotUVE().revision, initialRevision);
    EXPECT_FALSE(canvas.SetViewUVE({{0.0F, 0.0F}, 0.0F}).IsAppliedUVE());
    EXPECT_FALSE(canvas.SetViewUVE({{std::numeric_limits<float>::quiet_NaN(), 0.0F}, 1.0F}).IsAppliedUVE());
}

TEST(ScriptGraphCanvasPersistenceUVETest, EncodeDecodeAndApplyLayout_RoundTripsDeterministicallyWithUndo) {
    ScriptNodeRegistryUVE registry;
    RegisterTestNodesUVE(registry);
    ScriptGraphCanvasUVE canvas(registry);
    ASSERT_TRUE(canvas.AddNodeUVE({1U, "test.source"}, {10.0F, 20.0F}).IsAppliedUVE());
    ASSERT_TRUE(canvas.AddNodeUVE({2U, "test.sink"}, {30.0F, 40.0F}).IsAppliedUVE());
    ASSERT_TRUE(canvas.SetViewUVE({{12.0F, -4.0F}, 2.0F}).IsAppliedUVE());

    const ScriptGraphCanvasLayoutSnapshotUVE expected = canvas.GetLayoutSnapshotUVE();
    std::vector<ScriptPersistenceDiagnosticUVE> diagnostics;
    const std::string encoded = EncodeScriptGraphCanvasLayoutUVE(expected, diagnostics);
    ASSERT_TRUE(diagnostics.empty());
    const ScriptGraphCanvasLayoutDecodeResultUVE decoded = DecodeScriptGraphCanvasLayoutUVE(encoded);
    ASSERT_TRUE(decoded.IsSuccessUVE());
    ASSERT_EQ(*decoded.layout, expected);
    ScriptGraphCanvasLayoutSnapshotUVE incomplete = expected;
    incomplete.entries.pop_back();
    EXPECT_EQ(canvas.ApplyLayoutUVE(std::move(incomplete)).code,
              ScriptGraphCanvasCommandCodeUVE::Rejected);

    ASSERT_TRUE(canvas.MoveNodeUVE(1U, {100.0F, 200.0F}).IsAppliedUVE());
    ASSERT_TRUE(canvas.SetViewUVE({{-8.0F, 6.0F}, 1.0F}).IsAppliedUVE());
    ASSERT_TRUE(canvas.ApplyLayoutUVE(*decoded.layout).IsAppliedUVE());
    EXPECT_EQ(canvas.GetLayoutSnapshotUVE(), expected);
    ASSERT_TRUE(canvas.UndoUVE().IsAppliedUVE());
    EXPECT_EQ(canvas.GetLayoutSnapshotUVE().view, expected.view);
    EXPECT_EQ(canvas.GetLayoutSnapshotUVE().entries[0].position,
              (ScriptGraphCanvasPointUVE{100.0F, 200.0F}));
}

TEST(ScriptGraphCanvasPersistenceUVETest, DecodeLayout_RejectsMalformedVersionDuplicateAndTextLimit) {
    const ScriptGraphCanvasLayoutDecodeResultUVE malformed = DecodeScriptGraphCanvasLayoutUVE("{not-json");
    ASSERT_FALSE(malformed.IsSuccessUVE());
    EXPECT_EQ(malformed.diagnostics[0].code, ScriptPersistenceDiagnosticCodeUVE::InvalidJson);

    const ScriptGraphCanvasLayoutDecodeResultUVE duplicate = DecodeScriptGraphCanvasLayoutUVE(
        R"({"schemaVersion":1,"view":{"pan":{"x":0,"y":0},"zoom":1},"entries":[{"nodeId":1,"x":0,"y":0},{"nodeId":1,"x":1,"y":1}]})");
    ASSERT_FALSE(duplicate.IsSuccessUVE());
    EXPECT_EQ(duplicate.diagnostics[0].code, ScriptPersistenceDiagnosticCodeUVE::DuplicateEntry);

    ScriptGraphCanvasLayoutSnapshotUVE layout;
    layout.entries.push_back({1U, {0.0F, 0.0F}});
    std::vector<ScriptPersistenceDiagnosticUVE> diagnostics;
    EXPECT_TRUE(EncodeScriptGraphCanvasLayoutUVE(layout, diagnostics,
                                                  {128U, 8U}).empty());
    ASSERT_EQ(diagnostics.size(), 1U);
    EXPECT_EQ(diagnostics[0].code, ScriptPersistenceDiagnosticCodeUVE::LimitExceeded);
}

TEST(ScriptGraphCanvasUVETest, CommandsRejectStaleRevisionAndInvalidSelectionWithoutMutation) {
    ScriptNodeRegistryUVE registry;
    RegisterTestNodesUVE(registry);
    ScriptGraphCanvasUVE canvas(registry);
    ASSERT_TRUE(canvas.AddNodeUVE({1U, "test.source"}, {0.0F, 0.0F}).IsAppliedUVE());
    const std::uint64_t revision = canvas.GetSnapshotUVE().revision;
    ASSERT_TRUE(canvas.MoveNodeUVE(1U, {5.0F, 5.0F}, revision).IsAppliedUVE());
    const ScriptGraphCanvasSnapshotUVE beforeReject = canvas.GetSnapshotUVE();
    EXPECT_EQ(canvas.MoveNodeUVE(1U, {8.0F, 8.0F}, revision).code,
              ScriptGraphCanvasCommandCodeUVE::StaleRevision);
    EXPECT_EQ(canvas.SetSelectionUVE({1U, 1U}).code, ScriptGraphCanvasCommandCodeUVE::Rejected);
    EXPECT_EQ(canvas.GetSnapshotUVE(), beforeReject);
}

TEST(ScriptGraphCanvasUVETest, SnapshotProvidesSortedPaletteAndTypedNodePins) {
    ScriptNodeRegistryUVE registry;
    RegisterTestNodesUVE(registry);
    ScriptGraphCanvasUVE canvas(registry);
    const ScriptGraphCanvasSnapshotUVE initial = canvas.GetSnapshotUVE();
    EXPECT_FALSE(initial.dirty);
    EXPECT_FALSE(initial.canUndo);
    EXPECT_FALSE(initial.canRedo);
    ASSERT_TRUE(canvas.AddNodeUVE({1U, "test.sink"}, {0.0F, 0.0F}).IsAppliedUVE());
    const ScriptGraphCanvasSnapshotUVE snapshot = canvas.GetSnapshotUVE();
    EXPECT_EQ(snapshot.paletteNodeTypeIds, (std::vector<std::string>{"test.sink", "test.source"}));
    ASSERT_EQ(snapshot.nodes.size(), 1U);
    EXPECT_EQ(snapshot.nodes[0].displayName, "Test Sink");
    ASSERT_EQ(snapshot.nodes[0].pins.size(), 2U);
    EXPECT_EQ(snapshot.nodes[0].pins[0].name, "In");
    EXPECT_EQ(snapshot.nodes[0].pins[0].direction, ScriptPinDirectionUVE::Input);
    EXPECT_EQ(snapshot.nodes[0].category, "Uncategorized");
    EXPECT_EQ(snapshot.nodes[0].iconId, "node.default");
    EXPECT_EQ(snapshot.nodes[0].pins[0].role, ScriptPinRoleUVE::Data);
    EXPECT_TRUE(snapshot.dirty);
    EXPECT_TRUE(snapshot.canUndo);
    EXPECT_FALSE(snapshot.canRedo);

    ScriptGraphCanvasUVE selectionCanvas(registry);
    ASSERT_TRUE(selectionCanvas.AddNodeUVE({1U, "test.sink"}, {0.0F, 0.0F}).IsAppliedUVE());
    ASSERT_TRUE(selectionCanvas.SetSelectionUVE({}).IsAppliedUVE());
    EXPECT_TRUE(selectionCanvas.GetSnapshotUVE().dirty);

    ASSERT_TRUE(canvas.UndoUVE().IsAppliedUVE());
    const ScriptGraphCanvasSnapshotUVE undone = canvas.GetSnapshotUVE();
    EXPECT_TRUE(undone.dirty);
    EXPECT_FALSE(undone.canUndo);
    EXPECT_TRUE(undone.canRedo);
}

} // namespace UVE::Scripting


namespace UVE::Scripting {

TEST(ScriptDebuggerUVETest, ContinueUVE_PausesAtSourceNodeBreakpointAndContinueResumes) {
    ScriptBytecodeProgramUVE program;
    program.instructions.push_back({ScriptIrInstructionKindUVE::ExecuteNode, 10U, 0U, "test.source", {}, {}});
    program.instructions.push_back({ScriptIrInstructionKindUVE::TransferValue, 20U, 30U, {}, "Out", "In"});
    ScriptDebuggerUVE debugger;
    ASSERT_TRUE(debugger.AttachUVE(program));
    ASSERT_TRUE(debugger.SetBreakpointUVE(20U, true));
    const ScriptDebuggerSnapshotUVE paused = debugger.ContinueUVE();
    EXPECT_EQ(paused.state, ScriptDebuggerStateUVE::Paused);
    EXPECT_EQ(paused.instructionIndex, 1U);
    EXPECT_EQ(paused.sourceNodeId, 20U);
    EXPECT_EQ(paused.pauseReason, "Breakpoint reached.");
    ASSERT_EQ(paused.trace.size(), 1U);
    EXPECT_EQ(paused.trace.front().kind, ScriptVmTraceEventKindUVE::NodeExecuted);
    EXPECT_EQ(paused.trace.front().sourceNodeId, 10U);
    EXPECT_EQ(paused.trace.front().nodeTypeId, "test.source");
    const ScriptDebuggerSnapshotUVE completed = debugger.ContinueUVE();
    EXPECT_EQ(completed.state, ScriptDebuggerStateUVE::Completed);
    EXPECT_EQ(completed.executedInstructions, 2U);
    ASSERT_EQ(completed.trace.size(), 3U);
    EXPECT_EQ(completed.trace[1].kind, ScriptVmTraceEventKindUVE::ValueTransferred);
    EXPECT_EQ(completed.trace[2].kind, ScriptVmTraceEventKindUVE::Completed);
}

TEST(ScriptDebuggerUVETest, StepUVE_AdvancesOneInstructionAndReportsCompletion) {
    ScriptBytecodeProgramUVE program;
    program.instructions.resize(2U);
    ScriptDebuggerUVE debugger;
    ASSERT_TRUE(debugger.AttachUVE(program));
    const ScriptDebuggerSnapshotUVE first = debugger.StepUVE();
    EXPECT_EQ(first.state, ScriptDebuggerStateUVE::Paused);
    EXPECT_EQ(first.instructionIndex, 1U);
    EXPECT_EQ(first.executedInstructions, 1U);
    const ScriptDebuggerSnapshotUVE second = debugger.StepUVE();
    EXPECT_EQ(second.state, ScriptDebuggerStateUVE::Completed);
    EXPECT_EQ(second.instructionIndex, 2U);
}

TEST(ScriptDebuggerUVETest, StepUVE_ConditionalJumpUsesAttachedCopiedContext) {
    ScriptBytecodeProgramUVE program;
    program.instructions.push_back({ScriptIrInstructionKindUVE::ConditionalJump, 1U, 0U, "flow.branch",
                                    "Condition", {}, 2U, 1U});
    program.instructions.push_back({ScriptIrInstructionKindUVE::ExecuteNode, 20U, 0U, "test.false", {}, {}});
    program.instructions.push_back({ScriptIrInstructionKindUVE::ExecuteNode, 30U, 0U, "test.true", {}, {}});
    ScriptVmExecutionContextUVE context;
    ASSERT_TRUE(context.SetInputUVE(1U, "Condition", true));

    ScriptDebuggerUVE debugger;
    ASSERT_TRUE(debugger.AttachWithContextUVE(program, context));
    const ScriptDebuggerSnapshotUVE stepped = debugger.StepUVE();
    EXPECT_EQ(stepped.state, ScriptDebuggerStateUVE::Paused);
    EXPECT_EQ(stepped.instructionIndex, 2U);
    EXPECT_EQ(stepped.executedInstructions, 1U);
    ASSERT_EQ(stepped.trace.size(), 1U);
    EXPECT_EQ(stepped.trace.front().kind, ScriptVmTraceEventKindUVE::NodeExecuted);
    EXPECT_EQ(stepped.trace.front().message, "ConditionalJump evaluated true.");
}

TEST(ScriptDebuggerUVETest, StepUVE_SequenceDispatchContinuesToSecondTarget) {
    ScriptBytecodeProgramUVE program;
    program.instructions.push_back({ScriptIrInstructionKindUVE::SequenceDispatch, 1U, 0U, "flow.sequence",
                                    "Then", "Then2", 0U, 0U, 1U, 2U});
    program.instructions.push_back({ScriptIrInstructionKindUVE::ExecuteNode, 20U, 0U, "test.first", {}, {}});
    program.instructions.push_back({ScriptIrInstructionKindUVE::ExecuteNode, 30U, 0U, "test.second", {}, {}});

    ScriptDebuggerUVE debugger;
    ASSERT_TRUE(debugger.AttachUVE(program));
    const ScriptDebuggerSnapshotUVE sequence = debugger.StepUVE();
    EXPECT_EQ(sequence.state, ScriptDebuggerStateUVE::Paused);
    EXPECT_EQ(sequence.instructionIndex, 1U);
    ASSERT_EQ(sequence.trace.size(), 1U);
    EXPECT_EQ(sequence.trace.front().message, "SequenceDispatch selected ordered execution targets.");

    const ScriptDebuggerSnapshotUVE first = debugger.StepUVE();
    EXPECT_EQ(first.state, ScriptDebuggerStateUVE::Paused);
    EXPECT_EQ(first.instructionIndex, 2U);
    EXPECT_EQ(first.executedInstructions, 2U);

    const ScriptDebuggerSnapshotUVE second = debugger.StepUVE();
    EXPECT_EQ(second.state, ScriptDebuggerStateUVE::Completed);
    EXPECT_EQ(second.instructionIndex, 3U);
    EXPECT_EQ(second.executedInstructions, 3U);
}

TEST(ScriptCollectionValueUVETest, ValidationRejectsNonFiniteDuplicateAndUnorderedValues) {
    const ScriptArrayValueUVE array{ScriptCollectionElementTypeUVE::Number,
                                    {ScriptCollectionElementUVE{1.0F}, ScriptCollectionElementUVE{2.0F}}};
    EXPECT_TRUE(IsValidScriptArrayValueUVE(array));
    const ScriptArrayValueUVE wrongType{ScriptCollectionElementTypeUVE::Number,
                                        {ScriptCollectionElementUVE{true}}};
    EXPECT_FALSE(IsValidScriptArrayValueUVE(wrongType));

    const ScriptMapValueUVE map{ScriptCollectionElementTypeUVE::Boolean,
                                {{1.0F, ScriptCollectionElementUVE{false}},
                                 {2.0F, ScriptCollectionElementUVE{true}}}};
    EXPECT_TRUE(IsValidScriptMapValueUVE(map));
    const ScriptMapValueUVE duplicateMap{ScriptCollectionElementTypeUVE::Boolean,
                                         {{1.0F, ScriptCollectionElementUVE{false}},
                                          {1.0F, ScriptCollectionElementUVE{true}}}};
    EXPECT_FALSE(IsValidScriptMapValueUVE(duplicateMap));

    const ScriptSetValueUVE set{ScriptCollectionElementTypeUVE::Number,
                                {ScriptCollectionElementUVE{1.0F}, ScriptCollectionElementUVE{3.0F}}};
    EXPECT_TRUE(IsValidScriptSetValueUVE(set));
    const ScriptSetValueUVE duplicateSet{ScriptCollectionElementTypeUVE::Number,
                                         {ScriptCollectionElementUVE{1.0F}, ScriptCollectionElementUVE{1.0F}}};
    EXPECT_FALSE(IsValidScriptSetValueUVE(duplicateSet));

    const ScriptStructValueUVE structure{{{1U, ScriptCollectionElementUVE{1.0F}},
                                           {2U, ScriptCollectionElementUVE{ScriptVector3ValueUVE{{3.0F, 4.0F, 5.0F}}}}}};
    EXPECT_TRUE(IsValidScriptStructValueUVE(structure));
    const ScriptStructValueUVE unordered{{{2U, ScriptCollectionElementUVE{1.0F}},
                                          {1U, ScriptCollectionElementUVE{2.0F}}}};
    EXPECT_FALSE(IsValidScriptStructValueUVE(unordered));
}

TEST(ScriptVmUVETest, ExecuteScriptBytecodeUVE_StoresAndRetrievesTypedCollections) {
    const ScriptArrayValueUVE array{ScriptCollectionElementTypeUVE::Number,
                                    {ScriptCollectionElementUVE{1.0F}, ScriptCollectionElementUVE{2.0F}}};
    ScriptBytecodeProgramUVE program;
    program.instructions.push_back({ScriptIrInstructionKindUVE::ExecuteNode, 1U, 0U, "variable.make_array", {}, {}});
    program.instructions.push_back({ScriptIrInstructionKindUVE::ExecuteNode, 2U, 0U, "variable.get_array", {}, {}});
    ScriptVmExecutionContextUVE context;
    ASSERT_TRUE(context.SetInputUVE(1U, "Slot", 4.0F));
    ASSERT_TRUE(context.SetInputUVE(1U, "Value", array));
    ASSERT_TRUE(context.SetInputUVE(2U, "Slot", 4.0F));
    const ScriptVmExecutionResultUVE result = ExecuteScriptBytecodeUVE(program, context);
    ASSERT_TRUE(result.IsSuccessUVE());
    EXPECT_EQ(context.FindLocalVariableUVE(4U), std::optional<ScriptVmValueUVE>(array));
    EXPECT_EQ(context.FindOutputUVE(2U, "Result"), std::optional<ScriptVmValueUVE>(array));

    const ScriptMapValueUVE map{ScriptCollectionElementTypeUVE::Number,
                                {{1.0F, ScriptCollectionElementUVE{3.0F}}}};
    ASSERT_TRUE(context.SetInputUVE(3U, "Slot", 5.0F));
    ASSERT_TRUE(context.SetInputUVE(3U, "Value", map));
    ScriptBytecodeProgramUVE mapProgram;
    mapProgram.instructions.push_back({ScriptIrInstructionKindUVE::ExecuteNode, 3U, 0U, "variable.make_map", {}, {}});
    const ScriptVmExecutionResultUVE mapResult = ExecuteScriptBytecodeUVE(mapProgram, context);
    ASSERT_TRUE(mapResult.IsSuccessUVE());
    EXPECT_EQ(context.FindLocalVariableUVE(5U), std::optional<ScriptVmValueUVE>(map));
}

TEST(ScriptCompilerIRUVETest, CompileScriptGraphToIrUVE_StagesOneTypedCollectionTransfer) {
    ScriptNodeRegistryUVE registry;
    ASSERT_TRUE(RegisterBuiltInScriptNodesUVE(registry));
    ScriptGraphUVE graph;
    ASSERT_TRUE(graph.AddNodeUVE({1U, "variable.set_array"}));
    ASSERT_TRUE(graph.AddNodeUVE({2U, "variable.get_array"}));
    ASSERT_TRUE(graph.AddLinkUVE({{2U, "Result"}, {1U, "Value"}}));
    const ScriptIrCompileResultUVE result = CompileScriptGraphToIrUVE(graph, registry);
    ASSERT_TRUE(result.IsSuccessUVE());
    ASSERT_EQ(result.program->instructions.size(), 3U);
    EXPECT_EQ(result.program->instructions[0].nodeTypeId, "variable.get_array");
    EXPECT_EQ(result.program->instructions[1].kind, ScriptIrInstructionKindUVE::TransferValue);
    EXPECT_TRUE(result.program->instructions[1].isStagedTransfer);
    EXPECT_EQ(result.program->instructions[2].nodeTypeId, "variable.set_array");
}

TEST(ScriptDebuggerUVETest, StepUVE_FlowControlDispatchUsesAttachedContext) {
    ScriptBytecodeProgramUVE eventProgram;
    eventProgram.instructions.push_back({ScriptIrInstructionKindUVE::FlowControlDispatch, 1U, 0U, "flow.event",
                                          "Event", {}, 1U, 2U, 0U, 0U, false, 2U});
    eventProgram.instructions.push_back({ScriptIrInstructionKindUVE::FlowControlDispatch, 2U, 0U, "flow.return",
                                          "In", {}, 2U, 2U, 0U, 0U, false, 2U});
    ScriptDebuggerUVE eventDebugger;
    ASSERT_TRUE(eventDebugger.AttachUVE(eventProgram));
    const ScriptDebuggerSnapshotUVE eventStep = eventDebugger.StepUVE();
    EXPECT_EQ(eventStep.state, ScriptDebuggerStateUVE::Paused);
    EXPECT_EQ(eventStep.instructionIndex, 1U);
    ASSERT_EQ(eventStep.trace.size(), 1U);
    EXPECT_EQ(eventStep.trace.front().message, "Event fired Then.");
    const ScriptDebuggerSnapshotUVE eventReturn = eventDebugger.StepUVE();
    EXPECT_EQ(eventReturn.state, ScriptDebuggerStateUVE::Completed);
    EXPECT_EQ(eventReturn.instructionIndex, 2U);

    ScriptBytecodeProgramUVE delayProgram;
    delayProgram.instructions.push_back({ScriptIrInstructionKindUVE::FlowControlDispatch, 10U, 0U, "flow.delay",
                                          "In", {}, 1U, 1U, 0U, 0U, false, 1U});
    delayProgram.instructions.push_back({ScriptIrInstructionKindUVE::FlowControlDispatch, 11U, 0U, "flow.return",
                                          "In", {}, 2U, 2U, 0U, 0U, false, 2U});
    ScriptVmExecutionContextUVE context;
    ASSERT_TRUE(context.SetInputUVE(10U, "Frames", 1.0F));
    ScriptDebuggerUVE delayDebugger;
    ASSERT_TRUE(delayDebugger.AttachWithContextUVE(delayProgram, context));
    const ScriptDebuggerSnapshotUVE delayStart = delayDebugger.StepUVE();
    EXPECT_EQ(delayStart.state, ScriptDebuggerStateUVE::Paused);
    EXPECT_EQ(delayStart.instructionIndex, 0U);
    EXPECT_EQ(delayStart.trace.front().message, "Delay yielded until the next debugger step.");
    const ScriptDebuggerSnapshotUVE delayThen = delayDebugger.StepUVE();
    EXPECT_EQ(delayThen.state, ScriptDebuggerStateUVE::Paused);
    EXPECT_EQ(delayThen.instructionIndex, 1U);
    EXPECT_EQ(delayThen.trace.back().message, "Delay dispatched Then.");
    const ScriptDebuggerSnapshotUVE delayReturn = delayDebugger.StepUVE();
    EXPECT_EQ(delayReturn.state, ScriptDebuggerStateUVE::Completed);
    EXPECT_EQ(delayReturn.instructionIndex, 2U);
}

TEST(ScriptDebuggerUVETest, ContinueUVE_BoundsCopiedTraceHistory) {
    ScriptBytecodeProgramUVE program;
    program.instructions.resize(ScriptDebuggerUVE::kMaximumTraceEventsUVE + 1U);
    ScriptDebuggerUVE debugger;
    ASSERT_TRUE(debugger.AttachUVE(std::move(program)));

    const ScriptDebuggerSnapshotUVE snapshot = debugger.ContinueUVE();
    EXPECT_EQ(snapshot.state, ScriptDebuggerStateUVE::Completed);
    EXPECT_EQ(snapshot.trace.size(), ScriptDebuggerUVE::kMaximumTraceEventsUVE);
    EXPECT_TRUE(snapshot.traceTruncated);
}

TEST(ScriptDebuggerUVETest, SetBreakpointUVE_ProvidesSortedSnapshotAndRejectsEmptyNodeId) {
    ScriptDebuggerUVE debugger;
    EXPECT_FALSE(debugger.SetBreakpointUVE(0U, true));
    EXPECT_TRUE(debugger.SetBreakpointUVE(20U, true));
    EXPECT_TRUE(debugger.SetBreakpointUVE(10U, true));
    const ScriptDebuggerSnapshotUVE snapshot = debugger.GetSnapshotUVE();
    EXPECT_EQ(snapshot.breakpointNodeIds, (std::vector<std::uint32_t>{10U, 20U}));
}

} // namespace UVE::Scripting
namespace UVE::Scripting {
TEST(ScriptGraphCanvasUVETest, SetPinDefaultValueValidatesAndRecordsNativeHistory) {
    ScriptNodeRegistryUVE registry;
    ASSERT_TRUE(registry.RegisterNodeTypeUVE(ScriptNodeTypeDescriptorUVE{
        "test.default", "Default", {{"Value", ScriptPinDirectionUVE::Input, ScriptValueTypeUVE::Number,
                                        ScriptPinRoleUVE::Data, std::string("1.0")},
                                       {"Out", ScriptPinDirectionUVE::Output, ScriptValueTypeUVE::Number}}}));
    ScriptGraphCanvasUVE canvas(registry);
    ASSERT_TRUE(canvas.AddNodeUVE({1U, "test.default"}, {0.0F, 0.0F}).IsAppliedUVE());

    const auto applied = canvas.SetPinDefaultValueUVE(1U, "Value", "2.5");
    ASSERT_TRUE(applied.IsAppliedUVE());
    const ScriptGraphCanvasSnapshotUVE changed = canvas.GetSnapshotUVE();
    ASSERT_EQ(changed.nodes.size(), 1U);
    ASSERT_EQ(changed.nodes[0].pins.size(), 2U);
    EXPECT_EQ(changed.nodes[0].pins[0].defaultValue, std::optional<std::string>("2.5"));
    EXPECT_TRUE(changed.dirty);
    EXPECT_TRUE(changed.canUndo);

    EXPECT_FALSE(canvas.SetPinDefaultValueUVE(1U, "Value", "not-a-number").IsAppliedUVE());
    EXPECT_FALSE(canvas.SetPinDefaultValueUVE(1U, "Out", "2.5").IsAppliedUVE());
    ASSERT_TRUE(canvas.UndoUVE(changed.revision).IsAppliedUVE());
    const ScriptGraphCanvasSnapshotUVE restored = canvas.GetSnapshotUVE();
    ASSERT_EQ(restored.nodes.size(), 1U);
    EXPECT_EQ(restored.nodes[0].pins[0].defaultValue, std::optional<std::string>("1.0"));
}

TEST(ScriptGraphCanvasUVETest, SnapshotExposesDescriptorRichPaletteInDeterministicOrder) {
    ScriptNodeRegistryUVE registry;
    ASSERT_TRUE(registry.RegisterNodeTypeUVE(ScriptNodeTypeDescriptorUVE{
        "test.late", "Late", {{"Value", ScriptPinDirectionUVE::Input, ScriptValueTypeUVE::Number}},
        "FLOW", "node.branch", 20U, kScriptNodePresentationFlagCollapsibleUVE}));
    ASSERT_TRUE(registry.RegisterNodeTypeUVE(ScriptNodeTypeDescriptorUVE{
        "test.early", "Early", {{"Exec", ScriptPinDirectionUVE::Output, ScriptValueTypeUVE::Execution}},
        "EVENT", "node.event", 10U, kScriptNodePresentationFlagCompactUVE}));
    ScriptGraphCanvasUVE canvas(registry);
    ASSERT_TRUE(canvas.AddNodeTypeUVE("test.early", {24.0F, 48.0F}).IsAppliedUVE());

    const ScriptGraphCanvasSnapshotUVE snapshot = canvas.GetSnapshotUVE();

    ASSERT_EQ(snapshot.paletteNodeTypeIds.size(), 2U);
    ASSERT_EQ(snapshot.paletteDescriptors.size(), 2U);
    EXPECT_EQ(snapshot.paletteDescriptors[0].typeId, "test.early");
    EXPECT_EQ(snapshot.paletteDescriptors[0].category, "EVENT");
    EXPECT_EQ(snapshot.paletteDescriptors[0].displayOrder, 10U);
    ASSERT_EQ(snapshot.paletteDescriptors[0].pins.size(), 1U);
    EXPECT_EQ(snapshot.paletteDescriptors[0].pins[0].name, "Exec");
    EXPECT_EQ(snapshot.paletteDescriptors[1].typeId, "test.late");
    EXPECT_EQ(snapshot.paletteDescriptors[1].category, "FLOW");
}

TEST(ScriptHotReloadManagerUVETest, LoadInitialAndReloadUVE_PublishOnlyValidatedCandidates) {

    ScriptBytecodeProgramUVE initial;
    initial.instructions.push_back({ScriptIrInstructionKindUVE::ExecuteNode, 10U, 0U, "test.source", {}, {}});
    std::vector<std::uint8_t> initialBytes;
    std::vector<ScriptBytecodeDiagnosticUVE> diagnostics;
    initialBytes = EncodeScriptBytecodeUVE(initial, diagnostics);
    ASSERT_TRUE(diagnostics.empty());

    ScriptHotReloadManagerUVE manager;
    const ScriptHotReloadResultUVE loaded = manager.LoadInitialUVE(initialBytes);
    ASSERT_TRUE(loaded.IsAcceptedUVE());
    EXPECT_EQ(loaded.activeGeneration, 1U);
    EXPECT_TRUE(manager.GetSnapshotUVE().hasActiveProgram);

    std::vector<std::uint8_t> invalid = initialBytes;
    invalid[0U] = static_cast<std::uint8_t>('X');
    const ScriptHotReloadResultUVE rejected = manager.ReloadUVE(invalid);
    EXPECT_FALSE(rejected.IsAcceptedUVE());
    EXPECT_TRUE(rejected.lastKnownGoodRetained);
    EXPECT_EQ(rejected.activeGeneration, 1U);
    EXPECT_EQ(manager.GetActiveProgramUVE()->instructions.size(), 1U);

    initial.instructions.push_back({ScriptIrInstructionKindUVE::TransferValue, 10U, 20U, {}, "Out", "In"});
    diagnostics.clear();
    const std::vector<std::uint8_t> replacement = EncodeScriptBytecodeUVE(initial, diagnostics);
    ASSERT_TRUE(diagnostics.empty());
    const ScriptHotReloadResultUVE accepted = manager.ReloadUVE(replacement);
    EXPECT_TRUE(accepted.IsAcceptedUVE());
    EXPECT_EQ(accepted.activeGeneration, 2U);
    EXPECT_TRUE(accepted.compatibleStatePreserved);
    EXPECT_EQ(manager.GetSnapshotUVE().instructionCount, 2U);
}

TEST(ScriptHotReloadManagerUVETest, ReloadUVE_ReportsNoActiveProgramWhenInitialCandidateIsInvalid) {
    ScriptHotReloadManagerUVE manager;
    const ScriptHotReloadResultUVE result = manager.ReloadUVE({0x00U, 0x01U});
    EXPECT_EQ(result.code, ScriptHotReloadCodeUVE::NoActiveProgram);
    EXPECT_FALSE(result.lastKnownGoodRetained);
    EXPECT_FALSE(manager.GetSnapshotUVE().hasActiveProgram);
}

} // namespace UVE::Scripting


namespace UVE::Scripting {

TEST(ScriptVmUVETest, ExecuteScriptBytecodeUVE_PersistsTypedLocalVariablesAcrossMakeGetAndSet) {
    ScriptBytecodeProgramUVE program;
    program.instructions.push_back({ScriptIrInstructionKindUVE::ExecuteNode, 1U, 0U,
                                    "variable.make_number", {}, {}});
    program.instructions.push_back({ScriptIrInstructionKindUVE::ExecuteNode, 2U, 0U,
                                    "variable.set_number", {}, {}});
    program.instructions.push_back({ScriptIrInstructionKindUVE::ExecuteNode, 3U, 0U,
                                    "variable.get_number", {}, {}});
    program.instructions.push_back({ScriptIrInstructionKindUVE::ExecuteNode, 4U, 0U,
                                    "variable.make_boolean", {}, {}});
    program.instructions.push_back({ScriptIrInstructionKindUVE::ExecuteNode, 5U, 0U,
                                    "variable.get_boolean", {}, {}});
    program.instructions.push_back({ScriptIrInstructionKindUVE::ExecuteNode, 6U, 0U,
                                    "variable.make_vector3", {}, {}});
    program.instructions.push_back({ScriptIrInstructionKindUVE::ExecuteNode, 7U, 0U,
                                    "variable.set_vector3", {}, {}});
    program.instructions.push_back({ScriptIrInstructionKindUVE::ExecuteNode, 8U, 0U,
                                    "variable.get_vector3", {}, {}});

    ScriptVmExecutionContextUVE context;
    ASSERT_TRUE(context.SetInputUVE(1U, "Slot", 0.0F));
    ASSERT_TRUE(context.SetInputUVE(1U, "Value", 1.5F));
    ASSERT_TRUE(context.SetInputUVE(2U, "Slot", 0.0F));
    ASSERT_TRUE(context.SetInputUVE(2U, "Value", 4.0F));
    ASSERT_TRUE(context.SetInputUVE(3U, "Slot", 0.0F));
    ASSERT_TRUE(context.SetInputUVE(4U, "Slot", 1.0F));
    ASSERT_TRUE(context.SetInputUVE(4U, "Value", true));
    ASSERT_TRUE(context.SetInputUVE(5U, "Slot", 1.0F));
    ASSERT_TRUE(context.SetInputUVE(6U, "Slot", 2.0F));
    ASSERT_TRUE(context.SetInputUVE(6U, "Value", ScriptVector3ValueUVE{{1.0F, 2.0F, 3.0F}}));
    ASSERT_TRUE(context.SetInputUVE(7U, "Slot", 2.0F));
    ASSERT_TRUE(context.SetInputUVE(7U, "Value", ScriptVector3ValueUVE{{4.0F, 5.0F, 6.0F}}));
    ASSERT_TRUE(context.SetInputUVE(8U, "Slot", 2.0F));

    const ScriptVmExecutionResultUVE result = ExecuteScriptBytecodeUVE(program, context);
    ASSERT_TRUE(result.IsSuccessUVE());
    ASSERT_EQ(context.localVariables.size(), 3U);
    ASSERT_TRUE(context.FindOutputUVE(3U, "Result").has_value());
    EXPECT_FLOAT_EQ(std::get<float>(*context.FindOutputUVE(3U, "Result")), 4.0F);
    ASSERT_TRUE(context.FindOutputUVE(5U, "Result").has_value());
    EXPECT_TRUE(std::get<bool>(*context.FindOutputUVE(5U, "Result")));
    ASSERT_TRUE(context.FindOutputUVE(8U, "Result").has_value());
    EXPECT_EQ(std::get<ScriptVector3ValueUVE>(*context.FindOutputUVE(8U, "Result")),
              (ScriptVector3ValueUVE{{4.0F, 5.0F, 6.0F}}));

    EXPECT_TRUE(context.SetLocalVariableUVE(0U, 9.0F));
    EXPECT_FLOAT_EQ(std::get<float>(*context.FindLocalVariableUVE(0U)), 9.0F);
}

TEST(ScriptVmUVETest, LocalVariableContextUVE_RejectsTypeCollisionsAndExhaustedSlots) {
    ScriptVmExecutionContextUVE context;
    ASSERT_TRUE(context.InitializeLocalVariableUVE(0U, 1.0F));
    EXPECT_TRUE(context.InitializeLocalVariableUVE(0U, 2.0F));
    EXPECT_FALSE(context.InitializeLocalVariableUVE(0U, true));
    EXPECT_FALSE(context.SetLocalVariableUVE(0U, true));
    EXPECT_TRUE(context.SetLocalVariableUVE(0U, 3.0F));
    EXPECT_FALSE(context.SetLocalVariableUVE(0U, std::numeric_limits<float>::quiet_NaN()));
    EXPECT_FALSE(context.InitializeLocalVariableUVE(250U, ScriptVector3ValueUVE{{
        std::numeric_limits<float>::infinity(), 0.0F, 0.0F}}));
    for (std::uint32_t slot = 1U; slot < ScriptVmExecutionContextUVE::kMaximumLocalVariablesUVE; ++slot) {
        ASSERT_TRUE(context.InitializeLocalVariableUVE(slot, false));
    }
    EXPECT_EQ(context.localVariables.size(), ScriptVmExecutionContextUVE::kMaximumLocalVariablesUVE);
    EXPECT_FALSE(context.InitializeLocalVariableUVE(
        static_cast<std::uint32_t>(ScriptVmExecutionContextUVE::kMaximumLocalVariablesUVE), 0.0F));
}

TEST(ScriptCompilerIRUVETest, CompileScriptGraphToIrUVE_StagesBooleanVariableGetIntoBranchCondition) {
    ScriptNodeRegistryUVE registry;
    ASSERT_TRUE(RegisterBuiltInScriptNodesUVE(registry));
    ScriptGraphUVE graph;
    ASSERT_TRUE(graph.AddNodeUVE({10U, "variable.get_boolean"}));
    ASSERT_TRUE(graph.AddNodeUVE({20U, "flow.branch"}));
    ASSERT_TRUE(graph.AddLinkUVE({{10U, "Result"}, {20U, "Condition"}}));

    const ScriptIrCompileResultUVE compiled = CompileScriptGraphToIrUVE(graph, registry);
    ASSERT_TRUE(compiled.IsSuccessUVE());
    ASSERT_TRUE(compiled.program.has_value());
    ASSERT_GE(compiled.program->instructions.size(), 3U);
    EXPECT_EQ(compiled.program->instructions[0].nodeTypeId, "variable.get_boolean");
    EXPECT_EQ(compiled.program->instructions[1].kind, ScriptIrInstructionKindUVE::TransferValue);
    EXPECT_TRUE(compiled.program->instructions[1].isStagedTransfer);
    EXPECT_EQ(compiled.program->instructions[1].targetPinName, "Condition");
    EXPECT_EQ(compiled.program->instructions[2].kind, ScriptIrInstructionKindUVE::ConditionalJump);
}

TEST(ScriptRuntimeUVETest, TickDetailedUVE_PreservesLocalVariablesAndReportsSnapshotCount) {
    ScriptRuntimeUVE runtime;
    ScriptBytecodeProgramUVE program;
    program.instructions.push_back({ScriptIrInstructionKindUVE::ExecuteNode, 1U, 0U,
                                    "variable.make_number", {}, {}});
    program.instructions.push_back({ScriptIrInstructionKindUVE::ExecuteNode, 2U, 0U,
                                    "variable.get_number", {}, {}});
    ASSERT_TRUE(runtime.AttachUVE({44U, 1U}, program));
    ScriptRuntimeStateUVE state;
    ASSERT_TRUE(state.executionContext.SetInputUVE(1U, "Slot", 3.0F));
    ASSERT_TRUE(state.executionContext.SetInputUVE(1U, "Value", 6.0F));
    ASSERT_TRUE(state.executionContext.SetInputUVE(2U, "Slot", 3.0F));
    ASSERT_TRUE(runtime.SetStateUVE({44U, 1U}, state));

    ASSERT_TRUE(runtime.TickDetailedUVE().IsSuccessUVE());
    const auto snapshot = runtime.GetSnapshotUVE();
    ASSERT_EQ(snapshot.size(), 1U);
    EXPECT_EQ(snapshot[0].stateLocalVariableCount, 1U);
    const auto storedState = runtime.GetStateUVE({44U, 1U});
    ASSERT_TRUE(storedState.has_value());
    ASSERT_EQ(storedState->executionContext.localVariables.size(), 1U);
    EXPECT_FLOAT_EQ(std::get<float>(*storedState->executionContext.FindLocalVariableUVE(3U)), 6.0F);
}

} // namespace UVE::Scripting


namespace UVE::Scripting {

TEST(ScriptVmUVETest, ExecuteScriptBytecodeUVE_EvaluatesVector2FunctionFamily) {
    ScriptBytecodeProgramUVE program;
    program.instructions.push_back({ScriptIrInstructionKindUVE::ExecuteNode, 1U, 0U,
                                    "math.vector2.make", {}, {}});
    program.instructions.push_back({ScriptIrInstructionKindUVE::ExecuteNode, 2U, 0U,
                                    "math.vector2.add", {}, {}});
    program.instructions.push_back({ScriptIrInstructionKindUVE::ExecuteNode, 3U, 0U,
                                    "math.vector2.subtract", {}, {}});
    program.instructions.push_back({ScriptIrInstructionKindUVE::ExecuteNode, 4U, 0U,
                                    "math.vector2.multiply", {}, {}});
    program.instructions.push_back({ScriptIrInstructionKindUVE::ExecuteNode, 5U, 0U,
                                    "math.vector2.length", {}, {}});
    program.instructions.push_back({ScriptIrInstructionKindUVE::ExecuteNode, 6U, 0U,
                                    "math.vector2.normalize", {}, {}});

    ScriptVmExecutionContextUVE context;
    ASSERT_TRUE(context.SetInputUVE(1U, "X", 3.0F));
    ASSERT_TRUE(context.SetInputUVE(1U, "Y", 4.0F));
    ASSERT_TRUE(context.SetInputUVE(2U, "A", ScriptVector2ValueUVE{{3.0F, 4.0F}}));
    ASSERT_TRUE(context.SetInputUVE(2U, "B", ScriptVector2ValueUVE{{1.0F, 2.0F}}));
    ASSERT_TRUE(context.SetInputUVE(3U, "A", ScriptVector2ValueUVE{{3.0F, 4.0F}}));
    ASSERT_TRUE(context.SetInputUVE(3U, "B", ScriptVector2ValueUVE{{1.0F, 2.0F}}));
    ASSERT_TRUE(context.SetInputUVE(4U, "Vector", ScriptVector2ValueUVE{{3.0F, 4.0F}}));
    ASSERT_TRUE(context.SetInputUVE(4U, "Scale", 2.0F));
    ASSERT_TRUE(context.SetInputUVE(5U, "Vector", ScriptVector2ValueUVE{{3.0F, 4.0F}}));
    ASSERT_TRUE(context.SetInputUVE(6U, "Vector", ScriptVector2ValueUVE{{3.0F, 4.0F}}));

    const ScriptVmExecutionResultUVE result = ExecuteScriptBytecodeUVE(program, context);
    ASSERT_TRUE(result.IsSuccessUVE());
    EXPECT_EQ(std::get<ScriptVector2ValueUVE>(*context.FindOutputUVE(1U, "Vector")),
              (ScriptVector2ValueUVE{{3.0F, 4.0F}}));
    EXPECT_EQ(std::get<ScriptVector2ValueUVE>(*context.FindOutputUVE(2U, "Result")),
              (ScriptVector2ValueUVE{{4.0F, 6.0F}}));
    EXPECT_EQ(std::get<ScriptVector2ValueUVE>(*context.FindOutputUVE(3U, "Result")),
              (ScriptVector2ValueUVE{{2.0F, 2.0F}}));
    EXPECT_EQ(std::get<ScriptVector2ValueUVE>(*context.FindOutputUVE(4U, "Result")),
              (ScriptVector2ValueUVE{{6.0F, 8.0F}}));
    EXPECT_FLOAT_EQ(std::get<float>(*context.FindOutputUVE(5U, "Length")), 5.0F);
    EXPECT_EQ(std::get<ScriptVector2ValueUVE>(*context.FindOutputUVE(6U, "Result")),
              (ScriptVector2ValueUVE{{0.6F, 0.8F}}));

    ScriptBytecodeProgramUVE zeroProgram;
    zeroProgram.instructions.push_back({ScriptIrInstructionKindUVE::ExecuteNode, 7U, 0U,
                                        "math.vector2.normalize", {}, {}});
    ScriptVmExecutionContextUVE zeroContext;
    ASSERT_TRUE(zeroContext.SetInputUVE(7U, "Vector", ScriptVector2ValueUVE{{0.0F, 0.0F}}));
    const ScriptVmExecutionResultUVE zeroResult = ExecuteScriptBytecodeUVE(zeroProgram, zeroContext);
    EXPECT_FALSE(zeroResult.IsSuccessUVE());
    EXPECT_EQ(zeroResult.status, ScriptVmStatusUVE::NodeExecutionFailed);
}

TEST(ScriptVmUVETest, ExecuteScriptBytecodeUVE_EvaluatesVectorMathV2Nodes) {
    ScriptBytecodeProgramUVE program;
    program.instructions.push_back({ScriptIrInstructionKindUVE::ExecuteNode, 1U, 0U,
                                    "math.vector2.dot", {}, {}});
    program.instructions.push_back({ScriptIrInstructionKindUVE::ExecuteNode, 2U, 0U,
                                    "math.vector2.distance", {}, {}});
    program.instructions.push_back({ScriptIrInstructionKindUVE::ExecuteNode, 3U, 0U,
                                    "math.vector2.direction", {}, {}});
    program.instructions.push_back({ScriptIrInstructionKindUVE::ExecuteNode, 4U, 0U,
                                    "math.vector2.lerp", {}, {}});
    program.instructions.push_back({ScriptIrInstructionKindUVE::ExecuteNode, 5U, 0U,
                                    "math.vector3.distance", {}, {}});
    program.instructions.push_back({ScriptIrInstructionKindUVE::ExecuteNode, 6U, 0U,
                                    "math.vector3.direction", {}, {}});
    program.instructions.push_back({ScriptIrInstructionKindUVE::ExecuteNode, 7U, 0U,
                                    "math.vector3.lerp", {}, {}});
    ScriptVmExecutionContextUVE context;
    ASSERT_TRUE(context.SetInputUVE(1U, "A", ScriptVector2ValueUVE{{3.0F, 4.0F}}));
    ASSERT_TRUE(context.SetInputUVE(1U, "B", ScriptVector2ValueUVE{{1.0F, 2.0F}}));
    ASSERT_TRUE(context.SetInputUVE(2U, "A", ScriptVector2ValueUVE{{0.0F, 0.0F}}));
    ASSERT_TRUE(context.SetInputUVE(2U, "B", ScriptVector2ValueUVE{{3.0F, 4.0F}}));
    ASSERT_TRUE(context.SetInputUVE(3U, "From", ScriptVector2ValueUVE{{1.0F, 2.0F}}));
    ASSERT_TRUE(context.SetInputUVE(3U, "To", ScriptVector2ValueUVE{{4.0F, 6.0F}}));
    ASSERT_TRUE(context.SetInputUVE(4U, "A", ScriptVector2ValueUVE{{0.0F, 0.0F}}));
    ASSERT_TRUE(context.SetInputUVE(4U, "B", ScriptVector2ValueUVE{{4.0F, 6.0F}}));
    ASSERT_TRUE(context.SetInputUVE(4U, "Alpha", 0.5F));
    ASSERT_TRUE(context.SetInputUVE(5U, "A", ScriptVector3ValueUVE{{0.0F, 0.0F, 0.0F}}));
    ASSERT_TRUE(context.SetInputUVE(5U, "B", ScriptVector3ValueUVE{{3.0F, 4.0F, 0.0F}}));
    ASSERT_TRUE(context.SetInputUVE(6U, "From", ScriptVector3ValueUVE{{0.0F, 0.0F, 0.0F}}));
    ASSERT_TRUE(context.SetInputUVE(6U, "To", ScriptVector3ValueUVE{{0.0F, 3.0F, 4.0F}}));
    ASSERT_TRUE(context.SetInputUVE(7U, "A", ScriptVector3ValueUVE{{0.0F, 0.0F, 0.0F}}));
    ASSERT_TRUE(context.SetInputUVE(7U, "B", ScriptVector3ValueUVE{{4.0F, 6.0F, 8.0F}}));
    ASSERT_TRUE(context.SetInputUVE(7U, "Alpha", 0.5F));
    const ScriptVmExecutionResultUVE result = ExecuteScriptBytecodeUVE(program, context);
    ASSERT_TRUE(result.IsSuccessUVE());
    EXPECT_FLOAT_EQ(std::get<float>(*context.FindOutputUVE(1U, "Result")), 11.0F);
    EXPECT_FLOAT_EQ(std::get<float>(*context.FindOutputUVE(2U, "Distance")), 5.0F);
    EXPECT_EQ(std::get<ScriptVector2ValueUVE>(*context.FindOutputUVE(3U, "Result")),
              (ScriptVector2ValueUVE{{0.6F, 0.8F}}));
    EXPECT_EQ(std::get<ScriptVector2ValueUVE>(*context.FindOutputUVE(4U, "Result")),
              (ScriptVector2ValueUVE{{2.0F, 3.0F}}));
    EXPECT_FLOAT_EQ(std::get<float>(*context.FindOutputUVE(5U, "Distance")), 5.0F);
    EXPECT_EQ(std::get<ScriptVector3ValueUVE>(*context.FindOutputUVE(6U, "Result")),
              (ScriptVector3ValueUVE{{0.0F, 0.6F, 0.8F}}));
    EXPECT_EQ(std::get<ScriptVector3ValueUVE>(*context.FindOutputUVE(7U, "Result")),
              (ScriptVector3ValueUVE{{2.0F, 3.0F, 4.0F}}));

    ScriptBytecodeProgramUVE invalidProgram;
    invalidProgram.instructions.push_back({ScriptIrInstructionKindUVE::ExecuteNode, 8U, 0U,
                                           "math.vector2.direction", {}, {}});
    ScriptVmExecutionContextUVE invalidContext;
    ASSERT_TRUE(invalidContext.SetInputUVE(8U, "From", ScriptVector2ValueUVE{{1.0F, 1.0F}}));
    ASSERT_TRUE(invalidContext.SetInputUVE(8U, "To", ScriptVector2ValueUVE{{1.0F, 1.0F}}));
    EXPECT_FALSE(ExecuteScriptBytecodeUVE(invalidProgram, invalidContext).IsSuccessUVE());
}

TEST(ScriptVmUVETest, ExecuteScriptBytecodeUVE_EvaluatesExplicitConversionsAndDefaults) {
    ScriptBytecodeProgramUVE program;
    program.instructions.push_back({ScriptIrInstructionKindUVE::ExecuteNode, 1U, 0U,
                                    "convert.number_to_boolean", {}, {}});
    program.instructions.push_back({ScriptIrInstructionKindUVE::ExecuteNode, 2U, 0U,
                                    "convert.boolean_to_number", {}, {}});
    program.instructions.push_back({ScriptIrInstructionKindUVE::ExecuteNode, 3U, 0U,
                                    "convert.vector2_to_vector3", {}, {}});
    program.instructions.push_back({ScriptIrInstructionKindUVE::ExecuteNode, 4U, 0U,
                                    "convert.vector3_to_vector2", {}, {}});
    ScriptVmExecutionContextUVE context;
    ASSERT_TRUE(context.SetInputUVE(1U, "Value", 0.0F));
    ASSERT_TRUE(context.SetInputUVE(2U, "Value", true));
    ASSERT_TRUE(context.SetInputUVE(3U, "Vector", ScriptVector2ValueUVE{{3.0F, 4.0F}}));
    ASSERT_TRUE(context.SetInputUVE(4U, "Vector", ScriptVector3ValueUVE{{6.0F, 7.0F, 8.0F}}));

    const ScriptVmExecutionResultUVE result = ExecuteScriptBytecodeUVE(program, context);
    ASSERT_TRUE(result.IsSuccessUVE());
    EXPECT_FALSE(std::get<bool>(*context.FindOutputUVE(1U, "Result")));
    EXPECT_FLOAT_EQ(std::get<float>(*context.FindOutputUVE(2U, "Result")), 1.0F);
    EXPECT_EQ(std::get<ScriptVector3ValueUVE>(*context.FindOutputUVE(3U, "Result")),
              (ScriptVector3ValueUVE{{3.0F, 4.0F, 0.0F}}));
    EXPECT_EQ(std::get<ScriptVector2ValueUVE>(*context.FindOutputUVE(4U, "Result")),
              (ScriptVector2ValueUVE{{6.0F, 7.0F}}));

    ScriptBytecodeProgramUVE invalidProgram;
    invalidProgram.instructions.push_back({ScriptIrInstructionKindUVE::ExecuteNode, 5U, 0U,
                                           "convert.number_to_boolean", {}, {}});
    ScriptVmExecutionContextUVE invalidContext;
    ASSERT_TRUE(invalidContext.SetInputUVE(5U, "Value", std::numeric_limits<float>::quiet_NaN()));
    const ScriptVmExecutionResultUVE invalidResult = ExecuteScriptBytecodeUVE(invalidProgram, invalidContext);
    EXPECT_FALSE(invalidResult.IsSuccessUVE());
}

TEST(ScriptCompilerIRUVETest, CompileScriptGraphToIrUVE_StagesVector2ProducerBeforeLengthConsumer) {
    ScriptNodeRegistryUVE registry;
    ASSERT_TRUE(RegisterBuiltInScriptNodesUVE(registry));
    ScriptGraphUVE graph;
    ASSERT_TRUE(graph.AddNodeUVE({10U, "math.vector2.make"}));
    ASSERT_TRUE(graph.AddNodeUVE({20U, "math.vector2.length"}));
    ASSERT_TRUE(graph.AddLinkUVE({{10U, "Vector"}, {20U, "Vector"}}));

    const ScriptIrCompileResultUVE compiled = CompileScriptGraphToIrUVE(graph, registry);
    ASSERT_TRUE(compiled.IsSuccessUVE());
    ASSERT_TRUE(compiled.program.has_value());
    ASSERT_GE(compiled.program->instructions.size(), 3U);
    EXPECT_EQ(compiled.program->instructions[0].nodeTypeId, "math.vector2.make");
    EXPECT_EQ(compiled.program->instructions[1].kind, ScriptIrInstructionKindUVE::TransferValue);
    EXPECT_TRUE(compiled.program->instructions[1].isStagedTransfer);
    EXPECT_EQ(compiled.program->instructions[1].targetPinName, "Vector");
    EXPECT_EQ(compiled.program->instructions[2].nodeTypeId, "math.vector2.length");
}

} // namespace UVE::Scripting


namespace UVE::Scripting {

TEST(ScriptVmUVETest, ExecuteScriptBytecodeUVE_ExecutesEntityLifecycleCallbacksAndStoresTypedOutputs) {
    ScriptBytecodeProgramUVE program;
    program.instructions.push_back({ScriptIrInstructionKindUVE::ExecuteNode, 1U, 0U,
                                    "entity.spawn", {}, {}});
    program.instructions.push_back({ScriptIrInstructionKindUVE::ExecuteNode, 2U, 0U,
                                    "entity.destroy", {}, {}});
    program.instructions.push_back({ScriptIrInstructionKindUVE::ExecuteNode, 3U, 0U,
                                    "entity.find", {}, {}});
    program.instructions.push_back({ScriptIrInstructionKindUVE::ExecuteNode, 4U, 0U,
                                    "entity.get_entity", {}, {}});
    program.instructions.push_back({ScriptIrInstructionKindUVE::ExecuteNode, 5U, 0U,
                                    "entity.add_component", {}, {}});
    program.instructions.push_back({ScriptIrInstructionKindUVE::ExecuteNode, 6U, 0U,
                                    "entity.remove_component", {}, {}});

    const Scene::EntityUVE entity{42U, 3U};
    const ScriptComponentValueUVE component{Scene::kInvalidEntityUVE, "MeshComponentUVE", false};
    ScriptVmExecutionContextUVE context;
    ASSERT_TRUE(context.SetInputUVE(2U, "Entity", ScriptEntityValueUVE{entity}));
    ASSERT_TRUE(context.SetInputUVE(3U, "Component", component));
    ASSERT_TRUE(context.SetInputUVE(4U, "Handle", 7.0F));
    ASSERT_TRUE(context.SetInputUVE(5U, "Entity", ScriptEntityValueUVE{entity}));
    ASSERT_TRUE(context.SetInputUVE(5U, "Component", component));
    ASSERT_TRUE(context.SetInputUVE(6U, "Entity", ScriptEntityValueUVE{entity}));
    ASSERT_TRUE(context.SetInputUVE(6U, "Component", component));

    EntityLifecycleCaptureUVE capture;
    ScriptEngineCallBindingsUVE bindings{};
    bindings.userData = &capture;
    bindings.spawnEntity = CaptureEntitySpawnUVE;
    bindings.destroyEntity = CaptureEntityDestroyUVE;
    bindings.findEntityByComponent = CaptureEntityFindUVE;
    bindings.getEntityByHandle = CaptureEntityGetUVE;
    bindings.addComponent = CaptureEntityAddUVE;
    bindings.removeComponent = CaptureEntityRemoveUVE;
    ScriptVmExecutionOptionsUVE options;
    options.engineCallBindings = &bindings;

    const ScriptVmExecutionResultUVE result = ExecuteScriptBytecodeUVE(program, context, options);
    ASSERT_TRUE(result.IsSuccessUVE());
    EXPECT_EQ(result.instructionsExecuted, 6U);
    EXPECT_EQ(capture.spawnCount, 1U);
    EXPECT_EQ(capture.destroyCount, 1U);
    EXPECT_EQ(capture.findCount, 1U);
    EXPECT_EQ(capture.getCount, 1U);
    EXPECT_EQ(capture.addCount, 1U);
    EXPECT_EQ(capture.removeCount, 1U);
    EXPECT_EQ(capture.lastComponentType, "MeshComponentUVE");
    EXPECT_FLOAT_EQ(capture.lastHandle, 7.0F);
    EXPECT_EQ(std::get<ScriptEntityValueUVE>(*context.FindOutputUVE(1U, "Result")),
              (ScriptEntityValueUVE{entity}));
    EXPECT_TRUE(std::get<bool>(*context.FindOutputUVE(2U, "Result")));
    EXPECT_EQ(std::get<ScriptEntityValueUVE>(*context.FindOutputUVE(3U, "Result")),
              (ScriptEntityValueUVE{entity}));
    EXPECT_EQ(std::get<ScriptEntityValueUVE>(*context.FindOutputUVE(4U, "Result")),
              (ScriptEntityValueUVE{entity}));
    EXPECT_TRUE(std::get<bool>(*context.FindOutputUVE(5U, "Result")));
    EXPECT_TRUE(std::get<bool>(*context.FindOutputUVE(6U, "Result")));
}

TEST(ScriptVmUVETest, ExecuteScriptBytecodeUVE_ExecutesEntitySpawnThroughSchedulerPath) {
    ScriptBytecodeProgramUVE program;
    program.instructions.push_back({ScriptIrInstructionKindUVE::ExecuteNode, 1U, 0U,
                                    "entity.spawn", {}, {}});
    EntityLifecycleCaptureUVE capture;
    ScriptEngineCallBindingsUVE bindings{};
    bindings.userData = &capture;
    bindings.spawnEntity = CaptureEntitySpawnUVE;
    ScriptVmExecutionOptionsUVE options;
    options.engineCallBindings = &bindings;

    ScriptVmExecutionContextUVE context;
    const ScriptVmExecutionResultUVE result = ExecuteScriptBytecodeUVE(program, context, options);
    EXPECT_TRUE(result.IsSuccessUVE());
    EXPECT_EQ(result.instructionsExecuted, 1U);
    EXPECT_EQ(capture.spawnCount, 1U);
}

TEST(ScriptVmUVETest, ExecuteScriptBytecodeUVE_EntityNodesFailClosedWithoutRequiredBindings) {
    const Scene::EntityUVE entity{42U, 3U};
    const ScriptComponentValueUVE component{Scene::kInvalidEntityUVE, "MeshComponentUVE", false};

    ScriptBytecodeProgramUVE spawn;
    spawn.instructions.push_back({ScriptIrInstructionKindUVE::ExecuteNode, 1U, 0U,
                                  "entity.spawn", {}, {}});
    ScriptVmExecutionContextUVE spawnContext;
    EXPECT_EQ(ExecuteScriptBytecodeUVE(spawn, spawnContext).status, ScriptVmStatusUVE::NodeExecutionFailed);

    ScriptBytecodeProgramUVE destroy;
    destroy.instructions.push_back({ScriptIrInstructionKindUVE::ExecuteNode, 2U, 0U,
                                    "entity.destroy", {}, {}});
    ScriptVmExecutionContextUVE destroyContext;
    ASSERT_TRUE(destroyContext.SetInputUVE(2U, "Entity", ScriptEntityValueUVE{entity}));
    EXPECT_EQ(ExecuteScriptBytecodeUVE(destroy, destroyContext).status, ScriptVmStatusUVE::NodeExecutionFailed);

    ScriptBytecodeProgramUVE find;
    find.instructions.push_back({ScriptIrInstructionKindUVE::ExecuteNode, 3U, 0U,
                                 "entity.find", {}, {}});
    ScriptVmExecutionContextUVE findContext;
    ASSERT_TRUE(findContext.SetInputUVE(3U, "Component", component));
    EXPECT_EQ(ExecuteScriptBytecodeUVE(find, findContext).status, ScriptVmStatusUVE::NodeExecutionFailed);

    ScriptBytecodeProgramUVE getEntity;
    getEntity.instructions.push_back({ScriptIrInstructionKindUVE::ExecuteNode, 4U, 0U,
                                      "entity.get_entity", {}, {}});
    ScriptVmExecutionContextUVE getContext;
    ASSERT_TRUE(getContext.SetInputUVE(4U, "Handle", 7.0F));
    EXPECT_EQ(ExecuteScriptBytecodeUVE(getEntity, getContext).status, ScriptVmStatusUVE::NodeExecutionFailed);

    for (const char* nodeType : {"entity.add_component", "entity.remove_component"}) {
        ScriptBytecodeProgramUVE mutation;
        mutation.instructions.push_back({ScriptIrInstructionKindUVE::ExecuteNode, 5U, 0U,
                                         nodeType, {}, {}});
        ScriptVmExecutionContextUVE mutationContext;
        ASSERT_TRUE(mutationContext.SetInputUVE(5U, "Entity", ScriptEntityValueUVE{entity}));
        ASSERT_TRUE(mutationContext.SetInputUVE(5U, "Component", component));
        EXPECT_EQ(ExecuteScriptBytecodeUVE(mutation, mutationContext).status,
                  ScriptVmStatusUVE::NodeExecutionFailed);
    }
}

} // namespace UVE::Scripting


namespace UVE::Scripting {
namespace {

struct InputCameraCaptureUVE final {
    std::size_t keyCount = 0U;
    std::size_t mousePositionCount = 0U;
    std::size_t mouseButtonCount = 0U;
    std::size_t gamepadButtonCount = 0U;
    std::size_t axisCount = 0U;
    std::size_t actionCount = 0U;
    std::size_t cameraGetCount = 0U;
    std::size_t cameraPositionCount = 0U;
    std::size_t cameraRotationCount = 0U;
    std::size_t cameraLookAtCount = 0U;
    std::size_t cameraFovCount = 0U;
    std::size_t cameraShakeCount = 0U;
    std::size_t cameraActiveCount = 0U;
    Scene::EntityUVE camera{7U, 2U};
    float lastToken = 0.0F;
    float lastAxis = 0.0F;
    float lastFov = 0.0F;
    float lastAmplitude = 0.0F;
    float lastDuration = 0.0F;
    bool lastActive = false;
    bool booleanResult = true;
    Math::Vector2UVE mousePosition{12.0F, -4.0F};
    float axisResult = 0.5F;
};

bool CaptureInputKeyUVE(void* userData, const float token, bool* outResult) noexcept {
    auto* capture = static_cast<InputCameraCaptureUVE*>(userData);
    if (capture == nullptr || outResult == nullptr) {
        return false;
    }
    ++capture->keyCount;
    capture->lastToken = token;
    *outResult = capture->booleanResult;
    return true;
}

bool CaptureInputMousePositionUVE(void* userData, ScriptVector2ValueUVE* outPosition) noexcept {
    auto* capture = static_cast<InputCameraCaptureUVE*>(userData);
    if (capture == nullptr || outPosition == nullptr) {
        return false;
    }
    ++capture->mousePositionCount;
    outPosition->value = capture->mousePosition;
    return true;
}

bool CaptureInputMouseButtonUVE(void* userData, const float token, bool* outResult) noexcept {
    auto* capture = static_cast<InputCameraCaptureUVE*>(userData);
    if (capture == nullptr || outResult == nullptr) {
        return false;
    }
    ++capture->mouseButtonCount;
    capture->lastToken = token;
    *outResult = capture->booleanResult;
    return true;
}

bool CaptureInputGamepadButtonUVE(void* userData, const float gamepadToken, const float buttonToken,
                                  bool* outResult) noexcept {
    auto* capture = static_cast<InputCameraCaptureUVE*>(userData);
    if (capture == nullptr || outResult == nullptr) {
        return false;
    }
    ++capture->gamepadButtonCount;
    capture->lastToken = gamepadToken;
    capture->lastAxis = buttonToken;
    *outResult = capture->booleanResult;
    return true;
}

bool CaptureInputAxisUVE(void* userData, const float gamepadToken, const float axisToken,
                        float* outValue) noexcept {
    auto* capture = static_cast<InputCameraCaptureUVE*>(userData);
    if (capture == nullptr || outValue == nullptr) {
        return false;
    }
    ++capture->axisCount;
    capture->lastToken = gamepadToken;
    capture->lastAxis = axisToken;
    *outValue = capture->axisResult;
    return true;
}

bool CaptureInputActionUVE(void* userData, const float token, bool* outResult) noexcept {
    auto* capture = static_cast<InputCameraCaptureUVE*>(userData);
    if (capture == nullptr || outResult == nullptr) {
        return false;
    }
    ++capture->actionCount;
    capture->lastToken = token;
    *outResult = capture->booleanResult;
    return true;
}

bool CaptureCameraGetUVE(void* userData, Scene::EntityUVE* outCamera) noexcept {
    auto* capture = static_cast<InputCameraCaptureUVE*>(userData);
    if (capture == nullptr || outCamera == nullptr) {
        return false;
    }
    ++capture->cameraGetCount;
    *outCamera = capture->camera;
    return true;
}

bool CaptureCameraPositionUVE(void* userData, const Scene::EntityUVE camera,
                             const ScriptVector3ValueUVE& position) noexcept {
    auto* capture = static_cast<InputCameraCaptureUVE*>(userData);
    if (capture == nullptr || camera != capture->camera || !std::isfinite(position.value.x) ||
        !std::isfinite(position.value.y) || !std::isfinite(position.value.z)) {
        return false;
    }
    ++capture->cameraPositionCount;
    return true;
}

bool CaptureCameraRotationUVE(void* userData, const Scene::EntityUVE camera,
                             const ScriptRotationValueUVE& rotation) noexcept {
    auto* capture = static_cast<InputCameraCaptureUVE*>(userData);
    if (capture == nullptr || camera != capture->camera || !Math::IsFiniteUVE(rotation.value)) {
        return false;
    }
    ++capture->cameraRotationCount;
    return true;
}

bool CaptureCameraLookAtUVE(void* userData, const Scene::EntityUVE camera,
                            const ScriptVector3ValueUVE& target) noexcept {
    auto* capture = static_cast<InputCameraCaptureUVE*>(userData);
    if (capture == nullptr || camera != capture->camera || !std::isfinite(target.value.x) ||
        !std::isfinite(target.value.y) || !std::isfinite(target.value.z)) {
        return false;
    }
    ++capture->cameraLookAtCount;
    return true;
}

bool CaptureCameraFovUVE(void* userData, const Scene::EntityUVE camera, const float fov) noexcept {
    auto* capture = static_cast<InputCameraCaptureUVE*>(userData);
    if (capture == nullptr || camera != capture->camera || !std::isfinite(fov)) {
        return false;
    }
    ++capture->cameraFovCount;
    capture->lastFov = fov;
    return true;
}

bool CaptureCameraShakeUVE(void* userData, const Scene::EntityUVE camera, const float amplitude,
                           const float duration) noexcept {
    auto* capture = static_cast<InputCameraCaptureUVE*>(userData);
    if (capture == nullptr || camera != capture->camera || !std::isfinite(amplitude) || !std::isfinite(duration)) {
        return false;
    }
    ++capture->cameraShakeCount;
    capture->lastAmplitude = amplitude;
    capture->lastDuration = duration;
    return true;
}

bool CaptureCameraActiveUVE(void* userData, const Scene::EntityUVE camera, const bool active) noexcept {
    auto* capture = static_cast<InputCameraCaptureUVE*>(userData);
    if (capture == nullptr || camera != capture->camera) {
        return false;
    }
    ++capture->cameraActiveCount;
    capture->lastActive = active;
    return true;
}

ScriptEngineCallBindingsUVE MakeInputCameraBindingsUVE(InputCameraCaptureUVE& capture) {
    ScriptEngineCallBindingsUVE bindings{};
    bindings.userData = &capture;
    bindings.inputKeyPressed = CaptureInputKeyUVE;
    bindings.inputKeyReleased = CaptureInputKeyUVE;
    bindings.inputKeyDown = CaptureInputKeyUVE;
    bindings.inputMousePosition = CaptureInputMousePositionUVE;
    bindings.inputMouseButton = CaptureInputMouseButtonUVE;
    bindings.inputGamepadButton = CaptureInputGamepadButtonUVE;
    bindings.inputAxis = CaptureInputAxisUVE;
    bindings.inputAction = CaptureInputActionUVE;
    bindings.cameraGet = CaptureCameraGetUVE;
    bindings.cameraSetPosition = CaptureCameraPositionUVE;
    bindings.cameraSetRotation = CaptureCameraRotationUVE;
    bindings.cameraLookAt = CaptureCameraLookAtUVE;
    bindings.cameraSetFov = CaptureCameraFovUVE;
    bindings.cameraShake = CaptureCameraShakeUVE;
    bindings.cameraSetActive = CaptureCameraActiveUVE;
    return bindings;
}

} // namespace

TEST(ScriptVmUVETest, ExecuteScriptBytecodeUVE_ExecutesInputAndCameraFamiliesWithCopiedValues) {
    ScriptBytecodeProgramUVE program;
    const std::array<const char*, 15U> nodeTypes{
        "input.key_pressed", "input.key_released", "input.key_down", "input.mouse_position",
        "input.mouse_button", "input.gamepad_button", "input.get_axis", "input.get_action",
        "camera.get_camera", "camera.set_position", "camera.set_rotation", "camera.look_at",
        "camera.set_fov", "camera.shake", "camera.set_active"};
    for (std::size_t index = 0U; index < nodeTypes.size(); ++index) {
        program.instructions.push_back({ScriptIrInstructionKindUVE::ExecuteNode,
                                        static_cast<std::uint32_t>(index + 1U), 0U, nodeTypes[index], {}, {}});
    }

    const Scene::EntityUVE camera{7U, 2U};
    ScriptVmExecutionContextUVE context;
    ASSERT_TRUE(context.SetInputUVE(1U, "Key", 1.0F));
    ASSERT_TRUE(context.SetInputUVE(2U, "Key", 2.0F));
    ASSERT_TRUE(context.SetInputUVE(3U, "Key", 3.0F));
    ASSERT_TRUE(context.SetInputUVE(5U, "Button", 0.0F));
    ASSERT_TRUE(context.SetInputUVE(6U, "Gamepad", 0.0F));
    ASSERT_TRUE(context.SetInputUVE(6U, "Button", 1.0F));
    ASSERT_TRUE(context.SetInputUVE(7U, "Gamepad", 0.0F));
    ASSERT_TRUE(context.SetInputUVE(7U, "Axis", 2.0F));
    ASSERT_TRUE(context.SetInputUVE(8U, "Action", 7.0F));
    ASSERT_TRUE(context.SetInputUVE(10U, "Camera", ScriptEntityValueUVE{camera}));
    ASSERT_TRUE(context.SetInputUVE(10U, "Position", ScriptVector3ValueUVE{{1.0F, 2.0F, 3.0F}}));
    ASSERT_TRUE(context.SetInputUVE(11U, "Camera", ScriptEntityValueUVE{camera}));
    ASSERT_TRUE(context.SetInputUVE(11U, "Rotation", ScriptRotationValueUVE{{0.0F, 0.0F, 0.0F, 1.0F}}));
    ASSERT_TRUE(context.SetInputUVE(12U, "Camera", ScriptEntityValueUVE{camera}));
    ASSERT_TRUE(context.SetInputUVE(12U, "Target", ScriptVector3ValueUVE{{4.0F, 5.0F, 6.0F}}));
    ASSERT_TRUE(context.SetInputUVE(13U, "Camera", ScriptEntityValueUVE{camera}));
    ASSERT_TRUE(context.SetInputUVE(13U, "FOV", 75.0F));
    ASSERT_TRUE(context.SetInputUVE(14U, "Camera", ScriptEntityValueUVE{camera}));
    ASSERT_TRUE(context.SetInputUVE(14U, "Amplitude", 1.0F));
    ASSERT_TRUE(context.SetInputUVE(14U, "Duration", 0.25F));
    ASSERT_TRUE(context.SetInputUVE(15U, "Camera", ScriptEntityValueUVE{camera}));
    ASSERT_TRUE(context.SetInputUVE(15U, "Active", true));

    InputCameraCaptureUVE capture;
    ScriptEngineCallBindingsUVE bindings = MakeInputCameraBindingsUVE(capture);
    ScriptVmExecutionOptionsUVE options;
    options.engineCallBindings = &bindings;
    const ScriptVmExecutionResultUVE result = ExecuteScriptBytecodeUVE(program, context, options);

    ASSERT_TRUE(result.IsSuccessUVE());
    EXPECT_EQ(result.instructionsExecuted, 15U);
    EXPECT_EQ(capture.keyCount, 3U);
    EXPECT_EQ(capture.mousePositionCount, 1U);
    EXPECT_EQ(capture.mouseButtonCount, 1U);
    EXPECT_EQ(capture.gamepadButtonCount, 1U);
    EXPECT_EQ(capture.axisCount, 1U);
    EXPECT_EQ(capture.actionCount, 1U);
    EXPECT_EQ(capture.cameraGetCount, 1U);
    EXPECT_EQ(capture.cameraPositionCount, 1U);
    EXPECT_EQ(capture.cameraRotationCount, 1U);
    EXPECT_EQ(capture.cameraLookAtCount, 1U);
    EXPECT_EQ(capture.cameraFovCount, 1U);
    EXPECT_EQ(capture.cameraShakeCount, 1U);
    EXPECT_EQ(capture.cameraActiveCount, 1U);
    EXPECT_EQ(capture.lastToken, 7.0F);
    EXPECT_FLOAT_EQ(capture.lastAxis, 2.0F);
    EXPECT_FLOAT_EQ(capture.lastFov, 75.0F);
    EXPECT_FLOAT_EQ(capture.lastAmplitude, 1.0F);
    EXPECT_FLOAT_EQ(capture.lastDuration, 0.25F);
    EXPECT_TRUE(capture.lastActive);
    EXPECT_EQ(std::get<bool>(*context.FindOutputUVE(1U, "Result")), true);
    EXPECT_EQ(std::get<ScriptVector2ValueUVE>(*context.FindOutputUVE(4U, "Position")),
              (ScriptVector2ValueUVE{{12.0F, -4.0F}}));
    EXPECT_FLOAT_EQ(std::get<float>(*context.FindOutputUVE(7U, "Result")), 0.5F);
    EXPECT_EQ(std::get<bool>(*context.FindOutputUVE(8U, "Result")), true);
    EXPECT_EQ(std::get<ScriptEntityValueUVE>(*context.FindOutputUVE(9U, "Result")),
              (ScriptEntityValueUVE{camera}));
    EXPECT_EQ(std::get<bool>(*context.FindOutputUVE(15U, "Result")), true);
}

TEST(ScriptVmUVETest, ExecuteScriptBytecodeUVE_InputAndCameraSchedulerPathUsesCopiedContext) {
    ScriptBytecodeProgramUVE program;
    program.instructions.push_back({ScriptIrInstructionKindUVE::ExecuteNode, 1U, 0U,
                                    "input.mouse_position", {}, {}});
    InputCameraCaptureUVE capture;
    ScriptEngineCallBindingsUVE bindings = MakeInputCameraBindingsUVE(capture);
    ScriptVmExecutionOptionsUVE options;
    options.engineCallBindings = &bindings;
    ScriptVmExecutionContextUVE context;
    const ScriptVmExecutionResultUVE result = ExecuteScriptBytecodeUVE(program, context, options);
    EXPECT_TRUE(result.IsSuccessUVE());
    EXPECT_EQ(result.instructionsExecuted, 1U);
    EXPECT_EQ(capture.mousePositionCount, 1U);
}

TEST(ScriptVmUVETest, ExecuteScriptBytecodeUVE_InputAndCameraNodesFailClosedWithoutBindings) {
    const Scene::EntityUVE camera{7U, 2U};
    const std::array<const char*, 15U> nodeTypes{
        "input.key_pressed", "input.key_released", "input.key_down", "input.mouse_position",
        "input.mouse_button", "input.gamepad_button", "input.get_axis", "input.get_action",
        "camera.get_camera", "camera.set_position", "camera.set_rotation", "camera.look_at",
        "camera.set_fov", "camera.shake", "camera.set_active"};
    for (std::size_t index = 0U; index < nodeTypes.size(); ++index) {
        const std::uint32_t nodeId = static_cast<std::uint32_t>(index + 1U);
        ScriptBytecodeProgramUVE program;
        program.instructions.push_back({ScriptIrInstructionKindUVE::ExecuteNode, nodeId, 0U,
                                        nodeTypes[index], {}, {}});
        ScriptVmExecutionContextUVE context;
        if (index == 0U || index == 1U || index == 2U) {
            ASSERT_TRUE(context.SetInputUVE(nodeId, "Key", 1.0F));
        } else if (index == 4U) {
            ASSERT_TRUE(context.SetInputUVE(nodeId, "Button", 0.0F));
        } else if (index == 5U || index == 6U) {
            ASSERT_TRUE(context.SetInputUVE(nodeId, "Gamepad", 0.0F));
            ASSERT_TRUE(context.SetInputUVE(nodeId, index == 5U ? "Button" : "Axis", 0.0F));
        } else if (index == 7U) {
            ASSERT_TRUE(context.SetInputUVE(nodeId, "Action", 1.0F));
        } else if (index >= 9U) {
            ASSERT_TRUE(context.SetInputUVE(nodeId, "Camera", ScriptEntityValueUVE{camera}));
            if (index == 9U) {
                ASSERT_TRUE(context.SetInputUVE(nodeId, "Position", ScriptVector3ValueUVE{{1.0F, 2.0F, 3.0F}}));
            } else if (index == 10U) {
                ASSERT_TRUE(context.SetInputUVE(nodeId, "Rotation", ScriptRotationValueUVE{{0.0F, 0.0F, 0.0F, 1.0F}}));
            } else if (index == 11U) {
                ASSERT_TRUE(context.SetInputUVE(nodeId, "Target", ScriptVector3ValueUVE{{1.0F, 2.0F, 3.0F}}));
            } else if (index == 12U) {
                ASSERT_TRUE(context.SetInputUVE(nodeId, "FOV", 60.0F));
            } else if (index == 13U) {
                ASSERT_TRUE(context.SetInputUVE(nodeId, "Amplitude", 1.0F));
                ASSERT_TRUE(context.SetInputUVE(nodeId, "Duration", 0.5F));
            } else if (index == 14U) {
                ASSERT_TRUE(context.SetInputUVE(nodeId, "Active", true));
            }
        }
        EXPECT_EQ(ExecuteScriptBytecodeUVE(program, context).status, ScriptVmStatusUVE::NodeExecutionFailed)
            << nodeTypes[index];
    }
}

} // namespace UVE::Scripting


namespace UVE::Scripting {

TEST(ScriptCompilerIRUVETest, CompileScriptGraphToIrUVE_StagesInputAndCameraProducers) {
    ScriptNodeRegistryUVE registry;
    ASSERT_TRUE(RegisterBuiltInScriptNodesUVE(registry));

    const auto expectStaged = [](const ScriptIrCompileResultUVE& result, const char* producer,
                                 const char* targetPin, const char* consumer, const std::uint32_t producerId,
                                 const std::uint32_t consumerId) {
        ASSERT_TRUE(result.IsSuccessUVE());
        ASSERT_TRUE(result.program.has_value());
        ASSERT_EQ(result.program->instructions.size(), 3U);
        EXPECT_EQ(result.program->instructions[0].nodeTypeId, producer);
        EXPECT_EQ(result.program->instructions[1].kind, ScriptIrInstructionKindUVE::TransferValue);
        EXPECT_EQ(result.program->instructions[1].sourceNodeId, producerId);
        EXPECT_EQ(result.program->instructions[1].targetNodeId, consumerId);
        EXPECT_EQ(result.program->instructions[1].targetPinName, targetPin);
        EXPECT_EQ(result.program->instructions[2].nodeTypeId, consumer);
    };

    ScriptGraphUVE booleanGraph;
    ASSERT_TRUE(booleanGraph.AddNodeUVE({10U, "input.key_pressed"}));
    ASSERT_TRUE(booleanGraph.AddNodeUVE({20U, "logic.boolean.not"}));
    ASSERT_TRUE(booleanGraph.AddLinkUVE({{10U, "Result"}, {20U, "Value"}}));
    expectStaged(CompileScriptGraphToIrUVE(booleanGraph, registry), "input.key_pressed", "Value",
                 "logic.boolean.not", 10U, 20U);

    ScriptGraphUVE numberGraph;
    ASSERT_TRUE(numberGraph.AddNodeUVE({30U, "input.get_axis"}));
    ASSERT_TRUE(numberGraph.AddNodeUVE({40U, "math.float.add"}));
    ASSERT_TRUE(numberGraph.AddLinkUVE({{30U, "Result"}, {40U, "A"}}));
    expectStaged(CompileScriptGraphToIrUVE(numberGraph, registry), "input.get_axis", "A",
                 "math.float.add", 30U, 40U);

    ScriptGraphUVE vectorGraph;
    ASSERT_TRUE(vectorGraph.AddNodeUVE({50U, "input.mouse_position"}));
    ASSERT_TRUE(vectorGraph.AddNodeUVE({60U, "math.vector2.normalize"}));
    ASSERT_TRUE(vectorGraph.AddLinkUVE({{50U, "Position"}, {60U, "Vector"}}));
    expectStaged(CompileScriptGraphToIrUVE(vectorGraph, registry), "input.mouse_position", "Vector",
                 "math.vector2.normalize", 50U, 60U);

    ScriptGraphUVE cameraGraph;
    ASSERT_TRUE(cameraGraph.AddNodeUVE({70U, "camera.get_camera"}));
    ASSERT_TRUE(cameraGraph.AddNodeUVE({80U, "camera.set_active"}));
    ASSERT_TRUE(cameraGraph.AddLinkUVE({{70U, "Result"}, {80U, "Camera"}}));
    expectStaged(CompileScriptGraphToIrUVE(cameraGraph, registry), "camera.get_camera", "Camera",
                 "camera.set_active", 70U, 80U);
}

} // namespace UVE::Scripting


namespace UVE::Scripting {
namespace {

struct AnimationMotionCaptureUVE final {
    std::size_t animationClipCount = 0U;
    std::size_t animationPauseCount = 0U;
    std::size_t animationBlendCount = 0U;
    std::size_t animationBlendSpaceCount = 0U;
    std::size_t animationScalarCount = 0U;
    std::size_t animationMontageCount = 0U;
    std::size_t animationCurrentCount = 0U;
    std::size_t animationPlayingCount = 0U;
    std::size_t motionBuildCount = 0U;
    std::size_t motionSearchCount = 0U;
    std::size_t motionBestMatchCount = 0U;
    std::size_t motionTrajectoryCount = 0U;
    std::size_t motionPoseCount = 0U;
    std::size_t motionVectorCount = 0U;
    std::size_t motionYawCount = 0U;
    std::size_t motionTransitionCount = 0U;
    std::size_t motionWarpCount = 0U;
    float lastCandidateIndex = 0.0F;
    Scene::EntityUVE actor{9U, 1U};
};

bool CaptureAnimationClipUVE(void* userData, Scene::EntityUVE actor, float clipToken, float blendDuration,
                             bool* outResult) noexcept {
    auto* capture = static_cast<AnimationMotionCaptureUVE*>(userData);
    if (capture == nullptr || outResult == nullptr || actor != capture->actor) return false;
    ++capture->animationClipCount;
    (void)clipToken;
    (void)blendDuration;
    *outResult = true;
    return true;
}

bool CaptureAnimationPauseUVE(void* userData, Scene::EntityUVE actor, float clipToken,
                              bool* outResult) noexcept {
    auto* capture = static_cast<AnimationMotionCaptureUVE*>(userData);
    if (capture == nullptr || outResult == nullptr || actor != capture->actor) return false;
    ++capture->animationPauseCount;
    (void)clipToken;
    *outResult = true;
    return true;
}

bool CaptureAnimationBlendUVE(void* userData, Scene::EntityUVE actor, float clipAToken, float clipBToken,
                              float weight, bool* outResult) noexcept {
    auto* capture = static_cast<AnimationMotionCaptureUVE*>(userData);
    if (capture == nullptr || outResult == nullptr || actor != capture->actor) return false;
    ++capture->animationBlendCount;
    (void)clipAToken;
    (void)clipBToken;
    (void)weight;
    *outResult = true;
    return true;
}

bool CaptureAnimationBlendSpaceUVE(void* userData, Scene::EntityUVE actor, float blendSpaceToken,
                                   float x, float y, bool* outResult) noexcept {
    auto* capture = static_cast<AnimationMotionCaptureUVE*>(userData);
    if (capture == nullptr || outResult == nullptr || actor != capture->actor) return false;
    ++capture->animationBlendSpaceCount;
    (void)blendSpaceToken;
    (void)x;
    (void)y;
    *outResult = true;
    return true;
}

bool CaptureAnimationScalarUVE(void* userData, Scene::EntityUVE actor, float value,
                               bool* outResult) noexcept {
    auto* capture = static_cast<AnimationMotionCaptureUVE*>(userData);
    if (capture == nullptr || outResult == nullptr || actor != capture->actor) return false;
    ++capture->animationScalarCount;
    (void)value;
    *outResult = true;
    return true;
}

bool CaptureAnimationMontageUVE(void* userData, Scene::EntityUVE actor, float montageToken, float weight,
                                bool* outResult) noexcept {
    auto* capture = static_cast<AnimationMotionCaptureUVE*>(userData);
    if (capture == nullptr || outResult == nullptr || actor != capture->actor) return false;
    ++capture->animationMontageCount;
    (void)montageToken;
    (void)weight;
    *outResult = true;
    return true;
}

bool CaptureAnimationCurrentUVE(void* userData, Scene::EntityUVE actor, float* outClipToken) noexcept {
    auto* capture = static_cast<AnimationMotionCaptureUVE*>(userData);
    if (capture == nullptr || outClipToken == nullptr || actor != capture->actor) return false;
    ++capture->animationCurrentCount;
    *outClipToken = 8.0F;
    return true;
}

bool CaptureAnimationPlayingUVE(void* userData, Scene::EntityUVE actor, float clipToken,
                                bool* outResult) noexcept {
    auto* capture = static_cast<AnimationMotionCaptureUVE*>(userData);
    if (capture == nullptr || outResult == nullptr || actor != capture->actor) return false;
    ++capture->animationPlayingCount;
    (void)clipToken;
    *outResult = true;
    return true;
}

bool CaptureMotionBuildUVE(void* userData, Scene::EntityUVE actor, const ScriptVector3ValueUVE& velocity,
                           const ScriptVector3ValueUVE& facing, float deltaSeconds, bool* outResult) noexcept {
    auto* capture = static_cast<AnimationMotionCaptureUVE*>(userData);
    if (capture == nullptr || outResult == nullptr || actor != capture->actor ||
        !std::isfinite(velocity.value.x) || !std::isfinite(facing.value.z) || !std::isfinite(deltaSeconds)) return false;
    ++capture->motionBuildCount;
    *outResult = true;
    return true;
}

bool CaptureMotionSearchUVE(void* userData, Scene::EntityUVE actor, float maximumResults,
                            bool* outResult) noexcept {
    auto* capture = static_cast<AnimationMotionCaptureUVE*>(userData);
    if (capture == nullptr || outResult == nullptr || actor != capture->actor || !std::isfinite(maximumResults)) return false;
    ++capture->motionSearchCount;
    *outResult = true;
    return true;
}

bool CaptureMotionBestMatchUVE(void* userData, Scene::EntityUVE actor, float* outCandidateIndex) noexcept {
    auto* capture = static_cast<AnimationMotionCaptureUVE*>(userData);
    if (capture == nullptr || outCandidateIndex == nullptr || actor != capture->actor) return false;
    ++capture->motionBestMatchCount;
    capture->lastCandidateIndex = 12.0F;
    *outCandidateIndex = capture->lastCandidateIndex;
    return true;
}

bool CaptureMotionTrajectoryUVE(void* userData, Scene::EntityUVE actor, const ScriptVector3ValueUVE& sample,
                               float offsetSeconds, bool* outResult) noexcept {
    auto* capture = static_cast<AnimationMotionCaptureUVE*>(userData);
    if (capture == nullptr || outResult == nullptr || actor != capture->actor ||
        !std::isfinite(sample.value.x) || !std::isfinite(offsetSeconds)) return false;
    ++capture->motionTrajectoryCount;
    *outResult = true;
    return true;
}

bool CaptureMotionPoseUVE(void* userData, Scene::EntityUVE actor, const ScriptTransformValueUVE& pose,
                          bool* outResult) noexcept {
    auto* capture = static_cast<AnimationMotionCaptureUVE*>(userData);
    if (capture == nullptr || outResult == nullptr || actor != capture->actor ||
        !std::isfinite(pose.position.value.x)) return false;
    ++capture->motionPoseCount;
    *outResult = true;
    return true;
}

bool CaptureMotionVectorUVE(void* userData, Scene::EntityUVE actor, const ScriptVector3ValueUVE& value,
                           bool* outResult) noexcept {
    auto* capture = static_cast<AnimationMotionCaptureUVE*>(userData);
    if (capture == nullptr || outResult == nullptr || actor != capture->actor || !std::isfinite(value.value.y)) return false;
    ++capture->motionVectorCount;
    *outResult = true;
    return true;
}

bool CaptureMotionYawUVE(void* userData, Scene::EntityUVE actor, float yawDegrees,
                        bool* outResult) noexcept {
    auto* capture = static_cast<AnimationMotionCaptureUVE*>(userData);
    if (capture == nullptr || outResult == nullptr || actor != capture->actor || !std::isfinite(yawDegrees)) return false;
    ++capture->motionYawCount;
    *outResult = true;
    return true;
}

bool CaptureMotionTransitionUVE(void* userData, Scene::EntityUVE actor, float targetToken, float durationSeconds,
                                bool* outResult) noexcept {
    auto* capture = static_cast<AnimationMotionCaptureUVE*>(userData);
    if (capture == nullptr || outResult == nullptr || actor != capture->actor ||
        !std::isfinite(targetToken) || !std::isfinite(durationSeconds)) return false;
    ++capture->motionTransitionCount;
    *outResult = true;
    return true;
}

bool CaptureMotionWarpUVE(void* userData, Scene::EntityUVE actor, const ScriptVector3ValueUVE& target,
                          float weight, bool* outResult) noexcept {
    auto* capture = static_cast<AnimationMotionCaptureUVE*>(userData);
    if (capture == nullptr || outResult == nullptr || actor != capture->actor ||
        !std::isfinite(target.value.z) || !std::isfinite(weight)) return false;
    ++capture->motionWarpCount;
    *outResult = true;
    return true;
}

ScriptEngineCallBindingsUVE MakeAnimationMotionBindingsUVE(AnimationMotionCaptureUVE& capture) {
    ScriptEngineCallBindingsUVE bindings{};
    bindings.userData = &capture;
    bindings.animationPlay = CaptureAnimationClipUVE;
    bindings.animationStop = CaptureAnimationClipUVE;
    bindings.animationPause = CaptureAnimationPauseUVE;
    bindings.animationBlend = CaptureAnimationBlendUVE;
    bindings.animationBlendSpace = CaptureAnimationBlendSpaceUVE;
    bindings.animationSetSpeed = CaptureAnimationScalarUVE;
    bindings.animationSetWeight = CaptureAnimationScalarUVE;
    bindings.animationMontage = CaptureAnimationMontageUVE;
    bindings.animationGetCurrent = CaptureAnimationCurrentUVE;
    bindings.animationIsPlaying = CaptureAnimationPlayingUVE;
    bindings.motionQueryBuild = CaptureMotionBuildUVE;
    bindings.motionQuerySearch = CaptureMotionSearchUVE;
    bindings.motionQueryBestMatch = CaptureMotionBestMatchUVE;
    bindings.motionQuerySetTrajectory = CaptureMotionTrajectoryUVE;
    bindings.motionQuerySetPose = CaptureMotionPoseUVE;
    bindings.motionQuerySetVelocity = CaptureMotionVectorUVE;
    bindings.motionQuerySetFacing = CaptureMotionVectorUVE;
    bindings.motionQuerySetYaw = CaptureMotionYawUVE;
    bindings.motionQueryTransition = CaptureMotionTransitionUVE;
    bindings.motionQueryMotionWarp = CaptureMotionWarpUVE;
    return bindings;
}

void SetActorInputUVE(ScriptVmExecutionContextUVE& context, std::uint32_t nodeId,
                      const Scene::EntityUVE actor) {
    ASSERT_TRUE(context.SetInputUVE(nodeId, "Actor", ScriptEntityValueUVE{actor}));
}

} // namespace

TEST(ScriptVmUVETest, ExecuteScriptBytecodeUVE_ExecutesAnimationAndMotionQueryFamiliesWithCopiedValues) {
    const std::array<const char*, 20U> nodeTypes{
        "animation.play", "animation.stop", "animation.pause", "animation.blend", "animation.blend_space",
        "animation.set_speed", "animation.set_weight", "animation.montage", "animation.get_current_animation",
        "animation.is_playing", "motion.query.build", "motion.query.search", "motion.query.get_best_match",
        "motion.query.set_trajectory", "motion.query.set_pose", "motion.query.set_velocity", "motion.query.set_facing",
        "motion.query.set_yaw", "motion.query.transition", "motion.query.motion_warp"};
    ScriptBytecodeProgramUVE program;
    ScriptVmExecutionContextUVE context;
    const Scene::EntityUVE actor{9U, 1U};
    for (std::size_t index = 0U; index < nodeTypes.size(); ++index) {
        const std::uint32_t nodeId = static_cast<std::uint32_t>(index + 1U);
        program.instructions.push_back({ScriptIrInstructionKindUVE::ExecuteNode, nodeId, 0U, nodeTypes[index], {}, {}});
        SetActorInputUVE(context, nodeId, actor);
    }
    ASSERT_TRUE(context.SetInputUVE(1U, "Clip", 1.0F));
    ASSERT_TRUE(context.SetInputUVE(1U, "Blend Duration", 0.2F));
    ASSERT_TRUE(context.SetInputUVE(2U, "Clip", 1.0F));
    ASSERT_TRUE(context.SetInputUVE(3U, "Clip", 1.0F));
    ASSERT_TRUE(context.SetInputUVE(4U, "Clip A", 1.0F));
    ASSERT_TRUE(context.SetInputUVE(4U, "Clip B", 2.0F));
    ASSERT_TRUE(context.SetInputUVE(4U, "Weight", 0.5F));
    ASSERT_TRUE(context.SetInputUVE(5U, "Blend Space", 2.0F));
    ASSERT_TRUE(context.SetInputUVE(5U, "X", 1.0F));
    ASSERT_TRUE(context.SetInputUVE(5U, "Y", -0.5F));
    ASSERT_TRUE(context.SetInputUVE(6U, "Speed", 1.25F));
    ASSERT_TRUE(context.SetInputUVE(7U, "Weight", 0.75F));
    ASSERT_TRUE(context.SetInputUVE(8U, "Montage", 3.0F));
    ASSERT_TRUE(context.SetInputUVE(8U, "Weight", 0.5F));
    ASSERT_TRUE(context.SetInputUVE(10U, "Clip", 1.0F));
    ASSERT_TRUE(context.SetInputUVE(11U, "Velocity", ScriptVector3ValueUVE{{1.0F, 0.0F, 0.0F}}));
    ASSERT_TRUE(context.SetInputUVE(11U, "Facing", ScriptVector3ValueUVE{{0.0F, 0.0F, 1.0F}}));
    ASSERT_TRUE(context.SetInputUVE(11U, "Delta", 0.016F));
    ASSERT_TRUE(context.SetInputUVE(12U, "Max Results", 16.0F));
    ASSERT_TRUE(context.SetInputUVE(14U, "Sample", ScriptVector3ValueUVE{{2.0F, 0.0F, 1.0F}}));
    ASSERT_TRUE(context.SetInputUVE(14U, "Offset", 0.25F));
    ScriptTransformValueUVE pose{};
    pose.position = ScriptVector3ValueUVE{{1.0F, 2.0F, 3.0F}};
    pose.rotation = ScriptRotationValueUVE{{0.0F, 0.0F, 0.0F, 1.0F}};
    pose.scale = ScriptVector3ValueUVE{{1.0F, 1.0F, 1.0F}};
    ASSERT_TRUE(context.SetInputUVE(15U, "Pose", pose));
    ASSERT_TRUE(context.SetInputUVE(16U, "Velocity", ScriptVector3ValueUVE{{1.0F, 2.0F, 3.0F}}));
    ASSERT_TRUE(context.SetInputUVE(17U, "Facing", ScriptVector3ValueUVE{{0.0F, 0.0F, 1.0F}}));
    ASSERT_TRUE(context.SetInputUVE(18U, "Yaw", 90.0F));
    ASSERT_TRUE(context.SetInputUVE(19U, "Target", 4.0F));
    ASSERT_TRUE(context.SetInputUVE(19U, "Duration", 0.3F));
    ASSERT_TRUE(context.SetInputUVE(20U, "Target", ScriptVector3ValueUVE{{4.0F, 0.0F, 2.0F}}));
    ASSERT_TRUE(context.SetInputUVE(20U, "Weight", 0.8F));

    AnimationMotionCaptureUVE capture;
    ScriptEngineCallBindingsUVE bindings = MakeAnimationMotionBindingsUVE(capture);
    ScriptVmExecutionOptionsUVE options;
    options.engineCallBindings = &bindings;
    const ScriptVmExecutionResultUVE result = ExecuteScriptBytecodeUVE(program, context, options);

    ASSERT_TRUE(result.IsSuccessUVE());
    EXPECT_EQ(result.instructionsExecuted, 20U);
    EXPECT_EQ(capture.animationClipCount, 2U);
    EXPECT_EQ(capture.animationPauseCount, 1U);
    EXPECT_EQ(capture.animationBlendCount, 1U);
    EXPECT_EQ(capture.animationBlendSpaceCount, 1U);
    EXPECT_EQ(capture.animationScalarCount, 2U);
    EXPECT_EQ(capture.animationMontageCount, 1U);
    EXPECT_EQ(capture.animationCurrentCount, 1U);
    EXPECT_EQ(capture.animationPlayingCount, 1U);
    EXPECT_EQ(capture.motionBuildCount, 1U);
    EXPECT_EQ(capture.motionSearchCount, 1U);
    EXPECT_EQ(capture.motionBestMatchCount, 1U);
    EXPECT_EQ(capture.motionTrajectoryCount, 1U);
    EXPECT_EQ(capture.motionPoseCount, 1U);
    EXPECT_EQ(capture.motionVectorCount, 2U);
    EXPECT_EQ(capture.motionYawCount, 1U);
    EXPECT_EQ(capture.motionTransitionCount, 1U);
    EXPECT_EQ(capture.motionWarpCount, 1U);
    EXPECT_FLOAT_EQ(std::get<float>(*context.FindOutputUVE(9U, "Result")), 8.0F);
    EXPECT_FLOAT_EQ(std::get<float>(*context.FindOutputUVE(13U, "Result")), 12.0F);
    EXPECT_TRUE(std::get<bool>(*context.FindOutputUVE(20U, "Result")));
}

TEST(ScriptVmUVETest, ExecuteScriptBytecodeUVE_AnimationMotionQuerySchedulerUsesCopiedContext) {
    ScriptBytecodeProgramUVE program;
    program.instructions.push_back({ScriptIrInstructionKindUVE::ExecuteNode, 1U, 0U,
                                    "motion.query.get_best_match", {}, {}});
    ScriptVmExecutionContextUVE context;
    SetActorInputUVE(context, 1U, Scene::EntityUVE{9U, 1U});
    AnimationMotionCaptureUVE capture;
    ScriptEngineCallBindingsUVE bindings = MakeAnimationMotionBindingsUVE(capture);
    ScriptVmExecutionOptionsUVE options;
    options.engineCallBindings = &bindings;
    const ScriptVmExecutionResultUVE result = ExecuteScriptBytecodeUVE(program, context, options);
    EXPECT_TRUE(result.IsSuccessUVE());
    EXPECT_EQ(result.instructionsExecuted, 1U);
    EXPECT_EQ(capture.motionBestMatchCount, 1U);
    EXPECT_FLOAT_EQ(std::get<float>(*context.FindOutputUVE(1U, "Result")), 12.0F);
}

TEST(ScriptVmUVETest, ExecuteScriptBytecodeUVE_AnimationMotionQueryNodesFailClosedWithoutBindings) {
    const std::array<const char*, 20U> nodeTypes{
        "animation.play", "animation.stop", "animation.pause", "animation.blend", "animation.blend_space",
        "animation.set_speed", "animation.set_weight", "animation.montage", "animation.get_current_animation",
        "animation.is_playing", "motion.query.build", "motion.query.search", "motion.query.get_best_match",
        "motion.query.set_trajectory", "motion.query.set_pose", "motion.query.set_velocity", "motion.query.set_facing",
        "motion.query.set_yaw", "motion.query.transition", "motion.query.motion_warp"};
    const Scene::EntityUVE actor{9U, 1U};
    for (std::size_t index = 0U; index < nodeTypes.size(); ++index) {
        const std::uint32_t nodeId = static_cast<std::uint32_t>(index + 1U);
        ScriptBytecodeProgramUVE program;
        program.instructions.push_back({ScriptIrInstructionKindUVE::ExecuteNode, nodeId, 0U, nodeTypes[index], {}, {}});
        ScriptVmExecutionContextUVE context;
        SetActorInputUVE(context, nodeId, actor);
        if (index == 0U) {
            ASSERT_TRUE(context.SetInputUVE(nodeId, "Clip", 1.0F));
            ASSERT_TRUE(context.SetInputUVE(nodeId, "Blend Duration", 0.1F));
        } else if (index == 1U || index == 2U || index == 9U) {
            ASSERT_TRUE(context.SetInputUVE(nodeId, "Clip", 1.0F));
        } else if (index == 3U) {
            ASSERT_TRUE(context.SetInputUVE(nodeId, "Clip A", 1.0F));
            ASSERT_TRUE(context.SetInputUVE(nodeId, "Clip B", 2.0F));
            ASSERT_TRUE(context.SetInputUVE(nodeId, "Weight", 0.5F));
        } else if (index == 4U) {
            ASSERT_TRUE(context.SetInputUVE(nodeId, "Blend Space", 1.0F));
            ASSERT_TRUE(context.SetInputUVE(nodeId, "X", 0.0F));
            ASSERT_TRUE(context.SetInputUVE(nodeId, "Y", 0.0F));
        } else if (index == 5U) {
            ASSERT_TRUE(context.SetInputUVE(nodeId, "Speed", 1.0F));
        } else if (index == 6U) {
            ASSERT_TRUE(context.SetInputUVE(nodeId, "Weight", 0.5F));
        } else if (index == 7U) {
            ASSERT_TRUE(context.SetInputUVE(nodeId, "Montage", 1.0F));
            ASSERT_TRUE(context.SetInputUVE(nodeId, "Weight", 0.5F));
        } else if (index == 10U) {
            ASSERT_TRUE(context.SetInputUVE(nodeId, "Velocity", ScriptVector3ValueUVE{{1.0F, 0.0F, 0.0F}}));
            ASSERT_TRUE(context.SetInputUVE(nodeId, "Facing", ScriptVector3ValueUVE{{0.0F, 0.0F, 1.0F}}));
            ASSERT_TRUE(context.SetInputUVE(nodeId, "Delta", 0.016F));
        } else if (index == 11U) {
            ASSERT_TRUE(context.SetInputUVE(nodeId, "Max Results", 4.0F));
        } else if (index == 13U) {
            ASSERT_TRUE(context.SetInputUVE(nodeId, "Sample", ScriptVector3ValueUVE{{1.0F, 0.0F, 0.0F}}));
            ASSERT_TRUE(context.SetInputUVE(nodeId, "Offset", 0.1F));
        } else if (index == 14U) {
            ScriptTransformValueUVE pose{};
            pose.scale = ScriptVector3ValueUVE{{1.0F, 1.0F, 1.0F}};
            ASSERT_TRUE(context.SetInputUVE(nodeId, "Pose", pose));
        } else if (index == 15U || index == 16U) {
            ASSERT_TRUE(context.SetInputUVE(nodeId, index == 15U ? "Velocity" : "Facing",
                                            ScriptVector3ValueUVE{{1.0F, 0.0F, 0.0F}}));
        } else if (index == 17U) {
            ASSERT_TRUE(context.SetInputUVE(nodeId, "Yaw", 0.0F));
        } else if (index == 18U) {
            ASSERT_TRUE(context.SetInputUVE(nodeId, "Target", 1.0F));
            ASSERT_TRUE(context.SetInputUVE(nodeId, "Duration", 0.1F));
        } else if (index == 19U) {
            ASSERT_TRUE(context.SetInputUVE(nodeId, "Target", ScriptVector3ValueUVE{{1.0F, 0.0F, 0.0F}}));
            ASSERT_TRUE(context.SetInputUVE(nodeId, "Weight", 0.5F));
        }
        EXPECT_EQ(ExecuteScriptBytecodeUVE(program, context).status, ScriptVmStatusUVE::NodeExecutionFailed)
            << nodeTypes[index];
    }
}

} // namespace UVE::Scripting

namespace UVE::Scripting {

TEST(ScriptCompilerIRUVETest, CompileScriptGraphToIrUVE_StagesEntityProducerBeforeAnimationAndMotionQuery) {
    ScriptNodeRegistryUVE registry;
    ASSERT_TRUE(RegisterBuiltInScriptNodesUVE(registry));
    const auto expectStaged = [](const ScriptIrCompileResultUVE& result, const char* consumer,
                                 const std::uint32_t producerId, const std::uint32_t consumerId) {
        ASSERT_TRUE(result.IsSuccessUVE());
        ASSERT_TRUE(result.program.has_value());
        ASSERT_EQ(result.program->instructions.size(), 3U);
        EXPECT_EQ(result.program->instructions[1].kind, ScriptIrInstructionKindUVE::TransferValue);
        EXPECT_EQ(result.program->instructions[1].sourceNodeId, producerId);
        EXPECT_EQ(result.program->instructions[1].targetNodeId, consumerId);
        EXPECT_EQ(result.program->instructions[2].nodeTypeId, consumer);
    };
    ScriptGraphUVE animationGraph;
    ASSERT_TRUE(animationGraph.AddNodeUVE({10U, "camera.get_camera"}));
    ASSERT_TRUE(animationGraph.AddNodeUVE({20U, "animation.set_speed"}));
    ASSERT_TRUE(animationGraph.AddLinkUVE({{10U, "Result"}, {20U, "Actor"}}));
    expectStaged(CompileScriptGraphToIrUVE(animationGraph, registry), "animation.set_speed", 10U, 20U);
    ScriptGraphUVE motionGraph;
    ASSERT_TRUE(motionGraph.AddNodeUVE({30U, "entity.spawn"}));
    ASSERT_TRUE(motionGraph.AddNodeUVE({40U, "motion.query.search"}));
    ASSERT_TRUE(motionGraph.AddLinkUVE({{30U, "Result"}, {40U, "Actor"}}));
    expectStaged(CompileScriptGraphToIrUVE(motionGraph, registry), "motion.query.search", 30U, 40U);
    ScriptGraphUVE physicsGraph;
    ASSERT_TRUE(physicsGraph.AddNodeUVE({50U, "entity.spawn"}));
    ASSERT_TRUE(physicsGraph.AddNodeUVE({60U, "physics.apply_force"}));
    ASSERT_TRUE(physicsGraph.AddLinkUVE({{50U, "Result"}, {60U, "Body"}}));
    expectStaged(CompileScriptGraphToIrUVE(physicsGraph, registry), "physics.apply_force", 50U, 60U);
}

} // namespace UVE::Scripting

namespace UVE::Scripting {
namespace {

struct PhysicsCaptureUVE final {
    Scene::EntityUVE body{9U, 1U};
    std::size_t raycastCount = 0U;
    std::size_t sphereCastCount = 0U;
    std::size_t boxCastCount = 0U;
    std::size_t capsuleCastCount = 0U;
    std::size_t overlapCount = 0U;
    std::size_t forceCount = 0U;
    std::size_t impulseCount = 0U;
    std::size_t setVelocityCount = 0U;
    std::size_t getVelocityCount = 0U;
    std::size_t gravityCount = 0U;
    std::size_t collisionCount = 0U;
};

bool CapturePhysicsRaycastUVE(void* userData, const ScriptVector3ValueUVE& origin,
                              const ScriptVector3ValueUVE& direction, float maxDistance, std::uint32_t layerMask,
                              Scene::EntityUVE ignoreEntity, bool* outHit, Scene::EntityUVE* outEntity,
                              ScriptVector3ValueUVE* outPoint, ScriptVector3ValueUVE* outNormal,
                              float* outDistance) noexcept {
    auto* capture = static_cast<PhysicsCaptureUVE*>(userData);
    if (capture == nullptr || outHit == nullptr || outEntity == nullptr || outPoint == nullptr ||
        outNormal == nullptr || outDistance == nullptr || !std::isfinite(origin.value.x) ||
        !std::isfinite(direction.value.z) || !std::isfinite(maxDistance) || layerMask == 0U ||
        ignoreEntity != Scene::kInvalidEntityUVE) return false;
    ++capture->raycastCount;
    *outHit = true;
    *outEntity = capture->body;
    *outPoint = ScriptVector3ValueUVE{{1.0F, 2.0F, 3.0F}};
    *outNormal = ScriptVector3ValueUVE{{0.0F, 1.0F, 0.0F}};
    *outDistance = 2.0F;
    return true;
}

bool CapturePhysicsSphereCastUVE(void* userData, const ScriptVector3ValueUVE& origin,
                                 const ScriptVector3ValueUVE& direction, float radius, float maxDistance,
                                 std::uint32_t layerMask, Scene::EntityUVE ignoreEntity, bool* outHit,
                                 Scene::EntityUVE* outEntity, ScriptVector3ValueUVE* outPoint,
                                 float* outDistance) noexcept {
    auto* capture = static_cast<PhysicsCaptureUVE*>(userData);
    if (capture == nullptr || outHit == nullptr || outEntity == nullptr || outPoint == nullptr ||
        outDistance == nullptr || !std::isfinite(origin.value.x) || !std::isfinite(direction.value.y) ||
        !std::isfinite(radius) || !std::isfinite(maxDistance) || layerMask == 0U ||
        ignoreEntity != Scene::kInvalidEntityUVE) return false;
    ++capture->sphereCastCount;
    *outHit = true;
    *outEntity = capture->body;
    *outPoint = ScriptVector3ValueUVE{{2.0F, 3.0F, 4.0F}};
    *outDistance = 3.0F;
    return true;
}

bool CapturePhysicsBoxCastUVE(void* userData, const ScriptVector3ValueUVE& origin,
                              const ScriptVector3ValueUVE& halfExtents, const ScriptVector3ValueUVE& direction,
                              float maxDistance, std::uint32_t layerMask, Scene::EntityUVE ignoreEntity,
                              bool* outHit, Scene::EntityUVE* outEntity, ScriptVector3ValueUVE* outPoint,
                              float* outDistance) noexcept {
    auto* capture = static_cast<PhysicsCaptureUVE*>(userData);
    if (capture == nullptr || outHit == nullptr || outEntity == nullptr || outPoint == nullptr ||
        outDistance == nullptr || !std::isfinite(origin.value.x) || !std::isfinite(halfExtents.value.y) ||
        !std::isfinite(direction.value.z) || !std::isfinite(maxDistance) || layerMask == 0U ||
        ignoreEntity != Scene::kInvalidEntityUVE) return false;
    ++capture->boxCastCount;
    *outHit = true;
    *outEntity = capture->body;
    *outPoint = ScriptVector3ValueUVE{{3.0F, 4.0F, 5.0F}};
    *outDistance = 4.0F;
    return true;
}

bool CapturePhysicsCapsuleCastUVE(void* userData, const ScriptVector3ValueUVE& origin,
                                  const ScriptVector3ValueUVE& direction, float radius, float halfHeight,
                                  float maxDistance, std::uint32_t layerMask, Scene::EntityUVE ignoreEntity,
                                  bool* outHit, Scene::EntityUVE* outEntity, ScriptVector3ValueUVE* outPoint,
                                  float* outDistance) noexcept {
    auto* capture = static_cast<PhysicsCaptureUVE*>(userData);
    if (capture == nullptr || outHit == nullptr || outEntity == nullptr || outPoint == nullptr ||
        outDistance == nullptr || !std::isfinite(origin.value.z) || !std::isfinite(direction.value.x) ||
        !std::isfinite(radius) || !std::isfinite(halfHeight) || !std::isfinite(maxDistance) || layerMask == 0U ||
        ignoreEntity != Scene::kInvalidEntityUVE) return false;
    ++capture->capsuleCastCount;
    *outHit = true;
    *outEntity = capture->body;
    *outPoint = ScriptVector3ValueUVE{{4.0F, 5.0F, 6.0F}};
    *outDistance = 5.0F;
    return true;
}

bool CapturePhysicsOverlapUVE(void* userData, const ScriptVector3ValueUVE& origin,
                              const ScriptVector3ValueUVE& halfExtents, std::uint32_t layerMask,
                              std::uint32_t* outCount) noexcept {
    auto* capture = static_cast<PhysicsCaptureUVE*>(userData);
    if (capture == nullptr || outCount == nullptr || !std::isfinite(origin.value.x) ||
        !std::isfinite(halfExtents.value.y) || layerMask == 0U) return false;
    ++capture->overlapCount;
    *outCount = 3U;
    return true;
}

bool CapturePhysicsVectorMutationUVE(void* userData, Scene::EntityUVE body,
                                     const ScriptVector3ValueUVE& value, bool* outResult) noexcept {
    auto* capture = static_cast<PhysicsCaptureUVE*>(userData);
    if (capture == nullptr || outResult == nullptr || body != capture->body ||
        !std::isfinite(value.value.x) || !std::isfinite(value.value.y) || !std::isfinite(value.value.z)) return false;
    *outResult = true;
    return true;
}

bool CapturePhysicsGetVelocityUVE(void* userData, Scene::EntityUVE body,
                                  ScriptVector3ValueUVE* outValue) noexcept {
    auto* capture = static_cast<PhysicsCaptureUVE*>(userData);
    if (capture == nullptr || outValue == nullptr || body != capture->body) return false;
    ++capture->getVelocityCount;
    *outValue = ScriptVector3ValueUVE{{6.0F, 7.0F, 8.0F}};
    return true;
}

bool CapturePhysicsGravityUVE(void* userData, Scene::EntityUVE body, bool enabled, bool* outResult) noexcept {
    auto* capture = static_cast<PhysicsCaptureUVE*>(userData);
    if (capture == nullptr || outResult == nullptr || body != capture->body || !enabled) return false;
    ++capture->gravityCount;
    *outResult = true;
    return true;
}

bool CapturePhysicsCollisionUVE(void* userData, Scene::EntityUVE body, bool* outResult) noexcept {
    auto* capture = static_cast<PhysicsCaptureUVE*>(userData);
    if (capture == nullptr || outResult == nullptr || body != capture->body) return false;
    ++capture->collisionCount;
    *outResult = true;
    return true;
}

ScriptEngineCallBindingsUVE MakePhysicsBindingsUVE(PhysicsCaptureUVE& capture) {
    ScriptEngineCallBindingsUVE bindings{};
    bindings.userData = &capture;
    bindings.physicsRaycast = CapturePhysicsRaycastUVE;
    bindings.physicsSphereCast = CapturePhysicsSphereCastUVE;
    bindings.physicsBoxCast = CapturePhysicsBoxCastUVE;
    bindings.physicsCapsuleCast = CapturePhysicsCapsuleCastUVE;
    bindings.physicsOverlap = CapturePhysicsOverlapUVE;
    bindings.physicsApplyForce = CapturePhysicsVectorMutationUVE;
    bindings.physicsApplyImpulse = CapturePhysicsVectorMutationUVE;
    bindings.physicsSetVelocity = CapturePhysicsVectorMutationUVE;
    bindings.physicsGetVelocity = CapturePhysicsGetVelocityUVE;
    bindings.physicsEnableGravity = CapturePhysicsGravityUVE;
    bindings.physicsIsColliding = CapturePhysicsCollisionUVE;
    return bindings;
}

bool SetPhysicsInputsUVE(ScriptVmExecutionContextUVE& context, const std::uint32_t nodeId,
                        const std::size_t index) {
    const ScriptVector3ValueUVE origin{{0.0F, 0.0F, 0.0F}};
    const ScriptVector3ValueUVE direction{{0.0F, 0.0F, 1.0F}};
    if (index <= 3U) {
        if (!context.SetInputUVE(nodeId, "Origin", origin) || !context.SetInputUVE(nodeId, "Direction", direction) ||
            !context.SetInputUVE(nodeId, "Max Distance", 10.0F) || !context.SetInputUVE(nodeId, "Layer Mask", 1.0F)) return false;
        if (index == 1U) return context.SetInputUVE(nodeId, "Radius", 0.5F);
        if (index == 2U) {
            return context.SetInputUVE(nodeId, "Half Extents", ScriptVector3ValueUVE{{1.0F, 1.0F, 1.0F}});
        }
        if (index == 3U) {
            return context.SetInputUVE(nodeId, "Radius", 0.5F) && context.SetInputUVE(nodeId, "Half Height", 1.0F);
        }
        return true;
    }
    if (index == 4U) {
        return context.SetInputUVE(nodeId, "Origin", origin) &&
               context.SetInputUVE(nodeId, "Half Extents", ScriptVector3ValueUVE{{1.0F, 1.0F, 1.0F}}) &&
               context.SetInputUVE(nodeId, "Layer Mask", 1.0F);
    }
    if (index == 5U || index == 6U || index == 7U) {
        const char* pinName = index == 5U ? "Force" : index == 6U ? "Impulse" : "Velocity";
        return context.SetInputUVE(nodeId, "Body", ScriptEntityValueUVE{Scene::EntityUVE{9U, 1U}}) &&
               context.SetInputUVE(nodeId, pinName, ScriptVector3ValueUVE{{1.0F, 2.0F, 3.0F}});
    }
    if (index == 8U || index == 10U) {
        return context.SetInputUVE(nodeId, "Body", ScriptEntityValueUVE{Scene::EntityUVE{9U, 1U}});
    }
    return context.SetInputUVE(nodeId, "Body", ScriptEntityValueUVE{Scene::EntityUVE{9U, 1U}}) &&
           (index == 9U ? context.SetInputUVE(nodeId, "Enabled", true) : true);
}

} // namespace

TEST(ScriptVmUVETest, ExecuteScriptBytecodeUVE_ExecutesPhysicsFamilyWithCopiedValues) {
    const std::array<const char*, 11U> nodeTypes{
        "physics.raycast", "physics.sphere_cast", "physics.box_cast", "physics.capsule_cast", "physics.overlap",
        "physics.apply_force", "physics.apply_impulse", "physics.set_velocity", "physics.get_velocity",
        "physics.enable_gravity", "physics.is_colliding"};
    ScriptBytecodeProgramUVE program;
    ScriptVmExecutionContextUVE context;
    for (std::size_t index = 0U; index < nodeTypes.size(); ++index) {
        const std::uint32_t nodeId = static_cast<std::uint32_t>(index + 1U);
        program.instructions.push_back({ScriptIrInstructionKindUVE::ExecuteNode, nodeId, 0U, nodeTypes[index], {}, {}});
        ASSERT_TRUE(SetPhysicsInputsUVE(context, nodeId, index));
    }
    PhysicsCaptureUVE capture;
    ScriptEngineCallBindingsUVE bindings = MakePhysicsBindingsUVE(capture);
    ScriptVmExecutionOptionsUVE options;
    options.engineCallBindings = &bindings;
    const ScriptVmExecutionResultUVE result = ExecuteScriptBytecodeUVE(program, context, options);
    ASSERT_TRUE(result.IsSuccessUVE());
    EXPECT_EQ(result.instructionsExecuted, 11U);
    EXPECT_EQ(capture.raycastCount, 1U);
    EXPECT_EQ(capture.sphereCastCount, 1U);
    EXPECT_EQ(capture.boxCastCount, 1U);
    EXPECT_EQ(capture.capsuleCastCount, 1U);
    EXPECT_EQ(capture.overlapCount, 1U);
    EXPECT_EQ(capture.getVelocityCount, 1U);
    EXPECT_EQ(capture.gravityCount, 1U);
    EXPECT_EQ(capture.collisionCount, 1U);
    EXPECT_TRUE(std::get<bool>(*context.FindOutputUVE(1U, "Hit")));
    EXPECT_EQ(std::get<ScriptEntityValueUVE>(*context.FindOutputUVE(1U, "Entity")).entity, capture.body);
    EXPECT_FLOAT_EQ(std::get<float>(*context.FindOutputUVE(5U, "Count")), 3.0F);
    EXPECT_EQ(std::get<ScriptVector3ValueUVE>(*context.FindOutputUVE(9U, "Velocity")).value,
              (Math::Vector3UVE{6.0F, 7.0F, 8.0F}));
    EXPECT_TRUE(std::get<bool>(*context.FindOutputUVE(11U, "Result")));
}

TEST(ScriptVmUVETest, ExecuteScriptBytecodeUVE_PhysicsSchedulerUsesCopiedContext) {
    ScriptBytecodeProgramUVE program;
    program.instructions.push_back({ScriptIrInstructionKindUVE::ExecuteNode, 1U, 0U,
                                    "physics.get_velocity", {}, {}});
    ScriptVmExecutionContextUVE context;
    ASSERT_TRUE(SetPhysicsInputsUVE(context, 1U, 8U));
    PhysicsCaptureUVE capture;
    ScriptEngineCallBindingsUVE bindings = MakePhysicsBindingsUVE(capture);
    ScriptVmExecutionOptionsUVE options;
    options.engineCallBindings = &bindings;
    const ScriptVmExecutionResultUVE result = ExecuteScriptBytecodeUVE(program, context, options);
    EXPECT_TRUE(result.IsSuccessUVE());
    EXPECT_EQ(result.instructionsExecuted, 1U);
    EXPECT_EQ(capture.getVelocityCount, 1U);
}

TEST(ScriptVmUVETest, ExecuteScriptBytecodeUVE_PhysicsNodesFailClosedWithoutBindings) {
    const std::array<const char*, 11U> nodeTypes{
        "physics.raycast", "physics.sphere_cast", "physics.box_cast", "physics.capsule_cast", "physics.overlap",
        "physics.apply_force", "physics.apply_impulse", "physics.set_velocity", "physics.get_velocity",
        "physics.enable_gravity", "physics.is_colliding"};
    for (std::size_t index = 0U; index < nodeTypes.size(); ++index) {
        const std::uint32_t nodeId = static_cast<std::uint32_t>(index + 1U);
        ScriptBytecodeProgramUVE program;
        program.instructions.push_back({ScriptIrInstructionKindUVE::ExecuteNode, nodeId, 0U, nodeTypes[index], {}, {}});
        ScriptVmExecutionContextUVE context;
        ASSERT_TRUE(SetPhysicsInputsUVE(context, nodeId, index));
        EXPECT_EQ(ExecuteScriptBytecodeUVE(program, context).status, ScriptVmStatusUVE::NodeExecutionFailed)
            << nodeTypes[index];
    }
}

} // namespace UVE::Scripting


namespace UVE::Scripting {
namespace {

TEST(ScriptBuiltInNodeUVETest, RegisterBuiltInScriptNodesUVE_ContainsAudioSetVolumeDescriptor) {
    ScriptNodeRegistryUVE registry;
    ASSERT_TRUE(RegisterBuiltInScriptNodesUVE(registry));
    const ScriptNodeTypeDescriptorUVE* descriptor = registry.FindNodeTypeUVE("audio.set_volume");
    ASSERT_NE(descriptor, nullptr);
    EXPECT_EQ(descriptor->displayName, "Set Volume");
    EXPECT_EQ(descriptor->category, "Audio");
    ASSERT_EQ(descriptor->pins.size(), 3U);
    EXPECT_EQ(descriptor->pins[0].name, "Source");
    EXPECT_EQ(descriptor->pins[0].type, ScriptValueTypeUVE::Entity);
    EXPECT_EQ(descriptor->pins[1].name, "Volume");
    EXPECT_EQ(descriptor->pins[1].type, ScriptValueTypeUVE::Number);
    EXPECT_EQ(descriptor->pins[2].name, "Result");
    EXPECT_EQ(descriptor->pins[2].type, ScriptValueTypeUVE::Boolean);

    const ScriptNodeTypeDescriptorUVE* pitchDescriptor = registry.FindNodeTypeUVE("audio.set_pitch");
    ASSERT_NE(pitchDescriptor, nullptr);
    EXPECT_EQ(pitchDescriptor->displayName, "Set Pitch");
    EXPECT_EQ(pitchDescriptor->category, "Audio");
    ASSERT_EQ(pitchDescriptor->pins.size(), 3U);
    EXPECT_EQ(pitchDescriptor->pins[0].name, "Source");
    EXPECT_EQ(pitchDescriptor->pins[0].type, ScriptValueTypeUVE::Entity);
    EXPECT_EQ(pitchDescriptor->pins[1].name, "Pitch");
    EXPECT_EQ(pitchDescriptor->pins[1].type, ScriptValueTypeUVE::Number);
    EXPECT_EQ(pitchDescriptor->pins[2].name, "Result");
    EXPECT_EQ(pitchDescriptor->pins[2].type, ScriptValueTypeUVE::Boolean);

    const ScriptNodeTypeDescriptorUVE* positionDescriptor = registry.FindNodeTypeUVE("audio.set_3d_position");
    ASSERT_NE(positionDescriptor, nullptr);
    EXPECT_EQ(positionDescriptor->displayName, "Set 3D Position");
    EXPECT_EQ(positionDescriptor->category, "Audio");
    ASSERT_EQ(positionDescriptor->pins.size(), 3U);
    EXPECT_EQ(positionDescriptor->pins[0].name, "Source");
    EXPECT_EQ(positionDescriptor->pins[0].type, ScriptValueTypeUVE::Entity);
    EXPECT_EQ(positionDescriptor->pins[1].name, "Position");
    EXPECT_EQ(positionDescriptor->pins[1].type, ScriptValueTypeUVE::Vector3);
    EXPECT_EQ(positionDescriptor->pins[2].name, "Result");
    EXPECT_EQ(positionDescriptor->pins[2].type, ScriptValueTypeUVE::Boolean);

    const ScriptNodeTypeDescriptorUVE* playDescriptor = registry.FindNodeTypeUVE("audio.play_sound");
    ASSERT_NE(playDescriptor, nullptr);
    EXPECT_EQ(playDescriptor->displayName, "Play Sound");
    EXPECT_EQ(playDescriptor->category, "Audio");
    ASSERT_EQ(playDescriptor->pins.size(), 2U);
    EXPECT_EQ(playDescriptor->pins[0].name, "Source");
    EXPECT_EQ(playDescriptor->pins[0].type, ScriptValueTypeUVE::Entity);
    EXPECT_EQ(playDescriptor->pins[1].name, "Result");
    EXPECT_EQ(playDescriptor->pins[1].type, ScriptValueTypeUVE::Boolean);

    const ScriptNodeTypeDescriptorUVE* stopDescriptor = registry.FindNodeTypeUVE("audio.stop_sound");
    ASSERT_NE(stopDescriptor, nullptr);
    EXPECT_EQ(stopDescriptor->displayName, "Stop Sound");
    EXPECT_EQ(stopDescriptor->category, "Audio");
    ASSERT_EQ(stopDescriptor->pins.size(), 2U);
    EXPECT_EQ(stopDescriptor->pins[0].name, "Source");
    EXPECT_EQ(stopDescriptor->pins[0].type, ScriptValueTypeUVE::Entity);
    EXPECT_EQ(stopDescriptor->pins[1].name, "Result");
    EXPECT_EQ(stopDescriptor->pins[1].type, ScriptValueTypeUVE::Boolean);

    const ScriptNodeTypeDescriptorUVE* playingDescriptor = registry.FindNodeTypeUVE("audio.is_playing");
    ASSERT_NE(playingDescriptor, nullptr);
    EXPECT_EQ(playingDescriptor->displayName, "Is Playing");
    EXPECT_EQ(playingDescriptor->category, "Audio");
    ASSERT_EQ(playingDescriptor->pins.size(), 2U);
    EXPECT_EQ(playingDescriptor->pins[0].name, "Source");
    EXPECT_EQ(playingDescriptor->pins[0].type, ScriptValueTypeUVE::Entity);
    EXPECT_EQ(playingDescriptor->pins[1].name, "Result");
    EXPECT_EQ(playingDescriptor->pins[1].type, ScriptValueTypeUVE::Boolean);
}

TEST(ScriptCompilerIRUVETest, CompileScriptGraphToIrUVE_StagesEntityBeforeAudioSetVolume) {
    ScriptNodeRegistryUVE registry;
    ASSERT_TRUE(RegisterBuiltInScriptNodesUVE(registry));
    ScriptGraphUVE graph;
    ASSERT_TRUE(graph.AddNodeUVE({1U, "entity.spawn"}));
    ASSERT_TRUE(graph.AddNodeUVE({2U, "audio.set_volume"}));
    ASSERT_TRUE(graph.AddLinkUVE({{1U, "Result"}, {2U, "Source"}}));

    const ScriptIrCompileResultUVE result = CompileScriptGraphToIrUVE(graph, registry);

    EXPECT_TRUE(result.IsSuccessUVE());
    ASSERT_TRUE(result.program.has_value());
    ASSERT_EQ(result.program->instructions.size(), 3U);
    EXPECT_EQ(result.program->instructions[0].nodeTypeId, "entity.spawn");
    EXPECT_EQ(result.program->instructions[1].kind, ScriptIrInstructionKindUVE::TransferValue);
    EXPECT_EQ(result.program->instructions[2].nodeTypeId, "audio.set_volume");
}

struct AudioCaptureUVE final {
    Scene::EntityUVE source{9U, 1U};
    float volume = 0.0F;
    ScriptVector3ValueUVE position{};
    std::size_t setVolumeCount = 0U;
    std::size_t setPositionCount = 0U;
};

bool CaptureAudioSetVolumeUVE(void* userData, Scene::EntityUVE source, float volume,
                              bool* outResult) noexcept {
    auto* capture = static_cast<AudioCaptureUVE*>(userData);
    if (capture == nullptr || outResult == nullptr || source != capture->source ||
        !std::isfinite(volume) || volume < 0.0F || volume > 1.0F) {
        return false;
    }
    capture->volume = volume;
    ++capture->setVolumeCount;
    *outResult = true;
    return true;
}

TEST(ScriptVmUVETest, ExecuteScriptBytecodeUVE_ExecutesAudioSetVolumeWithCopiedValues) {
    ScriptBytecodeProgramUVE program;
    program.instructions.push_back({ScriptIrInstructionKindUVE::ExecuteNode, 1U, 0U,
                                    "audio.set_volume", {}, {}});
    ScriptVmExecutionContextUVE context;
    ASSERT_TRUE(context.SetInputUVE(1U, "Source", ScriptEntityValueUVE{Scene::EntityUVE{9U, 1U}}));
    ASSERT_TRUE(context.SetInputUVE(1U, "Volume", 0.35F));
    AudioCaptureUVE capture;
    ScriptEngineCallBindingsUVE bindings{};
    bindings.userData = &capture;
    bindings.audioSetVolume = CaptureAudioSetVolumeUVE;
    ScriptVmExecutionOptionsUVE options;
    options.engineCallBindings = &bindings;

    const ScriptVmExecutionResultUVE result = ExecuteScriptBytecodeUVE(program, context, options);

    ASSERT_TRUE(result.IsSuccessUVE());
    EXPECT_EQ(result.instructionsExecuted, 1U);
    EXPECT_EQ(capture.setVolumeCount, 1U);
    EXPECT_FLOAT_EQ(capture.volume, 0.35F);
    const std::optional<ScriptVmValueUVE> output = context.FindOutputUVE(1U, "Result");
    ASSERT_TRUE(output.has_value());
    ASSERT_TRUE(std::holds_alternative<bool>(*output));
    EXPECT_TRUE(std::get<bool>(*output));
}

TEST(ScriptVmUVETest, ExecuteScriptBytecodeUVE_AudioSetVolumeFailsClosedForMissingBindingAndInvalidVolume) {
    ScriptBytecodeProgramUVE program;
    program.instructions.push_back({ScriptIrInstructionKindUVE::ExecuteNode, 1U, 0U,
                                    "audio.set_volume", {}, {}});
    ScriptVmExecutionContextUVE context;
    ASSERT_TRUE(context.SetInputUVE(1U, "Source", ScriptEntityValueUVE{Scene::EntityUVE{9U, 1U}}));
    ASSERT_TRUE(context.SetInputUVE(1U, "Volume", 0.35F));
    EXPECT_EQ(ExecuteScriptBytecodeUVE(program, context).status, ScriptVmStatusUVE::NodeExecutionFailed);

    AudioCaptureUVE capture;
    ScriptEngineCallBindingsUVE bindings{};
    bindings.userData = &capture;
    bindings.audioSetVolume = CaptureAudioSetVolumeUVE;
    ScriptVmExecutionOptionsUVE options;
    options.engineCallBindings = &bindings;
    ASSERT_TRUE(context.SetInputUVE(1U, "Volume", 1.01F));

    const ScriptVmExecutionResultUVE invalidResult = ExecuteScriptBytecodeUVE(program, context, options);

    EXPECT_EQ(invalidResult.status, ScriptVmStatusUVE::NodeExecutionFailed);
    EXPECT_EQ(capture.setVolumeCount, 0U);
}

} // namespace
} // namespace UVE::Scripting


namespace UVE::Scripting {
namespace {

TEST(ScriptCompilerIRUVETest, CompileScriptGraphToIrUVE_StagesEntityBeforeAudioSetPitch) {
    ScriptNodeRegistryUVE registry;
    ASSERT_TRUE(RegisterBuiltInScriptNodesUVE(registry));
    ScriptGraphUVE graph;
    ASSERT_TRUE(graph.AddNodeUVE({1U, "entity.spawn"}));
    ASSERT_TRUE(graph.AddNodeUVE({2U, "audio.set_pitch"}));
    ASSERT_TRUE(graph.AddLinkUVE({{1U, "Result"}, {2U, "Source"}}));

    const ScriptIrCompileResultUVE result = CompileScriptGraphToIrUVE(graph, registry);

    ASSERT_TRUE(result.IsSuccessUVE());
    ASSERT_TRUE(result.program.has_value());
    ASSERT_EQ(result.program->instructions.size(), 3U);
    EXPECT_EQ(result.program->instructions[0].nodeTypeId, "entity.spawn");
    EXPECT_EQ(result.program->instructions[1].kind, ScriptIrInstructionKindUVE::TransferValue);
    EXPECT_EQ(result.program->instructions[2].nodeTypeId, "audio.set_pitch");
}

bool CaptureAudioSetPitchUVE(void* userData, Scene::EntityUVE source, float pitch,
                             bool* outResult) noexcept {
    auto* capture = static_cast<AudioCaptureUVE*>(userData);
    if (capture == nullptr || outResult == nullptr || source != capture->source ||
        !std::isfinite(pitch) || pitch <= 0.0F) {
        return false;
    }
    capture->volume = pitch;
    ++capture->setVolumeCount;
    *outResult = true;
    return true;
}

TEST(ScriptVmUVETest, ExecuteScriptBytecodeUVE_ExecutesAudioSetPitchWithCopiedValues) {
    ScriptBytecodeProgramUVE program;
    program.instructions.push_back({ScriptIrInstructionKindUVE::ExecuteNode, 1U, 0U,
                                    "audio.set_pitch", {}, {}});
    ScriptVmExecutionContextUVE context;
    ASSERT_TRUE(context.SetInputUVE(1U, "Source", ScriptEntityValueUVE{Scene::EntityUVE{9U, 1U}}));
    ASSERT_TRUE(context.SetInputUVE(1U, "Pitch", 1.75F));
    AudioCaptureUVE capture;
    ScriptEngineCallBindingsUVE bindings{};
    bindings.userData = &capture;
    bindings.audioSetPitch = CaptureAudioSetPitchUVE;
    ScriptVmExecutionOptionsUVE options;
    options.engineCallBindings = &bindings;

    const ScriptVmExecutionResultUVE result = ExecuteScriptBytecodeUVE(program, context, options);

    ASSERT_TRUE(result.IsSuccessUVE());
    EXPECT_EQ(result.instructionsExecuted, 1U);
    EXPECT_EQ(capture.setVolumeCount, 1U);
    EXPECT_FLOAT_EQ(capture.volume, 1.75F);
    const std::optional<ScriptVmValueUVE> output = context.FindOutputUVE(1U, "Result");
    ASSERT_TRUE(output.has_value());
    ASSERT_TRUE(std::holds_alternative<bool>(*output));
    EXPECT_TRUE(std::get<bool>(*output));
}

TEST(ScriptVmUVETest, ExecuteScriptBytecodeUVE_AudioSetPitchFailsClosedForNonPositivePitch) {
    ScriptBytecodeProgramUVE program;
    program.instructions.push_back({ScriptIrInstructionKindUVE::ExecuteNode, 1U, 0U,
                                    "audio.set_pitch", {}, {}});
    ScriptVmExecutionContextUVE context;
    ASSERT_TRUE(context.SetInputUVE(1U, "Source", ScriptEntityValueUVE{Scene::EntityUVE{9U, 1U}}));
    ASSERT_TRUE(context.SetInputUVE(1U, "Pitch", 0.0F));
    AudioCaptureUVE capture;
    ScriptEngineCallBindingsUVE bindings{};
    bindings.userData = &capture;
    bindings.audioSetPitch = CaptureAudioSetPitchUVE;
    ScriptVmExecutionOptionsUVE options;
    options.engineCallBindings = &bindings;

    const ScriptVmExecutionResultUVE result = ExecuteScriptBytecodeUVE(program, context, options);

    EXPECT_EQ(result.status, ScriptVmStatusUVE::NodeExecutionFailed);
    EXPECT_EQ(capture.setVolumeCount, 0U);
}

} // namespace
} // namespace UVE::Scripting


namespace UVE::Scripting {
namespace {

TEST(ScriptCompilerIRUVETest, CompileScriptGraphToIrUVE_StagesEntityBeforeAudioSet3dPosition) {
    ScriptNodeRegistryUVE registry;
    ASSERT_TRUE(RegisterBuiltInScriptNodesUVE(registry));
    ScriptGraphUVE graph;
    ASSERT_TRUE(graph.AddNodeUVE({1U, "entity.spawn"}));
    ASSERT_TRUE(graph.AddNodeUVE({2U, "audio.set_3d_position"}));
    ASSERT_TRUE(graph.AddLinkUVE({{1U, "Result"}, {2U, "Source"}}));

    const ScriptIrCompileResultUVE result = CompileScriptGraphToIrUVE(graph, registry);

    ASSERT_TRUE(result.IsSuccessUVE());
    ASSERT_TRUE(result.program.has_value());
    ASSERT_EQ(result.program->instructions.size(), 3U);
    EXPECT_EQ(result.program->instructions[0].nodeTypeId, "entity.spawn");
    EXPECT_EQ(result.program->instructions[1].kind, ScriptIrInstructionKindUVE::TransferValue);
    EXPECT_EQ(result.program->instructions[2].nodeTypeId, "audio.set_3d_position");
}

bool CaptureAudioSet3dPositionUVE(void* userData, Scene::EntityUVE source,
                                  const ScriptVector3ValueUVE& position, bool* outResult) noexcept {
    auto* capture = static_cast<AudioCaptureUVE*>(userData);
    if (capture == nullptr || outResult == nullptr || source != capture->source ||
        !std::isfinite(position.value.x) || !std::isfinite(position.value.y) ||
        !std::isfinite(position.value.z)) {
        return false;
    }
    capture->position = position;
    ++capture->setPositionCount;
    *outResult = true;
    return true;
}

TEST(ScriptVmUVETest, ExecuteScriptBytecodeUVE_ExecutesAudioSet3dPositionWithCopiedValues) {
    ScriptBytecodeProgramUVE program;
    program.instructions.push_back({ScriptIrInstructionKindUVE::ExecuteNode, 1U, 0U,
                                    "audio.set_3d_position", {}, {}});
    ScriptVmExecutionContextUVE context;
    ASSERT_TRUE(context.SetInputUVE(1U, "Source", ScriptEntityValueUVE{Scene::EntityUVE{9U, 1U}}));
    ASSERT_TRUE(context.SetInputUVE(1U, "Position", ScriptVector3ValueUVE{{1.0F, 2.0F, 3.0F}}));
    AudioCaptureUVE capture;
    ScriptEngineCallBindingsUVE bindings{};
    bindings.userData = &capture;
    bindings.audioSet3dPosition = CaptureAudioSet3dPositionUVE;
    ScriptVmExecutionOptionsUVE options;
    options.engineCallBindings = &bindings;

    const ScriptVmExecutionResultUVE result = ExecuteScriptBytecodeUVE(program, context, options);

    ASSERT_TRUE(result.IsSuccessUVE());
    EXPECT_EQ(result.instructionsExecuted, 1U);
    EXPECT_EQ(capture.setPositionCount, 1U);
    EXPECT_EQ(capture.position.value, (Math::Vector3UVE{1.0F, 2.0F, 3.0F}));
    const std::optional<ScriptVmValueUVE> output = context.FindOutputUVE(1U, "Result");
    ASSERT_TRUE(output.has_value());
    ASSERT_TRUE(std::holds_alternative<bool>(*output));
    EXPECT_TRUE(std::get<bool>(*output));
}

TEST(ScriptVmUVETest, ExecuteScriptBytecodeUVE_AudioSet3dPositionFailsClosedForNonFinitePosition) {
    ScriptBytecodeProgramUVE program;
    program.instructions.push_back({ScriptIrInstructionKindUVE::ExecuteNode, 1U, 0U,
                                    "audio.set_3d_position", {}, {}});
    ScriptVmExecutionContextUVE context;
    ASSERT_TRUE(context.SetInputUVE(1U, "Source", ScriptEntityValueUVE{Scene::EntityUVE{9U, 1U}}));
    ASSERT_TRUE(context.SetInputUVE(1U, "Position",
                                    ScriptVector3ValueUVE{{std::numeric_limits<float>::quiet_NaN(), 0.0F, 0.0F}}));
    AudioCaptureUVE capture;
    ScriptEngineCallBindingsUVE bindings{};
    bindings.userData = &capture;
    bindings.audioSet3dPosition = CaptureAudioSet3dPositionUVE;
    ScriptVmExecutionOptionsUVE options;
    options.engineCallBindings = &bindings;

    const ScriptVmExecutionResultUVE result = ExecuteScriptBytecodeUVE(program, context, options);

    EXPECT_EQ(result.status, ScriptVmStatusUVE::NodeExecutionFailed);
    EXPECT_EQ(capture.setPositionCount, 0U);
}

} // namespace
} // namespace UVE::Scripting


namespace UVE::Scripting {
namespace {

TEST(ScriptCompilerIRUVETest, CompileScriptGraphToIrUVE_StagesEntityBeforeAudioPlaySound) {
    ScriptNodeRegistryUVE registry;
    ASSERT_TRUE(RegisterBuiltInScriptNodesUVE(registry));
    ScriptGraphUVE graph;
    ASSERT_TRUE(graph.AddNodeUVE({1U, "entity.spawn"}));
    ASSERT_TRUE(graph.AddNodeUVE({2U, "audio.play_sound"}));
    ASSERT_TRUE(graph.AddLinkUVE({{1U, "Result"}, {2U, "Source"}}));

    const ScriptIrCompileResultUVE result = CompileScriptGraphToIrUVE(graph, registry);

    ASSERT_TRUE(result.IsSuccessUVE());
    ASSERT_TRUE(result.program.has_value());
    ASSERT_EQ(result.program->instructions.size(), 3U);
    EXPECT_EQ(result.program->instructions[0].nodeTypeId, "entity.spawn");
    EXPECT_EQ(result.program->instructions[1].kind, ScriptIrInstructionKindUVE::TransferValue);
    EXPECT_EQ(result.program->instructions[2].nodeTypeId, "audio.play_sound");
}

bool CaptureAudioPlaySoundUVE(void* userData, Scene::EntityUVE source, bool* outResult) noexcept {
    auto* capture = static_cast<AudioCaptureUVE*>(userData);
    if (capture == nullptr || outResult == nullptr || source != capture->source) {
        return false;
    }
    ++capture->setVolumeCount;
    *outResult = true;
    return true;
}

TEST(ScriptVmUVETest, ExecuteScriptBytecodeUVE_ExecutesAudioPlaySoundWithCopiedSource) {
    ScriptBytecodeProgramUVE program;
    program.instructions.push_back({ScriptIrInstructionKindUVE::ExecuteNode, 1U, 0U,
                                    "audio.play_sound", {}, {}});
    ScriptVmExecutionContextUVE context;
    ASSERT_TRUE(context.SetInputUVE(1U, "Source", ScriptEntityValueUVE{Scene::EntityUVE{9U, 1U}}));
    AudioCaptureUVE capture;
    ScriptEngineCallBindingsUVE bindings{};
    bindings.userData = &capture;
    bindings.audioPlaySound = CaptureAudioPlaySoundUVE;
    ScriptVmExecutionOptionsUVE options;
    options.engineCallBindings = &bindings;

    const ScriptVmExecutionResultUVE result = ExecuteScriptBytecodeUVE(program, context, options);

    ASSERT_TRUE(result.IsSuccessUVE());
    EXPECT_EQ(result.instructionsExecuted, 1U);
    EXPECT_EQ(capture.setVolumeCount, 1U);
    const std::optional<ScriptVmValueUVE> output = context.FindOutputUVE(1U, "Result");
    ASSERT_TRUE(output.has_value());
    ASSERT_TRUE(std::holds_alternative<bool>(*output));
    EXPECT_TRUE(std::get<bool>(*output));
}

TEST(ScriptVmUVETest, ExecuteScriptBytecodeUVE_AudioPlaySoundFailsClosedForMissingBinding) {
    ScriptBytecodeProgramUVE program;
    program.instructions.push_back({ScriptIrInstructionKindUVE::ExecuteNode, 1U, 0U,
                                    "audio.play_sound", {}, {}});
    ScriptVmExecutionContextUVE context;
    ASSERT_TRUE(context.SetInputUVE(1U, "Source", ScriptEntityValueUVE{Scene::EntityUVE{9U, 1U}}));

    EXPECT_EQ(ExecuteScriptBytecodeUVE(program, context).status, ScriptVmStatusUVE::NodeExecutionFailed);
}

} // namespace
} // namespace UVE::Scripting


namespace UVE::Scripting {
namespace {

TEST(ScriptCompilerIRUVETest, CompileScriptGraphToIrUVE_StagesEntityBeforeAudioStopSound) {
    ScriptNodeRegistryUVE registry;
    ASSERT_TRUE(RegisterBuiltInScriptNodesUVE(registry));
    ScriptGraphUVE graph;
    ASSERT_TRUE(graph.AddNodeUVE({1U, "entity.spawn"}));
    ASSERT_TRUE(graph.AddNodeUVE({2U, "audio.stop_sound"}));
    ASSERT_TRUE(graph.AddLinkUVE({{1U, "Result"}, {2U, "Source"}}));

    const ScriptIrCompileResultUVE result = CompileScriptGraphToIrUVE(graph, registry);

    ASSERT_TRUE(result.IsSuccessUVE());
    ASSERT_TRUE(result.program.has_value());
    ASSERT_EQ(result.program->instructions.size(), 3U);
    EXPECT_EQ(result.program->instructions[0].nodeTypeId, "entity.spawn");
    EXPECT_EQ(result.program->instructions[1].kind, ScriptIrInstructionKindUVE::TransferValue);
    EXPECT_EQ(result.program->instructions[2].nodeTypeId, "audio.stop_sound");
}

TEST(ScriptVmUVETest, ExecuteScriptBytecodeUVE_ExecutesAudioStopSoundWithCopiedSource) {
    ScriptBytecodeProgramUVE program;
    program.instructions.push_back({ScriptIrInstructionKindUVE::ExecuteNode, 1U, 0U,
                                    "audio.stop_sound", {}, {}});
    ScriptVmExecutionContextUVE context;
    ASSERT_TRUE(context.SetInputUVE(1U, "Source", ScriptEntityValueUVE{Scene::EntityUVE{9U, 1U}}));
    AudioCaptureUVE capture;
    ScriptEngineCallBindingsUVE bindings{};
    bindings.userData = &capture;
    bindings.audioStopSound = CaptureAudioPlaySoundUVE;
    ScriptVmExecutionOptionsUVE options;
    options.engineCallBindings = &bindings;

    const ScriptVmExecutionResultUVE result = ExecuteScriptBytecodeUVE(program, context, options);

    ASSERT_TRUE(result.IsSuccessUVE());
    EXPECT_EQ(result.instructionsExecuted, 1U);
    EXPECT_EQ(capture.setVolumeCount, 1U);
    const std::optional<ScriptVmValueUVE> output = context.FindOutputUVE(1U, "Result");
    ASSERT_TRUE(output.has_value());
    ASSERT_TRUE(std::holds_alternative<bool>(*output));
    EXPECT_TRUE(std::get<bool>(*output));
}

TEST(ScriptVmUVETest, ExecuteScriptBytecodeUVE_AudioStopSoundFailsClosedForMissingBinding) {
    ScriptBytecodeProgramUVE program;
    program.instructions.push_back({ScriptIrInstructionKindUVE::ExecuteNode, 1U, 0U,
                                    "audio.stop_sound", {}, {}});
    ScriptVmExecutionContextUVE context;
    ASSERT_TRUE(context.SetInputUVE(1U, "Source", ScriptEntityValueUVE{Scene::EntityUVE{9U, 1U}}));

    EXPECT_EQ(ExecuteScriptBytecodeUVE(program, context).status, ScriptVmStatusUVE::NodeExecutionFailed);
}

} // namespace
} // namespace UVE::Scripting


namespace UVE::Scripting {
namespace {

TEST(ScriptCompilerIRUVETest, CompileScriptGraphToIrUVE_StagesEntityBeforeAudioIsPlaying) {
    ScriptNodeRegistryUVE registry;
    ASSERT_TRUE(RegisterBuiltInScriptNodesUVE(registry));
    ScriptGraphUVE graph;
    ASSERT_TRUE(graph.AddNodeUVE({1U, "entity.spawn"}));
    ASSERT_TRUE(graph.AddNodeUVE({2U, "audio.is_playing"}));
    ASSERT_TRUE(graph.AddLinkUVE({{1U, "Result"}, {2U, "Source"}}));

    const ScriptIrCompileResultUVE result = CompileScriptGraphToIrUVE(graph, registry);

    ASSERT_TRUE(result.IsSuccessUVE());
    ASSERT_TRUE(result.program.has_value());
    ASSERT_EQ(result.program->instructions.size(), 3U);
    EXPECT_EQ(result.program->instructions[0].nodeTypeId, "entity.spawn");
    EXPECT_EQ(result.program->instructions[1].kind, ScriptIrInstructionKindUVE::TransferValue);
    EXPECT_EQ(result.program->instructions[2].nodeTypeId, "audio.is_playing");
}

bool CaptureAudioIsPlayingUVE(void* userData, Scene::EntityUVE source, bool* outResult) noexcept {
    auto* capture = static_cast<AudioCaptureUVE*>(userData);
    if (capture == nullptr || outResult == nullptr || source != capture->source) {
        return false;
    }
    *outResult = capture->setVolumeCount != 0U;
    return true;
}

TEST(ScriptVmUVETest, ExecuteScriptBytecodeUVE_ExecutesAudioIsPlayingAndPublishesQueryValue) {
    ScriptBytecodeProgramUVE program;
    program.instructions.push_back({ScriptIrInstructionKindUVE::ExecuteNode, 1U, 0U,
                                    "audio.is_playing", {}, {}});
    ScriptVmExecutionContextUVE context;
    ASSERT_TRUE(context.SetInputUVE(1U, "Source", ScriptEntityValueUVE{Scene::EntityUVE{9U, 1U}}));
    AudioCaptureUVE capture;
    capture.setVolumeCount = 1U;
    ScriptEngineCallBindingsUVE bindings{};
    bindings.userData = &capture;
    bindings.audioIsPlaying = CaptureAudioIsPlayingUVE;
    ScriptVmExecutionOptionsUVE options;
    options.engineCallBindings = &bindings;

    const ScriptVmExecutionResultUVE result = ExecuteScriptBytecodeUVE(program, context, options);

    ASSERT_TRUE(result.IsSuccessUVE());
    const std::optional<ScriptVmValueUVE> output = context.FindOutputUVE(1U, "Result");
    ASSERT_TRUE(output.has_value());
    ASSERT_TRUE(std::holds_alternative<bool>(*output));
    EXPECT_TRUE(std::get<bool>(*output));
}

TEST(ScriptVmUVETest, ExecuteScriptBytecodeUVE_AudioIsPlayingFailsClosedForMissingBinding) {
    ScriptBytecodeProgramUVE program;
    program.instructions.push_back({ScriptIrInstructionKindUVE::ExecuteNode, 1U, 0U,
                                    "audio.is_playing", {}, {}});
    ScriptVmExecutionContextUVE context;
    ASSERT_TRUE(context.SetInputUVE(1U, "Source", ScriptEntityValueUVE{Scene::EntityUVE{9U, 1U}}));

    EXPECT_EQ(ExecuteScriptBytecodeUVE(program, context).status, ScriptVmStatusUVE::NodeExecutionFailed);
}

} // namespace
} // namespace UVE::Scripting
