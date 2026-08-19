#pragma once

#include <cstdint>

namespace UVE::Save {

enum class SaveRevisionStatusUVE : std::uint8_t {
    Invalid = 0,
    Unchanged,
    LocalAhead,
    RemoteAhead,
    Conflict,
};

/// Classifies caller-owned base/local/remote save revisions without cloud or merge ownership.
[[nodiscard]] SaveRevisionStatusUVE EvaluateSaveRevisionUVE(
    std::uint64_t baseRevision, std::uint64_t localRevision,
    std::uint64_t remoteRevision) noexcept;

} // namespace UVE::Save
