//                                UniVex Engine
//
// UniVex Engine (UVE) — Proprietary Game Engine
// Copyright (c) 2026 UniVex Studios. All Rights Reserved.
// Unauthorized copying, modification, distribution, or use of this code
// in whole or in part is strictly prohibited without express written
// permission from UniVex Studios.
// Violators will be prosecuted to the fullest extent of the law.


#pragma once

namespace UVE::Core {

/// Controls whether EngineCoreUVE consumes normal fixed simulation time, holds it, or consumes one
/// explicitly requested fixed step. Core owns the execution decision; editor clients only request it.
enum class SimulationExecutionModeUVE {
    Running,
    Paused,
};

/// A narrow Core-owned control seam for transient editor simulation sessions. It deliberately does
/// not expose physics, timers, checkpoints, or renderer implementation details to editor code.
class ISimulationControlUVE {
public:
    virtual ~ISimulationControlUVE() = default;

    /// Sets fixed simulation execution. Paused mode retains frame maintenance and rendering while
    /// consuming no normal fixed physics steps.
    [[nodiscard]] virtual bool SetSimulationExecutionModeUVE(SimulationExecutionModeUVE mode) noexcept = 0;
    [[nodiscard]] virtual SimulationExecutionModeUVE GetSimulationExecutionModeUVE() const noexcept = 0;

    /// Queues exactly one fixed simulation step for the next engine update. Valid only while paused
    /// and while no prior request is pending.
    [[nodiscard]] virtual bool RequestSingleSimulationStepUVE() noexcept = 0;

    /// Marks an editor-owned transient simulation session. Core suppresses checkpoint/save-game
    /// advancement while this flag is active, independently from fixed execution mode.
    [[nodiscard]] virtual bool SetTransientSimulationSessionActiveUVE(bool active) noexcept = 0;
    [[nodiscard]] virtual bool IsTransientSimulationSessionActiveUVE() const noexcept = 0;
};

} // namespace UVE::Core
