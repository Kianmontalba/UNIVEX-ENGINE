// Copyright (c) 2026 UniVex Studios. All Rights Reserved.
#pragma once

#include "uve/scene/entity_uve.h"
#include "uve/scripting/script_bytecode_uve.h"
#include "uve/scripting/script_vector2_value_uve.h"
#include "uve/scripting/script_vector3_value_uve.h"
#include "uve/scripting/script_collection_value_uve.h"
#include "uve/scripting/script_transform_value_uve.h"

#include <cstddef>
#include <cstdint>
#include <optional>
#include <string>
#include <variant>
#include <vector>

namespace UVE::Scripting {

enum class ScriptVmStatusUVE : std::uint8_t {
    Completed = 0,
    InstructionBudgetExceeded,
    InvalidInstruction,
    NodeExecutionFailed,
};

struct ScriptEntityValueUVE final {
    Scene::EntityUVE entity = Scene::kInvalidEntityUVE;

    [[nodiscard]] bool IsValidUVE() const noexcept {
        return entity != Scene::kInvalidEntityUVE;
    }

    [[nodiscard]] bool operator==(const ScriptEntityValueUVE&) const = default;
};

struct ScriptComponentValueUVE final {
    Scene::EntityUVE entity = Scene::kInvalidEntityUVE;
    std::string componentTypeId;
    bool present = false;

    [[nodiscard]] bool IsValidUVE() const noexcept {
        return !componentTypeId.empty();
    }

    [[nodiscard]] bool IsValidQueryFactUVE() const noexcept {
        return entity != Scene::kInvalidEntityUVE && IsValidUVE();
    }

    [[nodiscard]] bool operator==(const ScriptComponentValueUVE&) const = default;
};

/// Typed VM values are intentionally value-only and bounded by the execution context; append new
/// alternatives so existing float/Vector3/Boolean variant indices remain stable for serialized callers.
using ScriptVmValueUVE =
    std::variant<float, ScriptVector3ValueUVE, bool, ScriptEntityValueUVE, ScriptComponentValueUVE,
                 ScriptVector2ValueUVE, ScriptRotationValueUVE, ScriptTransformValueUVE,
                 ScriptArrayValueUVE, ScriptMapValueUVE, ScriptSetValueUVE, ScriptStructValueUVE>;

struct ScriptVmLocalVariableUVE final {
    std::uint32_t slot = 0U;
    ScriptVmValueUVE value = 0.0F;

    [[nodiscard]] bool operator==(const ScriptVmLocalVariableUVE&) const = default;
};

struct ScriptVmValueBindingUVE final {
    std::uint32_t nodeId = 0U;
    std::string pinName;
    ScriptVmValueUVE value = 0.0F;

    [[nodiscard]] bool operator==(const ScriptVmValueBindingUVE&) const = default;
};

struct ScriptVmFlowControlLatchUVE final {
    std::uint32_t nodeId = 0U;
    bool fired = false;

    [[nodiscard]] bool operator==(const ScriptVmFlowControlLatchUVE&) const = default;
};

struct ScriptVmGateStateUVE final {
    std::uint32_t nodeId = 0U;
    bool open = false;

    [[nodiscard]] bool operator==(const ScriptVmGateStateUVE&) const = default;
};

struct ScriptVmLoopStateUVE final {
    std::uint32_t nodeId = 0U;
    std::uint32_t iteration = 0U;
    bool active = false;

    [[nodiscard]] bool operator==(const ScriptVmLoopStateUVE&) const = default;
};

struct ScriptVmDelayStateUVE final {
    std::uint32_t nodeId = 0U;
    std::uint32_t remainingFrames = 0U;
    bool armed = false;

    [[nodiscard]] bool operator==(const ScriptVmDelayStateUVE&) const = default;
};

struct ScriptVmExecutionContextUVE final {
    static constexpr std::size_t kMaximumBindingsUVE = 1024U;
    static constexpr std::size_t kMaximumComponentFactsUVE = 256U;
    static constexpr std::size_t kMaximumLocalVariablesUVE = 256U;
    static constexpr std::size_t kMaximumFlowControlLatchesUVE = 256U;
    static constexpr std::size_t kMaximumGateStatesUVE = 256U;
    static constexpr std::size_t kMaximumLoopStatesUVE = 256U;
    static constexpr std::size_t kMaximumDelayStatesUVE = 256U;
    static constexpr std::uint32_t kMaximumLoopIterationsUVE = 4096U;
    static constexpr std::uint32_t kMaximumDelayFramesUVE = 4096U;

