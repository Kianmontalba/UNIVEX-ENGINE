// Copyright (c) 2026 UniVex Studios. All Rights Reserved.

namespace UniVex.EditorHost;

/// <summary>
/// Centralizes the host's explicit v1 policy for replacement and close of a headless backend.
/// A fresh process is never a recovery/reconnect operation because the v1 bridge has no Save command.
/// </summary>
public static class SessionLossPolicy
{
    public static bool RequiresFreshSessionAcknowledgement(bool hasPriorBackend) => hasPriorBackend;

    public static bool CanStartBackend(HostSessionState state, bool hasOwnedBackend) =>
        (state is HostSessionState.Disconnected or HostSessionState.Failed) && !hasOwnedBackend;

    public static bool IsTerminalFailureState(HostSessionState state) =>
        state is HostSessionState.Failed or HostSessionState.ConfirmFreshSession;

    public static string DescribeFreshSessionLoss(bool? lastKnownDirty)
    {
        string evidence = lastKnownDirty switch
        {
            true => "The last copied snapshot reported sceneDirty=true; this is not a recovery mechanism.",
            false => "The last copied snapshot reported sceneDirty=false; this is not a guarantee that nothing was lost.",
            null => "No prior snapshot is available to describe the lost session.",
        };
        return "This starts a new headless C++ backend with a new EditorUVE session. " +
               "The previous backend cannot be recovered, and unsaved in-memory work may already be lost. " + evidence;
    }

    public static string DescribeDirtyCloseLoss() =>
        "The last copied backend snapshot reports sceneDirty=true. Increment 70 has no Save command. " +
        "Closing the host terminates its headless backend and discards this in-memory session. " +
        "Acknowledge only if you accept that loss.";
}
