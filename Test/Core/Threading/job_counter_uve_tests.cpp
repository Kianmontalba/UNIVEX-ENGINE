// Copyright (c) 2026 UniVex Studios. All Rights Reserved.


#include "uve/threading/job_counter_uve.h"

#include <atomic>
#include <chrono>
#include <thread>

#include <gtest/gtest.h>

#include "uve/platform/platform_uve.h"

namespace UVE::Threading::Tests {
namespace {

TEST(JobCounterUVETest, FreshCounter_WaitReturnsImmediately) {
    JobCounterUVE counter;
    counter.WaitUVE(); // must not hang: pending count starts at zero
}

TEST(JobCounterUVETest, IncrementThenDecrementFromAnotherThread_UnblocksWait) {
    JobCounterUVE counter;
    counter.IncrementUVE();

    std::thread worker([&counter] {
        std::this_thread::sleep_for(std::chrono::milliseconds(10));
        counter.DecrementAndNotifyUVE();
    });

    counter.WaitUVE(); // blocks until the worker thread decrements
    worker.join();
}

TEST(JobCounterUVETest, MultipleIncrementsRequireMatchingDecrements) {
    JobCounterUVE counter;
    counter.IncrementUVE();
    counter.IncrementUVE();
    counter.IncrementUVE();

    std::atomic<int> decrementsDone{0};
    std::thread worker([&counter, &decrementsDone] {
        for (int i = 0; i < 3; ++i) {
            std::this_thread::sleep_for(std::chrono::milliseconds(5));
            counter.DecrementAndNotifyUVE();
            ++decrementsDone;
        }
    });

    counter.WaitUVE();
    worker.join();
    EXPECT_EQ(decrementsDone.load(), 3);
}

#if UVE_DEBUG
TEST(JobCounterUVEDeathTest, DecrementWithoutIncrement_Asserts) {
    JobCounterUVE counter;
    EXPECT_DEATH({ counter.DecrementAndNotifyUVE(); }, "");
}
#else
TEST(JobCounterUVETest, DecrementWithoutIncrement_IgnoresExtraDecrementInsteadOfUnderflowing) {
    JobCounterUVE counter;
    EXPECT_NO_FATAL_FAILURE(counter.DecrementAndNotifyUVE());
    counter.WaitUVE(); // must not hang: the ignored extra decrement must not have gone negative

    // A legitimate subsequent increment/decrement pair still works correctly afterward.
    counter.IncrementUVE();
    counter.DecrementAndNotifyUVE();
    counter.WaitUVE();
}
#endif

} // namespace
} // namespace UVE::Threading::Tests