    std::vector<ScriptVmValueBindingUVE> inputs;
    std::vector<ScriptVmValueBindingUVE> outputs;
    std::vector<ScriptComponentValueUVE> componentFacts;
    std::vector<ScriptVmLocalVariableUVE> localVariables;
    std::vector<ScriptVmFlowControlLatchUVE> flowControlLatches;
    std::vector<ScriptVmGateStateUVE> gateStates;
    std::vector<ScriptVmLoopStateUVE> loopStates;
    std::vector<ScriptVmDelayStateUVE> delayStates;

    [[nodiscard]] bool InitializeLocalVariableUVE(std::uint32_t slot, ScriptVmValueUVE value);
    [[nodiscard]] bool SetLocalVariableUVE(std::uint32_t slot, ScriptVmValueUVE value);
    [[nodiscard]] std::optional<ScriptVmValueUVE> FindLocalVariableUVE(std::uint32_t slot) const;

    [[nodiscard]] bool InitializeDoOnceLatchUVE(std::uint32_t nodeId);
    [[nodiscard]] bool TryConsumeDoOnceLatchUVE(std::uint32_t nodeId);
    [[nodiscard]] bool ResetDoOnceLatchUVE(std::uint32_t nodeId);
    [[nodiscard]] std::optional<bool> FindDoOnceLatchUVE(std::uint32_t nodeId) const;
    [[nodiscard]] bool InitializeGateStateUVE(std::uint32_t nodeId);
    [[nodiscard]] bool SetGateStateUVE(std::uint32_t nodeId, bool open);
    [[nodiscard]] std::optional<bool> FindGateStateUVE(std::uint32_t nodeId) const;
    [[nodiscard]] bool InitializeLoopStateUVE(std::uint32_t nodeId);
    [[nodiscard]] bool SetLoopStateUVE(std::uint32_t nodeId, std::uint32_t iteration, bool active);
    [[nodiscard]] std::optional<ScriptVmLoopStateUVE> FindLoopStateUVE(std::uint32_t nodeId) const;
    [[nodiscard]] bool InitializeDelayStateUVE(std::uint32_t nodeId);
    [[nodiscard]] bool SetDelayStateUVE(std::uint32_t nodeId, std::uint32_t remainingFrames, bool armed);
    [[nodiscard]] std::optional<ScriptVmDelayStateUVE> FindDelayStateUVE(std::uint32_t nodeId) const;

    [[nodiscard]] bool SetInputUVE(std::uint32_t nodeId, std::string pinName, ScriptVmValueUVE value);
    [[nodiscard]] bool SetOutputUVE(std::uint32_t nodeId, std::string pinName, ScriptVmValueUVE value);
    [[nodiscard]] std::optional<ScriptVmValueUVE> FindInputUVE(std::uint32_t nodeId,
                                                                const std::string& pinName) const;
    [[nodiscard]] std::optional<ScriptVmValueUVE> FindOutputUVE(std::uint32_t nodeId,
                                                                 const std::string& pinName) const;
    [[nodiscard]] bool SetComponentFactUVE(Scene::EntityUVE entity, std::string componentTypeId,
                                            bool present);
    [[nodiscard]] std::optional<ScriptComponentValueUVE> FindComponentFactUVE(
        Scene::EntityUVE entity, const std::string& componentTypeId) const;
    void ClearOutputsUVE() noexcept;

    [[nodiscard]] bool operator==(const ScriptVmExecutionContextUVE&) const = default;
};

struct ScriptVmDiagnosticUVE final {
    std::size_t instructionIndex = 0U;
    std::string message;
};

enum class ScriptVmTraceEventKindUVE : std::uint8_t {
    NodeExecuted = 0,
    ValueTransferred,
    QueryFactsRefreshed,
    Completed,
    Failed,
    StagedValueTransferred,
};

struct ScriptVmTraceEventUVE final {
    ScriptVmTraceEventKindUVE kind = ScriptVmTraceEventKindUVE::Failed;
    Scene::EntityUVE entity = Scene::kInvalidEntityUVE;
    std::size_t instructionIndex = 0U;
    std::uint32_t sourceNodeId = 0U;
    std::uint32_t targetNodeId = 0U;
    std::string nodeTypeId;
    std::string message;

