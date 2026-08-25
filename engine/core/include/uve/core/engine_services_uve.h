//                                      UVE
//                                UniVex Engine
//
// UniVex Engine (UVE) — Proprietary Game Engine
// Copyright (c) 2026 UniVex Studios. All Rights Reserved.
// Unauthorized copying, modification, distribution, or use of this code
// in whole or in part is strictly prohibited without express written
// permission from UniVex Studios.
// Violators will be prosecuted to the fullest extent of the law.


#pragma once

#include "uve/asset/i_asset_bundle_uve.h"
#include "uve/asset/i_asset_database_uve.h"
#include "uve/asset/i_asset_importer_uve.h"
#include "uve/asset/i_asset_import_queue_uve.h"
#include "uve/asset/i_asset_manager_uve.h"
#include "uve/asset/i_derived_artifact_cache_uve.h"
#include "uve/asset/i_file_system_uve.h"
#include "uve/asset/i_hot_reload_uve.h"
#include "uve/asset/i_project_file_index_uve.h"
#include "uve/asset/i_project_change_watcher_uve.h"
#include "uve/audio/i_audio_device_uve.h"
#include "uve/audio/i_audio_source_system_uve.h"
#include "uve/audio/i_audio_system_uve.h"
#include "uve/commandline/i_command_line_uve.h"
#include "uve/config/i_config_manager_uve.h"
#include "uve/debug/i_logger_uve.h"
#include "uve/events/i_event_system_uve.h"
#include "uve/input/i_gamepad_input_system_uve.h"
#include "uve/input/i_input_system_uve.h"
#include "uve/input/i_mobile_gesture_system_uve.h"
#include "uve/input/i_mobile_input_system_uve.h"
#include "uve/memory/i_memory_manager_uve.h"
#include "uve/physics/i_collision_system_uve.h"
#include "uve/physics/i_physics_system_uve.h"
#include "uve/physics/i_physics_query_system_uve.h"
#include "uve/physics/physics_constraint_system_uve.h"
#include "uve/physics/i_raycast_system_uve.h"
#include "uve/render/i_camera_system_uve.h"
#include "uve/render/i_light_system_uve.h"
#include "uve/render/i_mesh_renderer_uve.h"
#include "uve/render/i_render_device_uve.h"
#include "uve/render/i_render_system_uve.h"
#include "uve/render/i_renderer_3d_uve.h"
#include "uve/render/shader/i_shader_manager_uve.h"
#include "uve/save/i_checkpoint_manager_uve.h"
#include "uve/save/i_save_game_system_uve.h"
#include "uve/scene/i_entity_manager_uve.h"
#include "uve/scene/i_particle_runtime_uve.h"
#include "uve/scene/i_prefab_system_uve.h"
#include "uve/scene/i_scene_graph_uve.h"
#include "uve/scene/i_scene_serializer_uve.h"
#include "uve/threading/i_thread_pool_uve.h"
#include "uve/utilities/i_timer_uve.h"
#include "uve/window/i_window_manager_uve.h"

