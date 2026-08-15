// Copyright (c) 2026 UniVex Studios. All Rights Reserved.

#include "uve/core/frame_scheduler_uve.h"

#include <algorithm>
#include <condition_variable>
#include <exception>
#include <limits>
#include <memory>
#include <mutex>
#include <string>
#include <utility>
#include <vector>

namespace UVE::Core {
namespace {

[[nodiscard]] std::size_t FindTaskIndexUVE(const std::vector<FrameTaskDefinitionUVE>& tasks,
                                           FrameTaskIdUVE taskId) noexcept {
    for (std::size_t index = 0U; index < tasks.size(); ++index) {
        if (tasks[index].id == taskId) {
            return index;
        }
    }
    return tasks.size();
}

[[nodiscard]] bool IsValidDomainUVE(FrameTaskDomainUVE domain) noexcept {
    return static_cast<std::uint8_t>(domain) <
           static_cast<std::uint8_t>(FrameTaskDomainUVE::Count);
}

[[nodiscard]] FrameTaskGraphMutationResultUVE MakeMutationErrorUVE(
    FrameTaskGraphMutationCodeUVE code, FrameTaskIdUVE taskId, const char* message) {
    return FrameTaskGraphMutationResultUVE{code, taskId, message};
}

[[nodiscard]] FrameTaskGraphValidationResultUVE MakeValidationErrorUVE(
    FrameTaskGraphValidationCodeUVE code, FrameTaskIdUVE taskId, const char* message) {
    return FrameTaskGraphValidationResultUVE{code, taskId, message};
}

[[nodiscard]] std::string ExceptionMessageUVE() {
    try {
        throw;
    } catch (const std::exception& exception) {
        return exception.what();
    } catch (...) {
        return "unknown non-standard exception";
    }
}

struct SchedulerStateUVE final {
    explicit SchedulerStateUVE(const FrameTaskGraphUVE& graphIn,
                               Threading::IThreadPoolUVE& threadPoolIn)
        : graph(graphIn), threadPool(threadPoolIn),
          remainingDependencies(graphIn.GetTaskCountUVE(), 0U),
          scheduled(graphIn.GetTaskCountUVE(), false), completed(graphIn.GetTaskCountUVE(), false),
          dependents(graphIn.GetTaskCountUVE()) {}

