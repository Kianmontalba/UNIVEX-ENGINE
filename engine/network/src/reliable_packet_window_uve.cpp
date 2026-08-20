// Copyright (c) 2026 UniVex Studios. All Rights Reserved.
#include "uve/network/reliable_packet_window_uve.h"

#include <cmath>
#include <new>
#include <utility>
namespace UVE::Network {
namespace {
constexpr std::uint32_t kHalfSequenceSpaceUVE = 0x80000000U;
[[nodiscard]] bool IsNewerSequenceUVE(const std::uint32_t candidate,
                                      const std::uint32_t reference) noexcept {
    const std::uint32_t distance = candidate - reference;
    return distance != 0U && distance < kHalfSequenceSpaceUVE;
}
} // namespace

bool SerializeReliablePacketHeaderUVE(const ReliablePacketHeaderUVE& header,
                                      std::vector<std::uint8_t>& outBytes) noexcept {
    if (header.sequence == 0U || header.acknowledgedSequence == 0U) {
        return false;
    }
    try {
        std::vector<std::uint8_t> bytes;
        bytes.reserve(kReliablePacketHeaderWireBytesUVE);
        const auto appendUint32 = [&bytes](const std::uint32_t value) {
            bytes.push_back(static_cast<std::uint8_t>(value & 0xFFU));
            bytes.push_back(static_cast<std::uint8_t>((value >> 8U) & 0xFFU));
            bytes.push_back(static_cast<std::uint8_t>((value >> 16U) & 0xFFU));
            bytes.push_back(static_cast<std::uint8_t>((value >> 24U) & 0xFFU));
        };
        appendUint32(header.sequence);
        appendUint32(header.acknowledgedSequence);
        appendUint32(header.selectiveAcknowledgementBits);
        outBytes = std::move(bytes);
        return true;
    } catch (const std::bad_alloc&) {
        return false;
    }
}

bool DeserializeReliablePacketHeaderUVE(const std::vector<std::uint8_t>& bytes,
                                        ReliablePacketHeaderUVE& outHeader) noexcept {
    if (bytes.size() != kReliablePacketHeaderWireBytesUVE) {
        return false;
    }
    const auto readUint32 = [&bytes](const std::size_t offset) {
        return static_cast<std::uint32_t>(bytes[offset]) |
               (static_cast<std::uint32_t>(bytes[offset + 1U]) << 8U) |
               (static_cast<std::uint32_t>(bytes[offset + 2U]) << 16U) |
               (static_cast<std::uint32_t>(bytes[offset + 3U]) << 24U);
    };
    const ReliablePacketHeaderUVE decoded{readUint32(0U), readUint32(4U), readUint32(8U)};
    if (decoded.sequence == 0U || decoded.acknowledgedSequence == 0U) {
        return false;
    }
    outHeader = decoded;
    return true;
}

ReliablePayloadReassemblyStatusUVE AcceptReliablePayloadFragmentUVE(
    const std::uint32_t messageId, const std::size_t fragmentIndex, const std::size_t fragmentCount,
    const std::vector<std::uint8_t>& fragmentBytes, ReliablePayloadReassemblyStateUVE& state,
    std::vector<std::uint8_t>& outPayload) noexcept {
    if (messageId == 0U || fragmentCount == 0U ||
        fragmentCount > kReliablePacketMaximumFragmentCountUVE || fragmentIndex >= fragmentCount ||
        fragmentBytes.empty() || fragmentBytes.size() > kReliablePacketMaximumPayloadBytesUVE) {
        return ReliablePayloadReassemblyStatusUVE::Invalid;
    }

    if (!state.IsActiveUVE()) {
        try {
            ReliablePayloadReassemblyStateUVE initialState;
            initialState.messageId = messageId;
            initialState.fragmentCount = fragmentCount;
            initialState.fragments.resize(fragmentCount);
            initialState.fragments[fragmentIndex] = fragmentBytes;
            initialState.receivedFragmentCount = 1U;
            initialState.receivedByteCount = fragmentBytes.size();
            state = std::move(initialState);
        } catch (const std::bad_alloc&) {
            state.ResetUVE();
            return ReliablePayloadReassemblyStatusUVE::Invalid;
        }
    } else {
        if (state.messageId != messageId || state.fragmentCount != fragmentCount ||
            state.fragments.size() != fragmentCount) {
            return ReliablePayloadReassemblyStatusUVE::Conflict;
        }
        std::vector<std::uint8_t>& storedFragment = state.fragments[fragmentIndex];
        if (!storedFragment.empty()) {
            return storedFragment == fragmentBytes ? ReliablePayloadReassemblyStatusUVE::Duplicate
                                                    : ReliablePayloadReassemblyStatusUVE::Conflict;
        }
        try {
            storedFragment = fragmentBytes;
            ++state.receivedFragmentCount;
            state.receivedByteCount += fragmentBytes.size();
        } catch (const std::bad_alloc&) {
            return ReliablePayloadReassemblyStatusUVE::Invalid;
        }
    }

    if (state.receivedFragmentCount < state.fragmentCount) {
        return ReliablePayloadReassemblyStatusUVE::Accepted;
    }

    try {
        std::vector<std::uint8_t> assembled;
        assembled.reserve(state.receivedByteCount);
        for (const std::vector<std::uint8_t>& storedFragment : state.fragments) {
            assembled.insert(assembled.end(), storedFragment.begin(), storedFragment.end());
        }
        outPayload = std::move(assembled);
    } catch (const std::bad_alloc&) {
        return ReliablePayloadReassemblyStatusUVE::Invalid;
    }
    state.ResetUVE();
    return ReliablePayloadReassemblyStatusUVE::Complete;
}

bool PlanReliablePayloadFragmentsUVE(const std::size_t payloadBytes,
                                      const std::size_t fragmentBytes,
                                      ReliablePayloadFragmentPlanUVE& outPlan,
                                      const std::size_t maximumFragments) noexcept {
    if (payloadBytes == 0U || fragmentBytes == 0U ||
        fragmentBytes > kReliablePacketMaximumPayloadBytesUVE || maximumFragments == 0U) {
        return false;
    }
    const std::size_t fragmentCount = payloadBytes / fragmentBytes +
                                      (payloadBytes % fragmentBytes == 0U ? 0U : 1U);
    const std::size_t boundedMaximumFragments =
        maximumFragments < kReliablePacketMaximumFragmentCountUVE
            ? maximumFragments
            : kReliablePacketMaximumFragmentCountUVE;
    if (fragmentCount == 0U || fragmentCount > boundedMaximumFragments) {
        return false;
    }
    const std::size_t remainder = payloadBytes % fragmentBytes;
    outPlan = ReliablePayloadFragmentPlanUVE{fragmentCount, fragmentBytes,
                                              remainder == 0U ? fragmentBytes : remainder,
                                              fragmentCount > 1U};
    return true;
}

bool ValidateReliablePayloadBudgetUVE(const std::size_t payloadBytes,
                                      const std::size_t maximumBytes) noexcept {
    return payloadBytes <= maximumBytes;
}

bool ComputeReliableRetryTimeoutUVE(const float baseTimeoutSeconds, const std::uint32_t retryCount,
                                     const float maximumTimeoutSeconds, float& outTimeoutSeconds) noexcept {
    if (!std::isfinite(baseTimeoutSeconds) || baseTimeoutSeconds <= 0.0F ||
        !std::isfinite(maximumTimeoutSeconds) || maximumTimeoutSeconds < baseTimeoutSeconds ||
        retryCount > 31U) {
        return false;
    }
    float timeout = baseTimeoutSeconds;
    for (std::uint32_t attempt = 0U; attempt < retryCount; ++attempt) {
        if (timeout >= maximumTimeoutSeconds * 0.5F) {
            timeout = maximumTimeoutSeconds;
            break;
        }
        timeout *= 2.0F;
    }
    outTimeoutSeconds = timeout > maximumTimeoutSeconds ? maximumTimeoutSeconds : timeout;
    return std::isfinite(outTimeoutSeconds);
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