namespace UVE::Core {

/// EngineServicesUVE is the engine's central dependency-provider / service
/// container: a small bundle of references to the core engine services
/// (Logger, Timer, EventSystem, MemoryManager, ThreadPool, CommandLine,
/// ConfigManager, EntityManager, SceneGraph, AssetDatabase, ProjectFileIndex, DerivedArtifactCache,
/// ProjectChangeWatcher, SceneSerializer, PrefabSystem, HotReload, AssetManager, AssetImporter, AssetImportQueue, AssetBundle,
/// FileSystem, RenderDevice, ShaderManager, RenderSystem, CameraSystem,
/// MeshRenderer, LightSystem, Renderer3D, CollisionSystem, PhysicsSystem,
/// PhysicsQuerySystem, RaycastSystem, PhysicsConstraintSystem, InputSystem, GamepadInputSystem, MobileInputSystem, MobileGestureSystem, AudioDevice, AudioSystem, AudioSourceSystem,
/// SaveGameSystem, CheckpointManager, WindowManager, ParticleRuntime), built once
/// EngineCoreUVE has constructed all thirty-nine. Any future subsystem that
/// needs access to one of these should receive an EngineServicesUVE&
/// (obtained from EngineCoreUVE::GetServicesUVE()) rather than a raw global
/// pointer — the logging macros' internal active-instance pointer remains
/// the one intentional exception to that rule (see
/// docs/CODING_STANDARDS.md).
/// Referenced through the
/// ILoggerUVE/ITimerUVE/IEventSystemUVE/IMemoryManagerUVE/IThreadPoolUVE/
/// ICommandLineUVE/IConfigManagerUVE/IEntityManagerUVE/ISceneGraphUVE/
/// IAssetDatabaseUVE/IProjectFileIndexUVE/IDerivedArtifactCacheUVE/IProjectChangeWatcherUVE/ISceneSerializerUVE/IPrefabSystemUVE/IHotReloadUVE/
/// IAssetManagerUVE/IAssetImporterUVE/IAssetImportQueueUVE/IAssetBundleUVE/IFileSystemUVE/
/// IRenderDeviceUVE/Shader::IShaderManagerUVE/IRenderSystemUVE/
/// ICameraSystemUVE/IMeshRendererUVE/ILightSystemUVE/
/// IRenderer3DUVE/ICollisionSystemUVE/IPhysicsSystemUVE/IPhysicsQuerySystemUVE/
/// IRaycastSystemUVE/PhysicsConstraintSystemUVE/IInputSystemUVE/IGamepadInputSystemUVE/IMobileInputSystemUVE/
/// IMobileGestureSystemUVE/IAudioDeviceUVE/IAudioSystemUVE/IAudioSourceSystemUVE/
/// ISaveGameSystemUVE/ICheckpointManagerUVE/IWindowManagerUVE/IParticleRuntimeUVE interfaces,
/// not the concrete types, so a future substitute implementation of any of
/// the thirty-nine requires no change here.
/// Thread-safety: EngineServicesUVE itself holds only non-owning pointers
/// and has no mutable state of its own; the thread-safety of each accessor
/// is whatever the referenced service documents.
class EngineServicesUVE final {
public:
    EngineServicesUVE(Debug::ILoggerUVE& logger, Utilities::ITimerUVE& timer,
                       Events::IEventSystemUVE& eventSystem,
                       Memory::IMemoryManagerUVE& memoryManager,
                       Threading::IThreadPoolUVE& threadPool,
                       CommandLine::ICommandLineUVE& commandLine,
                       Config::IConfigManagerUVE& configManager,
                       Scene::IEntityManagerUVE& entityManager,
                       Scene::ISceneGraphUVE& sceneGraph,
                       Asset::IAssetDatabaseUVE& assetDatabase,
                       Asset::IProjectFileIndexUVE& projectFileIndex,
                       Asset::IDerivedArtifactCacheUVE& derivedArtifactCache,
                       Asset::IProjectChangeWatcherUVE& projectChangeWatcher,
                       Scene::ISceneSerializerUVE& sceneSerializer,
                       Scene::IPrefabSystemUVE& prefabSystem,
                       Scene::IParticleRuntimeUVE& particleRuntime,
                       Asset::IHotReloadUVE& hotReload,
                       Asset::IAssetManagerUVE& assetManager,
                       Asset::IAssetImporterUVE& assetImporter,
                       Asset::IAssetImportQueueUVE& assetImportQueue,
                       Asset::IAssetBundleUVE& assetBundle,
                       Asset::IFileSystemUVE& fileSystem,
                       Render::IRenderDeviceUVE& renderDevice,
                       Render::Shader::IShaderManagerUVE& shaderManager,
                       Render::IRenderSystemUVE& renderSystem,
                       Render::ICameraSystemUVE& cameraSystem,
                       Render::IMeshRendererUVE& meshRenderer,
                       Render::ILightSystemUVE& lightSystem,
                       Render::IRenderer3DUVE& renderer3D,
                       Physics::ICollisionSystemUVE& collisionSystem,
                       Physics::IPhysicsSystemUVE& physicsSystem,
                       Physics::IPhysicsQuerySystemUVE& physicsQuerySystem,
                       Physics::IRaycastSystemUVE& raycastSystem,
                       Physics::PhysicsConstraintSystemUVE& physicsConstraintSystem,
                       Input::IInputSystemUVE& inputSystem,
                       Input::IGamepadInputSystemUVE& gamepadInputSystem,
                       Input::IMobileInputSystemUVE& mobileInputSystem,
                       Input::IMobileGestureSystemUVE& mobileGestureSystem,
                       Audio::IAudioDeviceUVE& audioDevice,
                       Audio::IAudioSystemUVE& audioSystem,
                       Audio::IAudioSourceSystemUVE& audioSourceSystem,
                       Save::ISaveGameSystemUVE& saveGameSystem,
                       Save::ICheckpointManagerUVE& checkpointManager,
                       Window::IWindowManagerUVE& windowManager) noexcept;

