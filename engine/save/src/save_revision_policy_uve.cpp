#include "uve/save/save_revision_policy_uve.h"

namespace UVE::Save {

SaveSyncActionUVE EvaluateSaveSyncActionUVE(const SaveRevisionStatusUVE status) noexcept {
    switch (status) {
        case SaveRevisionStatusUVE::Unchanged:
            return SaveSyncActionUVE::NoOp;
        case SaveRevisionStatusUVE::LocalAhead:
            return SaveSyncActionUVE::Upload;
        case SaveRevisionStatusUVE::RemoteAhead:
            return SaveSyncActionUVE::Download;
        case SaveRevisionStatusUVE::Conflict:
            return SaveSyncActionUVE::Conflict;
        case SaveRevisionStatusUVE::Invalid:
        default:
            return SaveSyncActionUVE::Invalid;
    }
}

SaveRevisionStatusUVE EvaluateSaveRevisionUVE(
    const std::uint64_t baseRevision, const std::uint64_t localRevision,
    const std::uint64_t remoteRevision) noexcept {
    if (baseRevision == 0U || localRevision == 0U || remoteRevision == 0U) {
        return SaveRevisionStatusUVE::Invalid;
    }
    if (localRevision == remoteRevision) {
        return SaveRevisionStatusUVE::Unchanged;
    }
    if (localRevision != baseRevision && remoteRevision == baseRevision) {
        return SaveRevisionStatusUVE::LocalAhead;
    }
    if (localRevision == baseRevision && remoteRevision != baseRevision) {
        return SaveRevisionStatusUVE::RemoteAhead;
    }
    return SaveRevisionStatusUVE::Conflict;
}

} // namespace UVE::Save
