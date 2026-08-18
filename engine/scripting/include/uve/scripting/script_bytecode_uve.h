// Copyright (c) 2026 UniVex Studios. All Rights Reserved.
#pragma once

#include "uve/scripting/script_compiler_ir_uve.h"

#include <cstddef>
#include <cstdint>
#include <optional>
#include <string>
#include <vector>

namespace UVE::Scripting {

enum class ScriptBytecodeDiagnosticCodeUVE : std::uint8_t {
    InvalidMagic = 0,
    UnsupportedVersion,
    Truncated,
    InstructionLimitExceeded,
    InvalidInstruction,
};

struct ScriptBytecodeDiagnosticUVE final {
    ScriptBytecodeDiagnosticCodeUVE code = ScriptBytecodeDiagnosticCodeUVE::InvalidMagic;
    std::size_t offset = 0U;
    std::string message;
};

struct ScriptBytecodeProgramUVE final {
    static constexpr std::uint32_t kLegacyVersionUVE = 1U;
    static constexpr std::uint32_t kConditionalJumpVersionUVE = 2U;
    static constexpr std::uint32_t kSequenceDispatchVersionUVE = 3U;
    static constexpr std::uint32_t kStagedTransferVersionUVE = 4U;
    static constexpr std::uint32_t kCurrentVersionUVE = kStagedTransferVersionUVE;
    static constexpr std::size_t kMaximumInstructionsUVE = 4096U;

    std::uint32_t version = kCurrentVersionUVE;
    std::vector<ScriptIrInstructionUVE> instructions;
};

struct ScriptBytecodeDecodeResultUVE final {
    std::optional<ScriptBytecodeProgramUVE> program;
    std::vector<ScriptBytecodeDiagnosticUVE> diagnostics;

    [[nodiscard]] bool IsSuccessUVE() const noexcept {
        return program.has_value() && diagnostics.empty();
    }
};

[[nodiscard]] std::optional<ScriptBytecodeProgramUVE> LowerIrToBytecodeUVE(
    const ScriptIrProgramUVE& ir,
    std::vector<ScriptBytecodeDiagnosticUVE>& diagnostics);

[[nodiscard]] std::vector<std::uint8_t> EncodeScriptBytecodeUVE(
    const ScriptBytecodeProgramUVE& program,
    std::vector<ScriptBytecodeDiagnosticUVE>& diagnostics);

[[nodiscard]] ScriptBytecodeDecodeResultUVE DecodeScriptBytecodeUVE(
    const std::vector<std::uint8_t>& bytes);

} // namespace UVE::Scripting
