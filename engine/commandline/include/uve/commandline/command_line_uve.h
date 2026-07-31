//------------------------------------------------------------------------------
// UniVex Engine (UVE) — Proprietary Game Engine
// Copyright (c) 2026 UniVex Studios. All Rights Reserved.
// Unauthorized copying, modification, distribution, or use of this code
// in whole or in part is strictly prohibited without express written
// permission from UniVex Studios.
// Violators will be prosecuted to the fullest extent of the law.
//------------------------------------------------------------------------------

#pragma once

#include <optional>
#include <string>
#include <unordered_map>
#include <vector>

#include "uve/commandline/i_command_line_uve.h"

namespace UVE::CommandLine {

/// CommandLineUVE is the concrete, engine-standard implementation of
/// ICommandLineUVE. Parses a raw argument list, in a single left-to-right
/// pass, at construction time: a token starting with `--` is a flag name;
/// if the next token exists and does not itself start with `--`, it is
/// consumed as that flag's value (the `--project <path>` shape), otherwise
/// the flag is recorded as presence-only (the `--server` shape, including
/// when it is the last token or is immediately followed by another flag).
/// Bare tokens not preceded by a recognized `--flag` are ignored — this
/// increment supports only flag/value startup arguments, not positional
/// ones. Every stored flag name has its leading `--` stripped
/// (normalized), so HasFlagUVE()/GetValueUVE() are always queried with the
/// bare name.
/// Thread-safety: immutable after construction — parsing happens once in
/// the constructor and `m_flags` is never modified again, so every method
/// is safe to call concurrently from any thread with no locking.
class CommandLineUVE final : public ICommandLineUVE {
public:
    /// Parses `args` (each element a single argument token, without the
    /// program path) into normalized flag/value pairs.
    explicit CommandLineUVE(std::vector<std::string> args);

    /// Convenience overload matching main()'s signature: builds the
    /// argument vector from `argv[1]` through `argv[argc - 1]` (skipping
    /// the program path at `argv[0]`) and delegates to the vector
    /// constructor above.
    CommandLineUVE(int argc, char** argv);

    [[nodiscard]] bool HasFlagUVE(std::string_view name) const noexcept override;
    [[nodiscard]] std::string GetValueUVE(std::string_view name,
                                           std::string_view defaultValue) const override;

private:
    /// Parses `args` into `m_flags`, called once from both constructors.
    void ParseUVE(const std::vector<std::string>& args);

    std::unordered_map<std::string, std::optional<std::string>> m_flags;
};

} // namespace UVE::CommandLine
