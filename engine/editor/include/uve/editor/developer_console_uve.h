// Copyright (c) 2026 UniVex Studios. All Rights Reserved.
#pragma once

#include <cstddef>
#include <cstdint>
#include <functional>
#include <map>
#include <string>
#include <string_view>
#include <vector>

namespace UVE::Editor {

class DeveloperConsoleUVE;
using DeveloperConsoleCommandHandlerUVE = std::function<void(DeveloperConsoleUVE&, std::string_view)>;

enum class DeveloperConsoleSeverityUVE : std::uint8_t {
    Info = 0,
    Warning,
    Error,
};

enum class DeveloperConsoleSeverityFilterUVE : std::uint8_t {
    All = 0,
    Info,
    Warning,
    Error,
};

enum class DeveloperConsoleBuildPolicyUVE : std::uint8_t {
    Development = 0,
    Shipping,
};

enum class DeveloperConsoleCommandRegistrationCodeUVE : std::uint8_t {
    Accepted = 0,
    InvalidIdentifier,
    InvalidHelp,
    MissingHandler,
    CapacityExceeded,
    DuplicateIdentifier,
};

struct DeveloperConsoleCommandRegistrationResultUVE final {
    DeveloperConsoleCommandRegistrationCodeUVE code = DeveloperConsoleCommandRegistrationCodeUVE::Accepted;
    std::string message;

    [[nodiscard]] bool IsAcceptedUVE() const noexcept {
        return code == DeveloperConsoleCommandRegistrationCodeUVE::Accepted;
    }
};

enum class DeveloperConsoleCVarRegistrationCodeUVE : std::uint8_t {
    Accepted = 0,
    InvalidName,
    InvalidValue,
    CapacityExceeded,
    DuplicateName,
};

struct DeveloperConsoleCVarRegistrationResultUVE final {
    DeveloperConsoleCVarRegistrationCodeUVE code = DeveloperConsoleCVarRegistrationCodeUVE::Accepted;
    std::string message;

    [[nodiscard]] bool IsAcceptedUVE() const noexcept {
        return code == DeveloperConsoleCVarRegistrationCodeUVE::Accepted;
    }
};

enum class DeveloperConsoleCVarMutationCodeUVE : std::uint8_t {
    Applied = 0,
    Unchanged,
    Unavailable,
    UnknownName,
    ReadOnly,
    InvalidValue,
};

struct DeveloperConsoleCVarMutationResultUVE final {
    DeveloperConsoleCVarMutationCodeUVE code = DeveloperConsoleCVarMutationCodeUVE::Applied;
    std::string message;

    [[nodiscard]] bool IsAcceptedUVE() const noexcept {
        return code == DeveloperConsoleCVarMutationCodeUVE::Applied ||
               code == DeveloperConsoleCVarMutationCodeUVE::Unchanged;
    }
};

enum class DeveloperConsoleExecutionCodeUVE : std::uint8_t {
    Executed = 0,
    Unavailable,
    InvalidInput,
    UnknownCommand,
};

struct DeveloperConsoleExecutionResultUVE final {
    DeveloperConsoleExecutionCodeUVE code = DeveloperConsoleExecutionCodeUVE::Executed;
    std::string message;

    [[nodiscard]] bool IsAcceptedUVE() const noexcept {
        return code == DeveloperConsoleExecutionCodeUVE::Executed;
    }
};

struct DeveloperConsoleCompletionUVE final {
    std::string identifier;
    std::string help;
    [[nodiscard]] bool operator==(const DeveloperConsoleCompletionUVE&) const = default;
};

struct DeveloperConsoleEntryUVE final {
    DeveloperConsoleSeverityUVE severity = DeveloperConsoleSeverityUVE::Info;
    std::string text;
    [[nodiscard]] bool operator==(const DeveloperConsoleEntryUVE&) const = default;
};

struct DeveloperConsoleCVarUVE final {
    std::string name;
    std::string value;
    bool readOnly = false;
    [[nodiscard]] bool operator==(const DeveloperConsoleCVarUVE&) const = default;
};

struct DeveloperConsoleSnapshotUVE final {
    std::uint64_t generation = 0U;
    bool available = true;
    bool developmentOnly = true;
    DeveloperConsoleSeverityFilterUVE severityFilter = DeveloperConsoleSeverityFilterUVE::All;
    std::int32_t historyCursor = -1;
    std::string historyEntry;
    bool outputTruncated = false;
    bool historyTruncated = false;
    bool cvarsTruncated = false;
    bool completionTruncated = false;
    std::vector<DeveloperConsoleEntryUVE> output;
    std::vector<std::string> history;
    std::vector<DeveloperConsoleCVarUVE> cvars;
    std::vector<DeveloperConsoleCompletionUVE> completions;
    [[nodiscard]] bool operator==(const DeveloperConsoleSnapshotUVE&) const = default;
};

class DeveloperConsoleUVE final {
public:
    static constexpr std::size_t kMaximumCommandsUVE = 64U;
    static constexpr std::size_t kMaximumHistoryUVE = 128U;
    static constexpr std::size_t kMaximumOutputUVE = 128U;
    static constexpr std::size_t kMaximumCVarsUVE = 128U;
    static constexpr std::size_t kMaximumCompletionsUVE = 128U;
    static constexpr std::size_t kMaximumIdentifierBytesUVE = 64U;
    static constexpr std::size_t kMaximumValueBytesUVE = 256U;

