// Copyright (c) 2026 UniVex Studios. All Rights Reserved.
#include "uve/network/reliable_packet_window_uve.h"
#include <limits>
#include <gtest/gtest.h>
namespace UVE::Network::Tests {
namespace {
TEST(ReliablePacketWindowUVETest, PlansReliablePayloadFragmentsUVE_CalculatesBoundaries) {
    ReliablePayloadFragmentPlanUVE plan{99U, 99U, 99U, true};
    ASSERT_TRUE(PlanReliablePayloadFragmentsUVE(500U, 1200U, plan));
    EXPECT_EQ(plan.fragmentCount, 1U);
    EXPECT_EQ(plan.maximumFragmentBytes, 1200U);
    EXPECT_EQ(plan.finalFragmentBytes, 500U);
    EXPECT_FALSE(plan.fragmented);
    ASSERT_TRUE(PlanReliablePayloadFragmentsUVE(2500U, 1000U, plan));
    EXPECT_EQ(plan.fragmentCount, 3U);
    EXPECT_EQ(plan.finalFragmentBytes, 500U);
    EXPECT_TRUE(plan.fragmented);
}

TEST(ReliablePacketWindowUVETest, PlansReliablePayloadFragmentsUVE_RejectsInvalidAndOverCapInputsAtomically) {
    const ReliablePayloadFragmentPlanUVE original{7U, 8U, 9U, true};
    ReliablePayloadFragmentPlanUVE plan = original;
    EXPECT_FALSE(PlanReliablePayloadFragmentsUVE(0U, 100U, plan));
    EXPECT_EQ(plan, original);
    EXPECT_FALSE(PlanReliablePayloadFragmentsUVE(100U, 0U, plan));
    EXPECT_EQ(plan, original);
    EXPECT_FALSE(PlanReliablePayloadFragmentsUVE(100U, kReliablePacketMaximumPayloadBytesUVE + 1U, plan));
    EXPECT_EQ(plan, original);
    EXPECT_FALSE(PlanReliablePayloadFragmentsUVE(2500U, 1000U, plan, 2U));
    EXPECT_EQ(plan, original);
    EXPECT_FALSE(PlanReliablePayloadFragmentsUVE(1025U, 1U, plan, 2048U));
    EXPECT_EQ(plan, original);
}

TEST(ReliablePacketWindowUVETest, ValidateReliablePayloadBudgetUVE_AcceptsBoundedPayloads) {
    EXPECT_TRUE(ValidateReliablePayloadBudgetUVE(0U));
    EXPECT_TRUE(ValidateReliablePayloadBudgetUVE(kReliablePacketMaximumPayloadBytesUVE));
    EXPECT_TRUE(ValidateReliablePayloadBudgetUVE(512U, 512U));
}
TEST(ReliablePacketWindowUVETest, ValidateReliablePayloadBudgetUVE_RejectsOversizedPayloads) {
    EXPECT_FALSE(ValidateReliablePayloadBudgetUVE(kReliablePacketMaximumPayloadBytesUVE + 1U));
    EXPECT_FALSE(ValidateReliablePayloadBudgetUVE(1U, 0U));
    EXPECT_TRUE(ValidateReliablePayloadBudgetUVE(kReliablePacketMaximumPayloadBytesUVE,
                                                 kReliablePacketMaximumPayloadBytesUVE + 1U));
    EXPECT_FALSE(ValidateReliablePayloadBudgetUVE(kReliablePacketMaximumPayloadBytesUVE + 1U,
                                                  kReliablePacketMaximumPayloadBytesUVE + 1U));
}
TEST(ReliablePacketWindowUVETest, RetransmitPolicyClassifiesWaitingDueAndExhausted) {
    EXPECT_EQ(EvaluateReliableRetransmitPolicyUVE({0.25F, 1.0F, 0U, 3U}), ReliableRetransmitStatusUVE::Waiting);
    EXPECT_EQ(EvaluateReliableRetransmitPolicyUVE({1.0F, 1.0F, 1U, 3U}), ReliableRetransmitStatusUVE::Due);
    EXPECT_EQ(EvaluateReliableRetransmitPolicyUVE({4.0F, 1.0F, 3U, 3U}), ReliableRetransmitStatusUVE::Exhausted);
}
TEST(ReliablePacketWindowUVETest, RetransmitPolicyRejectsInvalidTimingAndRetryInputs) {
    EXPECT_EQ(EvaluateReliableRetransmitPolicyUVE({-0.1F, 1.0F, 0U, 3U}), ReliableRetransmitStatusUVE::Invalid);
    EXPECT_EQ(EvaluateReliableRetransmitPolicyUVE({0.1F, 0.0F, 0U, 3U}), ReliableRetransmitStatusUVE::Invalid);
    EXPECT_EQ(EvaluateReliableRetransmitPolicyUVE({0.1F, 1.0F, 4U, 3U}), ReliableRetransmitStatusUVE::Invalid);
}
TEST(ReliablePacketWindowUVETest, RetryTimeoutBackoffDoublesAndClamps) {
    float timeout = 99.0F;
    ASSERT_TRUE(ComputeReliableRetryTimeoutUVE(0.25F, 0U, 4.0F, timeout));
    EXPECT_FLOAT_EQ(timeout, 0.25F);
    ASSERT_TRUE(ComputeReliableRetryTimeoutUVE(0.25F, 3U, 4.0F, timeout));
    EXPECT_FLOAT_EQ(timeout, 2.0F);
    ASSERT_TRUE(ComputeReliableRetryTimeoutUVE(0.25F, 5U, 4.0F, timeout));
    EXPECT_FLOAT_EQ(timeout, 4.0F);
}

TEST(ReliablePacketWindowUVETest, RetryTimeoutBackoffRejectsInvalidInputsAtomically) {
    float timeout = 7.0F;
    EXPECT_FALSE(ComputeReliableRetryTimeoutUVE(0.0F, 0U, 4.0F, timeout));
    EXPECT_FLOAT_EQ(timeout, 7.0F);
    EXPECT_FALSE(ComputeReliableRetryTimeoutUVE(-0.1F, 0U, 4.0F, timeout));
    EXPECT_FLOAT_EQ(timeout, 7.0F);
    EXPECT_FALSE(ComputeReliableRetryTimeoutUVE(1.0F, 0U, 0.5F, timeout));
    EXPECT_FLOAT_EQ(timeout, 7.0F);
    EXPECT_FALSE(ComputeReliableRetryTimeoutUVE(1.0F, 32U, 4.0F, timeout));
    EXPECT_FLOAT_EQ(timeout, 7.0F);
    EXPECT_FALSE(ComputeReliableRetryTimeoutUVE(std::numeric_limits<float>::quiet_NaN(), 0U, 4.0F, timeout));
    EXPECT_FLOAT_EQ(timeout, 7.0F);
}

TEST(ReliablePacketWindowUVETest, FirstSequenceIsAcceptedAndAdvertised) {
    ReliableAcknowledgementStateUVE state;
    EXPECT_EQ(AcceptReliableSequenceUVE(7U, state), ReliablePacketReceiveStatusUVE::Accepted);
    EXPECT_TRUE(state.hasReceivedSequence);
    EXPECT_EQ(state.latestReceivedSequence, 7U);
    EXPECT_EQ(state.receivedHistoryBits, 0U);
}
TEST(ReliablePacketWindowUVETest, NewAndOutOfOrderSequencesUpdateBoundedHistory) {
    ReliableAcknowledgementStateUVE state;
    ASSERT_EQ(AcceptReliableSequenceUVE(10U, state), ReliablePacketReceiveStatusUVE::Accepted);
    EXPECT_EQ(AcceptReliableSequenceUVE(12U, state), ReliablePacketReceiveStatusUVE::Accepted);
    EXPECT_EQ(state.latestReceivedSequence, 12U);
    EXPECT_EQ(state.receivedHistoryBits, 2U);
    EXPECT_EQ(AcceptReliableSequenceUVE(11U, state), ReliablePacketReceiveStatusUVE::Accepted);
    EXPECT_EQ(state.receivedHistoryBits, 3U);
    EXPECT_EQ(AcceptReliableSequenceUVE(11U, state), ReliablePacketReceiveStatusUVE::Duplicate);
}
TEST(ReliablePacketWindowUVETest, ZeroAndTooOldSequencesAreRejected) {
    ReliableAcknowledgementStateUVE state;
    EXPECT_EQ(AcceptReliableSequenceUVE(0U, state), ReliablePacketReceiveStatusUVE::Invalid);
    ASSERT_EQ(AcceptReliableSequenceUVE(100U, state), ReliablePacketReceiveStatusUVE::Accepted);
    ASSERT_EQ(AcceptReliableSequenceUVE(133U, state), ReliablePacketReceiveStatusUVE::Accepted);
    EXPECT_EQ(AcceptReliableSequenceUVE(100U, state), ReliablePacketReceiveStatusUVE::TooOld);
    EXPECT_EQ(state.latestReceivedSequence, 133U);
}
TEST(ReliablePacketWindowUVETest, SequenceOrderingWrapsAcrossUint32Boundary) {
    ReliableAcknowledgementStateUVE state;
    ASSERT_EQ(AcceptReliableSequenceUVE(0xFFFFFFFEU, state), ReliablePacketReceiveStatusUVE::Accepted);
    EXPECT_EQ(AcceptReliableSequenceUVE(1U, state), ReliablePacketReceiveStatusUVE::Accepted);
    EXPECT_EQ(state.latestReceivedSequence, 1U);
    EXPECT_EQ(state.receivedHistoryBits, 4U);
}
TEST(ReliablePacketWindowUVETest, CumulativeAcknowledgementClearsPendingPrefix) {
    std::uint32_t pending = 0xFFFFFFFFU;
    const ReliablePacketHeaderUVE header{102U, 102U, 0U};
    EXPECT_TRUE(ApplyReliableAcknowledgementsUVE(header, pending, 100U));
    EXPECT_EQ(pending, 0xFFFFFFF8U);
}
TEST(ReliablePacketWindowUVETest, CumulativeAcknowledgementClearsFullThirtyTwoEntryWindow) {
    std::uint32_t pending = 0xFFFFFFFFU;
    const ReliablePacketHeaderUVE header{132U, 131U, 0U};
    EXPECT_TRUE(ApplyReliableAcknowledgementsUVE(header, pending, 100U));
    EXPECT_EQ(pending, 0U);
}
TEST(ReliablePacketWindowUVETest, SelectiveAcknowledgementClearsForwardSlots) {
    std::uint32_t pending = 0xFFFFFFFFU;
    const ReliablePacketHeaderUVE header{200U, 200U, (1U << 1U) | (1U << 4U)};
    EXPECT_TRUE(ApplyReliableAcknowledgementsUVE(header, pending, 200U));
    EXPECT_EQ(pending, ~(1U << 0U | 1U << 2U | 1U << 5U));
}
TEST(ReliablePacketWindowUVETest, SelectiveAcknowledgementRejectsReservedZeroAfterUint32Wrap) {
    std::uint32_t pending = 0xFFFFFFFFU;
    const ReliablePacketHeaderUVE header{5U, 0xFFFFFFFEU, (1U << 0U) | (1U << 1U)};
    EXPECT_TRUE(ApplyReliableAcknowledgementsUVE(header, pending, 0xFFFFFFFEU));
    EXPECT_EQ(pending, ~((1U << 0U) | (1U << 1U)));

    pending = 0xFFFFFFFEU;
    const ReliablePacketHeaderUVE reservedOnly{5U, 0xFFFFFFFEU, (1U << 1U)};
    EXPECT_FALSE(ApplyReliableAcknowledgementsUVE(reservedOnly, pending, 0xFFFFFFFEU));
    EXPECT_EQ(pending, 0xFFFFFFFEU);
}

TEST(ReliablePacketWindowUVETest, InvalidAcknowledgementDoesNotMutatePendingMask) {
    std::uint32_t pending = 0xA5A5A5A5U;
    const ReliablePacketHeaderUVE header{1U, 0U, 0xFFFFFFFFU};
    EXPECT_FALSE(ApplyReliableAcknowledgementsUVE(header, pending, 1U));
    EXPECT_EQ(pending, 0xA5A5A5A5U);
}
TEST(ReliablePacketWindowUVETest, AcknowledgementRejectsReservedZeroPacketSequenceAtomically) {
    std::uint32_t pending = 0xA5A5A5A5U;
    const ReliablePacketHeaderUVE header{0U, 8U, 0xFFFFFFFFU};
    EXPECT_FALSE(ApplyReliableAcknowledgementsUVE(header, pending, 1U));
    EXPECT_EQ(pending, 0xA5A5A5A5U);
}

TEST(ReliablePacketWindowUVETest, ReliablePacketHeaderWireRoundTrip_IsLittleEndianAndExactSize) {
    const ReliablePacketHeaderUVE original{0x12345678U, 0x90ABCDEFU, 0x01020304U};
    std::vector<std::uint8_t> bytes;
    ASSERT_TRUE(SerializeReliablePacketHeaderUVE(original, bytes));
    EXPECT_EQ(bytes, (std::vector<std::uint8_t>{0x78U, 0x56U, 0x34U, 0x12U,
                                                  0xEFU, 0xCDU, 0xABU, 0x90U,
                                                  0x04U, 0x03U, 0x02U, 0x01U}));
    ReliablePacketHeaderUVE decoded;
    ASSERT_TRUE(DeserializeReliablePacketHeaderUVE(bytes, decoded));
    EXPECT_EQ(decoded.sequence, original.sequence);
    EXPECT_EQ(decoded.acknowledgedSequence, original.acknowledgedSequence);
    EXPECT_EQ(decoded.selectiveAcknowledgementBits, original.selectiveAcknowledgementBits);
}

TEST(ReliablePacketWindowUVETest, ReliablePacketHeaderWireRejectsInvalidAndMalformedInputsAtomically) {
    const ReliablePacketHeaderUVE original{7U, 8U, 9U};
    ReliablePacketHeaderUVE decoded = original;
    EXPECT_FALSE(DeserializeReliablePacketHeaderUVE({1U, 2U, 3U}, decoded));
    EXPECT_EQ(decoded.sequence, original.sequence);
    EXPECT_EQ(decoded.acknowledgedSequence, original.acknowledgedSequence);
    EXPECT_EQ(decoded.selectiveAcknowledgementBits, original.selectiveAcknowledgementBits);

    std::vector<std::uint8_t> bytes{5U, 6U};
    EXPECT_FALSE(SerializeReliablePacketHeaderUVE(ReliablePacketHeaderUVE{0U, 2U, 3U}, bytes));
    EXPECT_EQ(bytes, (std::vector<std::uint8_t>{5U, 6U}));
    EXPECT_FALSE(SerializeReliablePacketHeaderUVE(ReliablePacketHeaderUVE{1U, 0U, 3U}, bytes));
    EXPECT_EQ(bytes, (std::vector<std::uint8_t>{5U, 6U}));
    EXPECT_FALSE(DeserializeReliablePacketHeaderUVE(
        std::vector<std::uint8_t>{0U, 0U, 0U, 0U, 2U, 0U, 0U, 0U, 3U, 0U, 0U, 0U}, decoded));
    EXPECT_EQ(decoded.sequence, original.sequence);
    EXPECT_FALSE(DeserializeReliablePacketHeaderUVE(
        std::vector<std::uint8_t>{1U, 0U, 0U, 0U, 0U, 0U, 0U, 0U, 3U, 0U, 0U, 0U}, decoded));
    EXPECT_EQ(decoded.acknowledgedSequence, original.acknowledgedSequence);
}

TEST(ReliablePacketWindowUVETest, ReassemblesOutOfOrderFragmentsAndResetsState) {
    ReliablePayloadReassemblyStateUVE state;
    std::vector<std::uint8_t> payload;

    EXPECT_EQ(AcceptReliablePayloadFragmentUVE(17U, 1U, 2U, {3U, 4U}, state, payload),
              ReliablePayloadReassemblyStatusUVE::Accepted);
    EXPECT_TRUE(state.IsActiveUVE());
    EXPECT_EQ(state.receivedFragmentCount, 1U);
    EXPECT_EQ(AcceptReliablePayloadFragmentUVE(17U, 0U, 2U, {1U, 2U}, state, payload),
              ReliablePayloadReassemblyStatusUVE::Complete);
    EXPECT_EQ(payload, (std::vector<std::uint8_t>{1U, 2U, 3U, 4U}));
    EXPECT_FALSE(state.IsActiveUVE());
    EXPECT_EQ(state.receivedFragmentCount, 0U);
}

TEST(ReliablePacketWindowUVETest, DuplicateFragmentIsIdempotentAndConflictDoesNotMutateState) {
    ReliablePayloadReassemblyStateUVE state;
    std::vector<std::uint8_t> payload;
    ASSERT_EQ(AcceptReliablePayloadFragmentUVE(19U, 0U, 2U, {7U}, state, payload),
              ReliablePayloadReassemblyStatusUVE::Accepted);

    EXPECT_EQ(AcceptReliablePayloadFragmentUVE(19U, 0U, 2U, {7U}, state, payload),
              ReliablePayloadReassemblyStatusUVE::Duplicate);
    EXPECT_EQ(AcceptReliablePayloadFragmentUVE(19U, 0U, 2U, {8U}, state, payload),
              ReliablePayloadReassemblyStatusUVE::Conflict);
    EXPECT_EQ(state.receivedFragmentCount, 1U);
    EXPECT_TRUE(payload.empty());
}

TEST(ReliablePacketWindowUVETest, ReassemblyRejectsInvalidBoundsAndMessageConflicts) {
    ReliablePayloadReassemblyStateUVE state;
    std::vector<std::uint8_t> payload{9U};

    EXPECT_EQ(AcceptReliablePayloadFragmentUVE(0U, 0U, 1U, {1U}, state, payload),
              ReliablePayloadReassemblyStatusUVE::Invalid);
    EXPECT_EQ(AcceptReliablePayloadFragmentUVE(20U, 2U, 2U, {1U}, state, payload),
              ReliablePayloadReassemblyStatusUVE::Invalid);
    EXPECT_EQ(AcceptReliablePayloadFragmentUVE(20U, 0U, kReliablePacketMaximumFragmentCountUVE + 1U, {1U},
                                               state, payload),
              ReliablePayloadReassemblyStatusUVE::Invalid);
    EXPECT_EQ(AcceptReliablePayloadFragmentUVE(20U, 0U, 2U, {1U}, state, payload),
              ReliablePayloadReassemblyStatusUVE::Accepted);
    EXPECT_EQ(AcceptReliablePayloadFragmentUVE(21U, 1U, 2U, {2U}, state, payload),
              ReliablePayloadReassemblyStatusUVE::Conflict);
    EXPECT_EQ(state.messageId, 20U);
    EXPECT_EQ(state.receivedFragmentCount, 1U);
    EXPECT_EQ(payload, (std::vector<std::uint8_t>{9U}));
}

TEST(ReliablePacketWindowUVETest, ReassemblyRejectsOverBudgetAggregateStateAtomically) {
    ReliablePayloadReassemblyStateUVE state;
    state.messageId = 23U;
    state.fragmentCount = 2U;
    state.receivedFragmentCount = 1U;
    state.receivedByteCount = kReliablePacketMaximumReassembledPayloadBytesUVE;
    state.fragments.resize(2U);
    state.fragments[0U] = {1U};
    std::vector<std::uint8_t> payload{9U};

    EXPECT_EQ(AcceptReliablePayloadFragmentUVE(23U, 1U, 2U, {2U}, state, payload),
              ReliablePayloadReassemblyStatusUVE::Invalid);
    EXPECT_EQ(state.receivedByteCount, kReliablePacketMaximumReassembledPayloadBytesUVE);
    EXPECT_EQ(state.receivedFragmentCount, 1U);
    EXPECT_TRUE(state.fragments[1U].empty());
    EXPECT_EQ(payload, (std::vector<std::uint8_t>{9U}));
}

TEST(ReliablePacketWindowUVETest, SingleFragmentPublishesCopiedPayload) {
    ReliablePayloadReassemblyStateUVE state;
    std::vector<std::uint8_t> payload;
    const std::vector<std::uint8_t> fragment{4U, 5U, 6U};

    EXPECT_EQ(AcceptReliablePayloadFragmentUVE(22U, 0U, 1U, fragment, state, payload),
              ReliablePayloadReassemblyStatusUVE::Complete);
    EXPECT_EQ(payload, fragment);
    EXPECT_FALSE(state.IsActiveUVE());
}

} // namespace
} // namespace UVE::Network::Tests
