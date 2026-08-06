// Copyright (c) 2026 UniVex Studios. All Rights Reserved.


#include "uve/threading/thread_pool_uve.h"

#include <algorithm>
#include <atomic>
#include <chrono>
#include <cstddef>
#include <memory>
#include <stdexcept>
#include <string>
#include <thread>
#include <vector>

#include <gtest/gtest.h>

#include "uve/debug/log_sink_uve.h"
#include "uve/debug/logger_uve.h"
#include "uve/threading/job_counter_uve.h"

namespace UVE::Threading::Tests {
namespace {

TEST(ThreadPoolUVETest, ExplicitWorkerCount_IsHonored) {
    ThreadPoolUVE pool(3);
    EXPECT_EQ(pool.GetWorkerCountUVE(), 3U);
}

TEST(ThreadPoolUVETest, AutoWorkerCount_ResolvesToAtLeastOne) {
    ThreadPoolUVE pool(0);
    EXPECT_GE(pool.GetWorkerCountUVE(), 1U);
}

TEST(ThreadPoolUVETest, SubmitWithCounter_AllJobsRunExactlyOnce) {
    ThreadPoolUVE pool(4);
    JobCounterUVE counter;
    std::atomic<int> completed{0};
    constexpr int kJobCount = 50;

    for (int i = 0; i < kJobCount; ++i) {
        pool.SubmitUVE([&completed] { completed.fetch_add(1, std::memory_order_relaxed); }, counter);
    }
    counter.WaitUVE();

    EXPECT_EQ(completed.load(), kJobCount);
}

TEST(ThreadPoolUVETest, SubmitWithCounter_HighVolumeStress) {
    ThreadPoolUVE pool(4);
    JobCounterUVE counter;
    std::atomic<int> completed{0};
    constexpr int kJobCount = 1000;

    for (int i = 0; i < kJobCount; ++i) {
        pool.SubmitUVE([&completed] { completed.fetch_add(1, std::memory_order_relaxed); }, counter);
    }
    counter.WaitUVE();

    EXPECT_EQ(completed.load(), kJobCount);
}

TEST(ThreadPoolUVETest, SubmitWithoutCounter_JobEventuallyRuns) {
    ThreadPoolUVE pool(2);
    std::atomic<bool> ran{false};
    pool.SubmitUVE([&ran] { ran.store(true, std::memory_order_relaxed); });

    const auto deadline = std::chrono::steady_clock::now() + std::chrono::seconds(1);
    while (!ran.load(std::memory_order_relaxed) && std::chrono::steady_clock::now() < deadline) {
        std::this_thread::sleep_for(std::chrono::milliseconds(1));
    }
    EXPECT_TRUE(ran.load());
}

TEST(ThreadPoolUVETest, JobsRunOnWorkerThread_NotCallingThread) {
    ThreadPoolUVE pool(2);
    JobCounterUVE counter;
    const std::thread::id callingThreadId = std::this_thread::get_id();
    std::thread::id jobThreadId{};

    pool.SubmitUVE([&jobThreadId] { jobThreadId = std::this_thread::get_id(); }, counter);
    counter.WaitUVE();

    EXPECT_NE(jobThreadId, callingThreadId);
}

TEST(ThreadPoolUVETest, RecursiveSubmission_CompletesWithoutDeadlock) {
    ThreadPoolUVE pool(2);
    JobCounterUVE outerCounter;
    std::atomic<int> innerCompleted{0};

    pool.SubmitUVE(
        [&pool, &innerCompleted] {
            JobCounterUVE innerCounter;
            for (int i = 0; i < 5; ++i) {
                pool.SubmitUVE(
                    [&innerCompleted] { innerCompleted.fetch_add(1, std::memory_order_relaxed); },
                    innerCounter);
            }
            innerCounter.WaitUVE();
        },
        outerCounter);

    outerCounter.WaitUVE();
    EXPECT_EQ(innerCompleted.load(), 5);
}

TEST(ThreadPoolUVETest, DestructorDrainsPendingWork) {
    std::atomic<int> completed{0};
    constexpr int kJobCount = 20;
    {
        ThreadPoolUVE pool(2);
        for (int i = 0; i < kJobCount; ++i) {
            pool.SubmitUVE([&completed] {
                std::this_thread::sleep_for(std::chrono::milliseconds(1));
                completed.fetch_add(1, std::memory_order_relaxed);
            });
        }
        // pool destructs here, blocking until every worker has drained its queue and joined.
    }
    EXPECT_EQ(completed.load(), kJobCount);
}

TEST(ThreadPoolUVETest, ExceptionInJob_IsCaughtAndLoggedAsError_PoolKeepsRunning) {
    Debug::LoggerUVE logger;
    logger.Init(Debug::LogLevelUVE::Trace);
    auto memorySink = std::make_unique<Debug::MemorySinkUVE>();
    Debug::MemorySinkUVE* const memorySinkPtr = memorySink.get();
    logger.AddSink(std::move(memorySink));

    ThreadPoolUVE pool(2);
    JobCounterUVE counter;
    std::atomic<bool> normalJobRan{false};

    pool.SubmitUVE([] { throw std::runtime_error("boom"); }, counter);
    pool.SubmitUVE([&normalJobRan] { normalJobRan.store(true, std::memory_order_relaxed); }, counter);
    counter.WaitUVE();

    EXPECT_TRUE(normalJobRan.load());

    const std::vector<Debug::LogMessageUVE> messages = memorySinkPtr->GetMessagesUVE();
    const bool foundExceptionError =
        std::any_of(messages.begin(), messages.end(), [](const Debug::LogMessageUVE& message) {
            return message.level == Debug::LogLevelUVE::Error &&
                   message.message.find("boom") != std::string::npos;
        });
    EXPECT_TRUE(foundExceptionError);

    logger.Shutdown();
}

TEST(ThreadPoolUVETest, UnknownExceptionInJob_IsCaughtAndLoggedGenerically) {
    Debug::LoggerUVE logger;
    logger.Init(Debug::LogLevelUVE::Trace);
    auto memorySink = std::make_unique<Debug::MemorySinkUVE>();
    Debug::MemorySinkUVE* const memorySinkPtr = memorySink.get();
    logger.AddSink(std::move(memorySink));

    ThreadPoolUVE pool(2);
    JobCounterUVE counter;
    pool.SubmitUVE([] { throw 42; }, counter);
    counter.WaitUVE();

    const std::vector<Debug::LogMessageUVE> messages = memorySinkPtr->GetMessagesUVE();
    const bool foundGenericError =
        std::any_of(messages.begin(), messages.end(), [](const Debug::LogMessageUVE& message) {
            return message.level == Debug::LogLevelUVE::Error &&
                   message.message.find("unknown") != std::string::npos;
        });
    EXPECT_TRUE(foundGenericError);

    logger.Shutdown();
}

TEST(ThreadPoolUVETest, DebugStats_PendingCountRisesAndFallsBackToZero) {
    ThreadPoolUVE pool(1);
    JobCounterUVE counter;
    std::atomic<bool> canProceed{false};

    pool.SubmitUVE(
        [&canProceed] {
            while (!canProceed.load(std::memory_order_relaxed)) {
                std::this_thread::sleep_for(std::chrono::milliseconds(1));
            }
        },
        counter);

    // GetPendingJobCountUVE() is incremented synchronously inside SubmitUVE(), before the job is
    // even picked up by a worker, so this is observable immediately without a race.
    EXPECT_GE(pool.GetPendingJobCountUVE(), 1U);

    canProceed.store(true, std::memory_order_relaxed);
    counter.WaitUVE();
    EXPECT_EQ(pool.GetPendingJobCountUVE(), 0U);
}

TEST(ThreadPoolUVETest, DebugStats_ActiveWorkerCountNeverExceedsWorkerCount) {
    ThreadPoolUVE pool(3);
    JobCounterUVE counter;
    std::atomic<bool> canProceed{false};
    std::atomic<std::size_t> maxObservedActive{0};

    for (int i = 0; i < 6; ++i) {
        pool.SubmitUVE(
            [&canProceed] {
                while (!canProceed.load(std::memory_order_relaxed)) {
                    std::this_thread::sleep_for(std::chrono::milliseconds(1));
                }
            },
            counter);
    }

    for (int i = 0; i < 20; ++i) {
        const std::size_t active = pool.GetActiveWorkerCountUVE();
        std::size_t currentMax = maxObservedActive.load(std::memory_order_relaxed);
        while (active > currentMax && !maxObservedActive.compare_exchange_weak(currentMax, active)) {
        }
        std::this_thread::sleep_for(std::chrono::milliseconds(2));
    }

    canProceed.store(true, std::memory_order_relaxed);
    counter.WaitUVE();

    EXPECT_LE(maxObservedActive.load(), pool.GetWorkerCountUVE());
}

TEST(ThreadPoolUVETest, IsWorkerThreadUVE_TrueInsideJobFalseOnCallingThread) {
    ThreadPoolUVE pool(2);
    JobCounterUVE counter;
    std::atomic<bool> wasWorkerThreadInsideJob{false};

    EXPECT_FALSE(pool.IsWorkerThreadUVE());

    pool.SubmitUVE(
        [&pool, &wasWorkerThreadInsideJob] {
            wasWorkerThreadInsideJob.store(pool.IsWorkerThreadUVE(), std::memory_order_relaxed);
        },
        counter);
    counter.WaitUVE();

    EXPECT_TRUE(wasWorkerThreadInsideJob.load());
}

TEST(ThreadPoolUVETest, StealingIsObservedUnderContention_RetriedToAvoidFlakiness) {
    // Timing-sensitive: this asserts scheduler behavior, not a pure logical invariant, so it is
    // retried a handful of times before failing (see docs/CODING_STANDARDS.md's guidance on
    // this class of test) — every other ThreadPoolUVE/JobCounterUVE test in this suite is fully
    // deterministic.
    constexpr int kMaxAttempts = 5;
    bool stealingObserved = false;

    for (int attempt = 0; attempt < kMaxAttempts && !stealingObserved; ++attempt) {
        ThreadPoolUVE pool(4);
        JobCounterUVE counter;

        // The first 3 submissions (slow) round-robin onto queues 0/1/2, leaving queue 3 with
        // none of its own — its worker should finish quickly and steal from a still-busy queue.
        for (int i = 0; i < 3; ++i) {
            pool.SubmitUVE([] { std::this_thread::sleep_for(std::chrono::milliseconds(20)); },
                           counter);
        }
        for (int i = 0; i < 60; ++i) {
            pool.SubmitUVE([] {}, counter);
        }
        counter.WaitUVE();

        stealingObserved = pool.GetStolenJobCountUVE() > 0;
    }

    EXPECT_TRUE(stealingObserved);
}

} // namespace
} // namespace UVE::Threading::Tests
