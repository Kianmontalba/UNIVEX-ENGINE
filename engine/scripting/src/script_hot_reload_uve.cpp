// Copyright (c) 2026 UniVex Studios. All Rights Reserved.
#include "uve/scripting/script_hot_reload_uve.h"

#include <utility>

namespace UVE::Scripting {

ScriptHotReloadResultUVE ScriptHotReloadManagerUVE::LoadInitialUVE(const std::vector<std::uint8_t>& bytes) {
    if (m_activeProgram.has_value()) {
        return ReloadUVE(bytes);
    }
    const ScriptBytecodeDecodeResultUVE decoded = DecodeScriptBytecodeUVE(bytes);
    if (!decoded.IsSuccessUVE()) {
        RecordFailureUVE("Initial bytecode load was rejected; no active program exists.", decoded.diagnostics, false);
        return {ScriptHotReloadCodeUVE::RejectedInvalidProgram, 0U, false, false,
                decoded.diagnostics, m_status};
    }
    return ApplyCandidateUVE(*decoded.program, false);
}

ScriptHotReloadResultUVE ScriptHotReloadManagerUVE::ReloadUVE(const std::vector<std::uint8_t>& bytes) {
    const ScriptBytecodeDecodeResultUVE decoded = DecodeScriptBytecodeUVE(bytes);
    if (!decoded.IsSuccessUVE()) {
        const bool retained = m_activeProgram.has_value();
        RecordFailureUVE("Hot reload rejected invalid bytecode; last-known-good program was retained.",
                         decoded.diagnostics, retained);
        return {retained ? ScriptHotReloadCodeUVE::RejectedInvalidProgram
                         : ScriptHotReloadCodeUVE::NoActiveProgram,
                m_activeGeneration, retained, false, decoded.diagnostics, m_status};
    }
    return ApplyCandidateUVE(*decoded.program, true);
}

const ScriptBytecodeProgramUVE* ScriptHotReloadManagerUVE::GetActiveProgramUVE() const noexcept {
    return m_activeProgram.has_value() ? &*m_activeProgram : nullptr;
}

ScriptHotReloadSnapshotUVE ScriptHotReloadManagerUVE::GetSnapshotUVE() const {
    const std::uint32_t version = m_activeProgram.has_value() ? m_activeProgram->version : 0U;
    const std::size_t instructionCount = m_activeProgram.has_value() ? m_activeProgram->instructions.size() : 0U;
    return {m_activeGeneration, version, instructionCount, m_activeProgram.has_value(),
            m_lastReloadAccepted, m_compatibleStatePreserved, m_status};
}

ScriptHotReloadResultUVE ScriptHotReloadManagerUVE::ApplyCandidateUVE(ScriptBytecodeProgramUVE candidate,
                                                                        const bool replacing) {
    const bool compatibleStatePreserved = replacing && m_activeProgram.has_value() &&
        m_activeProgram->version == candidate.version;
    m_activeProgram = std::move(candidate);
    ++m_activeGeneration;
    m_lastReloadAccepted = true;
    m_compatibleStatePreserved = compatibleStatePreserved;
    m_status = replacing ? "Bytecode hot reload accepted." : "Initial bytecode program accepted.";
    return {ScriptHotReloadCodeUVE::Accepted, m_activeGeneration, false, compatibleStatePreserved,
            {}, m_status};
}

void ScriptHotReloadManagerUVE::RecordFailureUVE(const std::string& message,
                                                  const std::vector<ScriptBytecodeDiagnosticUVE>&,
                                                  const bool retained) noexcept {
    m_lastReloadAccepted = false;
    m_compatibleStatePreserved = false;
    m_status = retained ? message : "No valid bytecode program is active.";
}

} // namespace UVE::Scripting
