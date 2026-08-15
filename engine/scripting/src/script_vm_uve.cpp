// Copyright (c) 2026 UniVex Studios. All Rights Reserved.
#include "uve/scripting/script_vm_uve.h"

#include <utility>

namespace UVE::Scripting {

ScriptVmExecutionResultUVE ExecuteScriptBytecodeUVE(const ScriptBytecodeProgramUVE& program,
                                                     ScriptVmExecutionOptionsUVE options) {
    ScriptVmExecutionResultUVE result;
    if (program.version != ScriptBytecodeProgramUVE::kCurrentVersionUVE) {
        result.status = ScriptVmStatusUVE::InvalidInstruction;
        result.diagnostics.push_back({0U, "Unsupported bytecode version."});
        return result;
    }
    if (options.instructionBudget > ScriptBytecodeProgramUVE::kMaximumInstructionsUVE) {
        options.instructionBudget = ScriptBytecodeProgramUVE::kMaximumInstructionsUVE;
    }
    for (std::size_t index = 0U; index < program.instructions.size(); ++index) {
        if (result.instructionsExecuted >= options.instructionBudget) {
            result.status = ScriptVmStatusUVE::InstructionBudgetExceeded;
            result.diagnostics.push_back({index, "Instruction budget exceeded."});
            return result;
        }
        const ScriptIrInstructionKindUVE kind = program.instructions[index].kind;
        if (kind != ScriptIrInstructionKindUVE::ExecuteNode &&
            kind != ScriptIrInstructionKindUVE::TransferValue) {
            result.status = ScriptVmStatusUVE::InvalidInstruction;
            result.diagnostics.push_back({index, "Invalid instruction kind."});
            return result;
        }
        ++result.instructionsExecuted;
    }
    return result;
}

} // namespace UVE::Scripting
