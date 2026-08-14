// Copyright (c) 2026 UniVex Studios. All Rights Reserved.

namespace UniVex.EditorHost;

public enum HostSessionState
{
    Disconnected,
    Connecting,
    Connected,
    Failed,
    ConfirmFreshSession,
    ConfirmDiscardDirtySession,
}
