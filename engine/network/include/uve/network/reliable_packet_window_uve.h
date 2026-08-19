// Copyright (c) 2026 UniVex Studios. All Rights Reserved.
#pragma once
#include <cstdint>
namespace UVE::Network {
inline constexpr std::uint32_t kReliablePacketMaximumSelectiveAckBitsUVE = 32U;
struct ReliablePacketHeaderUVE final {
    std::uint32_t sequence = 0U;
    std::uint32_t acknowledgedSequence = 0U;
    std::uint32_t selectiveAcknowledgementBits = 0U;
};
enum class ReliablePacketReceiveStatusUVE : std::uint8_t {
    Accepted = 0,
    Duplicate,
    TooOld,
    Invalid,
};
struct ReliableAcknowledgementStateUVE final {
    std::uint32_t latestReceivedSequence = 0U;
    std::uint32_t receivedHistoryBits = 0U;
    bool hasReceivedSequence = false;
};
/// Updates a copied receive window using wrap-safe uint32 sequence ordering.
/// Sequence zero is reserved as the initial/no-packet state; this contract owns no socket or payload.
[[nodiscard]] ReliablePacketReceiveStatusUVE AcceptReliableSequenceUVE(
    std::uint32_t sequence, ReliableAcknowledgementStateUVE& state) noexcept;
/// Applies cumulative and 32-bit forward selective acknowledgements to one caller-owned pending mask.
/// Bit zero acknowledges acknowledgedSequence + 1, bit one acknowledges +2, and so on.
[[nodiscard]] bool ApplyReliableAcknowledgementsUVE(
    const ReliablePacketHeaderUVE& header, std::uint32_t& pendingSequenceMask,
    std::uint32_t oldestPendingSequence) noexcept;
} // namespace UVE::Network
