// Copyright (c) 2026 UniVex Studios. All Rights Reserved.
#pragma once
#include <cstddef>
#include <cstdint>
#include <vector>
namespace UVE::Network {
inline constexpr std::uint32_t kReliablePacketMaximumSelectiveAckBitsUVE = 32U;
inline constexpr std::size_t kReliablePacketMaximumPayloadBytesUVE = 1200U;
inline constexpr std::size_t kReliablePacketMaximumFragmentCountUVE = 1024U;
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
struct ReliablePayloadFragmentPlanUVE final {
    std::size_t fragmentCount = 0U;
    std::size_t maximumFragmentBytes = 0U;
    std::size_t finalFragmentBytes = 0U;
    bool fragmented = false;
    [[nodiscard]] bool operator==(const ReliablePayloadFragmentPlanUVE&) const noexcept = default;
};
enum class ReliablePayloadReassemblyStatusUVE : std::uint8_t {
    Accepted = 0,
    Duplicate,
    Complete,
    Conflict,
    Invalid,
};
struct ReliablePayloadReassemblyStateUVE final {
    std::uint32_t messageId = 0U;
    std::size_t fragmentCount = 0U;
    std::size_t receivedFragmentCount = 0U;
    std::size_t receivedByteCount = 0U;
    std::vector<std::vector<std::uint8_t>> fragments;
    [[nodiscard]] bool IsActiveUVE() const noexcept {
        return messageId != 0U;
    }
    void ResetUVE() noexcept {
        messageId = 0U;
        fragmentCount = 0U;
        receivedFragmentCount = 0U;
        receivedByteCount = 0U;
        fragments.clear();
    }
};
/// Accepts one copied caller-owned payload fragment into bounded reassembly state. The state owns
/// only fragment bytes; it does not own packet headers, sockets, timers, peers, retransmission, or
/// transport delivery. A complete message publishes ordered copied bytes and resets the state.
[[nodiscard]] ReliablePayloadReassemblyStatusUVE AcceptReliablePayloadFragmentUVE(
    std::uint32_t messageId, std::size_t fragmentIndex, std::size_t fragmentCount,
    const std::vector<std::uint8_t>& fragmentBytes, ReliablePayloadReassemblyStateUVE& state,
    std::vector<std::uint8_t>& outPayload) noexcept;
/// Plans bounded caller-owned payload fragments without serializing or retaining reassembly state.
[[nodiscard]] bool PlanReliablePayloadFragmentsUVE(
    std::size_t payloadBytes, std::size_t fragmentBytes,
    ReliablePayloadFragmentPlanUVE& outPlan,
    std::size_t maximumFragments = kReliablePacketMaximumFragmentCountUVE) noexcept;

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
