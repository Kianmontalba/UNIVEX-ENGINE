#include "uve/window/monitor_info_validation_uve.h"

#include <gtest/gtest.h>

namespace UVE::Window::Tests {

TEST(MonitorInfoValidationUVETest, AcceptsValidSnapshotWithAtMostOnePrimary) {
    EXPECT_TRUE(ValidateMonitorInfoUVE({"Primary", 1920U, 1080U, true}));
    EXPECT_TRUE(ValidateMonitorSnapshotUVE({
        {"Primary", 1920U, 1080U, true}, {"Secondary", 1280U, 1024U, false}}));
    EXPECT_TRUE(ValidateMonitorSnapshotUVE({}));
}

TEST(MonitorInfoValidationUVETest, RejectsInvalidEntryAndDuplicatePrimary) {
    EXPECT_FALSE(ValidateMonitorInfoUVE({"", 1920U, 1080U, true}));
    EXPECT_FALSE(ValidateMonitorInfoUVE({"Primary", 0U, 1080U, true}));
    EXPECT_FALSE(ValidateMonitorSnapshotUVE({
        {"Primary", 1920U, 1080U, true}, {"Secondary", 1280U, 720U, true}}));
}

} // namespace UVE::Window::Tests
