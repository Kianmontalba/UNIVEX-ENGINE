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
    bool outputTruncated = false;
    bool historyTruncated = false;
    bool cvarsTruncated = false;
    std::vector<DeveloperConsoleEntryUVE> output;
    std::vector<std::string> history;
    std::vector<DeveloperConsoleCVarUVE> cvars;
    [[nodiscard]] bool operator==(const DeveloperConsoleSnapshotUVE&) const = default;
};

class DeveloperConsoleUVE final {
public:
    static constexpr std::size_t kMaximumCommandsUVE = 64U;
    static constexpr std::size_t kMaximumHistoryUVE = 128U;
    static constexpr std::size_t kMaximumOutputUVE = 128U;
    static constexpr std::size_t kMaximumCVarsUVE = 128U;
    static constexpr std::size_t kMaximumIdentifierBytesUVE = 64U;
    static constexpr std::size_t kMaximumValueBytesUVE = 256U;

    DeveloperConsoleUVE();
    DeveloperConsoleUVE(const DeveloperConsoleUVE&) = delete;
    DeveloperConsoleUVE& operator=(const DeveloperConsoleUVE&) = delete;

    [[nodiscard]] bool RegisterCommand(std::string identifier, std::string help,
                                       DeveloperConsoleCommandHandlerUVE handler);
    [[nodiscard]] bool RegisterCVar(std::string name, std::string value, bool readOnly = false);
    [[nodiscard]] bool ExecuteUVE(std::string commandLine);
    [[nodiscard]] bool ClearUVE() noexcept;
    [[nodiscard]] bool SetCVarUVE(std::string_view name, std::string_view value);
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
    bool m_outputTruncated = false;
    bool m_historyTruncated = false;
    bool m_cvarsTruncated = false;
    std::uint64_t m_generation = 1U;
};

} // namespace UVE::Editor
