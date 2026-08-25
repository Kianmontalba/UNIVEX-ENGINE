// Copyright (c) 2026 UniVex Studios. All Rights Reserved.

#pragma once

#include "uve/core/animation_tree_uve.h"
#include "uve/physics/character_controller_uve.h"
#include "uve/scene/components/animation_player_component_uve.h"
#include "uve/scene/components/audio_source_component_uve.h"
#include "uve/scene/components/camera_component_uve.h"
#include "uve/scene/components/collider_component_uve.h"
#include "uve/scene/components/light_component_uve.h"
#include "uve/scene/components/mesh_component_uve.h"
#include "uve/scene/components/particle_emitter_component_uve.h"
#include "uve/scene/components/rigid_body_component_uve.h"
#include "uve/scene/components/script_component_uve.h"
#include "uve/scene/components/transform_component_uve.h"
#include "uve/scene/nodes/scene_node_registry_uve.h"

namespace UVE::Scene::Nodes {

/// User-facing AnimationTree node façade. Core::AnimationTreeUVE remains the owning runtime value.
using AnimationTreeNodeFacadeUVE = Core::AnimationTreeUVE;

/// User-facing scene animation player façade. The authored component remains the serialized value.
using AnimationPlayerNodeFacadeUVE = AnimationPlayerComponentUVE;

/// User-facing CharacterBody3D-style façade. Movement remains caller-owned through Physics.
using CharacterBody3DNodeFacadeUVE = Physics::CharacterControllerInputUVE;
using CharacterBody3DMoveResultUVE = Physics::CharacterControllerMoveResultUVE;

using Camera3DNodeFacadeUVE = CameraComponentUVE;
using MeshInstance3DNodeFacadeUVE = MeshComponentUVE;
using Light3DNodeFacadeUVE = LightComponentUVE;
using Collider3DNodeFacadeUVE = ColliderComponentUVE;
using RigidBody3DNodeFacadeUVE = RigidBodyComponentUVE;
using AudioSource3DNodeFacadeUVE = AudioSourceComponentUVE;
using ParticleEmitter3DNodeFacadeUVE = ParticleEmitterComponentUVE;
using ScriptNodeFacadeUVE = ScriptComponentUVE;
using TransformNodeFacadeUVE = TransformComponentUVE;

} // namespace UVE::Scene::Nodes
