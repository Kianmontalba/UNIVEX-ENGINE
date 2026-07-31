//------------------------------------------------------------------------------
// UniVex Engine (UVE) — Proprietary Game Engine
// Copyright (c) 2026 UniVex Studios. All Rights Reserved.
// Unauthorized copying, modification, distribution, or use of this code
// in whole or in part is strictly prohibited without express written
// permission from UniVex Studios.
// Violators will be prosecuted to the fullest extent of the law.
//------------------------------------------------------------------------------

#pragma once

#include "uve/asset/i_asset_bundle_uve.h"
#include "uve/asset/i_asset_database_uve.h"
#include "uve/asset/i_asset_importer_uve.h"
#include "uve/asset/i_asset_manager_uve.h"
#include "uve/asset/i_file_system_uve.h"
#include "uve/asset/i_hot_reload_uve.h"
#include "uve/commandline/i_command_line_uve.h"
#include "uve/config/i_config_manager_uve.h"
#include "uve/debug/i_logger_uve.h"
#include "uve/events/i_event_system_uve.h"
#include "uve/memory/i_memory_manager_uve.h"
#include "uve/scene/i_entity_manager_uve.h"
#include "uve/scene/i_prefab_system_uve.h"
#include "uve/scene/i_scene_graph_uve.h"
#include "uve/scene/i_scene_serializer_uve.h"
#include "uve/threading/i_thread_pool_uve.h"
#include "uve/utilities/i_timer_uve.h"

namespace UVE::Core {

/// EngineServicesUVE is the engine's central dependency-provider / service
/// container: a small bundle of references to the core engine services
/// (Logger, Timer, EventSystem, MemoryManager, ThreadPool, CommandLine,
/// ConfigManager, EntityManager, SceneGraph, AssetDatabase, SceneSerializer,
/// PrefabSystem, HotReload, AssetManager, AssetImporter, AssetBundle,
/// FileSystem), built once EngineCoreUVE has constructed all seventeen. Any
/// future subsystem that needs access to one of these should receive an
/// EngineServicesUVE& (obtained from EngineCoreUVE::GetServicesUVE()) rather
/// than a raw global pointer — the logging macros' internal active-instance
/// pointer remains the one intentional exception to that rule (see
/// docs/CODING_STANDARDS.md). Referenced through the
/// ILoggerUVE/ITimerUVE/IEventSystemUVE/IMemoryManagerUVE/IThreadPoolUVE/
/// ICommandLineUVE/IConfigManagerUVE/IEntityManagerUVE/ISceneGraphUVE/
/// IAssetDatabaseUVE/ISceneSerializerUVE/IPrefabSystemUVE/IHotReloadUVE/
/// IAssetManagerUVE/IAssetImporterUVE/IAssetBundleUVE/IFileSystemUVE
/// interfaces, not the concrete types, so a future substitute implementation
/// of any of the seventeen requires no change here.
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
                       Scene::ISceneSerializerUVE& sceneSerializer,
                       Scene::IPrefabSystemUVE& prefabSystem,
                       Asset::IHotReloadUVE& hotReload,
                       Asset::IAssetManagerUVE& assetManager,
                       Asset::IAssetImporterUVE& assetImporter,
                       Asset::IAssetBundleUVE& assetBundle,
                       Asset::IFileSystemUVE& fileSystem) noexcept;

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
    [[nodiscard]] Scene::ISceneSerializerUVE& GetSceneSerializerUVE() const noexcept;
    [[nodiscard]] Scene::IPrefabSystemUVE& GetPrefabSystemUVE() const noexcept;
    [[nodiscard]] Asset::IHotReloadUVE& GetHotReloadUVE() const noexcept;
    [[nodiscard]] Asset::IAssetManagerUVE& GetAssetManagerUVE() const noexcept;
    [[nodiscard]] Asset::IAssetImporterUVE& GetAssetImporterUVE() const noexcept;
    [[nodiscard]] Asset::IAssetBundleUVE& GetAssetBundleUVE() const noexcept;
    [[nodiscard]] Asset::IFileSystemUVE& GetFileSystemUVE() const noexcept;

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
    Scene::ISceneSerializerUVE* m_sceneSerializer;
    Scene::IPrefabSystemUVE* m_prefabSystem;
    Asset::IHotReloadUVE* m_hotReload;
    Asset::IAssetManagerUVE* m_assetManager;
    Asset::IAssetImporterUVE* m_assetImporter;
    Asset::IAssetBundleUVE* m_assetBundle;
    Asset::IFileSystemUVE* m_fileSystem;
};

} // namespace UVE::Core
