// Copyright (c) 2026 UniVex Studios. All Rights Reserved.
#pragma once
#include <cstddef>
#include <cstdint>
namespace UVE::Network {
inline constexpr std::uint32_t kReliablePacketMaximumSelectiveAckBitsUVE = 32U;
inline constexpr std::size_t kReliablePacketMaximumPayloadBytesUVE = 1200U;
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
enum class ReliableRetransmitStatusUVE : std::uint8_t {
    Waiting = 0,
    Due,
    Exhausted,
    Invalid,
};
struct ReliableRetransmitPolicyInputUVE final {
    float elapsedSeconds = 0.0F;
    float retryTimeoutSeconds = 0.0F;
    std::uint32_t retryCount = 0U;
    std::uint32_t maximumRetries = 0U;
};
/// Validates one caller-owned payload size against a bounded datagram budget.
[[nodiscard]] bool ValidateReliablePayloadBudgetUVE(
    std::size_t payloadBytes,
    std::size_t maximumBytes = kReliablePacketMaximumPayloadBytesUVE) noexcept;

/// Evaluates one caller-owned retry decision without owning a clock, timer, socket, or packet.
[[nodiscard]] ReliableRetransmitStatusUVE EvaluateReliableRetransmitPolicyUVE(
    const ReliableRetransmitPolicyInputUVE& input) noexcept;
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
