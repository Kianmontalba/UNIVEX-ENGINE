#pragma once

#include <cstdint>

namespace UVE::Save {

enum class SaveSyncActionUVE : std::uint8_t {
    Invalid = 0,
    NoOp,
    Upload,
    Download,
    Conflict,
};

enum class SaveRevisionStatusUVE : std::uint8_t {
    Invalid = 0,
    Unchanged,
    LocalAhead,
    RemoteAhead,
    Conflict,
};

/// Maps caller-owned revision status to a sync direction without performing cloud I/O.
[[nodiscard]] SaveSyncActionUVE EvaluateSaveSyncActionUVE(SaveRevisionStatusUVE status) noexcept;

/// Classifies caller-owned base/local/remote save revisions without cloud or merge ownership.
[[nodiscard]] SaveRevisionStatusUVE EvaluateSaveRevisionUVE(
    std::uint64_t baseRevision, std::uint64_t localRevision,
    std::uint64_t remoteRevision) noexcept;

} // namespace UVE::Save