    const FrameTaskGraphUVE& graph;
    Threading::IThreadPoolUVE& threadPool;
    std::vector<std::size_t> remainingDependencies;
    std::vector<bool> scheduled;
    std::vector<bool> completed;
    std::vector<std::vector<std::size_t>> dependents;
    std::mutex mutex;
    std::condition_variable condition;
    std::size_t pendingTaskCount = 0U;
    std::size_t completedTaskCount = 0U;
    bool failed = false;
    FrameScheduleResultCodeUVE failureCode = FrameScheduleResultCodeUVE::TaskFailed;
    std::string failureMessage;
};

void SubmitTaskUVE(const std::shared_ptr<SchedulerStateUVE>& state, std::size_t taskIndex);

void MarkSubmissionFailureUVE(const std::shared_ptr<SchedulerStateUVE>& state,
                              const std::string& message) {
    std::lock_guard<std::mutex> lock(state->mutex);
    state->failed = true;
    state->failureCode = FrameScheduleResultCodeUVE::SubmissionFailed;
    state->failureMessage = message;
    if (state->pendingTaskCount > 0U) {
        --state->pendingTaskCount;
    }
    state->condition.notify_all();
}

void CompleteTaskUVE(const std::shared_ptr<SchedulerStateUVE>& state, std::size_t taskIndex,
                     bool actionSucceeded, const std::string& actionError) {
    std::vector<std::size_t> readyTasks;
    {
        std::lock_guard<std::mutex> lock(state->mutex);
        if (state->pendingTaskCount > 0U) {
            --state->pendingTaskCount;
        }
        if (!actionSucceeded) {
            state->failed = true;
            state->failureCode = FrameScheduleResultCodeUVE::TaskFailed;
            state->failureMessage = actionError;
        } else if (!state->failed) {
            state->completed[taskIndex] = true;
            ++state->completedTaskCount;
            for (const std::size_t dependentIndex : state->dependents[taskIndex]) {
                if (state->remainingDependencies[dependentIndex] > 0U) {
                    --state->remainingDependencies[dependentIndex];
                }
                if (state->remainingDependencies[dependentIndex] == 0U &&
                    !state->scheduled[dependentIndex]) {
                    state->scheduled[dependentIndex] = true;
                    ++state->pendingTaskCount;
                    readyTasks.push_back(dependentIndex);
                }
            }
        }
        if (state->pendingTaskCount == 0U) {
            state->condition.notify_all();
        }
    }

    for (const std::size_t readyTask : readyTasks) {
        SubmitTaskUVE(state, readyTask);
    }
}

void ExecuteTaskUVE(const std::shared_ptr<SchedulerStateUVE>& state, std::size_t taskIndex) {
    {
        std::lock_guard<std::mutex> lock(state->mutex);
        if (state->failed) {
            if (state->pendingTaskCount > 0U) {
                --state->pendingTaskCount;
            }
            if (state->pendingTaskCount == 0U) {
                state->condition.notify_all();
            }
            return;
        }
    }

    bool actionSucceeded = true;
    std::string actionError;
    try {
        state->graph.GetTasksUVE()[taskIndex].action();
    } catch (...) {
        actionSucceeded = false;
        actionError = ExceptionMessageUVE();
    }
    CompleteTaskUVE(state, taskIndex, actionSucceeded, actionError);
}

void SubmitTaskUVE(const std::shared_ptr<SchedulerStateUVE>& state, std::size_t taskIndex) {
    try {
        state->threadPool.SubmitUVE(
            [state, taskIndex]() { ExecuteTaskUVE(state, taskIndex); });
    } catch (...) {
        MarkSubmissionFailureUVE(state, ExceptionMessageUVE());
    }
}

} // namespace

FrameTaskGraphMutationResultUVE FrameTaskGraphUVE::AddTaskUVE(FrameTaskDefinitionUVE task) {
    if (m_tasks.size() >= kMaximumTasksUVE) {
        return MakeMutationErrorUVE(FrameTaskGraphMutationCodeUVE::CapacityExceeded, task.id,
                                    "frame task graph exceeds its bounded task capacity");
    }
    if (task.id == 0U) {
        return MakeMutationErrorUVE(FrameTaskGraphMutationCodeUVE::InvalidId, task.id,
                                    "frame task identifier must be non-zero");
    }
    if (FindTaskIndexUVE(m_tasks, task.id) != m_tasks.size()) {
        return MakeMutationErrorUVE(FrameTaskGraphMutationCodeUVE::DuplicateId, task.id,
                                    "frame task identifier must be unique");
    }
    if (!IsValidDomainUVE(task.domain)) {
        return MakeMutationErrorUVE(FrameTaskGraphMutationCodeUVE::InvalidDomain, task.id,
                                    "frame task domain is invalid");
    }
    if (task.name.empty() || task.name.size() > kMaximumTaskNameBytesUVE) {
        return MakeMutationErrorUVE(FrameTaskGraphMutationCodeUVE::InvalidName, task.id,
                                    "frame task name is empty or too long");
    }
    if (!task.action) {
        return MakeMutationErrorUVE(FrameTaskGraphMutationCodeUVE::InvalidAction, task.id,
                                    "frame task action must be callable");
    }
    if (task.dependencies.size() > kMaximumDependenciesPerTaskUVE) {
        return MakeMutationErrorUVE(FrameTaskGraphMutationCodeUVE::TooManyDependencies, task.id,
                                    "frame task dependency list exceeds its bounded capacity");
    }
    for (std::size_t index = 0U; index < task.dependencies.size(); ++index) {
        const FrameTaskIdUVE dependencyId = task.dependencies[index];
        if (dependencyId == 0U || dependencyId == task.id) {
            return MakeMutationErrorUVE(FrameTaskGraphMutationCodeUVE::InvalidDependency, task.id,
                                        "frame task dependency identifier is invalid");
        }
        bool duplicateDependency = false;
        for (std::size_t previousIndex = 0U; previousIndex < index; ++previousIndex) {
            if (task.dependencies[previousIndex] == dependencyId) {
                duplicateDependency = true;
                break;
            }
        }
        if (duplicateDependency) {
            return MakeMutationErrorUVE(FrameTaskGraphMutationCodeUVE::DuplicateDependency, task.id,
                                        "frame task dependency identifiers must be unique");
        }
    }

    m_tasks.push_back(std::move(task));
    return FrameTaskGraphMutationResultUVE{FrameTaskGraphMutationCodeUVE::Accepted,
                                           m_tasks.back().id, "accepted"};
}

FrameTaskGraphValidationResultUVE FrameTaskGraphUVE::ValidateUVE() const {
    if (m_tasks.empty()) {
        return MakeValidationErrorUVE(FrameTaskGraphValidationCodeUVE::EmptyGraph, 0U,
                                      "frame task graph must contain at least one task");
    }

    for (const FrameTaskDefinitionUVE& task : m_tasks) {
        if (task.id == 0U || !IsValidDomainUVE(task.domain) || task.name.empty() ||
            task.name.size() > kMaximumTaskNameBytesUVE || !task.action ||
            task.dependencies.size() > kMaximumDependenciesPerTaskUVE) {
            return MakeValidationErrorUVE(FrameTaskGraphValidationCodeUVE::InvalidTask, task.id,
                                          "frame task contains invalid metadata or action");
        }
        for (const FrameTaskIdUVE dependencyId : task.dependencies) {
            if (FindTaskIndexUVE(m_tasks, dependencyId) == m_tasks.size()) {
                return MakeValidationErrorUVE(FrameTaskGraphValidationCodeUVE::UnknownDependency,
                                              task.id, "frame task references an unknown dependency");
            }
        }
    }

    std::vector<std::uint8_t> colors(m_tasks.size(), 0U);
    const auto visit = [&](const auto& self, std::size_t taskIndex) -> bool {
        if (colors[taskIndex] == 1U) {
            return false;
        }
        if (colors[taskIndex] == 2U) {
            return true;
        }
        colors[taskIndex] = 1U;
        for (const FrameTaskIdUVE dependencyId : m_tasks[taskIndex].dependencies) {
            const std::size_t dependencyIndex = FindTaskIndexUVE(m_tasks, dependencyId);
            if (dependencyIndex == m_tasks.size() || !self(self, dependencyIndex)) {
                return false;
            }
        }
        colors[taskIndex] = 2U;
        return true;
    };

    for (std::size_t index = 0U; index < m_tasks.size(); ++index) {
        if (!visit(visit, index)) {
            return MakeValidationErrorUVE(FrameTaskGraphValidationCodeUVE::CyclicDependency,
                                          m_tasks[index].id, "frame task graph contains a cycle");
        }
    }
    return FrameTaskGraphValidationResultUVE{FrameTaskGraphValidationCodeUVE::Valid, 0U, "valid"};
}

FrameScheduleResultUVE FrameSchedulerUVE::ExecuteUVE(const FrameTaskGraphUVE& graph) const {
    const FrameTaskGraphValidationResultUVE validation = graph.ValidateUVE();
    if (!validation.IsValidUVE()) {
        return FrameScheduleResultUVE{FrameScheduleResultCodeUVE::InvalidGraph, 0U,
                                      graph.GetTaskCountUVE(), validation.message};
    }

    const std::shared_ptr<SchedulerStateUVE> state =
        std::make_shared<SchedulerStateUVE>(graph, m_threadPool);
    std::vector<std::size_t> rootTasks;
    for (std::size_t index = 0U; index < graph.GetTasksUVE().size(); ++index) {
        const FrameTaskDefinitionUVE& task = graph.GetTasksUVE()[index];
        state->remainingDependencies[index] = task.dependencies.size();
        for (const FrameTaskIdUVE dependencyId : task.dependencies) {
            const std::size_t dependencyIndex = FindTaskIndexUVE(graph.GetTasksUVE(), dependencyId);
            state->dependents[dependencyIndex].push_back(index);
        }
        if (task.dependencies.empty()) {
            rootTasks.push_back(index);
        }
    }

    {
        std::lock_guard<std::mutex> lock(state->mutex);
        state->pendingTaskCount = rootTasks.size();
        for (const std::size_t rootTask : rootTasks) {
            state->scheduled[rootTask] = true;
        }
    }
    for (const std::size_t rootTask : rootTasks) {
        SubmitTaskUVE(state, rootTask);
    }

    std::unique_lock<std::mutex> lock(state->mutex);
    state->condition.wait(lock, [&state]() { return state->pendingTaskCount == 0U; });
    const FrameScheduleResultCodeUVE resultCode =
        state->failed ? state->failureCode : FrameScheduleResultCodeUVE::Completed;
    const std::string message = state->failed ? state->failureMessage : "completed";
    return FrameScheduleResultUVE{resultCode, state->completedTaskCount,
                                  graph.GetTaskCountUVE(), message};
}

} // namespace UVE::Core
