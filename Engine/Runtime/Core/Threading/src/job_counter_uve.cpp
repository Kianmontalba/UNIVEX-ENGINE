// Copyright (c) 2026 UniVex Studios. All Rights Reserved.


#include "uve/threading/job_counter_uve.h"

#include "uve/debug/assert_uve.h"

namespace UVE::Threading {

void JobCounterUVE::IncrementUVE() {
    const std::lock_guard<std::mutex> lock(m_mutex);
    ++m_pendingCount;
}

void JobCounterUVE::DecrementAndNotifyUVE() {
    const std::lock_guard<std::mutex> lock(m_mutex);
    UVE_ASSERT(m_pendingCount > 0);
    if (m_pendingCount <= 0) {
        UVE_ERROR("JobCounterUVE: DecrementAndNotifyUVE called more times than IncrementUVE "
                   "(double-decrement bug) — ignoring to avoid a permanent WaitUVE() hang");
    } else {
        --m_pendingCount;
    }
    // notify_all() must happen while still holding m_mutex, not after releasing it: the moment
    // the count reaches zero, a blocked WaitUVE() is free to return (its own std::condition_variable
    // predicate check reacquires m_mutex, sees zero, and returns without needing to have been
    // woken by this exact call) as soon as this lock is released - and the caller that owns this
    // JobCounterUVE (e.g. a stack-allocated one, or one embedded in a JobGraphUVE about to be
    // destroyed) is then free to destroy it. If notify_all() were called after unlocking, that
    // destruction could tear down m_condVar concurrently with this thread still calling
    // notify_all() on it - a real, observed (via ThreadSanitizer, under JobGraphUVE's own stress
    // test) destroyed-object race. Notifying under the lock closes that window: WaitUVE() cannot
    // resume past its own lock acquisition until this function has both updated the count AND
    // notified, i.e. until this function is about to return with nothing left to touch on `this`.
    m_condVar.notify_all();
}

void JobCounterUVE::WaitUVE() {
    std::unique_lock<std::mutex> lock(m_mutex);
    m_condVar.wait(lock, [this] { return m_pendingCount == 0; });
}

} // namespace UVE::Threading
