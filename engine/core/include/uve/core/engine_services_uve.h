//------------------------------------------------------------------------------
// UniVex Engine (UVE) — Proprietary Game Engine
// Copyright (c) 2026 UniVex Studios. All Rights Reserved.
// Unauthorized copying, modification, distribution, or use of this code
// in whole or in part is strictly prohibited without express written
// permission from UniVex Studios.
// Violators will be prosecuted to the fullest extent of the law.
//------------------------------------------------------------------------------

#pragma once

#include "uve/debug/i_logger_uve.h"
#include "uve/events/i_event_system_uve.h"
#include "uve/utilities/i_timer_uve.h"

namespace UVE::Core {

/// EngineServicesUVE is the engine's central dependency-provider / service
/// container: a small bundle of references to the core engine services
/// (Logger, Timer, EventSystem), built once EngineCoreUVE has constructed
/// all three. Any future subsystem that needs Logger/Timer/EventSystem
/// access should receive an EngineServicesUVE& (obtained from
/// EngineCoreUVE::GetServicesUVE()) rather than a raw global pointer — the
/// logging macros' internal active-instance pointer remains the one
/// intentional exception to that rule (see docs/CODING_STANDARDS.md).
/// Referenced through the ILoggerUVE/ITimerUVE/IEventSystemUVE interfaces,
/// not the concrete types, so a future substitute implementation of any of
/// the three requires no change here.
/// Thread-safety: EngineServicesUVE itself holds only non-owning pointers
/// and has no mutable state of its own; the thread-safety of each accessor
/// is whatever the referenced service documents.
class EngineServicesUVE final {
public:
    EngineServicesUVE(Debug::ILoggerUVE& logger, Utilities::ITimerUVE& timer,
                       Events::IEventSystemUVE& eventSystem) noexcept;

    [[nodiscard]] Debug::ILoggerUVE& GetLoggerUVE() const noexcept;
    [[nodiscard]] Utilities::ITimerUVE& GetTimerUVE() const noexcept;
    [[nodiscard]] Events::IEventSystemUVE& GetEventSystemUVE() const noexcept;

private:
    Debug::ILoggerUVE* m_logger;
    Utilities::ITimerUVE* m_timer;
    Events::IEventSystemUVE* m_eventSystem;
};

} // namespace UVE::Core