    [[nodiscard]] Debug::ILoggerUVE& GetLoggerUVE() const noexcept;
    [[nodiscard]] Utilities::ITimerUVE& GetTimerUVE() const noexcept;
    [[nodiscard]] Events::IEventSystemUVE& GetEventSystemUVE() const noexcept;
    [[nodiscard]] Memory::IMemoryManagerUVE& GetMemoryManagerUVE() const noexcept;
    [[nodiscard]] Threading::IThreadPoolUVE& GetThreadPoolUVE() const noexcept;
    [[nodiscard]] CommandLine::ICommandLineUVE& GetCommandLineUVE() const noexcept;
    [[nodiscard]] Config::IConfigManagerUVE& GetConfigManagerUVE() const noexcept;
    [[nodiscard]] Scene::IEntityManagerUVE& GetEntityManagerUVE() const noexcept;
    [[nodiscard]] Scene::ISceneGraphUVE& GetSceneGraphUVE() const noexcept;
    [[nodiscard]] Asset::IAssetDatabaseUVE& GetAssetDatabaseUVE() const noexcept;
    [[nodiscard]] Asset::IProjectFileIndexUVE& GetProjectFileIndexUVE() const noexcept;
    [[nodiscard]] Asset::IDerivedArtifactCacheUVE& GetDerivedArtifactCacheUVE() const noexcept;
    [[nodiscard]] Asset::IProjectChangeWatcherUVE& GetProjectChangeWatcherUVE() const noexcept;
    [[nodiscard]] Scene::ISceneSerializerUVE& GetSceneSerializerUVE() const noexcept;
    [[nodiscard]] Scene::IPrefabSystemUVE& GetPrefabSystemUVE() const noexcept;
    [[nodiscard]] Scene::IParticleRuntimeUVE& GetParticleRuntimeUVE() const noexcept;
    [[nodiscard]] Asset::IHotReloadUVE& GetHotReloadUVE() const noexcept;
    [[nodiscard]] Asset::IAssetManagerUVE& GetAssetManagerUVE() const noexcept;
    [[nodiscard]] Asset::IAssetImporterUVE& GetAssetImporterUVE() const noexcept;
    [[nodiscard]] Asset::IAssetImportQueueUVE& GetAssetImportQueueUVE() const noexcept;
    [[nodiscard]] Asset::IAssetBundleUVE& GetAssetBundleUVE() const noexcept;
    [[nodiscard]] Asset::IFileSystemUVE& GetFileSystemUVE() const noexcept;
    [[nodiscard]] Render::IRenderDeviceUVE& GetRenderDeviceUVE() const noexcept;
    [[nodiscard]] Render::Shader::IShaderManagerUVE& GetShaderManagerUVE() const noexcept;
    [[nodiscard]] Render::IRenderSystemUVE& GetRenderSystemUVE() const noexcept;
    [[nodiscard]] Render::ICameraSystemUVE& GetCameraSystemUVE() const noexcept;
    [[nodiscard]] Render::IMeshRendererUVE& GetMeshRendererUVE() const noexcept;
    [[nodiscard]] Render::ILightSystemUVE& GetLightSystemUVE() const noexcept;
    [[nodiscard]] Render::IRenderer3DUVE& GetRenderer3DUVE() const noexcept;
    [[nodiscard]] Physics::ICollisionSystemUVE& GetCollisionSystemUVE() const noexcept;
    [[nodiscard]] Physics::IPhysicsSystemUVE& GetPhysicsSystemUVE() const noexcept;
    [[nodiscard]] Physics::IPhysicsQuerySystemUVE& GetPhysicsQuerySystemUVE() const noexcept;
    [[nodiscard]] Physics::IRaycastSystemUVE& GetRaycastSystemUVE() const noexcept;
    [[nodiscard]] Physics::PhysicsConstraintSystemUVE& GetPhysicsConstraintSystemUVE() const noexcept;
    [[nodiscard]] Input::IInputSystemUVE& GetInputSystemUVE() const noexcept;
    [[nodiscard]] Input::IGamepadInputSystemUVE& GetGamepadInputSystemUVE() const noexcept;
    [[nodiscard]] Input::IMobileInputSystemUVE& GetMobileInputSystemUVE() const noexcept;
    [[nodiscard]] Input::IMobileGestureSystemUVE& GetMobileGestureSystemUVE() const noexcept;
    [[nodiscard]] Audio::IAudioDeviceUVE& GetAudioDeviceUVE() const noexcept;
    [[nodiscard]] Audio::IAudioSystemUVE& GetAudioSystemUVE() const noexcept;
    [[nodiscard]] Audio::IAudioSourceSystemUVE& GetAudioSourceSystemUVE() const noexcept;
    [[nodiscard]] Save::ISaveGameSystemUVE& GetSaveGameSystemUVE() const noexcept;
    [[nodiscard]] Save::ICheckpointManagerUVE& GetCheckpointManagerUVE() const noexcept;
    [[nodiscard]] Window::IWindowManagerUVE& GetWindowManagerUVE() const noexcept;

private:
    Debug::ILoggerUVE* m_logger;
    Utilities::ITimerUVE* m_timer;
    Events::IEventSystemUVE* m_eventSystem;
    Memory::IMemoryManagerUVE* m_memoryManager;
    Threading::IThreadPoolUVE* m_threadPool;
    CommandLine::ICommandLineUVE* m_commandLine;
    Config::IConfigManagerUVE* m_configManager;
    Scene::IEntityManagerUVE* m_entityManager;
    Scene::ISceneGraphUVE* m_sceneGraph;
    Asset::IAssetDatabaseUVE* m_assetDatabase;
    Asset::IProjectFileIndexUVE* m_projectFileIndex;
    Asset::IDerivedArtifactCacheUVE* m_derivedArtifactCache;
    Asset::IProjectChangeWatcherUVE* m_projectChangeWatcher;
    Scene::ISceneSerializerUVE* m_sceneSerializer;
    Scene::IPrefabSystemUVE* m_prefabSystem;
    Scene::IParticleRuntimeUVE* m_particleRuntime;
    Asset::IHotReloadUVE* m_hotReload;
    Asset::IAssetManagerUVE* m_assetManager;
    Asset::IAssetImporterUVE* m_assetImporter;
    Asset::IAssetImportQueueUVE* m_assetImportQueue;
    Asset::IAssetBundleUVE* m_assetBundle;
    Asset::IFileSystemUVE* m_fileSystem;
    Render::IRenderDeviceUVE* m_renderDevice;
    Render::Shader::IShaderManagerUVE* m_shaderManager;
    Render::IRenderSystemUVE* m_renderSystem;
    Render::ICameraSystemUVE* m_cameraSystem;
    Render::IMeshRendererUVE* m_meshRenderer;
    Render::ILightSystemUVE* m_lightSystem;
    Render::IRenderer3DUVE* m_renderer3D;
    Physics::ICollisionSystemUVE* m_collisionSystem;
    Physics::IPhysicsSystemUVE* m_physicsSystem;
    Physics::IPhysicsQuerySystemUVE* m_physicsQuerySystem;
    Physics::IRaycastSystemUVE* m_raycastSystem;
    Physics::PhysicsConstraintSystemUVE* m_physicsConstraintSystem;
    Input::IInputSystemUVE* m_inputSystem;
    Input::IGamepadInputSystemUVE* m_gamepadInputSystem;
    Input::IMobileInputSystemUVE* m_mobileInputSystem;
    Input::IMobileGestureSystemUVE* m_mobileGestureSystem;
    Audio::IAudioDeviceUVE* m_audioDevice;
    Audio::IAudioSystemUVE* m_audioSystem;
    Audio::IAudioSourceSystemUVE* m_audioSourceSystem;
    Save::ISaveGameSystemUVE* m_saveGameSystem;
    Save::ICheckpointManagerUVE* m_checkpointManager;
    Window::IWindowManagerUVE* m_windowManager;
};

} // namespace UVE::Core
