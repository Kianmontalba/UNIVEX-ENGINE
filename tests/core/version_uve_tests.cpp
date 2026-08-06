// Copyright (c) 2026 UniVex Studios. All Rights Reserved.


#include "uve/core/version_uve.h"

#include <gtest/gtest.h>

#include "uve/core/engine_core_uve.h"

namespace UVE::Core::Tests {
namespace {

TEST(VersionUVETest, ToStringUVE_FormatsAllFourComponents) {
    const VersionUVE version{1, 2, 3, 4};
    EXPECT_EQ(version.ToStringUVE(), "1.2.3+build.4");
}

TEST(VersionUVETest, EqualityOperators_CompareAllComponents) {
    const VersionUVE a{1, 2, 3, 4};
    const VersionUVE b{1, 2, 3, 4};
    const VersionUVE c{1, 2, 3, 5};

    EXPECT_EQ(a, b);
    EXPECT_NE(a, c);
}

TEST(VersionUVETest, EngineCoreUVE_ReturnsExpectedVersionConstant) {
    const VersionUVE version = EngineCoreUVE::GetEngineVersionUVE();
    EXPECT_EQ(version, (VersionUVE{0, 1, 0, 1}));
}

} // namespace
} // namespace UVE::Core::Tests
