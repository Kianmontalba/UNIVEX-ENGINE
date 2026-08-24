#include "uve/window/monitor_info_validation_uve.h"

#include <limits>

#include <gtest/gtest.h>

namespace UVE::Window::Tests {

TEST(MonitorInfoValidationUVETest, ValidatesSignedDimensionsBeforeUnsignedConversionAtomically) {
    std::uint32_t width = 77U;
    std::uint32_t height = 88U;
    EXPECT_FALSE(ValidateMonitorDimensionsUVE(0, 1080, width, height));
    EXPECT_EQ(width, 77U);
    EXPECT_EQ(height, 88U);
    EXPECT_FALSE(ValidateMonitorDimensionsUVE(1920, -1, width, height));
    EXPECT_EQ(width, 77U);
    EXPECT_EQ(height, 88U);
    EXPECT_TRUE(ValidateMonitorDimensionsUVE(std::numeric_limits<int>::max(),
                                             std::numeric_limits<int>::max(), width, height));
    EXPECT_EQ(width, static_cast<std::uint32_t>(std::numeric_limits<int>::max()));
    EXPECT_EQ(height, static_cast<std::uint32_t>(std::numeric_limits<int>::max()));
}

TEST(MonitorInfoValidationUVETest, AcceptsValidSnapshotWithAtMostOnePrimary) {
    EXPECT_TRUE(ValidateMonitorInfoUVE({"Primary", 1920U, 1080U, true}));
    EXPECT_TRUE(ValidateMonitorSnapshotUVE({
        {"Primary", 1920U, 1080U, true}, {"Secondary", 1280U, 1024U, false}}));
    EXPECT_TRUE(ValidateMonitorSnapshotUVE({}));
}

TEST(MonitorInfoValidationUVETest, RejectsEmbeddedNulMonitorName) {
    const std::string nameWithNul{"Monitor\0Name", 12U};
    EXPECT_FALSE(ValidateMonitorInfoUVE({nameWithNul, 1920U, 1080U, false}));
}

TEST(MonitorInfoValidationUVETest, RejectsMonitorSnapshotBeyondBoundedEntryCap) {
    std::vector<MonitorInfoUVE> monitors;
    monitors.resize(kMaximumMonitorSnapshotEntriesUVE + 1U,
                    MonitorInfoUVE{"Monitor", 1920U, 1080U, false});
    EXPECT_FALSE(ValidateMonitorSnapshotUVE(monitors));
}

TEST(MonitorInfoValidationUVETest, RejectsInvalidEntryAndDuplicatePrimary) {
    EXPECT_FALSE(ValidateMonitorInfoUVE({"", 1920U, 1080U, true}));
    EXPECT_FALSE(ValidateMonitorInfoUVE({"Primary", 0U, 1080U, true}));
    EXPECT_FALSE(ValidateMonitorSnapshotUVE({
        {"Primary", 1920U, 1080U, true}, {"Secondary", 1280U, 720U, true}}));
}

} // namespace UVE::Window::Tests
