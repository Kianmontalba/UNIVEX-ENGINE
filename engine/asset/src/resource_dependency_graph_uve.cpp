// Copyright (c) 2026 UniVex Studios. All Rights Reserved.

#include "uve/asset/resource_dependency_graph_uve.h"

#include <algorithm>
#include <limits>
#include <utility>

namespace UVE::Asset {
namespace {

[[nodiscard]] bool IsValidHandleUVE(const ResourceHandleUVE handle) noexcept {
    return handle.guid != kInvalidAssetGuidUVE && handle.generation != 0U;
}

[[nodiscard]] bool ContainsHandleUVE(const std::vector<ResourceHandleUVE>& handles,
                                     const ResourceHandleUVE handle) noexcept {
    return std::find(handles.begin(), handles.end(), handle) != handles.end();
}

} // namespace

ResourceDependencyResultUVE ResourceDependencyGraphUVE::RegisterResourceUVE(const ResourceHandleUVE handle) {
    if (!IsValidHandleUVE(handle)) {
        return {ResourceDependencyCodeUVE::InvalidHandle,
                "Resource registration requires a non-zero GUID and generation."};
    }
    if (FindExactUVE(handle) != nullptr) {
        return {ResourceDependencyCodeUVE::DuplicateResource,
                "Resource registration rejected an already registered handle."};
    }
    if (FindGuidUVE(handle.guid) != nullptr) {
        return {ResourceDependencyCodeUVE::StaleGeneration,
                "Resource registration rejected a second generation for the same GUID."};
    }
    if (m_entries.size() >= kMaximumResourcesUVE) {
        return {ResourceDependencyCodeUVE::CapacityExceeded,
                "Resource dependency graph capacity has been reached."};
    }
    m_entries.push_back(EntryUVE{handle, {}});
    BumpGenerationUVE();
    return {ResourceDependencyCodeUVE::Registered, "Resource handle was registered."};
}

ResourceDependencyResultUVE ResourceDependencyGraphUVE::SetDependenciesUVE(
    const ResourceHandleUVE handle, std::vector<ResourceHandleUVE> dependencies) {
    if (!IsValidHandleUVE(handle)) {
        return {ResourceDependencyCodeUVE::InvalidHandle,
                "Dependency updates require a non-zero GUID and generation."};
    }
    EntryUVE* source = FindExactUVE(handle);
    if (source == nullptr) {
        return FindGuidUVE(handle.guid) == nullptr
            ? ResourceDependencyResultUVE{ResourceDependencyCodeUVE::UnknownDependency,
                                          "Dependency update referenced an unknown resource."}
            : ResourceDependencyResultUVE{ResourceDependencyCodeUVE::StaleGeneration,
                                          "Dependency update used a stale resource generation."};
    }
    if (dependencies.size() > kMaximumDependenciesPerResourceUVE) {
        return {ResourceDependencyCodeUVE::CapacityExceeded,
                "Resource dependency count exceeds the bounded per-resource limit."};
    }
    for (std::size_t index = 0U; index < dependencies.size(); ++index) {
        const ResourceHandleUVE dependency = dependencies[index];
        if (!IsValidHandleUVE(dependency) || dependency == handle) {
            return {ResourceDependencyCodeUVE::InvalidHandle,
                    "Dependency updates reject invalid or self-referencing handles."};
        }
        if (std::find(dependencies.begin(), dependencies.begin() + static_cast<std::ptrdiff_t>(index), dependency) !=
            dependencies.begin() + static_cast<std::ptrdiff_t>(index)) {
            return {ResourceDependencyCodeUVE::DuplicateResource,
                    "Dependency updates reject duplicate dependency handles."};
        }
        if (FindExactUVE(dependency) == nullptr) {
            return FindGuidUVE(dependency.guid) == nullptr
                ? ResourceDependencyResultUVE{ResourceDependencyCodeUVE::UnknownDependency,
                                              "Dependency update referenced an unknown dependency."}
                : ResourceDependencyResultUVE{ResourceDependencyCodeUVE::StaleGeneration,
                                              "Dependency update referenced a stale dependency generation."};
        }
        std::vector<ResourceHandleUVE> visited;
        if (ReachesUVE(dependency, handle, visited)) {
            return {ResourceDependencyCodeUVE::CycleDetected,
                    "Dependency update would introduce a resource dependency cycle."};
        }
    }
    if (source->dependencies == dependencies) {
        return {ResourceDependencyCodeUVE::Updated, "Resource dependencies were unchanged."};
    }
    source->dependencies = std::move(dependencies);
    BumpGenerationUVE();
    return {ResourceDependencyCodeUVE::Updated, "Resource dependencies were updated."};
}

ResourceDependencyInvalidationPlanUVE ResourceDependencyGraphUVE::GetDependentClosureUVE(
    const ResourceHandleUVE root, const std::size_t maximumDependents) const {
    ResourceDependencyInvalidationPlanUVE plan;
    plan.graphGeneration = m_graphGeneration;
    plan.root = root;
    if (!IsValidHandleUVE(root)) {
        plan.code = ResourceDependencyCodeUVE::InvalidHandle;
        plan.message = "Dependent closure requires a non-zero GUID and generation.";
        return plan;
    }
    if (FindExactUVE(root) == nullptr) {
        plan.code = FindGuidUVE(root.guid) == nullptr ? ResourceDependencyCodeUVE::UnknownDependency
                                                        : ResourceDependencyCodeUVE::StaleGeneration;
        plan.message = plan.code == ResourceDependencyCodeUVE::UnknownDependency
                           ? "Dependent closure referenced an unknown resource."
                           : "Dependent closure used a stale resource generation.";
        return plan;
    }

    std::vector<ResourceHandleUVE> frontier{root};
    std::vector<ResourceHandleUVE> discovered;
    while (!frontier.empty()) {
        std::vector<ResourceHandleUVE> nextFrontier;
        for (const ResourceHandleUVE current : frontier) {
            for (const EntryUVE& entry : m_entries) {
                if (!ContainsHandleUVE(entry.dependencies, current) ||
                    ContainsHandleUVE(discovered, entry.handle) || ContainsHandleUVE(nextFrontier, entry.handle) ||
                    entry.handle == root) {
                    continue;
                }
                nextFrontier.push_back(entry.handle);
            }
        }
        std::sort(nextFrontier.begin(), nextFrontier.end(), [](const ResourceHandleUVE left,
                                                                const ResourceHandleUVE right) {
            if (left.guid.value != right.guid.value) {
                return left.guid.value < right.guid.value;
            }
            return left.generation < right.generation;
        });
        discovered.insert(discovered.end(), nextFrontier.begin(), nextFrontier.end());
        frontier = std::move(nextFrontier);
    }

    plan.dependentsTruncated = discovered.size() > maximumDependents;
    if (plan.dependentsTruncated) {
        discovered.resize(maximumDependents);
    }
    plan.dependents = std::move(discovered);
    plan.code = ResourceDependencyCodeUVE::DependentClosureReady;
    plan.message = plan.dependentsTruncated ? "Dependent closure was bounded and truncated."
                                             : "Dependent closure was captured.";
    return plan;
}

ResourceDependencyResultUVE ResourceDependencyGraphUVE::RemoveResourceUVE(const ResourceHandleUVE handle) {
    EntryUVE* entry = FindExactUVE(handle);
    if (entry == nullptr) {
        return FindGuidUVE(handle.guid) == nullptr
            ? ResourceDependencyResultUVE{ResourceDependencyCodeUVE::UnknownDependency,
                                          "Resource removal referenced an unknown handle."}
            : ResourceDependencyResultUVE{ResourceDependencyCodeUVE::StaleGeneration,
                                          "Resource removal used a stale generation."};
    }
    const bool hasDependents = std::any_of(m_entries.begin(), m_entries.end(), [handle](const EntryUVE& candidate) {
        return candidate.handle != handle && ContainsHandleUVE(candidate.dependencies, handle);
    });
    if (hasDependents) {
        return {ResourceDependencyCodeUVE::HasDependents,
                "Resource removal is blocked while another resource depends on this handle."};
    }
    m_entries.erase(std::remove_if(m_entries.begin(), m_entries.end(), [handle](const EntryUVE& candidate) {
        return candidate.handle == handle;
    }), m_entries.end());
    BumpGenerationUVE();
    return {ResourceDependencyCodeUVE::Removed, "Resource handle was removed."};
}

bool ResourceDependencyGraphUVE::HasResourceUVE(const ResourceHandleUVE handle) const noexcept {
    return FindExactUVE(handle) != nullptr;
}

ResourceDependencySnapshotUVE ResourceDependencyGraphUVE::GetSnapshotUVE() const {
    ResourceDependencySnapshotUVE snapshot;
    snapshot.graphGeneration = m_graphGeneration;
    snapshot.entries.reserve(m_entries.size());
    for (const EntryUVE& entry : m_entries) {
        snapshot.entries.push_back(ResourceDependencyEntryUVE{entry.handle, entry.dependencies});
    }
    std::sort(snapshot.entries.begin(), snapshot.entries.end(), [](const auto& left, const auto& right) {
        if (left.handle.guid.value != right.handle.guid.value) {
            return left.handle.guid.value < right.handle.guid.value;
        }
        return left.handle.generation < right.handle.generation;
    });
    return snapshot;
}

ResourceDependencyGraphUVE::EntryUVE* ResourceDependencyGraphUVE::FindExactUVE(const ResourceHandleUVE handle) noexcept {
    const auto iterator = std::find_if(m_entries.begin(), m_entries.end(), [handle](const EntryUVE& entry) {
        return entry.handle == handle;
    });
    return iterator == m_entries.end() ? nullptr : &*iterator;
}

const ResourceDependencyGraphUVE::EntryUVE* ResourceDependencyGraphUVE::FindExactUVE(
    const ResourceHandleUVE handle) const noexcept {
    const auto iterator = std::find_if(m_entries.cbegin(), m_entries.cend(), [handle](const EntryUVE& entry) {
        return entry.handle == handle;
    });
    return iterator == m_entries.cend() ? nullptr : &*iterator;
}

const ResourceDependencyGraphUVE::EntryUVE* ResourceDependencyGraphUVE::FindGuidUVE(
    const AssetGuidUVE guid) const noexcept {
    const auto iterator = std::find_if(m_entries.cbegin(), m_entries.cend(), [guid](const EntryUVE& entry) {
        return entry.handle.guid == guid;
    });
    return iterator == m_entries.cend() ? nullptr : &*iterator;
}

bool ResourceDependencyGraphUVE::ReachesUVE(const ResourceHandleUVE start, const ResourceHandleUVE target,
                                            std::vector<ResourceHandleUVE>& visited) const noexcept {
    if (start == target) {
        return true;
    }
    if (ContainsHandleUVE(visited, start)) {
        return false;
    }
    visited.push_back(start);
    const EntryUVE* entry = FindExactUVE(start);
    if (entry == nullptr) {
        return false;
    }
    return std::any_of(entry->dependencies.begin(), entry->dependencies.end(),
                       [this, target, &visited](const ResourceHandleUVE dependency) {
                           return ReachesUVE(dependency, target, visited);
                       });
}

ResourceDependencyResultUVE ResourceDependencyGraphUVE::ValidateHandleUVE(const ResourceHandleUVE handle) const noexcept {
    if (!IsValidHandleUVE(handle)) {
        return {ResourceDependencyCodeUVE::InvalidHandle, "Resource handle is invalid."};
    }
    return {ResourceDependencyCodeUVE::Registered, "Resource handle is valid."};
}

void ResourceDependencyGraphUVE::BumpGenerationUVE() noexcept {
    if (m_graphGeneration < std::numeric_limits<std::uint64_t>::max()) {
        ++m_graphGeneration;
    }
}

} // namespace UVE::Asset
