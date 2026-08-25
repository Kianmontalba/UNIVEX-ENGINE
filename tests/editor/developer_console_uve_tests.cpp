// Copyright (c) 2026 UniVex Studios. All Rights Reserved.
#include "uve/editor/developer_console_uve.h"

#include <gtest/gtest.h>

#include <algorithm>
#include <stdexcept>
#include <string>
#include <vector>

namespace UVE::Editor::Tests {

TEST(DeveloperConsoleUVE, BuiltInsAndRegisteredCVarAreDeterministic) {
    DeveloperConsoleUVE console;
    ASSERT_TRUE(console.RegisterCVar("r.vsync", "1"));

    ASSERT_TRUE(console.ExecuteUVE("help"));
    ASSERT_TRUE(console.ExecuteUVE("cvar r.vsync"));
    ASSERT_TRUE(console.ExecuteUVE("cvar r.vsync 0"));

    const DeveloperConsoleSnapshotUVE snapshot = console.GetSnapshotUVE();
    ASSERT_GE(snapshot.output.size(), 4U);
    EXPECT_EQ(snapshot.cvars.size(), 1U);
    EXPECT_EQ(snapshot.cvars.front().name, "r.vsync");
    EXPECT_EQ(snapshot.cvars.front().value, "0");
    EXPECT_EQ(snapshot.history.size(), 3U);
    EXPECT_FALSE(snapshot.outputTruncated);
}

TEST(DeveloperConsoleUVE, ExplicitCommandsAndUnknownInputStayNativeAndBounded) {
    DeveloperConsoleUVE console;
    std::string receivedArguments;
    ASSERT_TRUE(console.RegisterCommand("echo", "Append a diagnostic echo.",
                                        [&receivedArguments](DeveloperConsoleUVE& target, const std::string_view arguments) {
                                            receivedArguments = std::string(arguments);
                                            target.AppendUVE(DeveloperConsoleSeverityUVE::Info, receivedArguments);
                                        }));

    ASSERT_TRUE(console.ExecuteUVE("echo hello"));
    EXPECT_EQ(receivedArguments, "hello");
    EXPECT_FALSE(console.ExecuteUVE("notRegistered"));
    EXPECT_FALSE(console.ExecuteUVE(std::string(DeveloperConsoleUVE::kMaximumValueBytesUVE + 1U, 'x')));

    const DeveloperConsoleSnapshotUVE snapshot = console.GetSnapshotUVE();
    ASSERT_FALSE(snapshot.output.empty());
    EXPECT_EQ(snapshot.output.back().severity, DeveloperConsoleSeverityUVE::Error);
    EXPECT_EQ(snapshot.history.size(), 2U);
}

TEST(DeveloperConsoleUVE, RegisterCVarUVEReturnsStructuredDiagnosticsForEachRejectionCode) {
    DeveloperConsoleUVE console;

    const DeveloperConsoleCVarRegistrationResultUVE invalidName = console.RegisterCVarUVE("invalid name", "1");
    EXPECT_EQ(invalidName.code, DeveloperConsoleCVarRegistrationCodeUVE::InvalidName);
    EXPECT_FALSE(invalidName.IsAcceptedUVE());
    EXPECT_FALSE(invalidName.message.empty());

    const DeveloperConsoleCVarRegistrationResultUVE invalidValue =
        console.RegisterCVarUVE("r.invalid", std::string(DeveloperConsoleUVE::kMaximumValueBytesUVE + 1U, 'x'));
    EXPECT_EQ(invalidValue.code, DeveloperConsoleCVarRegistrationCodeUVE::InvalidValue);
    EXPECT_FALSE(invalidValue.IsAcceptedUVE());
    EXPECT_FALSE(invalidValue.message.empty());

    const DeveloperConsoleCVarRegistrationResultUVE accepted = console.RegisterCVarUVE("r.runtime", "1", true);
    EXPECT_EQ(accepted.code, DeveloperConsoleCVarRegistrationCodeUVE::Accepted);
    EXPECT_TRUE(accepted.IsAcceptedUVE());
    EXPECT_FALSE(accepted.message.empty());

    const DeveloperConsoleCVarRegistrationResultUVE duplicate = console.RegisterCVarUVE("r.runtime", "2");
    EXPECT_EQ(duplicate.code, DeveloperConsoleCVarRegistrationCodeUVE::DuplicateName);
    EXPECT_FALSE(duplicate.IsAcceptedUVE());
    EXPECT_FALSE(duplicate.message.empty());

    for (std::size_t index = 0U; index < DeveloperConsoleUVE::kMaximumCVarsUVE - 1U; ++index) {
        ASSERT_TRUE(console.RegisterCVar("r.custom." + std::to_string(index), "0"));
    }
    const DeveloperConsoleCVarRegistrationResultUVE capacity = console.RegisterCVarUVE("r.capacity", "0");
    EXPECT_EQ(capacity.code, DeveloperConsoleCVarRegistrationCodeUVE::CapacityExceeded);
    EXPECT_FALSE(capacity.IsAcceptedUVE());
    EXPECT_FALSE(capacity.message.empty());
}

TEST(DeveloperConsoleUVE, SetCVarDetailedUVEReturnsStructuredMutationDiagnostics) {
    DeveloperConsoleUVE console;
    ASSERT_TRUE(console.RegisterCVar("r.mutable", "1"));
    ASSERT_TRUE(console.RegisterCVar("r.locked", "1", true));

    const DeveloperConsoleCVarMutationResultUVE unknown = console.SetCVarDetailedUVE("r.unknown", "2");
    EXPECT_EQ(unknown.code, DeveloperConsoleCVarMutationCodeUVE::UnknownName);
    EXPECT_FALSE(unknown.IsAcceptedUVE());
    EXPECT_FALSE(unknown.message.empty());

    const DeveloperConsoleCVarMutationResultUVE readOnly = console.SetCVarDetailedUVE("r.locked", "2");
    EXPECT_EQ(readOnly.code, DeveloperConsoleCVarMutationCodeUVE::ReadOnly);
    EXPECT_FALSE(readOnly.IsAcceptedUVE());
    EXPECT_FALSE(readOnly.message.empty());

    const DeveloperConsoleCVarMutationResultUVE invalidValue =
        console.SetCVarDetailedUVE("r.mutable", "line\nbreak");
    EXPECT_EQ(invalidValue.code, DeveloperConsoleCVarMutationCodeUVE::InvalidValue);
    EXPECT_FALSE(invalidValue.IsAcceptedUVE());
    EXPECT_FALSE(invalidValue.message.empty());

    const DeveloperConsoleCVarMutationResultUVE unchanged = console.SetCVarDetailedUVE("r.mutable", "1");
    EXPECT_EQ(unchanged.code, DeveloperConsoleCVarMutationCodeUVE::Unchanged);
    EXPECT_TRUE(unchanged.IsAcceptedUVE());

    const DeveloperConsoleCVarMutationResultUVE applied = console.SetCVarDetailedUVE("r.mutable", "2");
    EXPECT_EQ(applied.code, DeveloperConsoleCVarMutationCodeUVE::Applied);
    EXPECT_TRUE(applied.IsAcceptedUVE());
    EXPECT_TRUE(console.SetCVarUVE("r.mutable", "3"));
    const DeveloperConsoleSnapshotUVE snapshot = console.GetSnapshotUVE();
    const auto mutableCVar = std::find_if(snapshot.cvars.begin(), snapshot.cvars.end(), [](const DeveloperConsoleCVarUVE& cvar) {
        return cvar.name == "r.mutable";
    });
    ASSERT_NE(mutableCVar, snapshot.cvars.end());
    EXPECT_EQ(mutableCVar->value, "3");

    DeveloperConsoleUVE shipping(DeveloperConsoleBuildPolicyUVE::Shipping);
    const DeveloperConsoleCommandRegistrationResultUVE commandUnavailable = shipping.RegisterCommandUVE(
        "r.shipping.command", "Unavailable command.", [](DeveloperConsoleUVE&, std::string_view) {});
    EXPECT_EQ(commandUnavailable.code, DeveloperConsoleCommandRegistrationCodeUVE::Unavailable);
    EXPECT_FALSE(commandUnavailable.IsAcceptedUVE());
    const DeveloperConsoleCVarRegistrationResultUVE cvarUnavailable =
        shipping.RegisterCVarUVE("r.shipping", "1");
    EXPECT_EQ(cvarUnavailable.code, DeveloperConsoleCVarRegistrationCodeUVE::Unavailable);
    EXPECT_FALSE(cvarUnavailable.IsAcceptedUVE());
    const DeveloperConsoleCVarMutationResultUVE unavailable = shipping.SetCVarDetailedUVE("r.shipping", "2");
    EXPECT_EQ(unavailable.code, DeveloperConsoleCVarMutationCodeUVE::Unavailable);
    EXPECT_FALSE(unavailable.IsAcceptedUVE());
    EXPECT_FALSE(unavailable.message.empty());
}

TEST(DeveloperConsoleUVE, ExecuteDetailedUVEReturnsStructuredDiagnosticsAndPreservesHistoryPolicy) {
    DeveloperConsoleUVE console;
    ASSERT_TRUE(console.RegisterCommand("echo", "Append a diagnostic echo.", [](DeveloperConsoleUVE& target, std::string_view arguments) {
        target.AppendUVE(DeveloperConsoleSeverityUVE::Info, std::string(arguments));
    }));

    const DeveloperConsoleExecutionResultUVE executed = console.ExecuteDetailedUVE("echo hello");
    EXPECT_EQ(executed.code, DeveloperConsoleExecutionCodeUVE::Executed);
    EXPECT_TRUE(executed.IsAcceptedUVE());
    EXPECT_FALSE(executed.message.empty());
    EXPECT_TRUE(console.ExecuteUVE("echo bool"));

    const DeveloperConsoleExecutionResultUVE invalidInput = console.ExecuteDetailedUVE("");
    EXPECT_EQ(invalidInput.code, DeveloperConsoleExecutionCodeUVE::InvalidInput);
    EXPECT_FALSE(invalidInput.IsAcceptedUVE());
    EXPECT_FALSE(invalidInput.message.empty());

    const DeveloperConsoleExecutionResultUVE unknown = console.ExecuteDetailedUVE("missing");
    EXPECT_EQ(unknown.code, DeveloperConsoleExecutionCodeUVE::UnknownCommand);
    EXPECT_FALSE(unknown.IsAcceptedUVE());
    EXPECT_FALSE(unknown.message.empty());
    EXPECT_EQ(console.GetSnapshotUVE().history.size(), 3U);

    DeveloperConsoleUVE shipping(DeveloperConsoleBuildPolicyUVE::Shipping);
    const DeveloperConsoleExecutionResultUVE unavailable = shipping.ExecuteDetailedUVE("help");
    EXPECT_EQ(unavailable.code, DeveloperConsoleExecutionCodeUVE::Unavailable);
    EXPECT_FALSE(unavailable.IsAcceptedUVE());
    EXPECT_FALSE(unavailable.message.empty());
    EXPECT_TRUE(shipping.GetSnapshotUVE().history.empty());
}

TEST(DeveloperConsoleUVE, RegisterCommandUVEReturnsStructuredDiagnosticsForEachRejectionCode) {
    DeveloperConsoleUVE console;
    const auto handler = [](DeveloperConsoleUVE&, std::string_view) {};

    const DeveloperConsoleCommandRegistrationResultUVE invalidIdentifier =
        console.RegisterCommandUVE("invalid identifier", "Help", handler);
    EXPECT_EQ(invalidIdentifier.code, DeveloperConsoleCommandRegistrationCodeUVE::InvalidIdentifier);
    EXPECT_FALSE(invalidIdentifier.IsAcceptedUVE());
    EXPECT_FALSE(invalidIdentifier.message.empty());

    const DeveloperConsoleCommandRegistrationResultUVE invalidHelp = console.RegisterCommandUVE("empty.help", "", handler);
    EXPECT_EQ(invalidHelp.code, DeveloperConsoleCommandRegistrationCodeUVE::InvalidHelp);
    EXPECT_FALSE(invalidHelp.IsAcceptedUVE());
    EXPECT_FALSE(invalidHelp.message.empty());

    const DeveloperConsoleCommandRegistrationResultUVE missingHandler =
        console.RegisterCommandUVE("missing.handler", "Help", DeveloperConsoleCommandHandlerUVE{});
    EXPECT_EQ(missingHandler.code, DeveloperConsoleCommandRegistrationCodeUVE::MissingHandler);
    EXPECT_FALSE(missingHandler.IsAcceptedUVE());
    EXPECT_FALSE(missingHandler.message.empty());

    const DeveloperConsoleCommandRegistrationResultUVE accepted =
        console.RegisterCommandUVE("runtime.inspect", "Inspect runtime state.", handler);
    EXPECT_EQ(accepted.code, DeveloperConsoleCommandRegistrationCodeUVE::Accepted);
    EXPECT_TRUE(accepted.IsAcceptedUVE());
    EXPECT_FALSE(accepted.message.empty());

    const DeveloperConsoleCommandRegistrationResultUVE duplicate =
        console.RegisterCommandUVE("runtime.inspect", "Inspect runtime state again.", handler);
    EXPECT_EQ(duplicate.code, DeveloperConsoleCommandRegistrationCodeUVE::DuplicateIdentifier);
    EXPECT_FALSE(duplicate.IsAcceptedUVE());
    EXPECT_FALSE(duplicate.message.empty());

    for (std::size_t index = 0U; index < DeveloperConsoleUVE::kMaximumCommandsUVE - 4U; ++index) {
        ASSERT_TRUE(console.RegisterCommand("custom." + std::to_string(index), "Custom command.", handler));
    }
    const DeveloperConsoleCommandRegistrationResultUVE capacity =
        console.RegisterCommandUVE("capacity.exceeded", "Capacity test.", handler);
    EXPECT_EQ(capacity.code, DeveloperConsoleCommandRegistrationCodeUVE::CapacityExceeded);
    EXPECT_FALSE(capacity.IsAcceptedUVE());
    EXPECT_FALSE(capacity.message.empty());
}

TEST(DeveloperConsoleUVE, SetSeverityFilterDetailedUVEReturnsStructuredDiagnostics) {
    DeveloperConsoleUVE console;
    console.AppendUVE(DeveloperConsoleSeverityUVE::Info, "info");
    console.AppendUVE(DeveloperConsoleSeverityUVE::Warning, "warning");

    const DeveloperConsoleSeverityFilterResultUVE applied =
        console.SetSeverityFilterDetailedUVE(DeveloperConsoleSeverityFilterUVE::Warning);
    EXPECT_EQ(applied.code, DeveloperConsoleSeverityFilterCodeUVE::Applied);
    EXPECT_TRUE(applied.IsAcceptedUVE());
    EXPECT_FALSE(applied.message.empty());

    const DeveloperConsoleSeverityFilterResultUVE unchanged =
        console.SetSeverityFilterDetailedUVE(DeveloperConsoleSeverityFilterUVE::Warning);
    EXPECT_EQ(unchanged.code, DeveloperConsoleSeverityFilterCodeUVE::Unchanged);
    EXPECT_TRUE(unchanged.IsAcceptedUVE());

    const DeveloperConsoleSeverityFilterResultUVE invalid = console.SetSeverityFilterDetailedUVE(
        static_cast<DeveloperConsoleSeverityFilterUVE>(255U));
    EXPECT_EQ(invalid.code, DeveloperConsoleSeverityFilterCodeUVE::InvalidFilter);
    EXPECT_FALSE(invalid.IsAcceptedUVE());
    EXPECT_FALSE(invalid.message.empty());
    ASSERT_TRUE(console.SetSeverityFilterUVE(DeveloperConsoleSeverityFilterUVE::Error));

    DeveloperConsoleUVE shipping(DeveloperConsoleBuildPolicyUVE::Shipping);
    const DeveloperConsoleSeverityFilterResultUVE unavailable =
        shipping.SetSeverityFilterDetailedUVE(DeveloperConsoleSeverityFilterUVE::Error);
    EXPECT_EQ(unavailable.code, DeveloperConsoleSeverityFilterCodeUVE::Unavailable);
    EXPECT_FALSE(unavailable.IsAcceptedUVE());
    EXPECT_FALSE(unavailable.message.empty());
}

TEST(DeveloperConsoleUVE, SetCompletionPrefixDetailedUVEReturnsStructuredDiagnostics) {
    DeveloperConsoleUVE console;
    ASSERT_TRUE(console.RegisterCommand("camera.reset", "Reset the active camera.", [](DeveloperConsoleUVE&, std::string_view) {}));

    const DeveloperConsoleCompletionPrefixResultUVE applied = console.SetCompletionPrefixDetailedUVE(" camera ");
    EXPECT_EQ(applied.code, DeveloperConsoleCompletionPrefixCodeUVE::Applied);
    EXPECT_TRUE(applied.IsAcceptedUVE());
    EXPECT_FALSE(applied.message.empty());
    ASSERT_TRUE(console.SetCompletionPrefixUVE("camera"));

    const DeveloperConsoleCompletionPrefixResultUVE unchanged = console.SetCompletionPrefixDetailedUVE("camera");
    EXPECT_EQ(unchanged.code, DeveloperConsoleCompletionPrefixCodeUVE::Unchanged);
    EXPECT_TRUE(unchanged.IsAcceptedUVE());

    const DeveloperConsoleCompletionPrefixResultUVE invalid =
        console.SetCompletionPrefixDetailedUVE(std::string(DeveloperConsoleUVE::kMaximumValueBytesUVE + 1U, 'x'));
    EXPECT_EQ(invalid.code, DeveloperConsoleCompletionPrefixCodeUVE::InvalidPrefix);
    EXPECT_FALSE(invalid.IsAcceptedUVE());
    EXPECT_FALSE(invalid.message.empty());
    ASSERT_EQ(console.GetSnapshotUVE().completions.size(), 1U);
    EXPECT_EQ(console.GetSnapshotUVE().completions.front().identifier, "camera.reset");

    DeveloperConsoleUVE shipping(DeveloperConsoleBuildPolicyUVE::Shipping);
    const DeveloperConsoleCompletionPrefixResultUVE unavailable =
        shipping.SetCompletionPrefixDetailedUVE("camera");
    EXPECT_EQ(unavailable.code, DeveloperConsoleCompletionPrefixCodeUVE::Unavailable);
    EXPECT_FALSE(unavailable.IsAcceptedUVE());
    EXPECT_FALSE(unavailable.message.empty());
}

TEST(DeveloperConsoleUVE, CompletionAndSeverityFilterAreNativeDeterministicFacts) {
    DeveloperConsoleUVE console;
    ASSERT_TRUE(console.RegisterCommand("camera.reset", "Reset the active camera.",
                                        [](DeveloperConsoleUVE& target, std::string_view) {
                                            target.AppendUVE(DeveloperConsoleSeverityUVE::Info, "camera reset");
                                        }));
    console.AppendUVE(DeveloperConsoleSeverityUVE::Info, "info");
    console.AppendUVE(DeveloperConsoleSeverityUVE::Warning, "warning");
    console.AppendUVE(DeveloperConsoleSeverityUVE::Error, "error");

    ASSERT_TRUE(console.SetCompletionPrefixUVE("camera"));
    DeveloperConsoleSnapshotUVE discovered = console.GetSnapshotUVE();
    ASSERT_EQ(discovered.completions.size(), 1U);
    EXPECT_EQ(discovered.completions.front().identifier, "camera.reset");
    EXPECT_FALSE(discovered.completionTruncated);

    ASSERT_TRUE(console.SetSeverityFilterUVE(DeveloperConsoleSeverityFilterUVE::Error));
    const DeveloperConsoleSnapshotUVE filtered = console.GetSnapshotUVE();
    ASSERT_EQ(filtered.output.size(), 1U);
    EXPECT_EQ(filtered.output.front().severity, DeveloperConsoleSeverityUVE::Error);
}

TEST(DeveloperConsoleUVE, SetAccessDetailedUVE_EnforcesBoundedAuthorizationWithoutMutationBypass) {
    DeveloperConsoleUVE console;
    ASSERT_TRUE(console.RegisterCVarUVE("r.runtime", "1").IsAcceptedUVE());

    const DeveloperConsoleAuthorizationResultUVE readOnly =
        console.SetAccessDetailedUVE(DeveloperConsoleAccessUVE::ReadOnly);
    EXPECT_EQ(readOnly.code, DeveloperConsoleAuthorizationCodeUVE::Applied);
    EXPECT_TRUE(readOnly.IsAcceptedUVE());
    EXPECT_EQ(console.GetSnapshotUVE().access, DeveloperConsoleAccessUVE::ReadOnly);

    const DeveloperConsoleCommandRegistrationResultUVE command =
        console.RegisterCommandUVE("blocked.command", "Blocked command.", [](DeveloperConsoleUVE&, std::string_view) {});
    EXPECT_EQ(command.code, DeveloperConsoleCommandRegistrationCodeUVE::Unauthorized);
    EXPECT_EQ(console.GetSnapshotUVE().completions.size(), 3U);
    EXPECT_EQ(console.ExecuteDetailedUVE("help").code, DeveloperConsoleExecutionCodeUVE::Unauthorized);
    EXPECT_EQ(console.SetCVarDetailedUVE("r.runtime", "2").code, DeveloperConsoleCVarMutationCodeUVE::Unauthorized);

    console.AppendUVE(DeveloperConsoleSeverityUVE::Info, "retained");
    EXPECT_EQ(console.ClearDetailedUVE().code, DeveloperConsoleClearCodeUVE::Unauthorized);
    EXPECT_EQ(console.GetSnapshotUVE().output.size(), 1U);

    const DeveloperConsoleAuthorizationResultUVE full = console.SetAccessDetailedUVE(DeveloperConsoleAccessUVE::Full);
    EXPECT_EQ(full.code, DeveloperConsoleAuthorizationCodeUVE::Applied);
    EXPECT_TRUE(console.RegisterCommand("allowed.command", "Allowed command.", [](DeveloperConsoleUVE&, std::string_view) {}));
    EXPECT_TRUE(console.ExecuteUVE("allowed.command"));
    EXPECT_TRUE(console.SetCVarUVE("r.runtime", "2"));

    const DeveloperConsoleAuthorizationResultUVE invalid =
        console.SetAccessDetailedUVE(static_cast<DeveloperConsoleAccessUVE>(99U));
    EXPECT_EQ(invalid.code, DeveloperConsoleAuthorizationCodeUVE::InvalidAccess);
}

TEST(DeveloperConsoleUVE, SetAccessDetailedUVE_ShippingRemainsPermanentlyDenied) {
    DeveloperConsoleUVE shipping(DeveloperConsoleBuildPolicyUVE::Shipping);
    const DeveloperConsoleSnapshotUVE initial = shipping.GetSnapshotUVE();
    EXPECT_EQ(initial.access, DeveloperConsoleAccessUVE::Denied);
    const DeveloperConsoleAuthorizationResultUVE denied = shipping.SetAccessDetailedUVE(DeveloperConsoleAccessUVE::ReadOnly);
    EXPECT_EQ(denied.code, DeveloperConsoleAuthorizationCodeUVE::Unavailable);
    EXPECT_EQ(shipping.GetSnapshotUVE().access, DeveloperConsoleAccessUVE::Denied);
}

TEST(DeveloperConsoleUVE, SetBuildPolicyDetailedUVEReturnsStructuredDiagnostics) {
    DeveloperConsoleUVE console;
    const DeveloperConsoleBuildPolicyResultUVE unchanged =
        console.SetBuildPolicyDetailedUVE(DeveloperConsoleBuildPolicyUVE::Development);
    EXPECT_EQ(unchanged.code, DeveloperConsoleBuildPolicyCodeUVE::Unchanged);
    EXPECT_FALSE(unchanged.IsAcceptedUVE());
    EXPECT_FALSE(unchanged.message.empty());

    const DeveloperConsoleBuildPolicyResultUVE applied =
        console.SetBuildPolicyDetailedUVE(DeveloperConsoleBuildPolicyUVE::Shipping);
    EXPECT_EQ(applied.code, DeveloperConsoleBuildPolicyCodeUVE::Applied);
    EXPECT_TRUE(applied.IsAcceptedUVE());
    EXPECT_FALSE(applied.message.empty());
    const DeveloperConsoleSnapshotUVE shippingSnapshot = console.GetSnapshotUVE();
    EXPECT_FALSE(shippingSnapshot.available);
    const DeveloperConsoleBuildPolicyResultUVE locked =
        console.SetBuildPolicyDetailedUVE(DeveloperConsoleBuildPolicyUVE::Development);
    EXPECT_EQ(locked.code, DeveloperConsoleBuildPolicyCodeUVE::Locked);
    EXPECT_FALSE(locked.IsAcceptedUVE());
    EXPECT_FALSE(locked.message.empty());
    const DeveloperConsoleSnapshotUVE afterRejectedDowngrade = console.GetSnapshotUVE();
    EXPECT_FALSE(afterRejectedDowngrade.available);
    EXPECT_EQ(afterRejectedDowngrade.generation, shippingSnapshot.generation);
    EXPECT_FALSE(console.SetBuildPolicyUVE(DeveloperConsoleBuildPolicyUVE::Development));
}

TEST(DeveloperConsoleUVE, ShippingPolicyUVE_ClosesRegistrationAndPreservesGenerationOnDowngrade) {
    DeveloperConsoleUVE shipping(DeveloperConsoleBuildPolicyUVE::Shipping);
    const DeveloperConsoleSnapshotUVE initial = shipping.GetSnapshotUVE();
    EXPECT_FALSE(initial.available);
    EXPECT_TRUE(initial.developmentOnly);
    EXPECT_EQ(initial.access, DeveloperConsoleAccessUVE::Denied);
    EXPECT_TRUE(initial.completions.empty());

    const DeveloperConsoleCommandRegistrationResultUVE command = shipping.RegisterCommandUVE(
        "late.command", "Late command.", [](DeveloperConsoleUVE&, std::string_view) {});
    EXPECT_EQ(command.code, DeveloperConsoleCommandRegistrationCodeUVE::Unavailable);
    const DeveloperConsoleCVarRegistrationResultUVE cvar = shipping.RegisterCVarUVE("late.cvar", "0");
    EXPECT_EQ(cvar.code, DeveloperConsoleCVarRegistrationCodeUVE::Unavailable);
    const DeveloperConsoleSnapshotUVE afterRegistration = shipping.GetSnapshotUVE();
    EXPECT_EQ(afterRegistration.generation, initial.generation);

    const DeveloperConsoleBuildPolicyResultUVE downgrade =
        shipping.SetBuildPolicyDetailedUVE(DeveloperConsoleBuildPolicyUVE::Development);
    EXPECT_EQ(downgrade.code, DeveloperConsoleBuildPolicyCodeUVE::Locked);
    EXPECT_EQ(shipping.GetSnapshotUVE().generation, initial.generation);
    EXPECT_FALSE(shipping.IsAvailableUVE());
}

TEST(DeveloperConsoleUVE, MoveHistoryDetailedUVEReturnsStructuredDiagnostics) {
    DeveloperConsoleUVE empty;
    const DeveloperConsoleHistoryNavigationResultUVE emptyHistory = empty.MoveHistoryDetailedUVE(-1);
    EXPECT_EQ(emptyHistory.code, DeveloperConsoleHistoryNavigationCodeUVE::EmptyHistory);
    EXPECT_FALSE(emptyHistory.IsAcceptedUVE());
    EXPECT_FALSE(emptyHistory.message.empty());

    ASSERT_TRUE(empty.ExecuteUVE("help"));
    const DeveloperConsoleHistoryNavigationResultUVE invalidDelta = empty.MoveHistoryDetailedUVE(0);
    EXPECT_EQ(invalidDelta.code, DeveloperConsoleHistoryNavigationCodeUVE::InvalidDelta);
    EXPECT_FALSE(invalidDelta.IsAcceptedUVE());
    EXPECT_FALSE(invalidDelta.message.empty());

    const DeveloperConsoleHistoryNavigationResultUVE applied = empty.MoveHistoryDetailedUVE(-1);
    EXPECT_EQ(applied.code, DeveloperConsoleHistoryNavigationCodeUVE::Applied);
    EXPECT_TRUE(applied.IsAcceptedUVE());
    EXPECT_TRUE(empty.MoveHistoryUVE(1));

    const DeveloperConsoleHistoryNavigationResultUVE boundary = empty.MoveHistoryDetailedUVE(1);
    EXPECT_EQ(boundary.code, DeveloperConsoleHistoryNavigationCodeUVE::Boundary);
    EXPECT_FALSE(boundary.IsAcceptedUVE());
    EXPECT_FALSE(boundary.message.empty());

    DeveloperConsoleUVE shipping(DeveloperConsoleBuildPolicyUVE::Shipping);
    const DeveloperConsoleHistoryNavigationResultUVE unavailable = shipping.MoveHistoryDetailedUVE(-1);
    EXPECT_EQ(unavailable.code, DeveloperConsoleHistoryNavigationCodeUVE::Unavailable);
    EXPECT_FALSE(unavailable.IsAcceptedUVE());
    EXPECT_FALSE(unavailable.message.empty());
}

TEST(DeveloperConsoleUVE, HistoryCursorAndShippingPolicyAreExplicit) {
    DeveloperConsoleUVE console;
    ASSERT_TRUE(console.ExecuteUVE("help"));
    ASSERT_TRUE(console.MoveHistoryUVE(-1));
    DeveloperConsoleSnapshotUVE history = console.GetSnapshotUVE();
    EXPECT_EQ(history.historyCursor, 0);
    EXPECT_EQ(history.historyEntry, "help");
    ASSERT_TRUE(console.MoveHistoryUVE(1));
    EXPECT_EQ(console.GetSnapshotUVE().historyCursor, -1);

    DeveloperConsoleUVE shipping(DeveloperConsoleBuildPolicyUVE::Shipping);
    const DeveloperConsoleSnapshotUVE unavailable = shipping.GetSnapshotUVE();
    EXPECT_FALSE(unavailable.available);
    EXPECT_TRUE(unavailable.developmentOnly);
    EXPECT_FALSE(shipping.ExecuteUVE("help"));
    EXPECT_FALSE(shipping.SetSeverityFilterUVE(DeveloperConsoleSeverityFilterUVE::Error));
    EXPECT_FALSE(shipping.SetCompletionPrefixUVE("h"));
    EXPECT_FALSE(shipping.MoveHistoryUVE(-1));
}

TEST(DeveloperConsoleUVE, ClearDetailedUVEReturnsStructuredDiagnostics) {
    DeveloperConsoleUVE console;
    const DeveloperConsoleClearResultUVE unchanged = console.ClearDetailedUVE();
    EXPECT_EQ(unchanged.code, DeveloperConsoleClearCodeUVE::Unchanged);
    EXPECT_FALSE(unchanged.IsAcceptedUVE());
    EXPECT_FALSE(unchanged.message.empty());

    console.AppendUVE(DeveloperConsoleSeverityUVE::Info, "entry");
    const DeveloperConsoleClearResultUVE applied = console.ClearDetailedUVE();
    EXPECT_EQ(applied.code, DeveloperConsoleClearCodeUVE::Applied);
    EXPECT_TRUE(applied.IsAcceptedUVE());
    EXPECT_FALSE(applied.message.empty());
    EXPECT_TRUE(console.GetSnapshotUVE().output.empty());
    EXPECT_FALSE(console.ClearUVE());

    DeveloperConsoleUVE shipping(DeveloperConsoleBuildPolicyUVE::Shipping);
    const DeveloperConsoleClearResultUVE unavailable = shipping.ClearDetailedUVE();
    EXPECT_EQ(unavailable.code, DeveloperConsoleClearCodeUVE::Unavailable);
    EXPECT_FALSE(unavailable.IsAcceptedUVE());
    EXPECT_FALSE(unavailable.message.empty());
}

TEST(DeveloperConsoleUVE, OutputAndHistoryExposeTruncationAndClearIsExplicit) {
    DeveloperConsoleUVE console;
    for (std::size_t index = 0U; index < DeveloperConsoleUVE::kMaximumOutputUVE + 4U; ++index) {
        console.AppendUVE(DeveloperConsoleSeverityUVE::Info, "entry");
    }
    for (std::size_t index = 0U; index < DeveloperConsoleUVE::kMaximumHistoryUVE + 4U; ++index) {
        ASSERT_TRUE(console.ExecuteUVE("help"));
    }

    const DeveloperConsoleSnapshotUVE beforeClear = console.GetSnapshotUVE();
    EXPECT_TRUE(beforeClear.outputTruncated);
    EXPECT_TRUE(beforeClear.historyTruncated);
    EXPECT_EQ(beforeClear.output.size(), DeveloperConsoleUVE::kMaximumOutputUVE);
    EXPECT_EQ(beforeClear.history.size(), DeveloperConsoleUVE::kMaximumHistoryUVE);
    EXPECT_TRUE(console.ClearUVE());
    EXPECT_FALSE(console.ClearUVE());
    EXPECT_TRUE(console.GetSnapshotUVE().output.empty());
}

TEST(DeveloperConsoleUVE, AuditSinkCapturesCopiedPrincipalAndSecurityTransitions) {
    DeveloperConsoleUVE console;
    std::vector<DeveloperConsoleAuditRecordUVE> records;
    console.SetAuditSinkUVE([&records](const DeveloperConsoleAuditRecordUVE& record) {
        records.push_back(record);
    });

    ASSERT_TRUE(console.SetAuditPrincipalUVE({"editor_user", "session_42"}));
    EXPECT_EQ(console.GetAuditPrincipalUVE(), (DeveloperConsolePrincipalUVE{"editor_user", "session_42"}));
    ASSERT_TRUE(console.RegisterCVar("r.audit", "0"));
    ASSERT_TRUE(console.SetCVarUVE("r.audit", "1"));
    ASSERT_TRUE(console.ExecuteUVE("help"));
    EXPECT_FALSE(console.ExecuteUVE("missing"));
    ASSERT_TRUE(console.SetAccessUVE(DeveloperConsoleAccessUVE::ReadOnly));
    EXPECT_FALSE(console.SetCVarUVE("r.audit", "2"));

    ASSERT_EQ(records.size(), 5U);
    EXPECT_EQ(records[0].sequence, 1U);
    EXPECT_EQ(records[0].action, DeveloperConsoleAuditActionUVE::CVarMutation);
    EXPECT_EQ(records[0].principal.subject, "editor_user");
    EXPECT_EQ(records[0].principal.session, "session_42");
    EXPECT_EQ(records[0].target, "r.audit");
    EXPECT_TRUE(records[0].accepted);
    EXPECT_EQ(records[1].action, DeveloperConsoleAuditActionUVE::CommandExecution);
    EXPECT_EQ(records[1].target, "help");
    EXPECT_TRUE(records[1].accepted);
    EXPECT_EQ(records[2].action, DeveloperConsoleAuditActionUVE::CommandExecution);
    EXPECT_EQ(records[2].target, "missing");
    EXPECT_FALSE(records[2].accepted);
    EXPECT_EQ(records[3].action, DeveloperConsoleAuditActionUVE::AccessChange);
    EXPECT_TRUE(records[3].accepted);
    EXPECT_EQ(records[4].action, DeveloperConsoleAuditActionUVE::CVarMutation);
    EXPECT_EQ(records[4].target, "r.audit");
    EXPECT_FALSE(records[4].accepted);
    for (std::size_t index = 1U; index < records.size(); ++index) {
        EXPECT_EQ(records[index].sequence, records[index - 1U].sequence + 1U);
    }
}

TEST(DeveloperConsoleUVE, AuditPrincipalRejectsUnboundedOrControlLabelsAtomically) {
    DeveloperConsoleUVE console;
    ASSERT_TRUE(console.SetAuditPrincipalUVE({"valid_user", "valid_session"}));
    const DeveloperConsolePrincipalUVE original = console.GetAuditPrincipalUVE();

    EXPECT_FALSE(console.SetAuditPrincipalUVE({"", "session"}));
    EXPECT_FALSE(console.SetAuditPrincipalUVE({std::string(DeveloperConsoleUVE::kMaximumPrincipalBytesUVE + 1U, 'x'), "session"}));
    EXPECT_FALSE(console.SetAuditPrincipalUVE({"user\n", "session"}));
    EXPECT_EQ(console.GetAuditPrincipalUVE(), original);
}

TEST(DeveloperConsoleUVE, AuditSinkExceptionsDoNotChangeConsoleBehavior) {
    DeveloperConsoleUVE console;
    console.SetAuditSinkUVE([](const DeveloperConsoleAuditRecordUVE&) {
        throw std::runtime_error("caller sink failure");
    });

    ASSERT_NO_THROW(EXPECT_TRUE(console.RegisterCVar("r.safe", "0")));
    ASSERT_NO_THROW(EXPECT_TRUE(console.SetCVarUVE("r.safe", "1")));
    ASSERT_NO_THROW(EXPECT_TRUE(console.ExecuteUVE("help")));
    EXPECT_EQ(console.GetSnapshotUVE().cvars.front().value, "1");
}

} // namespace UVE::Editor::Tests
