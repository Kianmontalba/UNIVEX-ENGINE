// Copyright (c) 2026 UniVex Studios. All Rights Reserved.
#include "uve/editor/developer_console_uve.h"

#include <algorithm>
#include <cctype>
#include <limits>
#include <new>
#include <utility>

namespace UVE::Editor {
namespace {

void IncrementGenerationUVE(std::uint64_t& generation) noexcept {
    if (generation < std::numeric_limits<std::uint64_t>::max()) {
        ++generation;
    }
}

[[nodiscard]] bool MatchesSeverityUVE(const DeveloperConsoleSeverityUVE severity,
                                      const DeveloperConsoleSeverityFilterUVE filter) noexcept {
    switch (filter) {
        case DeveloperConsoleSeverityFilterUVE::All:
            return true;
        case DeveloperConsoleSeverityFilterUVE::Info:
            return severity == DeveloperConsoleSeverityUVE::Info;
        case DeveloperConsoleSeverityFilterUVE::Warning:
            return severity == DeveloperConsoleSeverityUVE::Warning;
        case DeveloperConsoleSeverityFilterUVE::Error:
            return severity == DeveloperConsoleSeverityUVE::Error;
    }
    return false;
}

} // namespace

DeveloperConsoleUVE::DeveloperConsoleUVE(const DeveloperConsoleBuildPolicyUVE policy)
    : m_policy(policy), m_access(policy == DeveloperConsoleBuildPolicyUVE::Shipping ? DeveloperConsoleAccessUVE::Denied
                                                                                     : DeveloperConsoleAccessUVE::Full) {
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

bool DeveloperConsoleUVE::IsBoundedPrincipalUVE(const std::string_view value) noexcept {
    return !value.empty() && value.size() <= kMaximumPrincipalBytesUVE &&
           std::all_of(value.begin(), value.end(), [](const char character) {
               return std::iscntrl(static_cast<unsigned char>(character)) == 0;
           });
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

bool DeveloperConsoleUVE::IsAvailableUVE() const noexcept {
    return m_policy == DeveloperConsoleBuildPolicyUVE::Development;
}

bool DeveloperConsoleUVE::SetAuditPrincipalUVE(DeveloperConsolePrincipalUVE principal) {
    if (!IsBoundedPrincipalUVE(principal.subject) || !IsBoundedPrincipalUVE(principal.session)) {
        return false;
    }
    try {
        m_auditPrincipal = std::move(principal);
        return true;
    } catch (const std::bad_alloc&) {
        return false;
    }
}

void DeveloperConsoleUVE::SetAuditSinkUVE(DeveloperConsoleAuditSinkUVE sink) noexcept {
    m_auditSink = std::move(sink);
}

DeveloperConsolePrincipalUVE DeveloperConsoleUVE::GetAuditPrincipalUVE() const {
    return m_auditPrincipal;
}

void DeveloperConsoleUVE::EmitAuditUVE(const DeveloperConsoleAuditActionUVE action,
                                       const std::string_view target,
                                       const std::string_view detail,
                                       const bool accepted) noexcept {
    if (!m_auditSink) {
        return;
    }
    try {
        const std::size_t boundedTargetSize = std::min(target.size(), kMaximumValueBytesUVE);
        const std::size_t boundedDetailSize = std::min(detail.size(), kMaximumValueBytesUVE);
        const std::uint64_t sequence = m_auditSequence < std::numeric_limits<std::uint64_t>::max()
                                           ? ++m_auditSequence
                                           : m_auditSequence;
        const DeveloperConsoleAuditRecordUVE record{
            sequence,
            action,
            m_auditPrincipal,
            std::string(target.substr(0U, boundedTargetSize)),
            std::string(detail.substr(0U, boundedDetailSize)),
            accepted};
        m_auditSink(record);
    } catch (...) {
        // Caller-owned auditing must never change console behavior.
    }
}

DeveloperConsoleCommandRegistrationResultUVE DeveloperConsoleUVE::RegisterCommandUVE(
    std::string identifier, std::string help, DeveloperConsoleCommandHandlerUVE handler) {
    if (!IsAvailableUVE()) {
        return {DeveloperConsoleCommandRegistrationCodeUVE::Unavailable,
                "Command registration is unavailable under the Shipping build policy."};
    }
    if (m_access != DeveloperConsoleAccessUVE::Full) {
        return {DeveloperConsoleCommandRegistrationCodeUVE::Unauthorized,
                "Command registration requires Full console authorization."};
    }
    if (!IsBoundedIdentifierUVE(identifier)) {
        return {DeveloperConsoleCommandRegistrationCodeUVE::InvalidIdentifier,
                "Command identifier is empty or contains unsupported characters."};
    }
    if (help.empty() || !IsBoundedValueUVE(help)) {
        return {DeveloperConsoleCommandRegistrationCodeUVE::InvalidHelp,
                "Command help must be non-empty, bounded, and free of line breaks."};
    }
    if (!handler) {
        return {DeveloperConsoleCommandRegistrationCodeUVE::MissingHandler,
                "Command registration requires a callable handler."};
    }
    if (m_commands.size() >= kMaximumCommandsUVE) {
        return {DeveloperConsoleCommandRegistrationCodeUVE::CapacityExceeded,
                "The bounded command registry has reached its maximum capacity."};
    }
    if (m_commands.contains(identifier)) {
        return {DeveloperConsoleCommandRegistrationCodeUVE::DuplicateIdentifier,
                "A command with this identifier is already registered."};
    }

    m_commands.emplace(std::move(identifier), CommandUVE{std::move(help), std::move(handler)});
    IncrementGenerationUVE(m_generation);
    return {DeveloperConsoleCommandRegistrationCodeUVE::Accepted, "Command registered."};
}

bool DeveloperConsoleUVE::RegisterCommand(std::string identifier, std::string help,
                                           DeveloperConsoleCommandHandlerUVE handler) {
    return RegisterCommandUVE(std::move(identifier), std::move(help), std::move(handler)).IsAcceptedUVE();
}

DeveloperConsoleCVarRegistrationResultUVE DeveloperConsoleUVE::RegisterCVarUVE(std::string name, std::string value,
                                                                                const bool readOnly) {
    if (!IsAvailableUVE()) {
        return {DeveloperConsoleCVarRegistrationCodeUVE::Unavailable,
                "CVAR registration is unavailable under the Shipping build policy."};
    }
    if (m_access != DeveloperConsoleAccessUVE::Full) {
        return {DeveloperConsoleCVarRegistrationCodeUVE::Unauthorized,
                "CVAR registration requires Full console authorization."};
    }
    if (!IsBoundedIdentifierUVE(name)) {
        return {DeveloperConsoleCVarRegistrationCodeUVE::InvalidName,
                "CVAR name is empty or contains unsupported characters."};
    }
    if (!IsBoundedValueUVE(value)) {
        return {DeveloperConsoleCVarRegistrationCodeUVE::InvalidValue,
                "CVAR value exceeds the bounded size or contains a line break."};
    }
    if (m_cvars.size() >= kMaximumCVarsUVE) {
        return {DeveloperConsoleCVarRegistrationCodeUVE::CapacityExceeded,
                "The bounded CVAR registry has reached its maximum capacity."};
    }
    if (m_cvars.contains(name)) {
        return {DeveloperConsoleCVarRegistrationCodeUVE::DuplicateName,
                "A CVAR with this name is already registered."};
    }

    const std::string key = name;
    m_cvars.emplace(key, DeveloperConsoleCVarUVE{key, std::move(value), readOnly});
    IncrementGenerationUVE(m_generation);
    return {DeveloperConsoleCVarRegistrationCodeUVE::Accepted, "CVAR registered."};
}

bool DeveloperConsoleUVE::RegisterCVar(std::string name, std::string value, const bool readOnly) {
    return RegisterCVarUVE(std::move(name), std::move(value), readOnly).IsAcceptedUVE();
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
    m_historyCursor = -1;
}

DeveloperConsoleExecutionResultUVE DeveloperConsoleUVE::ExecuteDetailedUVE(std::string commandLine) {
    if (!IsAvailableUVE()) {
        EmitAuditUVE(DeveloperConsoleAuditActionUVE::CommandExecution, "", "Shipping policy unavailable", false);
        return {DeveloperConsoleExecutionCodeUVE::Unavailable,
                "Developer Console execution is unavailable under the Shipping build policy."};
    }
    if (m_access != DeveloperConsoleAccessUVE::Full) {
        EmitAuditUVE(DeveloperConsoleAuditActionUVE::CommandExecution, "", "Full console authorization required", false);
        return {DeveloperConsoleExecutionCodeUVE::Unauthorized,
                "Developer Console execution requires Full console authorization."};
    }
    commandLine = TrimUVE(commandLine);
    if (commandLine.empty() || commandLine.size() > kMaximumValueBytesUVE ||
        commandLine.find_first_of("\r\n") != std::string::npos) {
        EmitAuditUVE(DeveloperConsoleAuditActionUVE::CommandExecution, commandLine, "Invalid command input", false);
        return {DeveloperConsoleExecutionCodeUVE::InvalidInput,
                "Command input must be non-empty, bounded, and free of line breaks."};
    }

    const std::size_t separator = commandLine.find_first_of(" \t");
    const std::string identifier = commandLine.substr(0U, separator);
    const auto iterator = m_commands.find(identifier);
    if (iterator == m_commands.end()) {
        AddHistoryUVE(commandLine);
        AppendUVE(DeveloperConsoleSeverityUVE::Error, "Unknown console command: " + identifier);
        EmitAuditUVE(DeveloperConsoleAuditActionUVE::CommandExecution, identifier, "Unknown command", false);
        return {DeveloperConsoleExecutionCodeUVE::UnknownCommand, "No command is registered with this identifier."};
    }

    const std::string arguments = separator == std::string::npos ? std::string{} : TrimUVE(commandLine.substr(separator));
    AddHistoryUVE(commandLine);
    iterator->second.handler(*this, arguments);
    IncrementGenerationUVE(m_generation);
    EmitAuditUVE(DeveloperConsoleAuditActionUVE::CommandExecution, identifier, "Command executed", true);
    return {DeveloperConsoleExecutionCodeUVE::Executed, "Command executed."};
}

bool DeveloperConsoleUVE::ExecuteUVE(std::string commandLine) {
    return ExecuteDetailedUVE(std::move(commandLine)).IsAcceptedUVE();
}

DeveloperConsoleClearResultUVE DeveloperConsoleUVE::ClearDetailedUVE() noexcept {
    if (!IsAvailableUVE()) {
        return {DeveloperConsoleClearCodeUVE::Unavailable,
                "Console clearing is unavailable under the Shipping build policy."};
    }
    if (m_access != DeveloperConsoleAccessUVE::Full) {
        return {DeveloperConsoleClearCodeUVE::Unauthorized,
                "Console clearing requires Full console authorization."};
    }
    if (m_output.empty()) {
        return {DeveloperConsoleClearCodeUVE::Unchanged, "Console output is already clear."};
    }
    m_output.clear();
    m_outputTruncated = false;
    IncrementGenerationUVE(m_generation);
    return {DeveloperConsoleClearCodeUVE::Applied, "Console output cleared."};
}

bool DeveloperConsoleUVE::ClearUVE() noexcept {
    return ClearDetailedUVE().IsAcceptedUVE();
}

DeveloperConsoleCVarMutationResultUVE DeveloperConsoleUVE::SetCVarDetailedUVE(const std::string_view name,
                                                                                const std::string_view value) {
    if (!IsAvailableUVE()) {
        EmitAuditUVE(DeveloperConsoleAuditActionUVE::CVarMutation, name, "Shipping policy unavailable", false);
        return {DeveloperConsoleCVarMutationCodeUVE::Unavailable,
                "CVAR mutation is unavailable under the Shipping build policy."};
    }
    if (m_access != DeveloperConsoleAccessUVE::Full) {
        EmitAuditUVE(DeveloperConsoleAuditActionUVE::CVarMutation, name, "Full console authorization required", false);
        return {DeveloperConsoleCVarMutationCodeUVE::Unauthorized,
                "CVAR mutation requires Full console authorization."};
    }
    const auto iterator = m_cvars.find(name);
    if (iterator == m_cvars.end()) {
        EmitAuditUVE(DeveloperConsoleAuditActionUVE::CVarMutation, name, "Unknown CVAR", false);
        return {DeveloperConsoleCVarMutationCodeUVE::UnknownName,
                "No CVAR is registered with this name."};
    }
    if (iterator->second.readOnly) {
        EmitAuditUVE(DeveloperConsoleAuditActionUVE::CVarMutation, name, "CVAR is read-only", false);
        return {DeveloperConsoleCVarMutationCodeUVE::ReadOnly,
                "The registered CVAR is read-only."};
    }
    if (!IsBoundedValueUVE(value)) {
        EmitAuditUVE(DeveloperConsoleAuditActionUVE::CVarMutation, name, "Invalid CVAR value", false);
        return {DeveloperConsoleCVarMutationCodeUVE::InvalidValue,
                "CVAR value exceeds the bounded size or contains a line break."};
    }
    if (iterator->second.value == value) {
        EmitAuditUVE(DeveloperConsoleAuditActionUVE::CVarMutation, name, "CVAR unchanged", true);
        return {DeveloperConsoleCVarMutationCodeUVE::Unchanged, "CVAR value is unchanged."};
    }
    iterator->second.value = std::string(value);
    AppendUVE(DeveloperConsoleSeverityUVE::Info, std::string(name) + " = " + std::string(value));
    EmitAuditUVE(DeveloperConsoleAuditActionUVE::CVarMutation, name, "CVAR value updated", true);
    return {DeveloperConsoleCVarMutationCodeUVE::Applied, "CVAR value updated."};
}

bool DeveloperConsoleUVE::SetCVarUVE(const std::string_view name, const std::string_view value) {
    return SetCVarDetailedUVE(name, value).IsAcceptedUVE();
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

DeveloperConsoleBuildPolicyResultUVE DeveloperConsoleUVE::SetBuildPolicyDetailedUVE(
    const DeveloperConsoleBuildPolicyUVE policy) noexcept {
    if (m_policy == policy) {
        EmitAuditUVE(DeveloperConsoleAuditActionUVE::BuildPolicyChange, "build_policy", "Build policy unchanged", true);
        return {DeveloperConsoleBuildPolicyCodeUVE::Unchanged, "Build policy is unchanged."};
    }
    if (m_policy == DeveloperConsoleBuildPolicyUVE::Shipping &&
        policy == DeveloperConsoleBuildPolicyUVE::Development) {
        EmitAuditUVE(DeveloperConsoleAuditActionUVE::BuildPolicyChange, "build_policy", "Shipping policy is monotonic", false);
        return {DeveloperConsoleBuildPolicyCodeUVE::Locked,
                "Shipping build policy is monotonic and cannot be relaxed at runtime."};
    }
    m_policy = policy;
    if (policy == DeveloperConsoleBuildPolicyUVE::Shipping) {
        m_access = DeveloperConsoleAccessUVE::Denied;
    }
    m_completionPrefix.clear();
    m_historyCursor = -1;
    IncrementGenerationUVE(m_generation);
    EmitAuditUVE(DeveloperConsoleAuditActionUVE::BuildPolicyChange, "build_policy", "Build policy updated", true);
    return {DeveloperConsoleBuildPolicyCodeUVE::Applied, "Build policy updated."};
}

bool DeveloperConsoleUVE::SetBuildPolicyUVE(const DeveloperConsoleBuildPolicyUVE policy) noexcept {
    return SetBuildPolicyDetailedUVE(policy).IsAcceptedUVE();
}

DeveloperConsoleAuthorizationResultUVE DeveloperConsoleUVE::SetAccessDetailedUVE(
    const DeveloperConsoleAccessUVE access) noexcept {
    if (!IsAvailableUVE()) {
        EmitAuditUVE(DeveloperConsoleAuditActionUVE::AccessChange, "console_access", "Shipping policy unavailable", false);
        return {DeveloperConsoleAuthorizationCodeUVE::Unavailable,
                "Console authorization is unavailable under the Shipping build policy."};
    }
    if (static_cast<std::uint8_t>(access) > static_cast<std::uint8_t>(DeveloperConsoleAccessUVE::Full)) {
        EmitAuditUVE(DeveloperConsoleAuditActionUVE::AccessChange, "console_access", "Invalid access value", false);
        return {DeveloperConsoleAuthorizationCodeUVE::InvalidAccess,
                "Console authorization value is outside the supported range."};
    }
    if (m_access == access) {
        EmitAuditUVE(DeveloperConsoleAuditActionUVE::AccessChange, "console_access", "Access unchanged", true);
        return {DeveloperConsoleAuthorizationCodeUVE::Unchanged, "Console authorization is unchanged."};
    }
    m_access = access;
    IncrementGenerationUVE(m_generation);
    EmitAuditUVE(DeveloperConsoleAuditActionUVE::AccessChange, "console_access", "Access updated", true);
    return {DeveloperConsoleAuthorizationCodeUVE::Applied, "Console authorization updated."};
}

bool DeveloperConsoleUVE::SetAccessUVE(const DeveloperConsoleAccessUVE access) noexcept {
    return SetAccessDetailedUVE(access).IsAcceptedUVE();
}

DeveloperConsoleSeverityFilterResultUVE DeveloperConsoleUVE::SetSeverityFilterDetailedUVE(
    const DeveloperConsoleSeverityFilterUVE filter) noexcept {
    if (!IsAvailableUVE()) {
        return {DeveloperConsoleSeverityFilterCodeUVE::Unavailable,
                "Severity filtering is unavailable under the Shipping build policy."};
    }
    if (static_cast<std::uint8_t>(filter) > static_cast<std::uint8_t>(DeveloperConsoleSeverityFilterUVE::Error)) {
        return {DeveloperConsoleSeverityFilterCodeUVE::InvalidFilter,
                "Severity filter value is outside the supported filter range."};
    }
    if (m_severityFilter == filter) {
        return {DeveloperConsoleSeverityFilterCodeUVE::Unchanged, "Severity filter is unchanged."};
    }
    m_severityFilter = filter;
    IncrementGenerationUVE(m_generation);
    return {DeveloperConsoleSeverityFilterCodeUVE::Applied, "Severity filter updated."};
}

bool DeveloperConsoleUVE::SetSeverityFilterUVE(const DeveloperConsoleSeverityFilterUVE filter) noexcept {
    return SetSeverityFilterDetailedUVE(filter).IsAcceptedUVE();
}

DeveloperConsoleCompletionPrefixResultUVE DeveloperConsoleUVE::SetCompletionPrefixDetailedUVE(std::string prefix) {
    if (!IsAvailableUVE()) {
        return {DeveloperConsoleCompletionPrefixCodeUVE::Unavailable,
                "Completion prefix filtering is unavailable under the Shipping build policy."};
    }
    if (!IsBoundedValueUVE(prefix)) {
        return {DeveloperConsoleCompletionPrefixCodeUVE::InvalidPrefix,
                "Completion prefix exceeds the bounded size or contains a line break."};
    }
    prefix = TrimUVE(prefix);
    if (m_completionPrefix == prefix) {
        return {DeveloperConsoleCompletionPrefixCodeUVE::Unchanged, "Completion prefix is unchanged."};
    }
    m_completionPrefix = std::move(prefix);
    IncrementGenerationUVE(m_generation);
    return {DeveloperConsoleCompletionPrefixCodeUVE::Applied, "Completion prefix updated."};
}

bool DeveloperConsoleUVE::SetCompletionPrefixUVE(std::string prefix) {
    return SetCompletionPrefixDetailedUVE(std::move(prefix)).IsAcceptedUVE();
}

DeveloperConsoleHistoryNavigationResultUVE DeveloperConsoleUVE::MoveHistoryDetailedUVE(
    const std::int32_t delta) noexcept {
    if (!IsAvailableUVE()) {
        return {DeveloperConsoleHistoryNavigationCodeUVE::Unavailable,
                "History navigation is unavailable under the Shipping build policy."};
    }
    if (delta == 0) {
        return {DeveloperConsoleHistoryNavigationCodeUVE::InvalidDelta,
                "History navigation delta must be non-zero."};
    }
    if (m_history.empty()) {
        return {DeveloperConsoleHistoryNavigationCodeUVE::EmptyHistory,
                "History navigation requires at least one history entry."};
    }

    const std::int64_t current = m_historyCursor;
    std::int64_t next = current;
    if (current < 0) {
        next = delta < 0 ? static_cast<std::int64_t>(m_history.size() - 1U) : -1;
    } else if (delta > 0 && current == static_cast<std::int64_t>(m_history.size() - 1U)) {
        next = -1;
    } else {
        next += delta;
        next = std::clamp<std::int64_t>(next, 0, static_cast<std::int64_t>(m_history.size() - 1U));
    }
    if (next == current) {
        return {DeveloperConsoleHistoryNavigationCodeUVE::Boundary,
                "History navigation is already at the requested boundary."};
    }
    m_historyCursor = static_cast<std::int32_t>(next);
    IncrementGenerationUVE(m_generation);
    return {DeveloperConsoleHistoryNavigationCodeUVE::Applied, "History cursor updated."};
}

bool DeveloperConsoleUVE::MoveHistoryUVE(const std::int32_t delta) noexcept {
    return MoveHistoryDetailedUVE(delta).IsAcceptedUVE();
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
    snapshot.available = IsAvailableUVE();
    snapshot.developmentOnly = true;
    snapshot.access = m_access;
    snapshot.severityFilter = m_severityFilter;
    snapshot.historyCursor = m_historyCursor;
    if (m_historyCursor >= 0 && static_cast<std::size_t>(m_historyCursor) < m_history.size()) {
        snapshot.historyEntry = m_history[static_cast<std::size_t>(m_historyCursor)];
    }
    snapshot.outputTruncated = m_outputTruncated;
    snapshot.historyTruncated = m_historyTruncated;
    snapshot.cvarsTruncated = m_cvarsTruncated;
    snapshot.completionTruncated = m_completionTruncated;
    for (const DeveloperConsoleEntryUVE& entry : m_output) {
        if (MatchesSeverityUVE(entry.severity, m_severityFilter)) {
            snapshot.output.push_back(entry);
        }
    }
    snapshot.history = m_history;
    for (const auto& [name, cvar] : m_cvars) {
        snapshot.cvars.push_back(cvar);
    }
    for (const auto& [identifier, command] : m_commands) {
        if (!m_completionPrefix.empty() && identifier.compare(0U, m_completionPrefix.size(), m_completionPrefix) != 0) {
            continue;
        }
        if (snapshot.completions.size() >= kMaximumCompletionsUVE) {
            snapshot.completionTruncated = true;
            break;
        }
        snapshot.completions.push_back(DeveloperConsoleCompletionUVE{identifier, command.help});
    }
    return snapshot;
}

} // namespace UVE::Editor
