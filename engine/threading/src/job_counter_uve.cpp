// Copyright (c) 2026 UniVex Studios. All Rights Reserved.


#include "uve/threading/job_counter_uve.h"

#include "uve/debug/assert_uve.h"

namespace UVE::Threading {

void JobCounterUVE::IncrementUVE() {
    const std::lock_guard<std::mutex> lock(m_mutex);
    ++m_pendingCount;
}

void JobCounterUVE::DecrementAndNotifyUVE() {
    {
        const std::lock_guard<std::mutex> lock(m_mutex);
        UVE_ASSERT(m_pendingCount > 0);
        if (m_pendingCount <= 0) {
            UVE_ERROR("JobCounterUVE: DecrementAndNotifyUVE called more times than IncrementUVE "
                       "(double-decrement bug) — ignoring to avoid a permanent WaitUVE() hang");
        } else {
            --m_pendingCount;
        }
    }
    m_condVar.notify_all(); // still notify even on the error path, in case a legitimate waiter
                              // can now observe pendingCount == 0 from another thread's decrement
}

void JobCounterUVE::WaitUVE() {
    std::unique_lock<std::mutex> lock(m_mutex);
    m_condVar.wait(lock, [this] { return m_pendingCount == 0; });
}

} // namespace UVE::Threading