    [[nodiscard]] bool operator==(const ScriptVmTraceEventUVE&) const = default;
};

using ScriptEngineLogFunctionUVE = bool (*)(void* userData, float value) noexcept;
using ScriptDebugWarningFunctionUVE = bool (*)(void* userData, float value, bool* outResult) noexcept;
using ScriptEngineGetTimeFunctionUVE = bool (*)(void* userData, float* outSeconds) noexcept;
using ScriptEntitySpawnFunctionUVE = bool (*)(void* userData, Scene::EntityUVE* outEntity) noexcept;
using ScriptEntityDestroyFunctionUVE = bool (*)(void* userData, Scene::EntityUVE entity) noexcept;
using ScriptEntityFindByComponentFunctionUVE = bool (*)(
    void* userData, const ScriptComponentValueUVE& component, Scene::EntityUVE* outEntity) noexcept;
using ScriptEntityGetByHandleFunctionUVE = bool (*)(
    void* userData, float handle, Scene::EntityUVE* outEntity) noexcept;
using ScriptEntityComponentMutationFunctionUVE = bool (*)(
    void* userData, Scene::EntityUVE entity, const ScriptComponentValueUVE& component) noexcept;
using ScriptInputKeyQueryFunctionUVE = bool (*)(void* userData, float keyToken, bool* outResult) noexcept;
using ScriptInputMousePositionFunctionUVE = bool (*)(void* userData, ScriptVector2ValueUVE* outPosition) noexcept;
using ScriptInputMouseButtonQueryFunctionUVE = bool (*)(void* userData, float buttonToken, bool* outResult) noexcept;
using ScriptInputGamepadButtonQueryFunctionUVE = bool (*)(
    void* userData, float gamepadToken, float buttonToken, bool* outResult) noexcept;
using ScriptInputAxisQueryFunctionUVE = bool (*)(
    void* userData, float gamepadToken, float axisToken, float* outValue) noexcept;
using ScriptInputActionQueryFunctionUVE = bool (*)(void* userData, float actionToken, bool* outResult) noexcept;
using ScriptCameraGetFunctionUVE = bool (*)(void* userData, Scene::EntityUVE* outCamera) noexcept;
using ScriptCameraSetPositionFunctionUVE = bool (*)(
    void* userData, Scene::EntityUVE camera, const ScriptVector3ValueUVE& position) noexcept;
using ScriptCameraSetRotationFunctionUVE = bool (*)(
    void* userData, Scene::EntityUVE camera, const ScriptRotationValueUVE& rotation) noexcept;
using ScriptCameraLookAtFunctionUVE = bool (*)(
    void* userData, Scene::EntityUVE camera, const ScriptVector3ValueUVE& target) noexcept;
using ScriptCameraSetFovFunctionUVE = bool (*)(void* userData, Scene::EntityUVE camera, float fovDegrees) noexcept;
using ScriptCameraShakeFunctionUVE = bool (*)(
    void* userData, Scene::EntityUVE camera, float amplitude, float durationSeconds) noexcept;
using ScriptCameraSetActiveFunctionUVE = bool (*)(void* userData, Scene::EntityUVE camera, bool active) noexcept;
using ScriptAnimationClipControlFunctionUVE = bool (*) (
    void* userData, Scene::EntityUVE actor, float clipToken, float blendDuration, bool* outResult) noexcept;
using ScriptAnimationPauseFunctionUVE = bool (*) (
    void* userData, Scene::EntityUVE actor, float clipToken, bool* outResult) noexcept;
using ScriptAnimationBlendFunctionUVE = bool (*) (
    void* userData, Scene::EntityUVE actor, float clipAToken, float clipBToken, float weight,
    bool* outResult) noexcept;
using ScriptAnimationBlendSpaceFunctionUVE = bool (*) (
    void* userData, Scene::EntityUVE actor, float blendSpaceToken, float x, float y, bool* outResult) noexcept;
using ScriptAnimationScalarControlFunctionUVE = bool (*) (
    void* userData, Scene::EntityUVE actor, float value, bool* outResult) noexcept;
using ScriptAnimationMontageFunctionUVE = bool (*) (
    void* userData, Scene::EntityUVE actor, float montageToken, float weight, bool* outResult) noexcept;
using ScriptAnimationCurrentClipFunctionUVE = bool (*) (
    void* userData, Scene::EntityUVE actor, float* outClipToken) noexcept;
using ScriptAnimationPlayingFunctionUVE = bool (*) (
    void* userData, Scene::EntityUVE actor, float clipToken, bool* outResult) noexcept;
using ScriptMotionQueryBuildFunctionUVE = bool (*) (
    void* userData, Scene::EntityUVE actor, const ScriptVector3ValueUVE& velocity,
    const ScriptVector3ValueUVE& facing, float deltaSeconds, bool* outResult) noexcept;
using ScriptMotionQuerySearchFunctionUVE = bool (*) (
    void* userData, Scene::EntityUVE actor, float maximumResults, bool* outResult) noexcept;
using ScriptMotionQueryBestMatchFunctionUVE = bool (*) (
    void* userData, Scene::EntityUVE actor, float* outCandidateIndex) noexcept;
using ScriptMotionQueryTrajectoryFunctionUVE = bool (*) (
    void* userData, Scene::EntityUVE actor, const ScriptVector3ValueUVE& sample, float offsetSeconds,
    bool* outResult) noexcept;
using ScriptMotionQueryPoseFunctionUVE = bool (*) (
    void* userData, Scene::EntityUVE actor, const ScriptTransformValueUVE& pose, bool* outResult) noexcept;
using ScriptMotionQueryVectorFunctionUVE = bool (*) (
    void* userData, Scene::EntityUVE actor, const ScriptVector3ValueUVE& value, bool* outResult) noexcept;
using ScriptMotionQueryYawFunctionUVE = bool (*) (
    void* userData, Scene::EntityUVE actor, float yawDegrees, bool* outResult) noexcept;
using ScriptMotionQueryTransitionFunctionUVE = bool (*) (
    void* userData, Scene::EntityUVE actor, float targetToken, float durationSeconds, bool* outResult) noexcept;
using ScriptMotionQueryWarpFunctionUVE = bool (*) (
    void* userData, Scene::EntityUVE actor, const ScriptVector3ValueUVE& target, float weight,
    bool* outResult) noexcept;
using ScriptPhysicsRaycastFunctionUVE = bool (*) (
    void* userData, const ScriptVector3ValueUVE& origin, const ScriptVector3ValueUVE& direction,
    float maxDistance, std::uint32_t layerMask, Scene::EntityUVE ignoreEntity, bool* outHit,
    Scene::EntityUVE* outEntity, ScriptVector3ValueUVE* outPoint, ScriptVector3ValueUVE* outNormal,
    float* outDistance) noexcept;
using ScriptPhysicsSphereCastFunctionUVE = bool (*) (
    void* userData, const ScriptVector3ValueUVE& origin, const ScriptVector3ValueUVE& direction,
    float radius, float maxDistance, std::uint32_t layerMask, Scene::EntityUVE ignoreEntity, bool* outHit,
    Scene::EntityUVE* outEntity, ScriptVector3ValueUVE* outPoint, float* outDistance) noexcept;
using ScriptPhysicsBoxCastFunctionUVE = bool (*) (
    void* userData, const ScriptVector3ValueUVE& origin, const ScriptVector3ValueUVE& halfExtents,
    const ScriptVector3ValueUVE& direction, float maxDistance, std::uint32_t layerMask,
    Scene::EntityUVE ignoreEntity, bool* outHit, Scene::EntityUVE* outEntity,
    ScriptVector3ValueUVE* outPoint, float* outDistance) noexcept;
using ScriptPhysicsCapsuleCastFunctionUVE = bool (*) (
    void* userData, const ScriptVector3ValueUVE& origin, const ScriptVector3ValueUVE& direction,
    float radius, float halfHeight, float maxDistance, std::uint32_t layerMask,
    Scene::EntityUVE ignoreEntity, bool* outHit, Scene::EntityUVE* outEntity,
    ScriptVector3ValueUVE* outPoint, float* outDistance) noexcept;
using ScriptPhysicsOverlapFunctionUVE = bool (*) (
    void* userData, const ScriptVector3ValueUVE& origin, const ScriptVector3ValueUVE& halfExtents,
    std::uint32_t layerMask, std::uint32_t* outCount) noexcept;
using ScriptPhysicsBodyVectorMutationFunctionUVE = bool (*) (
    void* userData, Scene::EntityUVE body, const ScriptVector3ValueUVE& value, bool* outResult) noexcept;
using ScriptPhysicsBodyVectorGetFunctionUVE = bool (*) (
    void* userData, Scene::EntityUVE body, ScriptVector3ValueUVE* outValue) noexcept;
using ScriptPhysicsGravityFunctionUVE = bool (*) (
    void* userData, Scene::EntityUVE body, bool enabled, bool* outResult) noexcept;
using ScriptPhysicsCollisionQueryFunctionUVE = bool (*) (
    void* userData, Scene::EntityUVE body, bool* outResult) noexcept;
using ScriptAudioScalarControlFunctionUVE = bool (*) (
    void* userData, Scene::EntityUVE source, float value, bool* outResult) noexcept;
using ScriptAudioPositionControlFunctionUVE = bool (*) (
    void* userData, Scene::EntityUVE source, const ScriptVector3ValueUVE& position,
    bool* outResult) noexcept;
using ScriptAudioTriggerFunctionUVE = bool (*)(
    void* userData, Scene::EntityUVE source, bool* outResult) noexcept;
using ScriptAudioStateQueryFunctionUVE = bool (*)(
    void* userData, Scene::EntityUVE source, bool* outResult) noexcept;
using ScriptAudioAttenuationControlFunctionUVE = bool (*)(
    void* userData, Scene::EntityUVE source, float minDistance, float maxDistance,
    float model, bool* outResult) noexcept;

struct ScriptEngineCallBindingsUVE final {
    ScriptEngineLogFunctionUVE log = nullptr;
    void* userData = nullptr;
    ScriptEngineGetTimeFunctionUVE getTime = nullptr;
    ScriptEntitySpawnFunctionUVE spawnEntity = nullptr;
    ScriptEntityDestroyFunctionUVE destroyEntity = nullptr;
    ScriptEntityFindByComponentFunctionUVE findEntityByComponent = nullptr;
    ScriptEntityGetByHandleFunctionUVE getEntityByHandle = nullptr;
    ScriptEntityComponentMutationFunctionUVE addComponent = nullptr;
    ScriptEntityComponentMutationFunctionUVE removeComponent = nullptr;
    ScriptInputKeyQueryFunctionUVE inputKeyPressed = nullptr;
    ScriptInputKeyQueryFunctionUVE inputKeyReleased = nullptr;
    ScriptInputKeyQueryFunctionUVE inputKeyDown = nullptr;
    ScriptInputMousePositionFunctionUVE inputMousePosition = nullptr;
    ScriptInputMouseButtonQueryFunctionUVE inputMouseButton = nullptr;
    ScriptInputGamepadButtonQueryFunctionUVE inputGamepadButton = nullptr;
    ScriptInputAxisQueryFunctionUVE inputAxis = nullptr;
    ScriptInputActionQueryFunctionUVE inputAction = nullptr;
    ScriptCameraGetFunctionUVE cameraGet = nullptr;
    ScriptCameraSetPositionFunctionUVE cameraSetPosition = nullptr;
    ScriptCameraSetRotationFunctionUVE cameraSetRotation = nullptr;
    ScriptCameraLookAtFunctionUVE cameraLookAt = nullptr;
    ScriptCameraSetFovFunctionUVE cameraSetFov = nullptr;
    ScriptCameraShakeFunctionUVE cameraShake = nullptr;
    ScriptCameraSetActiveFunctionUVE cameraSetActive = nullptr;
    ScriptAnimationClipControlFunctionUVE animationPlay = nullptr;
    ScriptAnimationClipControlFunctionUVE animationStop = nullptr;
    ScriptAnimationPauseFunctionUVE animationPause = nullptr;
    ScriptAnimationBlendFunctionUVE animationBlend = nullptr;
    ScriptAnimationBlendSpaceFunctionUVE animationBlendSpace = nullptr;
    ScriptAnimationScalarControlFunctionUVE animationSetSpeed = nullptr;
    ScriptAnimationScalarControlFunctionUVE animationSetWeight = nullptr;
    ScriptAnimationMontageFunctionUVE animationMontage = nullptr;
    ScriptAnimationCurrentClipFunctionUVE animationGetCurrent = nullptr;
    ScriptAnimationPlayingFunctionUVE animationIsPlaying = nullptr;
    ScriptMotionQueryBuildFunctionUVE motionQueryBuild = nullptr;
    ScriptMotionQuerySearchFunctionUVE motionQuerySearch = nullptr;
    ScriptMotionQueryBestMatchFunctionUVE motionQueryBestMatch = nullptr;
    ScriptMotionQueryTrajectoryFunctionUVE motionQuerySetTrajectory = nullptr;
    ScriptMotionQueryPoseFunctionUVE motionQuerySetPose = nullptr;
    ScriptMotionQueryVectorFunctionUVE motionQuerySetVelocity = nullptr;
    ScriptMotionQueryVectorFunctionUVE motionQuerySetFacing = nullptr;
    ScriptMotionQueryYawFunctionUVE motionQuerySetYaw = nullptr;
    ScriptMotionQueryTransitionFunctionUVE motionQueryTransition = nullptr;
    ScriptMotionQueryWarpFunctionUVE motionQueryMotionWarp = nullptr;
    ScriptPhysicsRaycastFunctionUVE physicsRaycast = nullptr;
    ScriptPhysicsSphereCastFunctionUVE physicsSphereCast = nullptr;
    ScriptPhysicsBoxCastFunctionUVE physicsBoxCast = nullptr;
    ScriptPhysicsCapsuleCastFunctionUVE physicsCapsuleCast = nullptr;
    ScriptPhysicsOverlapFunctionUVE physicsOverlap = nullptr;
    ScriptPhysicsBodyVectorMutationFunctionUVE physicsApplyForce = nullptr;
    ScriptPhysicsBodyVectorMutationFunctionUVE physicsApplyImpulse = nullptr;
    ScriptPhysicsBodyVectorMutationFunctionUVE physicsSetVelocity = nullptr;
    ScriptPhysicsBodyVectorGetFunctionUVE physicsGetVelocity = nullptr;
    ScriptPhysicsGravityFunctionUVE physicsEnableGravity = nullptr;
    ScriptPhysicsCollisionQueryFunctionUVE physicsIsColliding = nullptr;
    ScriptAudioScalarControlFunctionUVE audioSetVolume = nullptr;
    ScriptAudioScalarControlFunctionUVE audioSetPitch = nullptr;
    ScriptAudioPositionControlFunctionUVE audioSet3dPosition = nullptr;
    ScriptAudioTriggerFunctionUVE audioPlaySound = nullptr;
    ScriptAudioTriggerFunctionUVE audioStopSound = nullptr;
    ScriptAudioStateQueryFunctionUVE audioIsPlaying = nullptr;
    ScriptAudioAttenuationControlFunctionUVE audioSetAttenuation = nullptr;
    ScriptDebugWarningFunctionUVE debugWarning = nullptr;
};

struct ScriptVmExecutionOptionsUVE final {
    std::size_t instructionBudget = 4096U;
    const ScriptEngineCallBindingsUVE* engineCallBindings = nullptr;
};

struct ScriptVmExecutionResultUVE final {
    static constexpr std::size_t kMaximumTraceEventsUVE = 512U;
    static constexpr std::size_t kMaximumTraceMessageBytesUVE = 256U;

