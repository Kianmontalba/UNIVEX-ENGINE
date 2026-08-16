// Copyright (c) 2026 UniVex Studios. All Rights Reserved.
#pragma once

#include "uve/plugins/motion_query_animation_node_uve.h"
#include "uve/plugins/motion_query_debugging_uve.h"
#include "uve/plugins/motion_query_editor_authoring_uve.h"
#include "uve/plugins/motion_query_live_debug_trace_persistence_uve.h"

#include <cstddef>
#include <cstdint>
#include <optional>
#include <string>
#include <vector>

namespace UVE::Plugins::Editor {

enum class MotionQueryLiveDebugCommandKindUVE : std::uint8_t {
    ReadSnapshot = 0,
    Attach,
    Detach,
    ClearTrace,
    ClearSession,
    SetFilter,
    SelectEvent,
    RemoveEvent,
    ToggleTraceEventPin,
    SetTraceEventComment,
    SetTraceEventCategory,
    ExportTrace,
    ImportTrace,
};

struct MotionQueryLiveDebugCommandUVE final {
    std::uint32_t protocolVersion = kMotionQueryEditorProtocolVersionUVE;
    std::uint64_t requestId = 0U;
    std::uint64_t expectedGeneration = 0U;
    MotionQueryLiveDebugCommandKindUVE kind = MotionQueryLiveDebugCommandKindUVE::ReadSnapshot;
    std::optional<UVE::Asset::ResourceHandleUVE> database;
    std::string filter;
    std::string payload;
    std::optional<std::uint64_t> eventSequence;
};

struct MotionQueryLiveDebugSnapshotUVE final {
    std::uint32_t protocolVersion = kMotionQueryEditorProtocolVersionUVE;
    std::uint64_t generation = 0U;
    bool active = false;
    std::optional<UVE::Asset::ResourceHandleUVE> database;
    std::string filter;
    std::size_t totalTraceEventCount = 0U;
    std::size_t visibleTraceEventCount = 0U;
    bool traceTruncated = false;
    MotionQueryDebuggerSnapshotUVE debugger;
    std::vector<MotionQueryTraceEventUVE> traceEvents;
    std::string diagnostic;

    [[nodiscard]] bool operator==(const MotionQueryLiveDebugSnapshotUVE&) const = default;
};

enum class MotionQueryLiveDebugResponseCodeUVE : std::uint8_t {
    Applied = 0,
    InvalidProtocol,
    StaleGeneration,
    InvalidCommand,
    InvalidDatabase,
    DatabaseNotFound,
    InvalidFilter,
};

struct MotionQueryLiveDebugResponseUVE final {
    std::uint32_t protocolVersion = kMotionQueryEditorProtocolVersionUVE;
    std::uint64_t requestId = 0U;
    bool applied = false;
    MotionQueryLiveDebugResponseCodeUVE code = MotionQueryLiveDebugResponseCodeUVE::InvalidCommand;
    std::string message;
    std::optional<std::string> payload;
    MotionQueryLiveDebugSnapshotUVE snapshot;
};

class MotionQueryLiveDebugSessionUVE final : public UVE::Plugins::IMotionQueryAnimationDebugSinkUVE {
public:
    [[nodiscard]] MotionQueryLiveDebugResponseUVE DispatchUVE(
        const MotionQueryLiveDebugCommandUVE& command,
        const MotionQueryEditorAuthoringSessionUVE& authoring) noexcept;

    void PublishUVE(const UVE::Plugins::MotionQueryAnimationNodeResultUVE& result,
                   std::uint64_t timestampNanoseconds,
                   std::uint64_t frameNumber) noexcept override;

    void ClearUVE() noexcept;

    [[nodiscard]] MotionQueryLiveDebugSnapshotUVE GetSnapshotUVE() const noexcept;

private:
    [[nodiscard]] MotionQueryLiveDebugResponseUVE MakeResponseUVE(
        const MotionQueryLiveDebugCommandUVE& command, bool applied,
        MotionQueryLiveDebugResponseCodeUVE code, std::string message) const noexcept;
    [[nodiscard]] static bool IsValidHandleUVE(UVE::Asset::ResourceHandleUVE resource) noexcept;
    [[nodiscard]] static bool ContainsFilterUVE(const MotionQueryTraceEventUVE& event,
                                                const std::string& filter) noexcept;
    [[nodiscard]] static const char* ResultKindUVE(
        UVE::Plugins::MotionQueryAnimationNodeCodeUVE code) noexcept;

    MotionQueryDebuggerUVE debugger_;
    MotionQueryTraceLoggerUVE trace_;
    std::optional<UVE::Asset::ResourceHandleUVE> database_;
    std::string filter_;
    std::uint64_t generation_ = 0U;
    bool active_ = false;
};

} // namespace UVE::Plugins::Editor