    explicit DeveloperConsoleUVE(DeveloperConsoleBuildPolicyUVE policy = DeveloperConsoleBuildPolicyUVE::Development);
    DeveloperConsoleUVE(const DeveloperConsoleUVE&) = delete;
    DeveloperConsoleUVE& operator=(const DeveloperConsoleUVE&) = delete;

    [[nodiscard]] DeveloperConsoleCommandRegistrationResultUVE RegisterCommandUVE(
        std::string identifier, std::string help, DeveloperConsoleCommandHandlerUVE handler);
    [[nodiscard]] bool RegisterCommand(std::string identifier, std::string help,
                                       DeveloperConsoleCommandHandlerUVE handler);
    [[nodiscard]] DeveloperConsoleCVarRegistrationResultUVE RegisterCVarUVE(
        std::string name, std::string value, bool readOnly = false);
    [[nodiscard]] bool RegisterCVar(std::string name, std::string value, bool readOnly = false);
    [[nodiscard]] DeveloperConsoleExecutionResultUVE ExecuteDetailedUVE(std::string commandLine);
    [[nodiscard]] bool ExecuteUVE(std::string commandLine);
    [[nodiscard]] bool ClearUVE() noexcept;
    [[nodiscard]] DeveloperConsoleCVarMutationResultUVE SetCVarDetailedUVE(std::string_view name,
                                                                            std::string_view value);
    [[nodiscard]] bool SetCVarUVE(std::string_view name, std::string_view value);
    [[nodiscard]] bool SetBuildPolicyUVE(DeveloperConsoleBuildPolicyUVE policy) noexcept;
    [[nodiscard]] bool SetSeverityFilterUVE(DeveloperConsoleSeverityFilterUVE filter) noexcept;
    [[nodiscard]] bool SetCompletionPrefixUVE(std::string prefix);
    [[nodiscard]] bool MoveHistoryUVE(std::int32_t delta) noexcept;
    [[nodiscard]] bool IsAvailableUVE() const noexcept;
    [[nodiscard]] DeveloperConsoleSnapshotUVE GetSnapshotUVE() const;

    void AppendUVE(DeveloperConsoleSeverityUVE severity, std::string text);

private:
    struct CommandUVE final {
        std::string help;
        DeveloperConsoleCommandHandlerUVE handler;
    };

    [[nodiscard]] static bool IsBoundedIdentifierUVE(std::string_view value) noexcept;
    [[nodiscard]] static bool IsBoundedValueUVE(std::string_view value) noexcept;
    [[nodiscard]] static std::string TrimUVE(std::string_view value);
    void RegisterBuiltInsUVE();
    void AddHistoryUVE(std::string value);
    [[nodiscard]] bool ExecuteCVarUVE(std::string_view arguments);

    std::map<std::string, CommandUVE, std::less<>> m_commands;
    std::map<std::string, DeveloperConsoleCVarUVE, std::less<>> m_cvars;
    std::vector<std::string> m_history;
    std::vector<DeveloperConsoleEntryUVE> m_output;
    DeveloperConsoleBuildPolicyUVE m_policy = DeveloperConsoleBuildPolicyUVE::Development;
    DeveloperConsoleSeverityFilterUVE m_severityFilter = DeveloperConsoleSeverityFilterUVE::All;
    std::string m_completionPrefix;
    std::int32_t m_historyCursor = -1;
    bool m_completionTruncated = false;
    bool m_outputTruncated = false;
    bool m_historyTruncated = false;
    bool m_cvarsTruncated = false;
    std::uint64_t m_generation = 1U;
};

} // namespace UVE::Editor
