// Copyright (c) 2026 UniVex Studios. All Rights Reserved.
#include "uve/scene/prefab_revision_policy_uve.h"
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
} // namespace
} // namespace UVE::Scene::Tests
