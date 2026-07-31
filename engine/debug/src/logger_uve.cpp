//------------------------------------------------------------------------------
// UniVex Engine (UVE) — Proprietary Game Engine
// Copyright (c) 2026 UniVex Studios. All Rights Reserved.
// Unauthorized copying, modification, distribution, or use of this code
// in whole or in part is strictly prohibited without express written
// permission from UniVex Studios.
// Violators will be prosecuted to the fullest extent of the law.
//------------------------------------------------------------------------------

#include "uve/debug/logger_uve.h"

#include <chrono>
#include <thread>
#include <utility>

namespace UVE::Debug {

std::atomic<LoggerUVE*> LoggerUVE::s_activeInstance{nullptr};

LoggerUVE::LoggerUVE() = default;

LoggerUVE::~LoggerUVE() {
    Shutdown();
}

void LoggerUVE::Init(LogLevelUVE minLevel) {
    m_minLevel.store(minLevel, std::memory_order_relaxed);
    s_activeInstance.store(this, std::memory_order_release);
}

void LoggerUVE::Shutdown() {
    LoggerUVE* expected = this;
    s_activeInstance.compare_exchange_strong(expected, nullptr, std::memory_order_acq_rel);

    const std::lock_guard<std::mutex> lock(m_sinkMutex);
    FlushAllSinksLocked();
    m_sinks.clear();
}

void LoggerUVE::AddSink(std::unique_ptr<ILogSinkUVE> sink) {
    const std::lock_guard<std::mutex> lock(m_sinkMutex);
    m_sinks.push_back(std::move(sink));
}

void LoggerUVE::SetMinLevel(LogLevelUVE level) {
    m_minLevel.store(level, std::memory_order_relaxed);
}

LogLevelUVE LoggerUVE::GetMinLevel() const {
    return m_minLevel.load(std::memory_order_relaxed);
}

void LoggerUVE::LogFormatted(LogLevelUVE level,
                              std::string_view category,
                              const char* sourceFile,
                              int sourceLine,
                              std::string formattedMessage) {
    if (level < GetMinLevel()) {
        return;
    }

    LogMessageUVE message;
    message.level = level;
    message.category = std::string(category);
    message.message = std::move(formattedMessage);
    message.sourceFile = sourceFile;
    message.sourceLine = sourceLine;
    message.timestamp = std::chrono::system_clock::now();
    message.threadId = std::this_thread::get_id();

    const std::lock_guard<std::mutex> lock(m_sinkMutex);
    for (const std::unique_ptr<ILogSinkUVE>& sink : m_sinks) {
        sink->Write(message);
    }
    if (level == LogLevelUVE::Fatal) {
        FlushAllSinksLocked();
    }
}

void LoggerUVE::FlushAllSinksLocked() {
    for (const std::unique_ptr<ILogSinkUVE>& sink : m_sinks) {
        sink->Flush();
    }
}

LoggerUVE* LoggerUVE::GetActiveInstanceUVE() noexcept {
    return s_activeInstance.load(std::memory_order_acquire);
}

} // namespace UVE::Debug
