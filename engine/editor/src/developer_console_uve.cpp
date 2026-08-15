// Copyright (c) 2026 UniVex Studios. All Rights Reserved.
#include "uve/editor/developer_console_uve.h"

#include <algorithm>
#include <cctype>
#include <limits>
#include <utility>

namespace UVE::Editor {
namespace {

void IncrementGenerationUVE(std::uint64_t& generation) noexcept {
    if (generation < std::numeric_limits<std::uint64_t>::max()) {
        ++generation;
    }
}

} // namespace

DeveloperConsoleUVE::DeveloperConsoleUVE() {
    RegisterBuiltInsUVE();
}

bool DeveloperConsoleUVE::IsBoundedIdentifierUVE(const std::string_view value) noexcept {
    if (value.empty() || value.size() > kMaximumIdentifierBytesUVE) {
        return false;
    }
    return std::all_of(value.begin(), value.end(), [](const char character) {
        return std::isalnum(static_cast<unsigned char>(character)) != 0 || character == '_' || character == '.' || character == '-';
    });
}

bool DeveloperConsoleUVE::IsBoundedValueUVE(const std::string_view value) noexcept {
    return value.size() <= kMaximumValueBytesUVE &&
           std::all_of(value.begin(), value.end(), [](const char character) { return character != '\n' && character != '\r'; });
}

std::string DeveloperConsoleUVE::TrimUVE(const std::string_view value) {
    std::size_t first = 0U;
    while (first < value.size() && std::isspace(static_cast<unsigned char>(value[first])) != 0) {
        ++first;
    }
    std::size_t last = value.size();
    while (last > first && std::isspace(static_cast<unsigned char>(value[last - 1U])) != 0) {
        --last;
    }
    return std::string(value.substr(first, last - first));
}

bool DeveloperConsoleUVE::RegisterCommand(std::string identifier, std::string help,
                                           DeveloperConsoleCommandHandlerUVE handler) {
    if (!IsBoundedIdentifierUVE(identifier) || !IsBoundedValueUVE(help) || !handler ||
        m_commands.size() >= kMaximumCommandsUVE || m_commands.contains(identifier)) {
        return false;
    }
    m_commands.emplace(std::move(identifier), CommandUVE{std::move(help), std::move(handler)});
    IncrementGenerationUVE(m_generation);
    return true;
}

bool DeveloperConsoleUVE::RegisterCVar(std::string name, std::string value, const bool readOnly) {
    if (!IsBoundedIdentifierUVE(name) || !IsBoundedValueUVE(value) || m_cvars.size() >= kMaximumCVarsUVE ||
        m_cvars.contains(name)) {
        return false;
    }
    const std::string key = name;
    m_cvars.emplace(key, DeveloperConsoleCVarUVE{key, std::move(value), readOnly});
    IncrementGenerationUVE(m_generation);
    return true;
}

void DeveloperConsoleUVE::RegisterBuiltInsUVE() {
    (void)RegisterCommand("help", "List registered native commands.", [](DeveloperConsoleUVE& console, std::string_view) {
        for (const auto& [identifier, command] : console.m_commands) {
            console.AppendUVE(DeveloperConsoleSeverityUVE::Info, identifier + " — " + command.help);
        }
    });
    (void)RegisterCommand("clear", "Clear console output while retaining command history.", [](DeveloperConsoleUVE& console, std::string_view) {
        (void)console.ClearUVE();
    });
    (void)RegisterCommand("cvar", "Read or set a registered cvar: cvar <name> [value].", [](DeveloperConsoleUVE& console, const std::string_view arguments) {
        (void)console.ExecuteCVarUVE(arguments);
    });
}

void DeveloperConsoleUVE::AddHistoryUVE(std::string value) {
    if (m_history.size() >= kMaximumHistoryUVE) {
        m_history.erase(m_history.begin());
        m_historyTruncated = true;
    }
    m_history.push_back(std::move(value));
}

bool DeveloperConsoleUVE::ExecuteUVE(std::string commandLine) {
    commandLine = TrimUVE(commandLine);
    if (commandLine.empty() || commandLine.size() > kMaximumValueBytesUVE ||
        commandLine.find_first_of("\r\n") != std::string::npos) {
        return false;
    }

    const std::size_t separator = commandLine.find_first_of(" \t");
    const std::string identifier = commandLine.substr(0U, separator);
    const auto iterator = m_commands.find(identifier);
    if (iterator == m_commands.end()) {
        AddHistoryUVE(commandLine);
        AppendUVE(DeveloperConsoleSeverityUVE::Error, "Unknown console command: " + identifier);
        return false;
    }

    const std::string arguments = separator == std::string::npos ? std::string{} : TrimUVE(commandLine.substr(separator));
    AddHistoryUVE(commandLine);
    iterator->second.handler(*this, arguments);
    IncrementGenerationUVE(m_generation);
    return true;
}

bool DeveloperConsoleUVE::ClearUVE() noexcept {
    if (m_output.empty()) {
        return false;
    }
    m_output.clear();
    IncrementGenerationUVE(m_generation);
    return true;
}

bool DeveloperConsoleUVE::SetCVarUVE(const std::string_view name, const std::string_view value) {
    const auto iterator = m_cvars.find(name);
    if (iterator == m_cvars.end() || iterator->second.readOnly || !IsBoundedValueUVE(value)) {
        return false;
    }
    if (iterator->second.value == value) {
        return true;
    }
    iterator->second.value = std::string(value);
    AppendUVE(DeveloperConsoleSeverityUVE::Info, std::string(name) + " = " + std::string(value));
    return true;
}

bool DeveloperConsoleUVE::ExecuteCVarUVE(const std::string_view arguments) {
    const std::string trimmed = TrimUVE(arguments);
    const std::size_t separator = trimmed.find_first_of(" \t");
    const std::string name = trimmed.substr(0U, separator);
    const auto iterator = m_cvars.find(name);
    if (iterator == m_cvars.end()) {
        AppendUVE(DeveloperConsoleSeverityUVE::Error, "Unknown cvar: " + name);
        return false;
    }
    if (separator == std::string::npos) {
        AppendUVE(DeveloperConsoleSeverityUVE::Info, iterator->second.name + " = " + iterator->second.value);
        return true;
    }
    return SetCVarUVE(name, TrimUVE(trimmed.substr(separator)));
}

void DeveloperConsoleUVE::AppendUVE(const DeveloperConsoleSeverityUVE severity, std::string text) {
    if (text.size() > kMaximumValueBytesUVE) {
        text.resize(kMaximumValueBytesUVE);
        m_outputTruncated = true;
    }
    if (m_output.size() >= kMaximumOutputUVE) {
        m_output.erase(m_output.begin());
        m_outputTruncated = true;
    }
    m_output.push_back(DeveloperConsoleEntryUVE{severity, std::move(text)});
    IncrementGenerationUVE(m_generation);
}

DeveloperConsoleSnapshotUVE DeveloperConsoleUVE::GetSnapshotUVE() const {
    DeveloperConsoleSnapshotUVE snapshot{};
    snapshot.generation = m_generation;
    snapshot.outputTruncated = m_outputTruncated;
    snapshot.historyTruncated = m_historyTruncated;
    snapshot.cvarsTruncated = m_cvarsTruncated;
    snapshot.output = m_output;
    snapshot.history = m_history;
    for (const auto& [name, cvar] : m_cvars) {
        snapshot.cvars.push_back(cvar);
    }
    return snapshot;
}

} // namespace UVE::Editor
