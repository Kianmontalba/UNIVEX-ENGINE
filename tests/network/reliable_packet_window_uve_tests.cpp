// Copyright (c) 2026 UniVex Studios. All Rights Reserved.
#include "uve/network/reliable_packet_window_uve.h"
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
TEST(ReliablePacketWindowUVETest, SelectiveAcknowledgementClearsForwardSlots) {
    std::uint32_t pending = 0xFFFFFFFFU;
    const ReliablePacketHeaderUVE header{200U, 200U, (1U << 1U) | (1U << 4U)};
    EXPECT_TRUE(ApplyReliableAcknowledgementsUVE(header, pending, 200U));
    EXPECT_EQ(pending, ~(1U << 0U | 1U << 2U | 1U << 5U));
}
TEST(ReliablePacketWindowUVETest, InvalidAcknowledgementDoesNotMutatePendingMask) {
    std::uint32_t pending = 0xA5A5A5A5U;
    const ReliablePacketHeaderUVE header{1U, 0U, 0xFFFFFFFFU};
    EXPECT_FALSE(ApplyReliableAcknowledgementsUVE(header, pending, 1U));
    EXPECT_EQ(pending, 0xA5A5A5A5U);
}
} // namespace
} // namespace UVE::Network::Tests
