//------------------------------------------------------------------------------
// UniVex Engine (UVE) — Proprietary Game Engine
// Copyright (c) 2026 UniVex Studios. All Rights Reserved.
// Unauthorized copying, modification, distribution, or use of this code
// in whole or in part is strictly prohibited without express written
// permission from UniVex Studios.
// Violators will be prosecuted to the fullest extent of the law.
//------------------------------------------------------------------------------

#include "uve/core/engine_services_uve.h"

namespace UVE::Core {

EngineServicesUVE::EngineServicesUVE(Debug::ILoggerUVE& logger, Utilities::ITimerUVE& timer,
                                      Events::IEventSystemUVE& eventSystem,
                                      Memory::IMemoryManagerUVE& memoryManager,
                                      Threading::IThreadPoolUVE& threadPool,
                                      CommandLine::ICommandLineUVE& commandLine,
                                      Config::IConfigManagerUVE& configManager,
                                      Scene::IEntityManagerUVE& entityManager,
                                      Scene::ISceneGraphUVE& sceneGraph,
                                      Asset::IAssetDatabaseUVE& assetDatabase,
                                      Scene::ISceneSerializerUVE& sceneSerializer,
                                      Scene::IPrefabSystemUVE& prefabSystem,
                                      Asset::IHotReloadUVE& hotReload,
                                      Asset::IAssetManagerUVE& assetManager,
                                      Asset::IAssetImporterUVE& assetImporter,
                                      Asset::IAssetBundleUVE& assetBundle,
                                      Asset::IFileSystemUVE& fileSystem,
                                      Render::IRenderDeviceUVE& renderDevice,
                                      Render::IRenderSystemUVE& renderSystem,
                                      Render::ICameraSystemUVE& cameraSystem,
                                      Render::IMeshRendererUVE& meshRenderer,
                                      Render::IRenderer3DUVE& renderer3D,
                                      Physics::ICollisionSystemUVE& collisionSystem,
                                      Physics::IPhysicsSystemUVE& physicsSystem) noexcept
    : m_logger(&logger), m_timer(&timer), m_eventSystem(&eventSystem),
      m_memoryManager(&memoryManager), m_threadPool(&threadPool), m_commandLine(&commandLine),
      m_configManager(&configManager), m_entityManager(&entityManager), m_sceneGraph(&sceneGraph),
      m_assetDatabase(&assetDatabase), m_sceneSerializer(&sceneSerializer),
      m_prefabSystem(&prefabSystem), m_hotReload(&hotReload), m_assetManager(&assetManager),
      m_assetImporter(&assetImporter), m_assetBundle(&assetBundle), m_fileSystem(&fileSystem),
      m_renderDevice(&renderDevice), m_renderSystem(&renderSystem), m_cameraSystem(&cameraSystem),
      m_meshRenderer(&meshRenderer), m_renderer3D(&renderer3D), m_collisionSystem(&collisionSystem),
      m_physicsSystem(&physicsSystem) {}

Debug::ILoggerUVE& EngineServicesUVE::GetLoggerUVE() const noexcept {
    return *m_logger;
}

Utilities::ITimerUVE& EngineServicesUVE::GetTimerUVE() const noexcept {
    return *m_timer;
}

Events::IEventSystemUVE& EngineServicesUVE::GetEventSystemUVE() const noexcept {
    return *m_eventSystem;
}

Memory::IMemoryManagerUVE& EngineServicesUVE::GetMemoryManagerUVE() const noexcept {
    return *m_memoryManager;
}

Threading::IThreadPoolUVE& EngineServicesUVE::GetThreadPoolUVE() const noexcept {
    return *m_threadPool;
}

CommandLine::ICommandLineUVE& EngineServicesUVE::GetCommandLineUVE() const noexcept {
    return *m_commandLine;
}

Config::IConfigManagerUVE& EngineServicesUVE::GetConfigManagerUVE() const noexcept {
    return *m_configManager;
}

Scene::IEntityManagerUVE& EngineServicesUVE::GetEntityManagerUVE() const noexcept {
    return *m_entityManager;
}

Scene::ISceneGraphUVE& EngineServicesUVE::GetSceneGraphUVE() const noexcept {
    return *m_sceneGraph;
}

Asset::IAssetDatabaseUVE& EngineServicesUVE::GetAssetDatabaseUVE() const noexcept {
    return *m_assetDatabase;
}

Scene::ISceneSerializerUVE& EngineServicesUVE::GetSceneSerializerUVE() const noexcept {
    return *m_sceneSerializer;
}

Scene::IPrefabSystemUVE& EngineServicesUVE::GetPrefabSystemUVE() const noexcept {
    return *m_prefabSystem;
}

Asset::IHotReloadUVE& EngineServicesUVE::GetHotReloadUVE() const noexcept {
    return *m_hotReload;
}

Asset::IAssetManagerUVE& EngineServicesUVE::GetAssetManagerUVE() const noexcept {
    return *m_assetManager;
}

Asset::IAssetImporterUVE& EngineServicesUVE::GetAssetImporterUVE() const noexcept {
    return *m_assetImporter;
}

Asset::IAssetBundleUVE& EngineServicesUVE::GetAssetBundleUVE() const noexcept {
    return *m_assetBundle;
}

Asset::IFileSystemUVE& EngineServicesUVE::GetFileSystemUVE() const noexcept {
    return *m_fileSystem;
}

Render::IRenderDeviceUVE& EngineServicesUVE::GetRenderDeviceUVE() const noexcept {
    return *m_renderDevice;
}

Render::IRenderSystemUVE& EngineServicesUVE::GetRenderSystemUVE() const noexcept {
    return *m_renderSystem;
}

Render::ICameraSystemUVE& EngineServicesUVE::GetCameraSystemUVE() const noexcept {
    return *m_cameraSystem;
}

Render::IMeshRendererUVE& EngineServicesUVE::GetMeshRendererUVE() const noexcept {
    return *m_meshRenderer;
}

Render::IRenderer3DUVE& EngineServicesUVE::GetRenderer3DUVE() const noexcept {
    return *m_renderer3D;
}

Physics::ICollisionSystemUVE& EngineServicesUVE::GetCollisionSystemUVE() const noexcept {
    return *m_collisionSystem;
}

Physics::IPhysicsSystemUVE& EngineServicesUVE::GetPhysicsSystemUVE() const noexcept {
    return *m_physicsSystem;
}

} // namespace UVE::Core
