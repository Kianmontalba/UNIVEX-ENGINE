// Copyright (c) 2026 UniVex Studios. All Rights Reserved.
#pragma once

#include "uve/scripting/script_bytecode_uve.h"

#include <cstddef>
#include <cstdint>
#include <optional>
#include <string>
#include <vector>

namespace UVE::Scripting {

enum class ScriptHotReloadCodeUVE : std::uint8_t {
    Accepted = 0,
    RejectedInvalidProgram,
    NoActiveProgram,
};

struct ScriptHotReloadResultUVE final {
    ScriptHotReloadCodeUVE code = ScriptHotReloadCodeUVE::RejectedInvalidProgram;
    std::uint64_t activeGeneration = 0U;
    bool lastKnownGoodRetained = false;
    bool compatibleStatePreserved = false;
    std::vector<ScriptBytecodeDiagnosticUVE> diagnostics;
    std::string message;

    [[nodiscard]] bool IsAcceptedUVE() const noexcept {
        return code == ScriptHotReloadCodeUVE::Accepted;
    }
};

struct ScriptHotReloadSnapshotUVE final {
    std::uint64_t activeGeneration = 0U;
    std::uint32_t activeVersion = 0U;
    std::size_t instructionCount = 0U;
    bool hasActiveProgram = false;
    bool lastReloadAccepted = false;
    bool compatibleStatePreserved = false;
    std::string status;
};

class ScriptHotReloadManagerUVE final {
public:
    ScriptHotReloadManagerUVE() = default;
    ScriptHotReloadManagerUVE(const ScriptHotReloadManagerUVE&) = delete;
    ScriptHotReloadManagerUVE& operator=(const ScriptHotReloadManagerUVE&) = delete;

    [[nodiscard]] ScriptHotReloadResultUVE LoadInitialUVE(const std::vector<std::uint8_t>& bytes);
    [[nodiscard]] ScriptHotReloadResultUVE ReloadUVE(const std::vector<std::uint8_t>& bytes);
    [[nodiscard]] const ScriptBytecodeProgramUVE* GetActiveProgramUVE() const noexcept;
    [[nodiscard]] ScriptHotReloadSnapshotUVE GetSnapshotUVE() const;

private:
    [[nodiscard]] ScriptHotReloadResultUVE ApplyCandidateUVE(ScriptBytecodeProgramUVE candidate,
                                                              bool replacing);
    void RecordFailureUVE(const std::string& message,
                          const std::vector<ScriptBytecodeDiagnosticUVE>& diagnostics,
                          bool retained) noexcept;

    std::optional<ScriptBytecodeProgramUVE> m_activeProgram;
    std::uint64_t m_activeGeneration = 0U;
    bool m_lastReloadAccepted = false;
    bool m_compatibleStatePreserved = false;
    std::string m_status = "No active bytecode program.";
};

} // namespace UVE::Scripting
