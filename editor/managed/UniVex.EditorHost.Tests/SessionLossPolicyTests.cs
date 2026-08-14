// Copyright (c) 2026 UniVex Studios. All Rights Reserved.

namespace UniVex.EditorHost.Tests;

public sealed class SessionLossPolicyTests
{
    [Fact]
    public void RequiresFreshSessionAcknowledgement_OnlyPriorBackendCanGateReplacement()
    {
        Assert.True(SessionLossPolicy.RequiresFreshSessionAcknowledgement(true));
        Assert.False(SessionLossPolicy.RequiresFreshSessionAcknowledgement(false));
    }

    [Theory]
    [InlineData(HostSessionState.Disconnected, false)]
    [InlineData(HostSessionState.Connecting, false)]
    [InlineData(HostSessionState.Connected, false)]
    [InlineData(HostSessionState.ConfirmDiscardDirtySession, false)]
    [InlineData(HostSessionState.Failed, true)]
    [InlineData(HostSessionState.ConfirmFreshSession, true)]
    public void IsTerminalFailureState_OnlyPreservesExistingFailureAndFreshSessionAcknowledgement(
        HostSessionState state, bool expected)
    {
        Assert.Equal(expected, SessionLossPolicy.IsTerminalFailureState(state));
    }

    [Fact]
    public void DescribeFreshSessionLoss_DirtySnapshotMakesLossNonRecoverabilityExplicit()
    {
        string message = SessionLossPolicy.DescribeFreshSessionLoss(true);

        Assert.Contains("new EditorUVE session", message, StringComparison.Ordinal);
        Assert.Contains("cannot be recovered", message, StringComparison.Ordinal);
        Assert.Contains("sceneDirty=true", message, StringComparison.Ordinal);
    }

    [Fact]
    public void DescribeFreshSessionLoss_UnknownSnapshotDoesNotClaimRecoveryEvidence()
    {
        string message = SessionLossPolicy.DescribeFreshSessionLoss(null);

        Assert.Contains("No prior snapshot", message, StringComparison.Ordinal);
        Assert.Contains("may already be lost", message, StringComparison.Ordinal);
    }

    [Fact]
    public void DescribeDirtyCloseLoss_StatesNoSaveCommandAndDiscardOutcome()
    {
        string message = SessionLossPolicy.DescribeDirtyCloseLoss();

        Assert.Contains("no Save command", message, StringComparison.Ordinal);
        Assert.Contains("discards this in-memory session", message, StringComparison.Ordinal);
    }
}
