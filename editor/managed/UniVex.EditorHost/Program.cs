// Copyright (c) 2026 UniVex Studios. All Rights Reserved.

using Avalonia;

namespace UniVex.EditorHost;

internal static class Program
{
    [STAThread]
    public static int Main(string[] args)
    {
        if (args.Length == 2 && args[0] == "--probe")
        {
            return ProbeBackendAsync(args[1]).GetAwaiter().GetResult();
        }
        BuildAvaloniaApp().StartWithClassicDesktopLifetime(args);
        return 0;
    }

    public static AppBuilder BuildAvaloniaApp() => AppBuilder.Configure<App>()
        .UsePlatformDetect()
        .WithInterFont()
        .LogToTrace();

    private static async Task<int> ProbeBackendAsync(string backendExecutablePath)
    {
        try
        {
            await using BridgeBackendSession session = await BridgeBackendSession.StartAsync(
                backendExecutablePath, scenePath: null, CancellationToken.None).ConfigureAwait(false);
            BridgeEditorSnapshot initial = await session.RefreshSnapshotAsync(CancellationToken.None).ConfigureAwait(false);
            BridgeMotionQueryFeature feature = new(
                new BridgeMotionQueryVector3(0F, 0F, 0F),
                new BridgeMotionQueryVector3(0F, 0F, 1F),
                Array.Empty<BridgeMotionQueryTrajectorySample>(),
                new BridgeMotionQuerySkeleton(string.Empty, Array.Empty<BridgeMotionQuerySkeletonJoint>()),
                new BridgeMotionQueryPose(string.Empty, Array.Empty<BridgeMotionQueryTransform>()),
                new BridgeMotionQueryEvaluationContext(
                    new BridgeMotionQueryTimeState(0UL, 0D, 0D, 0D, 0D, 0D, 0D, 0D, 0D, false), 0D));
            BridgeMotionQueryCandidate firstCandidate = new("probe-candidate-0", "probe-walk", 0D, feature);
            BridgeMotionQueryDatabaseEntry database = new(
                new BridgeMotionQueryResourceHandle(7001UL, 1UL),
                "Probe Database",
                false,
                new BridgeMotionQueryDatabaseContract(
                    new BridgeMotionQueryDatabaseContext("probe-database", 1UL),
                    new BridgeMotionQueryDatabaseSchema(1U, "probe-schema", Array.Empty<double>(), Array.Empty<string>()),
                    new BridgeMotionQueryDatabaseSettings(4UL, true),
                    new[] { firstCandidate },
                    Array.Empty<BridgeMotionQueryDatabaseEvent>()));
            BridgeCommandResult registered = await session.DispatchAsync(
                new BridgeCommand(initial.Revision, "dispatchMotionQueryCommand")
                {
                    MotionQueryCommandExpectedRevision = initial.MotionQuery.Authoring.Revision,
                    MotionQueryCommandKind = "registerDatabase",
                    MotionQueryDatabase = database,
                }, CancellationToken.None).ConfigureAwait(false);
            if (!registered.Applied || registered.Snapshot.MotionQuery.Authoring.Databases.Count != 1)
            {
                throw new BridgeProtocolException("bridge.motion_query.registration.invalid",
                    $"The native bridge did not register the typed Motion Query database payload: {registered.Code}: {registered.Message}");
            }
            BridgeCommandResult added = await session.DispatchAsync(
                new BridgeCommand(registered.Snapshot.Revision, "dispatchMotionQueryCommand")
                {
                    MotionQueryCommandExpectedRevision = registered.Snapshot.MotionQuery.Authoring.Revision,
                    MotionQueryCommandKind = "addCandidate",
                    MotionQueryResource = database.Resource,
                    MotionQueryCandidate = new BridgeMotionQueryCandidate(
                        "probe-candidate-1", "probe-run", 0.25D, feature),
                }, CancellationToken.None).ConfigureAwait(false);
            if (!added.Applied || added.Snapshot.MotionQuery.Authoring.Databases[0].CandidateCount != 2)
            {
                throw new BridgeProtocolException("bridge.motion_query.candidate_add.invalid",
                    $"The native bridge did not add the typed Motion Query candidate payload: {added.Code}: {added.Message}");
            }
            BridgeCommandResult removed = await session.DispatchAsync(
                new BridgeCommand(added.Snapshot.Revision, "dispatchMotionQueryCommand")
                {
                    MotionQueryCommandExpectedRevision = added.Snapshot.MotionQuery.Authoring.Revision,
                    MotionQueryCommandKind = "removeCandidate",
                    MotionQueryResource = database.Resource,
                    MotionQueryCandidateIndex = 1UL,
                }, CancellationToken.None).ConfigureAwait(false);
            if (!removed.Applied || removed.Snapshot.MotionQuery.Authoring.Databases[0].CandidateCount != 1)
            {
                throw new BridgeProtocolException("bridge.motion_query.candidate_remove.invalid",
                    $"The native bridge did not remove the typed Motion Query candidate payload: {removed.Code}: {removed.Message}");
            }
            BridgeCommandResult undone = await session.DispatchAsync(
                new BridgeCommand(removed.Snapshot.Revision, "dispatchMotionQueryCommand")
                {
                    MotionQueryCommandExpectedRevision = removed.Snapshot.MotionQuery.Authoring.Revision,
                    MotionQueryCommandKind = "undo",
                }, CancellationToken.None).ConfigureAwait(false);
            if (!undone.Applied || undone.Snapshot.MotionQuery.Authoring.Databases[0].CandidateCount != 2)
            {
                throw new BridgeProtocolException("bridge.motion_query.candidate_undo.invalid",
                    "The native bridge did not restore the candidate through authoring undo.");
            }
            return 0;
        }
        catch (BridgeProtocolException exception)
        {
            await Console.Error.WriteLineAsync($"{exception.Code}: {exception.Message}").ConfigureAwait(false);
            return 2;
        }
        catch (Exception exception)
        {
            await Console.Error.WriteLineAsync($"bridge.host.probe.failed: {exception.Message}").ConfigureAwait(false);
            return 3;
        }
    }
}
