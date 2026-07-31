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
                                      Config::IConfigManagerUVE& configManager) noexcept
    : m_logger(&logger), m_timer(&timer), m_eventSystem(&eventSystem),
      m_memoryManager(&memoryManager), m_threadPool(&threadPool), m_commandLine(&commandLine),
      m_configManager(&configManager) {}

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

} // namespace UVE::Core