    ScriptVmStatusUVE status = ScriptVmStatusUVE::Completed;
    std::size_t instructionsExecuted = 0U;
    std::vector<ScriptVmDiagnosticUVE> diagnostics;
    std::vector<ScriptVmTraceEventUVE> trace;
    bool traceTruncated = false;

    [[nodiscard]] bool IsSuccessUVE() const noexcept {
        return status == ScriptVmStatusUVE::Completed && diagnostics.empty();
    }

    void AppendTraceEventUVE(ScriptVmTraceEventUVE event);
    void PrependTraceEventsUVE(std::vector<ScriptVmTraceEventUVE> prefix, bool prefixTruncated = false);

    [[nodiscard]] bool operator==(const ScriptVmExecutionResultUVE&) const = default;
};

[[nodiscard]] ScriptVmExecutionResultUVE ExecuteScriptBytecodeUVE(
    const ScriptBytecodeProgramUVE& program,
    ScriptVmExecutionOptionsUVE options = {});

[[nodiscard]] ScriptVmExecutionResultUVE ExecuteScriptBytecodeUVE(
    const ScriptBytecodeProgramUVE& program,
    ScriptVmExecutionContextUVE& context,
    ScriptVmExecutionOptionsUVE options = {});

} // namespace UVE::Scripting
