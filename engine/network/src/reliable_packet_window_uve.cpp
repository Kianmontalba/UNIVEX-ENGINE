// Copyright (c) 2026 UniVex Studios. All Rights Reserved.
#include "uve/network/reliable_packet_window_uve.h"

#include <cmath>
namespace UVE::Network {
namespace {
constexpr std::uint32_t kHalfSequenceSpaceUVE = 0x80000000U;
[[nodiscard]] bool IsNewerSequenceUVE(const std::uint32_t candidate,
                                      const std::uint32_t reference) noexcept {
    const std::uint32_t distance = candidate - reference;
    return distance != 0U && distance < kHalfSequenceSpaceUVE;
}
} // namespace

bool ValidateReliablePayloadBudgetUVE(const std::size_t payloadBytes,
                                      const std::size_t maximumBytes) noexcept {
    return payloadBytes <= maximumBytes;
}

ReliableRetransmitStatusUVE EvaluateReliableRetransmitPolicyUVE(
    const ReliableRetransmitPolicyInputUVE& input) noexcept {
    if (!std::isfinite(input.elapsedSeconds) || input.elapsedSeconds < 0.0F ||
        !std::isfinite(input.retryTimeoutSeconds) || input.retryTimeoutSeconds <= 0.0F ||
        input.retryCount > input.maximumRetries) {
        return ReliableRetransmitStatusUVE::Invalid;
    }
    if (input.retryCount == input.maximumRetries) {
        return ReliableRetransmitStatusUVE::Exhausted;
    }
    return input.elapsedSeconds >= input.retryTimeoutSeconds ? ReliableRetransmitStatusUVE::Due
                                                               : ReliableRetransmitStatusUVE::Waiting;
}
ReliablePacketReceiveStatusUVE AcceptReliableSequenceUVE(
    const std::uint32_t sequence, ReliableAcknowledgementStateUVE& state) noexcept {
    if (sequence == 0U) {
        return ReliablePacketReceiveStatusUVE::Invalid;
    }
    if (!state.hasReceivedSequence) {
        state.latestReceivedSequence = sequence;
        state.receivedHistoryBits = 0U;
        state.hasReceivedSequence = true;
        return ReliablePacketReceiveStatusUVE::Accepted;
    }
    if (IsNewerSequenceUVE(sequence, state.latestReceivedSequence)) {
        const std::uint32_t distance = sequence - state.latestReceivedSequence;
        if (distance >= kReliablePacketMaximumSelectiveAckBitsUVE + 1U) {
            state.receivedHistoryBits = 0U;
        } else {
            state.receivedHistoryBits <<= distance;
            state.receivedHistoryBits |= 1U << (distance - 1U);
        }
        state.latestReceivedSequence = sequence;
        return ReliablePacketReceiveStatusUVE::Accepted;
    }
    const std::uint32_t distance = state.latestReceivedSequence - sequence;
    if (distance == 0U) {
        return ReliablePacketReceiveStatusUVE::Duplicate;
    }
    if (distance > kReliablePacketMaximumSelectiveAckBitsUVE) {
        return ReliablePacketReceiveStatusUVE::TooOld;
    }
    const std::uint32_t bit = 1U << (distance - 1U);
    if ((state.receivedHistoryBits & bit) != 0U) {
        return ReliablePacketReceiveStatusUVE::Duplicate;
    }
    state.receivedHistoryBits |= bit;
    return ReliablePacketReceiveStatusUVE::Accepted;
}
bool ApplyReliableAcknowledgementsUVE(
    const ReliablePacketHeaderUVE& header, std::uint32_t& pendingSequenceMask,
    const std::uint32_t oldestPendingSequence) noexcept {
    if (header.acknowledgedSequence == 0U || oldestPendingSequence == 0U) {
        return false;
    }
    bool changed = false;
    const std::uint32_t cumulativeDistance = header.acknowledgedSequence - oldestPendingSequence;
    if (cumulativeDistance < kHalfSequenceSpaceUVE &&
        cumulativeDistance < kReliablePacketMaximumSelectiveAckBitsUVE) {
        const std::uint32_t cumulativeMask = (1U << (cumulativeDistance + 1U)) - 1U;
        const std::uint32_t before = pendingSequenceMask;
        pendingSequenceMask &= ~cumulativeMask;
        changed = before != pendingSequenceMask;
    }
    for (std::uint32_t bitIndex = 0U; bitIndex < kReliablePacketMaximumSelectiveAckBitsUVE; ++bitIndex) {
        if ((header.selectiveAcknowledgementBits & (1U << bitIndex)) == 0U) {
            continue;
        }
        const std::uint32_t selectiveSequence = header.acknowledgedSequence + bitIndex + 1U;
        const std::uint32_t pendingDistance = selectiveSequence - oldestPendingSequence;
        if (pendingDistance >= kReliablePacketMaximumSelectiveAckBitsUVE) {
            continue;
        }
        const std::uint32_t before = pendingSequenceMask;
        pendingSequenceMask &= ~(1U << pendingDistance);
        changed = changed || before != pendingSequenceMask;
    }
    return changed;
}
} // namespace UVE::Network
