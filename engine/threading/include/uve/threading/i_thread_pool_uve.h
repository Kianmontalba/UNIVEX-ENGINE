//------------------------------------------------------------------------------
// UniVex Engine (UVE) — Proprietary Game Engine
// Copyright (c) 2026 UniVex Studios. All Rights Reserved.
// Unauthorized copying, modification, distribution, or use of this code
// in whole or in part is strictly prohibited without express written
// permission from UniVex Studios.
// Violators will be prosecuted to the fullest extent of the law.
//------------------------------------------------------------------------------

#pragma once

#include <cstddef>
#include <cstdint>
#include <functional>

#include "uve/threading/job_counter_uve.h"

namespace UVE::Threading {

/// JobUVE is the engine's one callable-job type: a zero-argument, no-return unit of work
/// submitted to an IThreadPoolUVE. Used consistently across the public API and internal queue
/// storage — a seam for a future non-std::function job representation without changing any
/// call site's signature.
using JobUVE = std::function<void()>;

/// IThreadPoolUVE is the interface for the engine's job system. Exists so future builds
/// (dedicated server, editor, tests) can substitute a different implementation without changing
/// any call site that consumes an IThreadPoolUVE&. Unlike IEventSystemUVE, no per-call type
/// parameter is needed here — every job is already uniformly JobUVE — so these are plain
/// virtuals with no type-erased-primitive layer underneath.
/// Thread-safety: implementations must support every method being called concurrently from any
/// thread, including SubmitUVE() called from inside a currently-running job (recursive/nested
/// submission — see ThreadPoolUVE).
class IThreadPoolUVE {
public:
    virtual ~IThreadPoolUVE() = default;

    /// Submits `job` for execution on some worker thread. Fire-and-forget: there is no way to
    /// wait on this specific job via this overload — use the counter overload below if the
    /// caller needs to know when it (and/or a batch of others) has finished.
    virtual void SubmitUVE(JobUVE job) = 0;

    /// Submits `job` for execution on some worker thread, incrementing `counter` before the job
    /// can possibly start running and decrementing it (waking anyone blocked in
    /// `counter.WaitUVE()`) once the job finishes — whether it completed normally or threw.
    /// `counter` must outlive the job's execution.
    virtual void SubmitUVE(JobUVE job, JobCounterUVE& counter) = 0;

    /// Number of worker threads this pool owns.
    [[nodiscard]] virtual std::size_t GetWorkerCountUVE() const noexcept = 0;

    /// Number of jobs accepted (via either SubmitUVE overload) but not yet finished executing.
    /// Debugging/query only.
    [[nodiscard]] virtual std::size_t GetPendingJobCountUVE() const noexcept = 0;

    /// Number of worker threads currently executing a job (as opposed to idle/waiting/stealing).
    /// Debugging/query only; never exceeds GetWorkerCountUVE().
    [[nodiscard]] virtual std::size_t GetActiveWorkerCountUVE() const noexcept = 0;

    /// Total number of jobs ever picked up via the work-stealing path (as opposed to a worker
    /// popping from its own queue). Debugging/query only.
    [[nodiscard]] virtual std::uint64_t GetStolenJobCountUVE() const noexcept = 0;

    /// True iff called from one of this pool's own worker threads. Detection only — nothing in
    /// this increment changes behavior based on it, but future systems can use it to, for
    /// example, avoid a blocking call from within a worker.
    [[nodiscard]] virtual bool IsWorkerThreadUVE() const noexcept = 0;
};

} // namespace UVE::Threading
