//------------------------------------------------------------------------------
// UniVex Engine (UVE) — Proprietary Game Engine
// Copyright (c) 2026 UniVex Studios. All Rights Reserved.
// Unauthorized copying, modification, distribution, or use of this code
// in whole or in part is strictly prohibited without express written
// permission from UniVex Studios.
// Violators will be prosecuted to the fullest extent of the law.
//------------------------------------------------------------------------------

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
        --m_pendingCount;
    }
    m_condVar.notify_all();
}

void JobCounterUVE::WaitUVE() {
    std::unique_lock<std::mutex> lock(m_mutex);
    m_condVar.wait(lock, [this] { return m_pendingCount == 0; });
}

} // namespace UVE::Threading
