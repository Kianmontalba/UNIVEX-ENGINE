// Copyright (c) 2026 UniVex Studios. All Rights Reserved.
#include "uve/network/reliable_packet_window_uve.h"

#include <cmath>
#include <limits>
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

[[nodiscard]] bool IsConsistentReliableAcknowledgementStateUVE(
    const ReliableAcknowledgementStateUVE& state) noexcept {
    if (!state.hasReceivedSequence) {
        return state.latestReceivedSequence == 0U && state.receivedHistoryBits == 0U;
    }
    return state.latestReceivedSequence != 0U;
}

[[nodiscard]] bool IsConsistentInactiveReassemblyStateUVE(
    const ReliablePayloadReassemblyStateUVE& state) noexcept {
    return !state.IsActiveUVE() && state.fragmentCount == 0U && state.receivedFragmentCount == 0U &&
           state.receivedByteCount == 0U && state.fragments.empty();
}

[[nodiscard]] bool IsConsistentActiveReassemblyStateUVE(
    const ReliablePayloadReassemblyStateUVE& state) noexcept {
    if (!state.IsActiveUVE() || state.fragmentCount == 0U ||
        state.fragmentCount > kReliablePacketMaximumFragmentCountUVE ||
        state.fragments.size() != state.fragmentCount || state.receivedFragmentCount == 0U ||
        state.receivedFragmentCount > state.fragmentCount ||
        state.receivedByteCount > kReliablePacketMaximumReassembledPayloadBytesUVE) {
        return false;
    }

    std::size_t actualFragmentCount = 0U;
    std::size_t actualByteCount = 0U;
    for (const std::vector<std::uint8_t>& fragment : state.fragments) {
        if (fragment.empty()) {
            continue;
        }
        ++actualFragmentCount;
        if (fragment.size() > kReliablePacketMaximumPayloadBytesUVE ||
            actualByteCount > kReliablePacketMaximumReassembledPayloadBytesUVE ||
            fragment.size() > kReliablePacketMaximumReassembledPayloadBytesUVE - actualByteCount) {
            return false;
        }
        actualByteCount += fragment.size();
    }
    return actualFragmentCount == state.receivedFragmentCount && actualByteCount == state.receivedByteCount;
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

bool SerializeReliablePayloadFragmentUVE(const ReliablePayloadFragmentUVE& fragment,
                                         std::vector<std::uint8_t>& outBytes) noexcept {
    if (fragment.messageId == 0U || fragment.fragmentCount == 0U ||
        fragment.fragmentCount > kReliablePacketMaximumFragmentCountUVE ||
        fragment.fragmentIndex >= fragment.fragmentCount || fragment.payloadBytes.empty() ||
        fragment.payloadBytes.size() > kReliablePacketMaximumPayloadBytesUVE) {
        return false;
    }
    try {
        std::vector<std::uint8_t> bytes;
        bytes.reserve(kReliablePayloadFragmentHeaderWireBytesUVE + fragment.payloadBytes.size());
        const auto appendUint32 = [&bytes](const std::uint32_t value) {
            bytes.push_back(static_cast<std::uint8_t>(value & 0xFFU));
            bytes.push_back(static_cast<std::uint8_t>((value >> 8U) & 0xFFU));
            bytes.push_back(static_cast<std::uint8_t>((value >> 16U) & 0xFFU));
            bytes.push_back(static_cast<std::uint8_t>((value >> 24U) & 0xFFU));
        };
        appendUint32(fragment.messageId);
        appendUint32(fragment.fragmentIndex);
        appendUint32(fragment.fragmentCount);
        appendUint32(static_cast<std::uint32_t>(fragment.payloadBytes.size()));
        bytes.insert(bytes.end(), fragment.payloadBytes.begin(), fragment.payloadBytes.end());
        outBytes = std::move(bytes);
        return true;
    } catch (const std::bad_alloc&) {
        return false;
    }
}

bool DeserializeReliablePayloadFragmentUVE(const std::vector<std::uint8_t>& bytes,
                                           ReliablePayloadFragmentUVE& outFragment) noexcept {
    if (bytes.size() < kReliablePayloadFragmentHeaderWireBytesUVE ||
        bytes.size() > kReliablePayloadFragmentHeaderWireBytesUVE + kReliablePacketMaximumPayloadBytesUVE) {
        return false;
    }
    const auto readUint32 = [&bytes](const std::size_t offset) {
        return static_cast<std::uint32_t>(bytes[offset]) |
               (static_cast<std::uint32_t>(bytes[offset + 1U]) << 8U) |
               (static_cast<std::uint32_t>(bytes[offset + 2U]) << 16U) |
               (static_cast<std::uint32_t>(bytes[offset + 3U]) << 24U);
    };
    const std::uint32_t messageId = readUint32(0U);
    const std::uint32_t fragmentIndex = readUint32(4U);
    const std::uint32_t fragmentCount = readUint32(8U);
    const std::uint32_t payloadSize = readUint32(12U);
    if (messageId == 0U || fragmentCount == 0U || fragmentCount > kReliablePacketMaximumFragmentCountUVE ||
        fragmentIndex >= fragmentCount || payloadSize == 0U ||
        payloadSize > kReliablePacketMaximumPayloadBytesUVE ||
        static_cast<std::size_t>(payloadSize) != bytes.size() - kReliablePayloadFragmentHeaderWireBytesUVE) {
        return false;
    }
    try {
        ReliablePayloadFragmentUVE decoded;
        decoded.messageId = messageId;
        decoded.fragmentIndex = fragmentIndex;
        decoded.fragmentCount = fragmentCount;
        decoded.payloadBytes.assign(bytes.begin() + static_cast<std::ptrdiff_t>(kReliablePayloadFragmentHeaderWireBytesUVE),
                                     bytes.end());
        outFragment = std::move(decoded);
        return true;
    } catch (const std::bad_alloc&) {
        return false;
    }
}

bool BuildReliablePayloadFragmentsUVE(const std::uint32_t messageId,
                                       const std::vector<std::uint8_t>& payloadBytes,
                                       const std::size_t maximumFragmentBytes,
                                       std::vector<ReliablePayloadFragmentUVE>& outFragments) noexcept {
    ReliablePayloadFragmentPlanUVE plan;
    if (messageId == 0U || !PlanReliablePayloadFragmentsUVE(payloadBytes.size(), maximumFragmentBytes, plan)) {
        return false;
    }
    try {
        std::vector<ReliablePayloadFragmentUVE> fragments;
        fragments.reserve(plan.fragmentCount);
        std::size_t payloadOffset = 0U;
        for (std::size_t index = 0U; index < plan.fragmentCount; ++index) {
            const std::size_t remainingBytes = payloadBytes.size() - payloadOffset;
            const std::size_t currentBytes = std::min(remainingBytes, maximumFragmentBytes);
            ReliablePayloadFragmentUVE fragment;
            fragment.messageId = messageId;
            fragment.fragmentIndex = static_cast<std::uint32_t>(index);
            fragment.fragmentCount = static_cast<std::uint32_t>(plan.fragmentCount);
            fragment.payloadBytes.assign(
                payloadBytes.begin() + static_cast<std::ptrdiff_t>(payloadOffset),
                payloadBytes.begin() + static_cast<std::ptrdiff_t>(payloadOffset + currentBytes));
            fragments.push_back(std::move(fragment));
            payloadOffset += currentBytes;
        }
        outFragments = std::move(fragments);
        return true;
    } catch (const std::bad_alloc&) {
        return false;
    }
}

bool SerializeReliablePacketFragmentUVE(const ReliablePacketHeaderUVE& header,
                                         const ReliablePayloadFragmentUVE& fragment,
                                         std::vector<std::uint8_t>& outBytes) noexcept {
    try {
        std::vector<std::uint8_t> headerBytes;
        std::vector<std::uint8_t> fragmentBytes;
        if (!SerializeReliablePacketHeaderUVE(header, headerBytes) ||
            !SerializeReliablePayloadFragmentUVE(fragment, fragmentBytes)) {
            return false;
        }
        std::vector<std::uint8_t> bytes;
        bytes.reserve(headerBytes.size() + fragmentBytes.size());
        bytes.insert(bytes.end(), headerBytes.begin(), headerBytes.end());
        bytes.insert(bytes.end(), fragmentBytes.begin(), fragmentBytes.end());
        outBytes = std::move(bytes);
        return true;
    } catch (const std::bad_alloc&) {
        return false;
    }
}

bool DeserializeReliablePacketFragmentUVE(const std::vector<std::uint8_t>& bytes,
                                           ReliablePacketHeaderUVE& outHeader,
                                           ReliablePayloadFragmentUVE& outFragment) noexcept {
    if (bytes.size() < kReliablePacketFragmentWireHeaderBytesUVE ||
        bytes.size() > kReliablePacketFragmentWireHeaderBytesUVE + kReliablePacketMaximumPayloadBytesUVE) {
        return false;
    }
    try {
        const auto headerEnd = bytes.begin() + static_cast<std::ptrdiff_t>(kReliablePacketHeaderWireBytesUVE);
        std::vector<std::uint8_t> headerBytes(bytes.begin(), headerEnd);
        std::vector<std::uint8_t> fragmentBytes(headerEnd, bytes.end());
        ReliablePacketHeaderUVE decodedHeader;
        ReliablePayloadFragmentUVE decodedFragment;
        if (!DeserializeReliablePacketHeaderUVE(headerBytes, decodedHeader) ||
            !DeserializeReliablePayloadFragmentUVE(fragmentBytes, decodedFragment)) {
            return false;
        }
        outHeader = decodedHeader;
        outFragment = std::move(decodedFragment);
        return true;
    } catch (const std::bad_alloc&) {
        return false;
    }
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
        if (!IsConsistentInactiveReassemblyStateUVE(state)) {
            return ReliablePayloadReassemblyStatusUVE::Invalid;
        }
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
        if (!IsConsistentActiveReassemblyStateUVE(state)) {
            return ReliablePayloadReassemblyStatusUVE::Invalid;
        }
        if (state.messageId != messageId || state.fragmentCount != fragmentCount ||
            state.fragments.size() != fragmentCount) {
            return ReliablePayloadReassemblyStatusUVE::Conflict;
        }
        if (state.receivedByteCount > kReliablePacketMaximumReassembledPayloadBytesUVE ||
            fragmentBytes.size() > kReliablePacketMaximumReassembledPayloadBytesUVE - state.receivedByteCount) {
            return ReliablePayloadReassemblyStatusUVE::Invalid;
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
    const std::size_t boundedMaximumBytes =
        maximumBytes < kReliablePacketMaximumPayloadBytesUVE ? maximumBytes : kReliablePacketMaximumPayloadBytesUVE;
    return payloadBytes <= boundedMaximumBytes;
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

bool ReliableRetryScheduleUVE::ConfigureUVE(const float baseTimeoutSeconds,
                                           const float maximumTimeoutSeconds,
                                           const std::uint32_t maximumRetries) noexcept {
    if (!std::isfinite(baseTimeoutSeconds) || baseTimeoutSeconds <= 0.0F ||
        !std::isfinite(maximumTimeoutSeconds) || maximumTimeoutSeconds < baseTimeoutSeconds ||
        maximumRetries > 31U) {
        return false;
    }
    m_baseTimeoutSeconds = baseTimeoutSeconds;
    m_maximumTimeoutSeconds = maximumTimeoutSeconds;
    m_elapsedSeconds = 0.0F;
    m_retryCount = 0U;
    m_maximumRetries = maximumRetries;
    m_configured = true;
    return true;
}

ReliableRetransmitStatusUVE ReliableRetryScheduleUVE::AdvanceUVE(const float elapsedSeconds) noexcept {
    if (!m_configured || !std::isfinite(elapsedSeconds) || elapsedSeconds < 0.0F) {
        return ReliableRetransmitStatusUVE::Invalid;
    }
    float retryTimeoutSeconds = 0.0F;
    if (!ComputeReliableRetryTimeoutUVE(m_baseTimeoutSeconds, m_retryCount, m_maximumTimeoutSeconds,
                                        retryTimeoutSeconds)) {
        return ReliableRetransmitStatusUVE::Invalid;
    }
    const float candidateElapsedSeconds = m_elapsedSeconds + elapsedSeconds;
    if (!std::isfinite(candidateElapsedSeconds)) {
        return ReliableRetransmitStatusUVE::Invalid;
    }
    m_elapsedSeconds = candidateElapsedSeconds;
    return EvaluateReliableRetransmitPolicyUVE(
        ReliableRetransmitPolicyInputUVE{m_elapsedSeconds, retryTimeoutSeconds, m_retryCount, m_maximumRetries});
}

bool ReliableRetryScheduleUVE::CommitRetryUVE() noexcept {
    if (!m_configured || m_retryCount >= m_maximumRetries) {
        return false;
    }
    float retryTimeoutSeconds = 0.0F;
    if (!ComputeReliableRetryTimeoutUVE(m_baseTimeoutSeconds, m_retryCount, m_maximumTimeoutSeconds,
                                        retryTimeoutSeconds) ||
        EvaluateReliableRetransmitPolicyUVE(
            ReliableRetransmitPolicyInputUVE{m_elapsedSeconds, retryTimeoutSeconds, m_retryCount, m_maximumRetries}) !=
            ReliableRetransmitStatusUVE::Due) {
        return false;
    }
    ++m_retryCount;
    m_elapsedSeconds = 0.0F;
    return true;
}

void ReliableRetryScheduleUVE::ResetUVE() noexcept {
    m_baseTimeoutSeconds = 0.0F;
    m_maximumTimeoutSeconds = 0.0F;
    m_elapsedSeconds = 0.0F;
    m_retryCount = 0U;
    m_maximumRetries = 0U;
    m_configured = false;
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
    if (sequence == 0U || !IsConsistentReliableAcknowledgementStateUVE(state)) {
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
        if (distance > kReliablePacketMaximumSelectiveAckBitsUVE) {
            state.receivedHistoryBits = 0U;
        } else if (distance == kReliablePacketMaximumSelectiveAckBitsUVE) {
            state.receivedHistoryBits = 1U << (kReliablePacketMaximumSelectiveAckBitsUVE - 1U);
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
    if (header.sequence == 0U || header.acknowledgedSequence == 0U || oldestPendingSequence == 0U) {
        return false;
    }
    bool changed = false;
    const std::uint32_t cumulativeDistance = header.acknowledgedSequence - oldestPendingSequence;
    if (cumulativeDistance < kHalfSequenceSpaceUVE &&
        cumulativeDistance < kReliablePacketMaximumSelectiveAckBitsUVE) {
        const std::uint32_t cumulativeMask =
            cumulativeDistance == kReliablePacketMaximumSelectiveAckBitsUVE - 1U
                ? std::numeric_limits<std::uint32_t>::max()
                : (1U << (cumulativeDistance + 1U)) - 1U;
        const std::uint32_t before = pendingSequenceMask;
        pendingSequenceMask &= ~cumulativeMask;
        changed = before != pendingSequenceMask;
    }
    for (std::uint32_t bitIndex = 0U; bitIndex < kReliablePacketMaximumSelectiveAckBitsUVE; ++bitIndex) {
        if ((header.selectiveAcknowledgementBits & (1U << bitIndex)) == 0U) {
            continue;
        }
        const std::uint32_t selectiveSequence = header.acknowledgedSequence + bitIndex + 1U;
        // Reject reserved zero and any arithmetic wrap that is not newer in sequence space.
        if (selectiveSequence == 0U ||
            !IsNewerSequenceUVE(selectiveSequence, header.acknowledgedSequence)) {
            continue;
        }
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
