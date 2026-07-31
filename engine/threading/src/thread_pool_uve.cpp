//------------------------------------------------------------------------------
// UniVex Engine (UVE) — Proprietary Game Engine
// Copyright (c) 2026 UniVex Studios. All Rights Reserved.
// Unauthorized copying, modification, distribution, or use of this code
// in whole or in part is strictly prohibited without express written
// permission from UniVex Studios.
// Violators will be prosecuted to the fullest extent of the law.
//------------------------------------------------------------------------------

#include "uve/threading/thread_pool_uve.h"

#include <cstdio>
#include <exception>
#include <utility>

#if defined(__linux__)
#include <pthread.h>
#endif

#include "uve/debug/assert_uve.h"
#include "uve/debug/logging_macros_uve.h"

namespace UVE::Threading {

namespace {

thread_local const ThreadPoolUVE* t_currentThreadPool = nullptr;

#if defined(__linux__)
// Linux thread names are capped at 15 visible characters (16 bytes including the NUL
// terminator) — pthread_setname_np() itself enforces that (returning ERANGE, silently ignored
// here, if exceeded) — purely a debugging aid (visible in gdb/perf/htop), not load-bearing. The
// local buffer is sized for the true worst case (std::size_t can print up to 20 digits) purely
// so snprintf() itself can never be accused of truncating into it — GCC's -Wformat-truncation
// (an -O2+-only analysis, which is why this only surfaced in the Release build) cannot prove
// workerIndex stays small from this function's local context alone.
void SetWorkerThreadNameUVE(std::thread& worker, std::size_t workerIndex) {
    char name[32];
    std::snprintf(name, sizeof(name), "UVEWorker%zu", workerIndex);
    pthread_setname_np(worker.native_handle(), name);
}
#endif

} // namespace

ThreadPoolUVE::ThreadPoolUVE(std::size_t workerCount) {
    std::size_t resolvedWorkerCount = workerCount;
    if (resolvedWorkerCount == 0) {
        const unsigned int detected = std::thread::hardware_concurrency();
        resolvedWorkerCount = (detected > 1) ? static_cast<std::size_t>(detected - 1) : std::size_t{1};
    }

    m_queues.resize(resolvedWorkerCount);
    m_workers.reserve(resolvedWorkerCount);
    for (std::size_t workerIndex = 0; workerIndex < resolvedWorkerCount; ++workerIndex) {
        m_workers.emplace_back([this, workerIndex] { WorkerLoopUVE(workerIndex); });
#if defined(__linux__)
        SetWorkerThreadNameUVE(m_workers.back(), workerIndex);
#endif
    }
}

ThreadPoolUVE::~ThreadPoolUVE() {
    {
        const std::lock_guard<std::mutex> lock(m_mutex);
        m_stopRequested = true;
    }
    m_condVar.notify_all();
    for (std::thread& worker : m_workers) {
        worker.join();
    }
}

void ThreadPoolUVE::SubmitUVE(JobUVE job) {
    SubmitInternalUVE(QueuedJobUVE{std::move(job), nullptr});
}

void ThreadPoolUVE::SubmitUVE(JobUVE job, JobCounterUVE& counter) {
    counter.IncrementUVE();
    SubmitInternalUVE(QueuedJobUVE{std::move(job), &counter});
}

void ThreadPoolUVE::SubmitInternalUVE(QueuedJobUVE job) {
    const std::size_t workerCount = m_workers.size();
    UVE_ASSERT(workerCount > 0);
    const std::size_t targetQueue =
        m_nextSubmitIndex.fetch_add(1, std::memory_order_relaxed) % workerCount;

    m_pendingJobCount.fetch_add(1, std::memory_order_relaxed);
    {
        const std::lock_guard<std::mutex> lock(m_mutex);
        m_queues[targetQueue].push_back(std::move(job));
    }
    m_condVar.notify_one();
}

void ThreadPoolUVE::WorkerLoopUVE(std::size_t workerIndex) {
    t_currentThreadPool = this;

    while (true) {
        QueuedJobUVE job;
        {
            std::unique_lock<std::mutex> lock(m_mutex);
            m_condVar.wait(lock, [this] { return m_stopRequested || HasAnyWorkLockedUVE(); });

            if (!HasAnyWorkLockedUVE()) {
                return; // stop requested and genuinely nothing left anywhere
            }

            if (!m_queues[workerIndex].empty()) {
                job = std::move(m_queues[workerIndex].back());
                m_queues[workerIndex].pop_back();
            } else {
                const std::size_t victimIndex = FindStealVictimLockedUVE(workerIndex);
                job = std::move(m_queues[victimIndex].front());
                m_queues[victimIndex].pop_front();
                m_stolenJobCount.fetch_add(1, std::memory_order_relaxed);
            }
        }
        RunJobUVE(job);
    }
}

void ThreadPoolUVE::RunJobUVE(QueuedJobUVE& job) {
    m_activeWorkerCount.fetch_add(1, std::memory_order_relaxed);

    // A job throwing must never escape RunJobUVE(): letting it unwind through WorkerLoopUVE()
    // would terminate this worker thread, permanently shrinking the pool. Catch, log, move on.
    try {
        job.function();
    } catch (const std::exception& exception) {
        UVE_ERROR("ThreadPoolUVE: job threw an exception: {}", exception.what());
    } catch (...) {
        UVE_ERROR("ThreadPoolUVE: job threw an unknown, non-std::exception value");
    }

    m_activeWorkerCount.fetch_sub(1, std::memory_order_relaxed);

    if (job.counter != nullptr) {
        job.counter->DecrementAndNotifyUVE();
    }
    m_pendingJobCount.fetch_sub(1, std::memory_order_relaxed);
}

bool ThreadPoolUVE::HasAnyWorkLockedUVE() const noexcept {
    for (const std::deque<QueuedJobUVE>& queue : m_queues) {
        if (!queue.empty()) {
            return true;
        }
    }
    return false;
}

std::size_t ThreadPoolUVE::FindStealVictimLockedUVE(std::size_t workerIndex) const noexcept {
    const std::size_t queueCount = m_queues.size();
    for (std::size_t offset = 1; offset < queueCount; ++offset) {
        const std::size_t candidateIndex = (workerIndex + offset) % queueCount;
        if (!m_queues[candidateIndex].empty()) {
            return candidateIndex;
        }
    }
    UVE_ASSERT(false && "FindStealVictimLockedUVE called with no stealable work available");
    return workerIndex;
}

std::size_t ThreadPoolUVE::GetWorkerCountUVE() const noexcept {
    return m_workers.size();
}

std::size_t ThreadPoolUVE::GetPendingJobCountUVE() const noexcept {
    return m_pendingJobCount.load(std::memory_order_relaxed);
}

std::size_t ThreadPoolUVE::GetActiveWorkerCountUVE() const noexcept {
    return m_activeWorkerCount.load(std::memory_order_relaxed);
}

std::uint64_t ThreadPoolUVE::GetStolenJobCountUVE() const noexcept {
    return m_stolenJobCount.load(std::memory_order_relaxed);
}

bool ThreadPoolUVE::IsWorkerThreadUVE() const noexcept {
    return t_currentThreadPool == this;
}

} // namespace UVE::Threading
