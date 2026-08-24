// Copyright (c) 2026 UniVex Studios. All Rights Reserved.
#include "uve/scene/prefab_revision_policy_uve.h"
#include "uve/scene/components/prefab_instance_component_uve.h"
#include <gtest/gtest.h>
namespace UVE::Scene::Tests {
namespace {
TEST(PrefabRevisionPolicyUVETest, EqualNonzeroRevisionsAreCurrent) {
    EXPECT_EQ(EvaluatePrefabRevisionUVE(7U, 7U), PrefabRevisionStatusUVE::Current);
}
TEST(PrefabRevisionPolicyUVETest, MismatchedNonzeroRevisionsAreStale) {
    EXPECT_EQ(EvaluatePrefabRevisionUVE(8U, 7U), PrefabRevisionStatusUVE::Stale);
}
TEST(PrefabRevisionPolicyUVETest, ZeroRevisionIsInvalid) {
    EXPECT_EQ(EvaluatePrefabRevisionUVE(0U, 7U), PrefabRevisionStatusUVE::Invalid);
    EXPECT_EQ(EvaluatePrefabRevisionUVE(7U, 0U), PrefabRevisionStatusUVE::Invalid);
}
TEST(PrefabRevisionPolicyUVETest, RefreshPrefabInstanceRevisionUVE_UpdatesBothRevisionsWithoutOverrides) {
    PrefabInstanceComponentUVE instance{Asset::AssetGuidUVE{7U}, {}, 3U, 3U};

    EXPECT_TRUE(RefreshPrefabInstanceRevisionUVE(instance, 9U));
    EXPECT_EQ(instance.sourceRevision, 9U);
    EXPECT_EQ(instance.instanceRevision, 9U);
}
TEST(PrefabRevisionPolicyUVETest, RefreshPrefabInstanceRevisionUVE_RejectsLocalOverridesAtomically) {
    PrefabInstanceComponentUVE instance{Asset::AssetGuidUVE{8U}, {{"Transform.position", "[1,2,3]"}}, 3U, 3U};

    EXPECT_FALSE(RefreshPrefabInstanceRevisionUVE(instance, 9U));
    EXPECT_EQ(instance.sourceRevision, 3U);
    EXPECT_EQ(instance.instanceRevision, 3U);
    ASSERT_EQ(instance.overrides.size(), 1U);
    EXPECT_EQ(instance.overrides.front().serializedValue, "[1,2,3]");
}
TEST(PrefabRevisionPolicyUVETest, RefreshPrefabInstanceRevisionUVE_RejectsRevisionRegressionAtomically) {
    PrefabInstanceComponentUVE instance{Asset::AssetGuidUVE{9U}, {}, 7U, 7U};

    EXPECT_FALSE(RefreshPrefabInstanceRevisionUVE(instance, 6U));
    EXPECT_EQ(instance.sourceRevision, 7U);
    EXPECT_EQ(instance.instanceRevision, 7U);
}
TEST(PrefabRevisionPolicyUVETest, EvaluatePrefabRevisionRefreshDecisionUVE_DistinguishesNoOpRefreshAndMerge) {
    EXPECT_EQ(EvaluatePrefabRevisionRefreshDecisionUVE(7U, 7U, false),
              PrefabRevisionRefreshDecisionUVE::NoOp);
    EXPECT_EQ(EvaluatePrefabRevisionRefreshDecisionUVE(9U, 7U, false),
              PrefabRevisionRefreshDecisionUVE::Refresh);
    EXPECT_EQ(EvaluatePrefabRevisionRefreshDecisionUVE(9U, 7U, true),
              PrefabRevisionRefreshDecisionUVE::MergeRequired);
}
TEST(PrefabRevisionPolicyUVETest, EvaluatePrefabRevisionRefreshDecisionUVE_RejectsInvalidRevisionOrder) {
    EXPECT_EQ(EvaluatePrefabRevisionRefreshDecisionUVE(0U, 7U, false),
              PrefabRevisionRefreshDecisionUVE::Invalid);
    EXPECT_EQ(EvaluatePrefabRevisionRefreshDecisionUVE(6U, 7U, false),
              PrefabRevisionRefreshDecisionUVE::Invalid);
}
} // namespace
} // namespace UVE::Scene::Tests
