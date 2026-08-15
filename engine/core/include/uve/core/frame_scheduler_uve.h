// Copyright (c) 2026 UniVex Studios. All Rights Reserved.

#pragma once

#include "uve/threading/i_thread_pool_uve.h"

#include <cstddef>
#include <cstdint>
#include <functional>
#include <string>
#include <vector>

namespace UVE::Core {

enum class FrameTaskDomainUVE : std::uint8_t {
    Animation = 0,
    ECS,
    RenderPreparation,
    Physics,
    Streaming,
    Audio,
    Assets,
    Scripting,
    Editor,
    Count,
};

using FrameTaskIdUVE = std::uint32_t;

struct FrameTaskDefinitionUVE final {
    FrameTaskIdUVE id = 0U;
    FrameTaskDomainUVE domain = FrameTaskDomainUVE::Animation;
    std::string name;
    std::vector<FrameTaskIdUVE> dependencies;
    std::function<void()> action;
};

enum class FrameTaskGraphMutationCodeUVE : std::uint8_t {
    Accepted = 0,
    CapacityExceeded,
    InvalidId,
    DuplicateId,
    InvalidDomain,
    InvalidName,
    InvalidAction,
    TooManyDependencies,
    InvalidDependency,
    DuplicateDependency,
};

struct FrameTaskGraphMutationResultUVE final {
    FrameTaskGraphMutationCodeUVE code = FrameTaskGraphMutationCodeUVE::InvalidId;
    FrameTaskIdUVE taskId = 0U;
    std::string message;

    [[nodiscard]] bool IsAcceptedUVE() const noexcept {
        return code == FrameTaskGraphMutationCodeUVE::Accepted;
    }
};

enum class FrameTaskGraphValidationCodeUVE : std::uint8_t {
    Valid = 0,
    EmptyGraph,
    InvalidTask,
    UnknownDependency,
    CyclicDependency,
};

struct FrameTaskGraphValidationResultUVE final {
    FrameTaskGraphValidationCodeUVE code = FrameTaskGraphValidationCodeUVE::EmptyGraph;
    FrameTaskIdUVE taskId = 0U;
    std::string message;

    [[nodiscard]] bool IsValidUVE() const noexcept {
        return code == FrameTaskGraphValidationCodeUVE::Valid;
    }
};

class FrameTaskGraphUVE final {
public:
    static constexpr std::size_t kMaximumTasksUVE = 256U;
    static constexpr std::size_t kMaximumDependenciesPerTaskUVE = 32U;
    static constexpr std::size_t kMaximumTaskNameBytesUVE = 128U;

    [[nodiscard]] FrameTaskGraphMutationResultUVE AddTaskUVE(FrameTaskDefinitionUVE task);
    [[nodiscard]] FrameTaskGraphValidationResultUVE ValidateUVE() const;

    [[nodiscard]] std::size_t GetTaskCountUVE() const noexcept {
        return m_tasks.size();
    }

    [[nodiscard]] const std::vector<FrameTaskDefinitionUVE>& GetTasksUVE() const noexcept {
        return m_tasks;
    }

private:
    std::vector<FrameTaskDefinitionUVE> m_tasks;
};

enum class FrameScheduleResultCodeUVE : std::uint8_t {
    Completed = 0,
    InvalidGraph,
    TaskFailed,
    SubmissionFailed,
};

struct FrameScheduleResultUVE final {
    FrameScheduleResultCodeUVE code = FrameScheduleResultCodeUVE::InvalidGraph;
    std::size_t completedTaskCount = 0U;
    std::size_t taskCount = 0U;
    std::string message;

    [[nodiscard]] bool IsCompletedUVE() const noexcept {
        return code == FrameScheduleResultCodeUVE::Completed;
    }
};

class FrameSchedulerUVE final {
public:
    explicit FrameSchedulerUVE(Threading::IThreadPoolUVE& threadPool) noexcept
        : m_threadPool(threadPool) {}

    FrameSchedulerUVE(const FrameSchedulerUVE&) = delete;
    FrameSchedulerUVE& operator=(const FrameSchedulerUVE&) = delete;

    [[nodiscard]] FrameScheduleResultUVE ExecuteUVE(const FrameTaskGraphUVE& graph) const;

private:
    Threading::IThreadPoolUVE& m_threadPool;
};

} // namespace UVE::Core
