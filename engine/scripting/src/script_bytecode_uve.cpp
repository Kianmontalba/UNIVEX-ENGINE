// Copyright (c) 2026 UniVex Studios. All Rights Reserved.
#include "uve/scripting/script_bytecode_uve.h"

#include <cstring>
#include <utility>

namespace UVE::Scripting {
namespace {
constexpr std::uint8_t kMagic[] = {'U', 'V', 'E', 'S'};
constexpr std::size_t kHeaderSize = 8U;
constexpr std::size_t kInstructionSize = 30U;

void WriteU32(std::vector<std::uint8_t>& bytes, const std::uint32_t value) {
    for (std::size_t index = 0U; index < 4U; ++index) {
        bytes.push_back(static_cast<std::uint8_t>((value >> (index * 8U)) & 0xFFU));
    }
}

bool ReadU32(const std::vector<std::uint8_t>& bytes, std::size_t& offset, std::uint32_t& value) {
    if (offset + 4U > bytes.size()) {
        return false;
    }
    value = 0U;
    for (std::size_t index = 0U; index < 4U; ++index) {
        value |= static_cast<std::uint32_t>(bytes[offset + index]) << (index * 8U);
    }
    offset += 4U;
    return true;
}

void AddDiagnostic(std::vector<ScriptBytecodeDiagnosticUVE>& diagnostics,
                   const ScriptBytecodeDiagnosticCodeUVE code,
                   const std::size_t offset,
                   std::string message) {
    diagnostics.push_back({code, offset, std::move(message)});
}

} // namespace

std::optional<ScriptBytecodeProgramUVE> LowerIrToBytecodeUVE(
    const ScriptIrProgramUVE& ir,
    std::vector<ScriptBytecodeDiagnosticUVE>& diagnostics) {
    if (ir.version != ScriptIrProgramUVE::kCurrentVersionUVE) {
        AddDiagnostic(diagnostics, ScriptBytecodeDiagnosticCodeUVE::UnsupportedVersion, 0U,
                      "Unsupported IR version.");
        return std::nullopt;
    }
    if (ir.instructions.size() > ScriptIrProgramUVE::kMaximumInstructionsUVE) {
        AddDiagnostic(diagnostics, ScriptBytecodeDiagnosticCodeUVE::InstructionLimitExceeded, 0U,
                      "IR instruction count exceeds the maximum of " +
                          std::to_string(ScriptIrProgramUVE::kMaximumInstructionsUVE) + ".");
        return std::nullopt;
    }
    ScriptBytecodeProgramUVE program;
    program.instructions = ir.instructions;
    return program;
}

std::vector<std::uint8_t> EncodeScriptBytecodeUVE(
    const ScriptBytecodeProgramUVE& program,
    std::vector<ScriptBytecodeDiagnosticUVE>& diagnostics) {
    if (program.version != ScriptBytecodeProgramUVE::kCurrentVersionUVE) {
        AddDiagnostic(diagnostics, ScriptBytecodeDiagnosticCodeUVE::UnsupportedVersion, 4U,
                      "Unsupported bytecode version.");
        return {};
    }
    if (program.instructions.size() > ScriptBytecodeProgramUVE::kMaximumInstructionsUVE) {
        AddDiagnostic(diagnostics, ScriptBytecodeDiagnosticCodeUVE::InstructionLimitExceeded, 8U,
                      "Bytecode instruction limit exceeded.");
        return {};
    }
    std::vector<std::uint8_t> bytes;
    bytes.reserve(kHeaderSize + 4U + program.instructions.size() * kInstructionSize);
    bytes.insert(bytes.end(), std::begin(kMagic), std::end(kMagic));
    WriteU32(bytes, program.version);
    WriteU32(bytes, static_cast<std::uint32_t>(program.instructions.size()));
    for (const ScriptIrInstructionUVE& instruction : program.instructions) {
        bytes.push_back(static_cast<std::uint8_t>(instruction.kind));
        WriteU32(bytes, instruction.sourceNodeId);
        WriteU32(bytes, instruction.targetNodeId);
        if (instruction.kind == ScriptIrInstructionKindUVE::ConditionalJump) {
            if (instruction.trueTargetInstructionIndex > program.instructions.size() ||
                instruction.falseTargetInstructionIndex > program.instructions.size()) {
                AddDiagnostic(diagnostics, ScriptBytecodeDiagnosticCodeUVE::InvalidInstruction, bytes.size(),
                              "ConditionalJump target is outside the bytecode instruction range.");
                return {};
            }
            WriteU32(bytes, instruction.trueTargetInstructionIndex);
            WriteU32(bytes, instruction.falseTargetInstructionIndex);
        } else if (instruction.kind == ScriptIrInstructionKindUVE::SequenceDispatch) {
            if (instruction.firstTargetInstructionIndex > program.instructions.size() ||
                instruction.secondTargetInstructionIndex > program.instructions.size()) {
                AddDiagnostic(diagnostics, ScriptBytecodeDiagnosticCodeUVE::InvalidInstruction, bytes.size(),
                              "SequenceDispatch target is outside the bytecode instruction range.");
                return {};
            }
            WriteU32(bytes, instruction.firstTargetInstructionIndex);
            WriteU32(bytes, instruction.secondTargetInstructionIndex);
        }
        bytes.push_back(static_cast<std::uint8_t>(instruction.nodeTypeId.size()));
        bytes.push_back(static_cast<std::uint8_t>(instruction.sourcePinName.size()));
        bytes.push_back(static_cast<std::uint8_t>(instruction.targetPinName.size()));
        bytes.insert(bytes.end(), instruction.nodeTypeId.begin(), instruction.nodeTypeId.end());
        bytes.insert(bytes.end(), instruction.sourcePinName.begin(), instruction.sourcePinName.end());
        bytes.insert(bytes.end(), instruction.targetPinName.begin(), instruction.targetPinName.end());
        if (program.version >= ScriptBytecodeProgramUVE::kStagedTransferVersionUVE) {
            bytes.push_back(instruction.isStagedTransfer ? 1U : 0U);
        }
    }
    return bytes;
}

ScriptBytecodeDecodeResultUVE DecodeScriptBytecodeUVE(const std::vector<std::uint8_t>& bytes) {
    ScriptBytecodeDecodeResultUVE result;
    if (bytes.size() < sizeof(kMagic) || std::memcmp(bytes.data(), kMagic, sizeof(kMagic)) != 0) {
        AddDiagnostic(result.diagnostics, ScriptBytecodeDiagnosticCodeUVE::InvalidMagic, 0U,
                      "Invalid bytecode magic.");
        return result;
    }
    if (bytes.size() < kHeaderSize + 4U) {
        AddDiagnostic(result.diagnostics, ScriptBytecodeDiagnosticCodeUVE::Truncated, bytes.size(),
                      "Truncated bytecode header.");
        return result;
    }
    std::size_t offset = 4U;
    std::uint32_t version = 0U;
    std::uint32_t count = 0U;
    if (!ReadU32(bytes, offset, version) || !ReadU32(bytes, offset, count)) {
        AddDiagnostic(result.diagnostics, ScriptBytecodeDiagnosticCodeUVE::Truncated, offset,
                      "Truncated bytecode header.");
        return result;
    }
    if (version != ScriptBytecodeProgramUVE::kLegacyVersionUVE &&
        version != ScriptBytecodeProgramUVE::kConditionalJumpVersionUVE &&
        version != ScriptBytecodeProgramUVE::kSequenceDispatchVersionUVE &&
        version != ScriptBytecodeProgramUVE::kStagedTransferVersionUVE) {
        AddDiagnostic(result.diagnostics, ScriptBytecodeDiagnosticCodeUVE::UnsupportedVersion, 4U,
                      "Unsupported bytecode version.");
        return result;
    }
    if (count > ScriptBytecodeProgramUVE::kMaximumInstructionsUVE) {
        AddDiagnostic(result.diagnostics, ScriptBytecodeDiagnosticCodeUVE::InstructionLimitExceeded, 8U,
                      "Bytecode instruction limit exceeded.");
        return result;
    }
    ScriptBytecodeProgramUVE program;
    program.version = version;
    program.instructions.reserve(count);
    for (std::uint32_t index = 0U; index < count; ++index) {
        if (offset + 13U > bytes.size()) {
            AddDiagnostic(result.diagnostics, ScriptBytecodeDiagnosticCodeUVE::Truncated, offset,
                          "Truncated bytecode instruction.");
            return result;
        }
        const auto kind = static_cast<ScriptIrInstructionKindUVE>(bytes[offset++]);
        ScriptIrInstructionUVE instruction;
        instruction.kind = kind;
        if (kind == ScriptIrInstructionKindUVE::ConditionalJump &&
            version == ScriptBytecodeProgramUVE::kLegacyVersionUVE) {
            AddDiagnostic(result.diagnostics, ScriptBytecodeDiagnosticCodeUVE::InvalidInstruction, offset - 1U,
                          "ConditionalJump requires bytecode version 2 or newer.");
            return result;
        }
        if (kind == ScriptIrInstructionKindUVE::SequenceDispatch &&
            version < ScriptBytecodeProgramUVE::kSequenceDispatchVersionUVE) {
            AddDiagnostic(result.diagnostics, ScriptBytecodeDiagnosticCodeUVE::InvalidInstruction, offset - 1U,
                          "SequenceDispatch requires bytecode version 3.");
            return result;
        }
        if (!ReadU32(bytes, offset, instruction.sourceNodeId) ||
            !ReadU32(bytes, offset, instruction.targetNodeId)) {
            AddDiagnostic(result.diagnostics, ScriptBytecodeDiagnosticCodeUVE::Truncated, offset,
                          "Truncated bytecode instruction identifiers.");
            return result;
        }
        if (kind == ScriptIrInstructionKindUVE::ConditionalJump &&
            (!ReadU32(bytes, offset, instruction.trueTargetInstructionIndex) ||
             !ReadU32(bytes, offset, instruction.falseTargetInstructionIndex))) {
            AddDiagnostic(result.diagnostics, ScriptBytecodeDiagnosticCodeUVE::Truncated, offset,
                          "Truncated ConditionalJump targets.");
            return result;
        }
        if (kind == ScriptIrInstructionKindUVE::SequenceDispatch &&
            (!ReadU32(bytes, offset, instruction.firstTargetInstructionIndex) ||
             !ReadU32(bytes, offset, instruction.secondTargetInstructionIndex))) {
            AddDiagnostic(result.diagnostics, ScriptBytecodeDiagnosticCodeUVE::Truncated, offset,
                          "Truncated SequenceDispatch targets.");
            return result;
        }
        const std::size_t typeLength = bytes[offset++];
        const std::size_t sourceLength = bytes[offset++];
        const std::size_t targetLength = bytes[offset++];
        const std::size_t totalLength = typeLength + sourceLength + targetLength;
        if (offset + totalLength > bytes.size()) {
            AddDiagnostic(result.diagnostics, ScriptBytecodeDiagnosticCodeUVE::Truncated, offset,
                          "Truncated bytecode instruction strings.");
            return result;
        }
        instruction.nodeTypeId.assign(reinterpret_cast<const char*>(bytes.data() + offset), typeLength);
        offset += typeLength;
        instruction.sourcePinName.assign(reinterpret_cast<const char*>(bytes.data() + offset), sourceLength);
        offset += sourceLength;
        instruction.targetPinName.assign(reinterpret_cast<const char*>(bytes.data() + offset), targetLength);
        offset += targetLength;
        if (version >= ScriptBytecodeProgramUVE::kStagedTransferVersionUVE) {
            if (offset >= bytes.size()) {
                AddDiagnostic(result.diagnostics, ScriptBytecodeDiagnosticCodeUVE::Truncated, offset,
                              "Truncated staged-transfer metadata.");
                return result;
            }
            const std::uint8_t stagedTransfer = bytes[offset++];
            if (stagedTransfer > 1U) {
                AddDiagnostic(result.diagnostics, ScriptBytecodeDiagnosticCodeUVE::InvalidInstruction, offset - 1U,
                              "Invalid staged-transfer metadata.");
                return result;
            }
            instruction.isStagedTransfer = stagedTransfer != 0U;
        }
        if (kind != ScriptIrInstructionKindUVE::ExecuteNode &&
            kind != ScriptIrInstructionKindUVE::TransferValue &&
            kind != ScriptIrInstructionKindUVE::ConditionalJump &&
            kind != ScriptIrInstructionKindUVE::SequenceDispatch) {
            AddDiagnostic(result.diagnostics, ScriptBytecodeDiagnosticCodeUVE::InvalidInstruction, offset,
                          "Unknown bytecode instruction kind.");
            return result;
        }
        if (kind == ScriptIrInstructionKindUVE::ConditionalJump &&
            (instruction.trueTargetInstructionIndex > count || instruction.falseTargetInstructionIndex > count)) {
            AddDiagnostic(result.diagnostics, ScriptBytecodeDiagnosticCodeUVE::InvalidInstruction, offset,
                          "ConditionalJump target is outside the bytecode instruction range.");
            return result;
        }
        if (kind == ScriptIrInstructionKindUVE::SequenceDispatch &&
            (instruction.firstTargetInstructionIndex > count || instruction.secondTargetInstructionIndex > count)) {
            AddDiagnostic(result.diagnostics, ScriptBytecodeDiagnosticCodeUVE::InvalidInstruction, offset,
                          "SequenceDispatch target is outside the bytecode instruction range.");
            return result;
        }
        program.instructions.push_back(std::move(instruction));
    }
    if (offset != bytes.size()) {
        AddDiagnostic(result.diagnostics, ScriptBytecodeDiagnosticCodeUVE::InvalidInstruction, offset,
                      "Trailing bytecode bytes are not allowed.");
        return result;
    }
    result.program = std::move(program);
    return result;
}

} // namespace UVE::Scripting
