//------------------------------------------------------------------------------
// UniVex Engine (UVE) — Proprietary Game Engine
// Copyright (c) 2026 UniVex Studios. All Rights Reserved.
// Unauthorized copying, modification, distribution, or use of this code
// in whole or in part is strictly prohibited without express written
// permission from UniVex Studios.
// Violators will be prosecuted to the fullest extent of the law.
//------------------------------------------------------------------------------

#include "uve/debug/log_sink_uve.h"

#include <ctime>
#include <iomanip>
#include <iostream>
#include <sstream>

namespace UVE::Debug {

namespace {

/// Formats a time_point as an ISO-8601 UTC timestamp ("YYYY-MM-DDTHH:MM:SSZ").
/// Uses gmtime_r (POSIX, thread-safe) rather than std::gmtime, since this
/// increment targets Linux only.
std::string FormatTimestampUVE(std::chrono::system_clock::time_point timePoint) {
    const std::time_t timeT = std::chrono::system_clock::to_time_t(timePoint);
    std::tm tmValue{};
    gmtime_r(&timeT, &tmValue);
    std::ostringstream stream;
    stream << std::put_time(&tmValue, "%Y-%m-%dT%H:%M:%SZ");
    return stream.str();
}

/// Writes the shared human-readable line format used by both ConsoleSinkUVE
/// and FileSinkUVE: "[LEVEL] [category] message (file:line)". `category` and
/// the source-location suffix are omitted when not present on the message.
void WriteLineUVE(std::ostream& out, const LogMessageUVE& message, bool includeTimestamp) {
    if (includeTimestamp) {
        out << '[' << FormatTimestampUVE(message.timestamp) << "] ";
    }
    out << '[' << ToStringUVE(message.level) << "] ";
    if (!message.category.empty()) {
        out << '[' << message.category << "] ";
    }
    out << message.message;
    if (message.sourceFile != nullptr) {
        out << " (" << message.sourceFile << ':' << message.sourceLine << ')';
    }
    out << '\n';
}

} // namespace

void ConsoleSinkUVE::Write(const LogMessageUVE& message) {
    std::ostream& out = (message.level >= LogLevelUVE::Warning) ? std::cerr : std::cout;
    const std::lock_guard<std::mutex> lock(m_mutex);
    WriteLineUVE(out, message, /*includeTimestamp=*/false);
}

void ConsoleSinkUVE::Flush() {
    const std::lock_guard<std::mutex> lock(m_mutex);
    std::cout.flush();
    std::cerr.flush();
}

FileSinkUVE::FileSinkUVE(const std::filesystem::path& filePath)
    : m_file(filePath, std::ios::out | std::ios::app) {
    // Deliberately does not assert/throw if the file failed to open: sinks
    // are leaf components with no dependency on the logging/assert
    // machinery that is itself built on top of them. A file that failed to
    // open simply results in Write() silently discarding messages (see
    // class doc comment on FileSinkUVE).
}

FileSinkUVE::~FileSinkUVE() {
    Flush();
}

void FileSinkUVE::Write(const LogMessageUVE& message) {
    const std::lock_guard<std::mutex> lock(m_mutex);
    if (!m_file.is_open()) {
        return;
    }
    WriteLineUVE(m_file, message, /*includeTimestamp=*/true);
    if (message.level == LogLevelUVE::Fatal) {
        m_file.flush();
    }
}

void FileSinkUVE::Flush() {
    const std::lock_guard<std::mutex> lock(m_mutex);
    if (m_file.is_open()) {
        m_file.flush();
    }
}

void MemorySinkUVE::Write(const LogMessageUVE& message) {
    const std::lock_guard<std::mutex> lock(m_mutex);
    m_messages.push_back(message);
}

void MemorySinkUVE::Flush() {
    // No buffering to flush; messages are already stored on Write().
}

std::vector<LogMessageUVE> MemorySinkUVE::GetMessagesUVE() const {
    const std::lock_guard<std::mutex> lock(m_mutex);
    return m_messages;
}

void MemorySinkUVE::Clear() {
    const std::lock_guard<std::mutex> lock(m_mutex);
    m_messages.clear();
}

} // namespace UVE::Debug
