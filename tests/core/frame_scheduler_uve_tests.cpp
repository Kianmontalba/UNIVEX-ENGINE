// Copyright (c) 2026 UniVex Studios. All Rights Reserved.

#include "uve/core/frame_scheduler_uve.h"

#include "uve/threading/thread_pool_uve.h"

#include <gtest/gtest.h>

#include <algorithm>
#include <mutex>
#include <stdexcept>
#include <vector>
#include <utility>

namespace UVE::Core {
namespace {

FrameTaskDefinitionUVE MakeTaskUVE(FrameTaskIdUVE id, const char* name,
                                  std::vector<FrameTaskIdUVE> dependencies,
                                  std::function<void()> action) {
    return FrameTaskDefinitionUVE{id, FrameTaskDomainUVE::Animation, name,
                                 std::move(dependencies), std::move(action)};
}

} // namespace

TEST(FrameTaskGraphUVETest, AddTaskUVE_RejectsDuplicateEdgesAndAcceptsCrossDomainTasks) {
    FrameTaskGraphUVE graph;
    EXPECT_TRUE(graph.AddTaskUVE(MakeTaskUVE(1U, "animation", {}, [] {})).IsAcceptedUVE());
    EXPECT_TRUE(graph.AddTaskUVE(FrameTaskDefinitionUVE{
        2U, FrameTaskDomainUVE::RenderPreparation, "render", {1U}, [] {}})
                    .IsAcceptedUVE());

    const FrameTaskGraphMutationResultUVE duplicate = graph.AddTaskUVE(
        MakeTaskUVE(3U, "duplicate-edge", {1U, 1U}, [] {}));
    EXPECT_EQ(duplicate.code, FrameTaskGraphMutationCodeUVE::DuplicateDependency);
    EXPECT_EQ(graph.GetTaskCountUVE(), 2U);
    EXPECT_TRUE(graph.ValidateUVE().IsValidUVE());
}

TEST(FrameTaskGraphUVETest, ValidateUVE_RejectsUnknownAndCyclicDependencies) {
    FrameTaskGraphUVE unknown;
    ASSERT_TRUE(unknown.AddTaskUVE(MakeTaskUVE(1U, "consumer", {99U}, [] {})).IsAcceptedUVE());
    EXPECT_EQ(unknown.ValidateUVE().code, FrameTaskGraphValidationCodeUVE::UnknownDependency);

    FrameTaskGraphUVE cyclic;
    ASSERT_TRUE(cyclic.AddTaskUVE(MakeTaskUVE(1U, "first", {2U}, [] {})).IsAcceptedUVE());
    ASSERT_TRUE(cyclic.AddTaskUVE(MakeTaskUVE(2U, "second", {1U}, [] {})).IsAcceptedUVE());
    EXPECT_EQ(cyclic.ValidateUVE().code, FrameTaskGraphValidationCodeUVE::CyclicDependency);
}

TEST(FrameSchedulerUVETest, ExecuteUVE_RunsDependenciesBeforeDependentsAcrossDomains) {
    FrameTaskGraphUVE graph;
    std::mutex orderMutex;
    std::vector<FrameTaskIdUVE> order;
    const auto record = [&orderMutex, &order](FrameTaskIdUVE id) {
        const std::lock_guard<std::mutex> lock(orderMutex);
        order.push_back(id);
    };

    ASSERT_TRUE(graph.AddTaskUVE(FrameTaskDefinitionUVE{
        1U, FrameTaskDomainUVE::Animation, "animation", {}, [&record] { record(1U); }})
                    .IsAcceptedUVE());
    ASSERT_TRUE(graph.AddTaskUVE(FrameTaskDefinitionUVE{
        2U, FrameTaskDomainUVE::Physics, "physics", {1U}, [&record] { record(2U); }})
                    .IsAcceptedUVE());
    ASSERT_TRUE(graph.AddTaskUVE(FrameTaskDefinitionUVE{
        3U, FrameTaskDomainUVE::RenderPreparation, "render", {1U, 2U},
        [&record] { record(3U); }})
                    .IsAcceptedUVE());

    Threading::ThreadPoolUVE pool(2U);
    const FrameScheduleResultUVE result = FrameSchedulerUVE(pool).ExecuteUVE(graph);

    ASSERT_TRUE(result.IsCompletedUVE());
    EXPECT_EQ(result.completedTaskCount, 3U);
    EXPECT_EQ(result.taskCount, 3U);
    ASSERT_EQ(order.size(), 3U);
    EXPECT_EQ(order[0], 1U);
    EXPECT_EQ(order[1], 2U);
    EXPECT_EQ(order[2], 3U);
}

TEST(FrameSchedulerUVETest, ExecuteUVE_ContainsTaskFailureAndDoesNotRunDependents) {
    FrameTaskGraphUVE graph;
    bool dependentRan = false;
    ASSERT_TRUE(graph.AddTaskUVE(MakeTaskUVE(1U, "failing", {}, [] {
        throw std::runtime_error("synthetic task failure");
    }))
                    .IsAcceptedUVE());
    ASSERT_TRUE(graph.AddTaskUVE(MakeTaskUVE(2U, "dependent", {1U}, [&dependentRan] {
        dependentRan = true;
    }))
                    .IsAcceptedUVE());

    Threading::ThreadPoolUVE pool(2U);
    const FrameScheduleResultUVE result = FrameSchedulerUVE(pool).ExecuteUVE(graph);

    EXPECT_EQ(result.code, FrameScheduleResultCodeUVE::TaskFailed);
    EXPECT_EQ(result.completedTaskCount, 0U);
    EXPECT_EQ(result.taskCount, 2U);
    EXPECT_FALSE(dependentRan);
    EXPECT_EQ(result.message, "synthetic task failure");
}

TEST(FrameSchedulerUVETest, ExecuteUVE_RejectsEmptyGraphBeforeSubmittingWork) {
    FrameTaskGraphUVE graph;
    Threading::ThreadPoolUVE pool(1U);

    const FrameScheduleResultUVE result = FrameSchedulerUVE(pool).ExecuteUVE(graph);

    EXPECT_EQ(result.code, FrameScheduleResultCodeUVE::InvalidGraph);
    EXPECT_EQ(result.completedTaskCount, 0U);
    EXPECT_EQ(result.taskCount, 0U);
}

} // namespace UVE::Core
