// Copyright (c) 2026 UniVex Studios. All Rights Reserved.


#pragma once

#include <string>
#include <string_view>

namespace UVE::CommandLine {

/// ICommandLineUVE is the parsed-startup-arguments query interface. An
/// implementation parses a raw argument list once at construction time
/// (see CommandLineUVE) into flag/value pairs, then exposes them through
/// this read-only surface for the rest of the engine to query — e.g.
/// `--project <path>`, `--build <platform>`, and presence-only flags like
/// `--server` (the shapes the Hub launches the engine executable with).
/// Every accessor here is queried by the flag's *normalized* name (the
/// raw `--` prefix stripped) — e.g. HasFlagUVE("server") for a `--server`
/// argument — never the raw `--`-prefixed spelling.
/// Thread-safety: implementations are expected to be immutable after
/// construction, so every method here must be safe to call concurrently
/// from any thread with no external synchronization.
class ICommandLineUVE {
public:
    virtual ~ICommandLineUVE() = default;

    /// True iff a flag with this normalized name appeared anywhere in the
    /// parsed arguments, whether or not it captured a value.
    [[nodiscard]] virtual bool HasFlagUVE(std::string_view name) const noexcept = 0;

    /// Returns the value captured for the normalized flag `name` (the
    /// token immediately following it, if that token did not itself start
    /// with `--`), or `defaultValue` if the flag was absent or was
    /// presence-only (no following value).
    [[nodiscard]] virtual std::string GetValueUVE(std::string_view name,
                                                   std::string_view defaultValue) const = 0;
};

} // namespace UVE::CommandLine
