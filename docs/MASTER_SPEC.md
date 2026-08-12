# UniVex Engine (UVE) — Master Specification Matrix

## 1. Engine Overview & Core Philosophy

| Component | Detailed Explanation & Requirements |
| :--- | :--- |
| **Engine Identity** | **UniVex Engine (UVE)**: A professional-grade 3D game engine targeting Windows PC, Linux PC, Android Mobile, and iOS Mobile. |
| **Language Stack** | **Modern C++ (C++20/23)**: Core Engine API uses smart pointers, RAII, and constexpr. No raw new/delete. STL is avoided in hot paths in favor of custom allocators. |
| **Core Philosophy** | Minimal built-in nodes; everything else is a **Plugin**. The Editor is independent, responsive, and supports full touch manipulation for Android. |

## 2. Custom File Format System (.uve*)

| Extension | Purpose & Magic Header: `UVE\0` |
| :--- | :--- |
| **.uveproj** | Project file containing scenes list, settings, and build configurations. |
| **.uvescene** | Scene file storing entity hierarchy, component data, and references. |
| **.uvemodel** | 3D Mesh/Model data including vertices, indices, UVs, normals, tangents, bone weights, and LODs. |
| **.uvetex** | Texture data supporting mipmaps and compression formats like ASTC, ETC2, BC7, and DXT. |
| **.uvemat** | Material definitions including PBR parameters, shader graph references, and texture slots. |
| **.uveshader** | Shader source code for GLSL (Vulkan/OpenGL), HLSL (DirectX), and MSL (Metal). |
| **.uvescript** | Visual Script Graph storing node positions, connections, and validated native bytecode. |

## 3. Naming Conventions & Standards

| Category | Required Standard |
| :--- | :--- |
| **Engine API** | **UVE Suffix Required**: Classes (e.g., `RenderSystemUVE`), Structs (`Vector3UVE`), Enums (`RenderModeUVE`), and Functions (`InitializeEngineUVE()`). |
| **Visual Scripting** | **UVE Suffix Required**: `ScriptGraphUVE`, `CompileGraphUVE()`, `OnBeginPlayUVE`. |
| **User-Facing Nodes** | **NO UVE Suffix**: Clean industry-standard names like `Node3D`, `Mesh3D`, `Camera3D`, `RigidBody3D`. |
| **Copyright** | Every file outside `engine/core` must start with: `// Copyright (c) 2026 UniVex Studios. All Rights Reserved.` |

## 4. Built-in Core Nodes (Minimal Set)

| Node Name | Functionality Description |
| :--- | :--- |
| **Node3D** | Base 3D node with Transform properties (Position, Rotation, Scale). |
| **Mesh3D** | Static mesh renderer requiring a `MeshFilter3D` and `Material3D`. |
| **Camera3D** | Perspective or Orthographic camera with FOV and clipping plane settings. |
| **Lights** | `DirectionalLight3D` (cascaded shadows), `PointLight3D` (attenuation), and `SpotLight3D` (cone angles). |
| **Physics** | `CollisionShape3D`, `RigidBody3D` (dynamic), `StaticBody3D`, `CharacterBody3D` (kinematic controller with move_and_slide support), and `Area3D` (trigger volumes). |
| **Audio & Anim** | `AudioSource3D` for positional sound, `AnimationPlayer3D` for `.uveanim` playback, and `AnimationTree` for complex state-machine and blend-tree logic. |

## 5. General IK Retarget Subsystem (Part 27)

| Pipeline Operation | Detailed Implementation Requirement |
| :--- | :--- |
| **1. InitializeTargetPose** | Creates the target rest/base pose from the skeleton data. |
| **2. ApplyPoseCorrections** | Applies source and target reference-pose alignment using local-space offsets relative to rest poses. |
| **3. TransferRootMotion** | Processes root translation and rotation according to policy (Direct, Scaled, or Horizontal-only). |
| **4. TransferPelvisMotion** | Handles pelvis movement with independent horizontal, vertical, and scale controls based on rig metrics. |
| **5. TransferFKChains** | Computes source motion relative to reference, distributing rotation across target chains with length compensation. |
| **6. ResolveIKGoals** | Derives target end-effector goals (feet, hands, head) from source motion transforms. |
| **7. SolveIKChains** | Runs UVE-native solvers, including analytic two-segment and iterative multi-segment solvers. |
| **8. ApplyContactConstraints** | Optional foot-contact correction supporting planted-foot locking and ground height sampling. |
| **9. FinalizeRetargetedPose** | Normalizes rotations, enforces joint limits, and emits the final valid output pose. |

---
*Document compiled and structured by **Manus AI**.*
