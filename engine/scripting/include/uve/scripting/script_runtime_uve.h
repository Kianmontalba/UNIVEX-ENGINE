// Copyright (c) 2026 UniVex Studios. All Rights Reserved.
#pragma once

#include "uve/scene/entity_uve.h"
#include "uve/scripting/script_vm_uve.h"

#include <cstddef>
#include <cstdint>
#include <optional>
#include <string>
#include <unordered_map>
#include <vector>

namespace UVE::Scripting {

struct ScriptRuntimeStateUVE final {
    std::vector<std::int64_t> values;

    [[nodiscard]] bool operator==(const ScriptRuntimeStateUVE&) const = default;
};

enum class ScriptRuntimeAttachCodeUVE : std::uint8_t {
    Accepted = 0,
    InvalidEntity,
    InvalidProgram,
    CapacityExceeded,
    DuplicateInstance,
};

struct ScriptRuntimeAttachResultUVE final {
    ScriptRuntimeAttachCodeUVE code = ScriptRuntimeAttachCodeUVE::InvalidProgram;
    std::vector<ScriptBytecodeDiagnosticUVE> diagnostics;
    std::string message;

    [[nodiscard]] bool IsAcceptedUVE() const noexcept {
        return code == ScriptRuntimeAttachCodeUVE::Accepted;
    }
};

enum class ScriptRuntimeDetachCodeUVE : std::uint8_t {
    Applied = 0,
    NoActiveInstance,
};

struct ScriptRuntimeDetachResultUVE final {
    ScriptRuntimeDetachCodeUVE code = ScriptRuntimeDetachCodeUVE::Applied;
    std::string message;

    [[nodiscard]] bool IsAcceptedUVE() const noexcept {
        return code == ScriptRuntimeDetachCodeUVE::Applied;
    }
};

enum class ScriptRuntimeStateUpdateCodeUVE : std::uint8_t {
    Applied = 0,
    Unchanged,
    NoActiveInstance,
    CapacityExceeded,
};

struct ScriptRuntimeStateUpdateResultUVE final {
    ScriptRuntimeStateUpdateCodeUVE code = ScriptRuntimeStateUpdateCodeUVE::Applied;
    std::string message;

    [[nodiscard]] bool IsAcceptedUVE() const noexcept {
        return code == ScriptRuntimeStateUpdateCodeUVE::Applied ||
               code == ScriptRuntimeStateUpdateCodeUVE::Unchanged;
    }
};

enum class ScriptRuntimeEnabledUpdateCodeUVE : std::uint8_t {
    Applied = 0,
    Unchanged,
    NoActiveInstance,
};

struct ScriptRuntimeEnabledUpdateResultUVE final {
    ScriptRuntimeEnabledUpdateCodeUVE code = ScriptRuntimeEnabledUpdateCodeUVE::Applied;
    std::string message;

    [[nodiscard]] bool IsAcceptedUVE() const noexcept {
        return code == ScriptRuntimeEnabledUpdateCodeUVE::Applied ||
               code == ScriptRuntimeEnabledUpdateCodeUVE::Unchanged;
    }
};

struct ScriptRuntimeInstanceUVE final {
    Scene::EntityUVE entity;
    ScriptBytecodeProgramUVE program;
    ScriptRuntimeStateUVE state;
    std::uint64_t generation = 1U;
    bool enabled = true;
};

struct ScriptRuntimeTickResultUVE final {
    Scene::EntityUVE entity;
    ScriptVmExecutionResultUVE execution;
};

enum class ScriptRuntimeReloadCodeUVE : std::uint8_t {
    Accepted = 0,
    RejectedInvalidProgram,
    NoActiveInstance,
};

struct ScriptRuntimeReloadResultUVE final {
    ScriptRuntimeReloadCodeUVE code = ScriptRuntimeReloadCodeUVE::RejectedInvalidProgram;
    std::uint64_t activeGeneration = 0U;
    bool lastKnownGoodRetained = false;
    bool compatibleStatePreserved = false;
    std::vector<ScriptBytecodeDiagnosticUVE> diagnostics;
    std::string message;

    [[nodiscard]] bool IsAcceptedUVE() const noexcept {
        return code == ScriptRuntimeReloadCodeUVE::Accepted;
    }
};

class ScriptRuntimeUVE final {
public:
    static constexpr std::size_t kMaximumInstancesUVE = 4096U;
    static constexpr std::size_t kMaximumStateValuesUVE = 256U;

    ScriptRuntimeUVE() = default;
    ScriptRuntimeUVE(const ScriptRuntimeUVE&) = delete;
    ScriptRuntimeUVE& operator=(const ScriptRuntimeUVE&) = delete;

    [[nodiscard]] ScriptRuntimeAttachResultUVE AttachDetailedUVE(Scene::EntityUVE entity,
                                                                  ScriptBytecodeProgramUVE program);
    [[nodiscard]] bool AttachUVE(Scene::EntityUVE entity, ScriptBytecodeProgramUVE program);
    [[nodiscard]] ScriptRuntimeReloadResultUVE ReloadUVE(Scene::EntityUVE entity,
                                                          ScriptBytecodeProgramUVE program);
    [[nodiscard]] ScriptRuntimeDetachResultUVE DetachDetailedUVE(Scene::EntityUVE entity) noexcept;
    [[nodiscard]] bool DetachUVE(Scene::EntityUVE entity) noexcept;
    [[nodiscard]] ScriptRuntimeEnabledUpdateResultUVE SetEnabledDetailedUVE(Scene::EntityUVE entity,
                                                                              bool enabled) noexcept;
    [[nodiscard]] bool SetEnabledUVE(Scene::EntityUVE entity, bool enabled) noexcept;
    [[nodiscard]] ScriptRuntimeStateUpdateResultUVE SetStateDetailedUVE(Scene::EntityUVE entity,
                                                                         ScriptRuntimeStateUVE state);
    [[nodiscard]] bool SetStateUVE(Scene::EntityUVE entity, ScriptRuntimeStateUVE state);
    [[nodiscard]] std::optional<ScriptRuntimeStateUVE> GetStateUVE(Scene::EntityUVE entity) const;
    [[nodiscard]] bool HasInstanceUVE(Scene::EntityUVE entity) const noexcept;
    [[nodiscard]] std::size_t GetInstanceCountUVE() const noexcept;
    [[nodiscard]] std::vector<ScriptRuntimeTickResultUVE> TickUVE(
        ScriptVmExecutionOptionsUVE options = {}) const;

private:
    std::unordered_map<Scene::EntityUVE, ScriptRuntimeInstanceUVE> m_instances;
};

} // namespace UVE::Scripting
