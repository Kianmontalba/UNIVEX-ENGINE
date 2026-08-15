// Copyright (c) 2026 UniVex Studios. All Rights Reserved.


#include "uve/render/shader/shader_manager_uve.h"

#include <algorithm>
#include <filesystem>
#include <mutex>
#include <optional>
#include <string>
#include <system_error>
#include <unordered_map>
#include <utility>
#include <vector>

#include "shader_binary_cache_uve.h"
#include "shader_diagnostics_parser_uve.h"
#include "shader_preprocessor_uve.h"
#include "uve/debug/logging_macros_uve.h"
#include "uve/render/shader/shader_events_uve.h"
#include "uve/threading/job_counter_uve.h"

namespace UVE::Render::Shader {

struct ShaderManagerUVE::ImplUVE {
    Threading::IThreadPoolUVE& threadPool;
    Events::IEventSystemUVE& eventSystem;
    IRenderDeviceUVE& renderDevice;
    Asset::IFileSystemUVE& fileSystem;
    ShaderManagerConfigUVE config;

    Threading::JobCounterUVE pendingJobs;
    mutable std::mutex mutex;
    std::size_t pendingJobCount = 0;

    struct SourceJobUVE {
        std::shared_ptr<ShaderSourceUVE> target;
        ShaderSourceCompileDescUVE desc;
        Detail::PreprocessResultUVE preprocess;
    };
    std::vector<SourceJobUVE> completedSourceJobs; // guarded by mutex - written from worker threads

    /// Internal normalized representation shared by the legacy unified-source API and the
    /// Increment 34 separate-stage API. Source hot-reload flags are disabled here because linked
    /// program tracking owns the union of both stage dependency closures.
    struct ProgramRequestDescUVE {
        ShaderSourceCompileDescUVE vertexSource;
        ShaderSourceCompileDescUVE fragmentSource;
        std::vector<VertexAttributeUVE> vertexLayout;
        std::uint32_t vertexStride = 0;
        PrimitiveTopologyUVE topology = PrimitiveTopologyUVE::Triangles;
        bool depthTestEnabled = true;
        bool depthWriteEnabled = true;
        PipelineBlendModeUVE blendMode = PipelineBlendModeUVE::Opaque;
        bool hotReloadEnabledUVE = true;
        std::string debugNameUVE;
    };

    struct PendingProgramLinkUVE {
        std::shared_ptr<ShaderProgramUVE> program;
        std::shared_ptr<ShaderSourceUVE> vertexSource;
        std::shared_ptr<ShaderSourceUVE> fragmentSource;
        ProgramRequestDescUVE desc;
    };
    std::vector<PendingProgramLinkUVE> pendingProgramLinks; // main-thread only

    struct TrackedDependenciesUVE {
        std::vector<std::string> dependencyClosure;
        std::unordered_map<std::string, std::filesystem::file_time_type> lastKnownWriteTimes;
    };
    struct TrackedSourceUVE {
        std::weak_ptr<ShaderSourceUVE> target;
        ShaderSourceCompileDescUVE desc;
        TrackedDependenciesUVE dependencies;
    };
    struct TrackedProgramUVE {
        std::weak_ptr<ShaderProgramUVE> target;
        ProgramRequestDescUVE desc;
        TrackedDependenciesUVE dependencies;
    };
    std::vector<TrackedSourceUVE> trackedSources;   // main-thread only
    std::vector<TrackedProgramUVE> trackedPrograms; // main-thread only

    double hotReloadAccumulatorSeconds = 0.0;
    bool lastCompileUsedCache = false;

    ImplUVE(Threading::IThreadPoolUVE& threadPoolIn, Events::IEventSystemUVE& eventSystemIn,
            IRenderDeviceUVE& renderDeviceIn, Asset::IFileSystemUVE& fileSystemIn, ShaderManagerConfigUVE configIn)
        : threadPool(threadPoolIn),
          eventSystem(eventSystemIn),
          renderDevice(renderDeviceIn),
          fileSystem(fileSystemIn),
          config(std::move(configIn)) {}
};

namespace {

[[nodiscard]] const char* ShaderStageDefineNameUVE(ShaderStageUVE stage) noexcept {
    switch (stage) {
        case ShaderStageUVE::Vertex:
            return "VERTEX_SHADER";
        case ShaderStageUVE::Fragment:
            return "FRAGMENT_SHADER";
        case ShaderStageUVE::Compute:
            return "COMPUTE_SHADER";
        case ShaderStageUVE::Geometry:
            return "GEOMETRY_SHADER";
    }
    return "";
}

[[nodiscard]] std::vector<std::pair<std::string, std::string>> BuildDefinesUVE(
    ShaderStageUVE stage, bool injectDebugDefine, const std::vector<std::pair<std::string, std::string>>& extraDefines) {
    std::vector<std::pair<std::string, std::string>> defines;
    defines.reserve(extraDefines.size() + 4);
    defines.emplace_back("UVE_DEBUG", injectDebugDefine ? "1" : "0");
    defines.emplace_back("UVE_MOBILE", "0"); // No mobile backend exists yet - reserved.
    defines.emplace_back("UVE_BACKEND_GL", "1"); // Reserved for a future non-GL backend to define its own instead.
    defines.emplace_back(ShaderStageDefineNameUVE(stage), "1");
    defines.insert(defines.end(), extraDefines.begin(), extraDefines.end());
    return defines;
}

[[nodiscard]] std::filesystem::file_time_type GetRealFileWriteTimeUVE(Asset::IFileSystemUVE& fileSystem,
                                                                       const std::string& virtualPath) {
    const std::filesystem::path realPath = fileSystem.ResolveRealPathUVE(virtualPath);
    std::error_code errorCode;
    const auto writeTime = std::filesystem::last_write_time(realPath, errorCode);
    return errorCode ? std::filesystem::file_time_type{} : writeTime;
}

[[nodiscard]] std::uint64_t ComputeSourceContentHashUVE(IRenderDeviceUVE& renderDevice, const std::string& resolvedSource,
                                                         ShaderStageUVE stage, const std::string& entryPoint) {
    std::string combined = resolvedSource;
    combined += '|';
    combined += ShaderStageDefineNameUVE(stage);
    combined += '|';
    combined += entryPoint;
    combined += '|';
    combined += std::string(renderDevice.GetBackendNameUVE());
    return Detail::ComputeFnv1aHashUVE(combined);
}

[[nodiscard]] std::uint64_t ComputeProgramContentHashUVE(const ShaderSourceUVE& vertexSource,
                                                          const ShaderSourceUVE& fragmentSource) {
    const std::string combined =
        std::to_string(vertexSource.GetContentHashUVE()) + "|" + std::to_string(fragmentSource.GetContentHashUVE());
    return Detail::ComputeFnv1aHashUVE(combined);
}

[[nodiscard]] ShaderSourceCompileDescUVE NormalizeProgramStageDescUVE(ShaderSourceCompileDescUVE desc,
                                                                        ShaderStageUVE stage,
                                                                        const std::string& programDebugName) {
    desc.stage = stage;
    // Dependency tracking happens at the linked-program level (the union of both stage closures),
    // so individual child sources never create competing tracking entries.
    desc.hotReloadEnabledUVE = false;
    if (desc.debugNameUVE.empty()) {
        desc.debugNameUVE = programDebugName;
    }
    desc.debugNameUVE += stage == ShaderStageUVE::Vertex ? " (vertex)" : " (fragment)";
    return desc;
}

[[nodiscard]] ShaderManagerUVE::ImplUVE::ProgramRequestDescUVE BuildProgramRequestDescUVE(
    const ShaderProgramDescUVE& desc) {
    ShaderManagerUVE::ImplUVE::ProgramRequestDescUVE request;
    request.debugNameUVE = desc.debugNameUVE;
    request.vertexSource = NormalizeProgramStageDescUVE(
        ShaderSourceCompileDescUVE{ShaderStageUVE::Vertex, desc.virtualFilePath, desc.embeddedFallbackSourceCode,
                                   desc.extraDefines, desc.entryPointName, false, {}},
        ShaderStageUVE::Vertex, request.debugNameUVE);
    request.fragmentSource = NormalizeProgramStageDescUVE(
        ShaderSourceCompileDescUVE{ShaderStageUVE::Fragment, desc.virtualFilePath, desc.embeddedFallbackSourceCode,
                                   desc.extraDefines, desc.entryPointName, false, {}},
        ShaderStageUVE::Fragment, request.debugNameUVE);
    request.vertexLayout = desc.vertexLayout;
    request.vertexStride = desc.vertexStride;
    request.topology = desc.topology;
    request.depthTestEnabled = desc.depthTestEnabled;
    request.depthWriteEnabled = desc.depthWriteEnabled;
    request.blendMode = desc.blendMode;
    request.hotReloadEnabledUVE = desc.hotReloadEnabledUVE;
    return request;
}

[[nodiscard]] ShaderManagerUVE::ImplUVE::ProgramRequestDescUVE BuildProgramRequestDescUVE(
    const ShaderProgramStagesDescUVE& desc) {
    ShaderManagerUVE::ImplUVE::ProgramRequestDescUVE request;
    request.debugNameUVE = desc.debugNameUVE;
    request.vertexSource = NormalizeProgramStageDescUVE(desc.vertexSource, ShaderStageUVE::Vertex, request.debugNameUVE);
    request.fragmentSource =
        NormalizeProgramStageDescUVE(desc.fragmentSource, ShaderStageUVE::Fragment, request.debugNameUVE);
    request.vertexLayout = desc.vertexLayout;
    request.vertexStride = desc.vertexStride;
    request.topology = desc.topology;
    request.depthTestEnabled = desc.depthTestEnabled;
    request.depthWriteEnabled = desc.depthWriteEnabled;
    request.hotReloadEnabledUVE = desc.hotReloadEnabledUVE;
    return request;
}

} // namespace

std::shared_ptr<ShaderSourceUVE> ShaderManagerUVE::MakeSourceUVE(IRenderDeviceUVE& renderDevice, ShaderStageUVE stage) {
    auto* const renderDevicePtr = &renderDevice;
    std::shared_ptr<ShaderSourceUVE> source(new ShaderSourceUVE(), [renderDevicePtr](ShaderSourceUVE* pointer) {
        if (pointer->m_valid) {
            renderDevicePtr->DestroyShaderUVE(pointer->m_handle);
        }
        delete pointer;
    });
    source->m_stage = stage;
    return source;
}

std::shared_ptr<ShaderProgramUVE> ShaderManagerUVE::MakeProgramUVE(IRenderDeviceUVE& renderDevice) {
    auto* const renderDevicePtr = &renderDevice;
    std::shared_ptr<ShaderProgramUVE> program(new ShaderProgramUVE(), [renderDevicePtr](ShaderProgramUVE* pointer) {
        if (pointer->m_valid) {
            renderDevicePtr->DestroyPipelineUVE(pointer->m_pipeline);
        }
        delete pointer;
    });
    return program;
}

void ShaderManagerUVE::SubmitSourceCompileJobUVE(ImplUVE& impl, const std::shared_ptr<ShaderSourceUVE>& target,
                                                  const ShaderSourceCompileDescUVE& desc) {
    {
        std::lock_guard<std::mutex> lock(impl.mutex);
        ++impl.pendingJobCount;
    }
    impl.threadPool.SubmitUVE(
        [&impl, target, desc]() {
            const std::vector<std::pair<std::string, std::string>> defines =
                BuildDefinesUVE(desc.stage, impl.config.injectDebugDefineUVE, desc.extraDefines);
            Detail::PreprocessResultUVE preprocess = Detail::PreprocessShaderSourceUVE(
                impl.fileSystem, desc.virtualFilePath, desc.embeddedFallbackSourceCode, defines);

            std::lock_guard<std::mutex> lock(impl.mutex);
            impl.completedSourceJobs.push_back(ImplUVE::SourceJobUVE{target, desc, std::move(preprocess)});
        },
        impl.pendingJobs);
}

void ShaderManagerUVE::DrainCompletedSourceJobsUVE(ImplUVE& impl) {
    std::vector<ImplUVE::SourceJobUVE> completedJobs;
    {
        std::lock_guard<std::mutex> lock(impl.mutex);
        completedJobs.swap(impl.completedSourceJobs);
        impl.pendingJobCount -= completedJobs.size();
    }

    for (auto& job : completedJobs) {
        ShaderSourceUVE& source = *job.target;

        if (!job.preprocess.success) {
            source.m_ready = true;
            source.m_valid = false; // A failed hot-reload preprocess leaves any prior valid handle untouched.
            source.m_diagnostics = ShaderCompileDiagnosticsUVE{false, {}, job.preprocess.errorMessage};
            impl.eventSystem.QueueEvent(
                ShaderCompileFailedEventUVE{job.desc.debugNameUVE, job.desc.stage, source.m_diagnostics});
        } else {
            std::string infoLog;
            const ShaderHandleUVE newHandle = impl.renderDevice.CreateShaderUVE(
                ShaderDescUVE{job.desc.stage, job.preprocess.resolvedSource, job.desc.entryPointName}, &infoLog);
            const bool succeeded = (newHandle != kInvalidShaderHandleUVE);
            const std::vector<ShaderCompileErrorUVE> diagnosticsList =
                Detail::ParseGlInfoLogUVE(infoLog, job.preprocess.fileIndexTable);

            if (succeeded) {
                if (source.m_valid) {
                    // Hot-reload swap: the new shader compiled successfully, so it's safe to
                    // destroy the old one now - never before a replacement is confirmed live.
                    impl.renderDevice.DestroyShaderUVE(source.m_handle);
                }
                source.m_handle = newHandle;
                source.m_resolvedSource = job.preprocess.resolvedSource;
                source.m_contentHash = ComputeSourceContentHashUVE(impl.renderDevice, job.preprocess.resolvedSource,
                                                                    job.desc.stage, job.desc.entryPointName);
                source.m_dependencyClosure = job.preprocess.dependencyClosure;
                source.m_valid = true;
            }
            // else: leave the prior handle/valid/resolvedSource untouched - a program using this
            // source keeps rendering its last-known-good state rather than going dark on a typo.

            source.m_ready = true;
            source.m_diagnostics = ShaderCompileDiagnosticsUVE{succeeded, diagnosticsList, infoLog};

            if (!succeeded) {
                impl.eventSystem.QueueEvent(
                    ShaderCompileFailedEventUVE{job.desc.debugNameUVE, job.desc.stage, source.m_diagnostics});
            }
        }

        if (job.desc.hotReloadEnabledUVE && !job.preprocess.dependencyClosure.empty()) {
            bool found = false;
            for (auto& tracked : impl.trackedSources) {
                if (tracked.target.lock() == job.target) {
                    tracked.desc = job.desc;
                    tracked.dependencies.dependencyClosure = job.preprocess.dependencyClosure;
                    for (const std::string& path : job.preprocess.dependencyClosure) {
                        if (!tracked.dependencies.lastKnownWriteTimes.contains(path)) {
                            tracked.dependencies.lastKnownWriteTimes[path] =
                                GetRealFileWriteTimeUVE(impl.fileSystem, path);
                        }
                    }
                    found = true;
                    break;
                }
            }
            if (!found) {
                ImplUVE::TrackedSourceUVE entry;
                entry.target = job.target;
                entry.desc = job.desc;
                entry.dependencies.dependencyClosure = job.preprocess.dependencyClosure;
                for (const std::string& path : job.preprocess.dependencyClosure) {
                    entry.dependencies.lastKnownWriteTimes[path] = GetRealFileWriteTimeUVE(impl.fileSystem, path);
                }
                impl.trackedSources.push_back(std::move(entry));
            }
        }
    }
}

void ShaderManagerUVE::ApplyPendingProgramLinksUVE(ImplUVE& impl) {
    std::vector<ImplUVE::PendingProgramLinkUVE> stillPendingLinks;
    for (auto& pending : impl.pendingProgramLinks) {
        if (!(pending.vertexSource->IsReadyUVE() && pending.fragmentSource->IsReadyUVE())) {
            stillPendingLinks.push_back(std::move(pending));
            continue;
        }

        ShaderProgramUVE& program = *pending.program;
        const bool stagesValid = pending.vertexSource->IsValidUVE() && pending.fragmentSource->IsValidUVE();

        bool linkSucceeded = false;
        std::vector<ShaderCompileErrorUVE> diagnosticsList;
        std::string infoLog;

        if (stagesValid) {
            const std::uint64_t programHash =
                ComputeProgramContentHashUVE(*pending.vertexSource, *pending.fragmentSource);
            const std::filesystem::path cacheFilePath =
                Detail::GetCacheFilePathUVE(impl.config.cachePath, programHash);
            const PipelineBinaryDescUVE binaryDesc{pending.desc.vertexLayout, pending.desc.vertexStride,
                                                    pending.desc.topology, pending.desc.depthTestEnabled,
                                                    pending.desc.depthWriteEnabled, pending.desc.blendMode};

            PipelineHandleUVE newPipeline = kInvalidPipelineHandleUVE;
            bool usedCache = false;

            const std::optional<Detail::CacheEntryUVE> cacheEntry = Detail::ReadCacheEntryUVE(cacheFilePath);
            if (cacheEntry.has_value()) {
                newPipeline = impl.renderDevice.CreatePipelineFromBinaryUVE(
                    cacheEntry->payload, cacheEntry->glBinaryFormat, binaryDesc);
                usedCache = (newPipeline != kInvalidPipelineHandleUVE);
            }

            if (newPipeline == kInvalidPipelineHandleUVE) {
                PipelineDescUVE pipelineDesc;
                pipelineDesc.vertexShader = pending.vertexSource->GetHandleUVE();
                pipelineDesc.fragmentShader = pending.fragmentSource->GetHandleUVE();
                pipelineDesc.vertexLayout = pending.desc.vertexLayout;
                pipelineDesc.topology = pending.desc.topology;
                pipelineDesc.depthTestEnabled = pending.desc.depthTestEnabled;
                pipelineDesc.depthWriteEnabled = pending.desc.depthWriteEnabled;
                pipelineDesc.blendMode = pending.desc.blendMode;
                pipelineDesc.vertexStride = pending.desc.vertexStride;
                newPipeline = impl.renderDevice.CreatePipelineUVE(pipelineDesc, &infoLog);

                if (newPipeline != kInvalidPipelineHandleUVE) {
                    std::vector<std::byte> binary;
                    std::uint32_t binaryFormat = 0;
                    if (impl.renderDevice.GetPipelineBinaryUVE(newPipeline, binary, binaryFormat)) {
                        // Best-effort: a cache write failure is never fatal, just logged by
                        // WriteCacheEntryUVE itself - the next startup simply recompiles from
                        // source.
                        static_cast<void>(Detail::WriteCacheEntryUVE(cacheFilePath, binaryFormat, binary));
                    }
                }
            }

            impl.lastCompileUsedCache = usedCache;
            diagnosticsList = Detail::ParseGlInfoLogUVE(infoLog, {});
            linkSucceeded = (newPipeline != kInvalidPipelineHandleUVE);

            if (linkSucceeded) {
                if (program.m_valid) {
                    impl.renderDevice.DestroyPipelineUVE(program.m_pipeline);
                }
                program.m_pipeline = newPipeline;
                program.m_uniforms = impl.renderDevice.GetPipelineUniformsUVE(newPipeline);
                program.m_contentHash = programHash;
                program.m_valid = true;
            }
        } else {
            impl.lastCompileUsedCache = false;
            diagnosticsList = pending.vertexSource->GetDiagnosticsUVE().diagnostics;
            const std::vector<ShaderCompileErrorUVE>& fragmentDiagnostics =
                pending.fragmentSource->GetDiagnosticsUVE().diagnostics;
            diagnosticsList.insert(diagnosticsList.end(), fragmentDiagnostics.begin(), fragmentDiagnostics.end());
            infoLog = pending.vertexSource->GetDiagnosticsUVE().rawInfoLog +
                      pending.fragmentSource->GetDiagnosticsUVE().rawInfoLog;
        }

        program.m_ready = true;
        program.m_diagnostics = ShaderCompileDiagnosticsUVE{linkSucceeded, diagnosticsList, infoLog};
        impl.eventSystem.QueueEvent(ShaderProgramReloadedEventUVE{pending.desc.debugNameUVE, linkSucceeded});

        if (pending.desc.hotReloadEnabledUVE) {
            std::vector<std::string> combinedClosure = pending.vertexSource->m_dependencyClosure;
            for (const std::string& path : pending.fragmentSource->m_dependencyClosure) {
                if (std::find(combinedClosure.begin(), combinedClosure.end(), path) == combinedClosure.end()) {
                    combinedClosure.push_back(path);
                }
            }
            if (!combinedClosure.empty()) {
                bool found = false;
                for (auto& tracked : impl.trackedPrograms) {
                    if (tracked.target.lock() == pending.program) {
                        tracked.desc = pending.desc;
                        tracked.dependencies.dependencyClosure = combinedClosure;
                        for (const std::string& path : combinedClosure) {
                            if (!tracked.dependencies.lastKnownWriteTimes.contains(path)) {
                                tracked.dependencies.lastKnownWriteTimes[path] =
                                    GetRealFileWriteTimeUVE(impl.fileSystem, path);
                            }
                        }
                        found = true;
                        break;
                    }
                }
                if (!found) {
                    ImplUVE::TrackedProgramUVE entry;
                    entry.target = pending.program;
                    entry.desc = pending.desc;
                    entry.dependencies.dependencyClosure = combinedClosure;
                    for (const std::string& path : combinedClosure) {
                        entry.dependencies.lastKnownWriteTimes[path] = GetRealFileWriteTimeUVE(impl.fileSystem, path);
                    }
                    impl.trackedPrograms.push_back(std::move(entry));
                }
            }
        }
    }
    impl.pendingProgramLinks = std::move(stillPendingLinks);
}

void ShaderManagerUVE::PollHotReloadUVE(ImplUVE& impl) {
    std::vector<ImplUVE::TrackedSourceUVE> stillTrackedSources;
    for (auto& tracked : impl.trackedSources) {
        std::shared_ptr<ShaderSourceUVE> target = tracked.target.lock();
        if (!target) {
            continue; // Expired - the caller released it; drop the tracking entry.
        }
        bool dirty = false;
        for (const std::string& path : tracked.dependencies.dependencyClosure) {
            const auto currentWriteTime = GetRealFileWriteTimeUVE(impl.fileSystem, path);
            std::filesystem::file_time_type& lastKnown = tracked.dependencies.lastKnownWriteTimes[path];
            if (currentWriteTime != std::filesystem::file_time_type{} && currentWriteTime != lastKnown) {
                dirty = true;
            }
            lastKnown = currentWriteTime;
        }
        if (dirty) {
            SubmitSourceCompileJobUVE(impl, target, tracked.desc);
        }
        stillTrackedSources.push_back(std::move(tracked));
    }
    impl.trackedSources = std::move(stillTrackedSources);

    std::vector<ImplUVE::TrackedProgramUVE> stillTrackedPrograms;
    for (auto& tracked : impl.trackedPrograms) {
        std::shared_ptr<ShaderProgramUVE> target = tracked.target.lock();
        if (!target) {
            continue;
        }
        bool dirty = false;
        for (const std::string& path : tracked.dependencies.dependencyClosure) {
            const auto currentWriteTime = GetRealFileWriteTimeUVE(impl.fileSystem, path);
            std::filesystem::file_time_type& lastKnown = tracked.dependencies.lastKnownWriteTimes[path];
            if (currentWriteTime != std::filesystem::file_time_type{} && currentWriteTime != lastKnown) {
                dirty = true;
            }
            lastKnown = currentWriteTime;
        }
        if (dirty) {
            std::shared_ptr<ShaderSourceUVE> vertexSource = MakeSourceUVE(impl.renderDevice, ShaderStageUVE::Vertex);
            std::shared_ptr<ShaderSourceUVE> fragmentSource =
                MakeSourceUVE(impl.renderDevice, ShaderStageUVE::Fragment);
            SubmitSourceCompileJobUVE(impl, vertexSource, tracked.desc.vertexSource);
            SubmitSourceCompileJobUVE(impl, fragmentSource, tracked.desc.fragmentSource);
            std::lock_guard<std::mutex> lock(impl.mutex);
            impl.pendingProgramLinks.push_back(
                ImplUVE::PendingProgramLinkUVE{target, vertexSource, fragmentSource, tracked.desc});
        }
        stillTrackedPrograms.push_back(std::move(tracked));
    }
    impl.trackedPrograms = std::move(stillTrackedPrograms);
}

ShaderManagerUVE::ShaderManagerUVE(Threading::IThreadPoolUVE& threadPool, Events::IEventSystemUVE& eventSystem,
                                    IRenderDeviceUVE& renderDevice, Asset::IFileSystemUVE& fileSystem,
                                    ShaderManagerConfigUVE config)
    : m_impl(std::make_unique<ImplUVE>(threadPool, eventSystem, renderDevice, fileSystem, std::move(config))) {}

ShaderManagerUVE::~ShaderManagerUVE() {
    // Every ShaderSourceUVE/ShaderProgramUVE this manager handed out must be released by the
    // caller before this destructor runs in practice (their deleters only capture
    // IRenderDeviceUVE&, not this manager, so a slightly-late release remains safely
    // destructible) - but a background job still referencing this ImplUVE must never still be
    // running when it's freed, hence this unconditional drain.
    m_impl->pendingJobs.WaitUVE();
}

std::shared_ptr<ShaderSourceUVE> ShaderManagerUVE::CreateSourceUVE(const ShaderSourceCompileDescUVE& desc) {
    std::shared_ptr<ShaderSourceUVE> target = MakeSourceUVE(m_impl->renderDevice, desc.stage);
    SubmitSourceCompileJobUVE(*m_impl, target, desc);
    return target;
}

std::shared_ptr<ShaderProgramUVE> ShaderManagerUVE::CreateProgramUVE(const ShaderProgramDescUVE& desc) {
    const ImplUVE::ProgramRequestDescUVE request = BuildProgramRequestDescUVE(desc);
    std::shared_ptr<ShaderProgramUVE> program = MakeProgramUVE(m_impl->renderDevice);
    std::shared_ptr<ShaderSourceUVE> vertexSource = MakeSourceUVE(m_impl->renderDevice, ShaderStageUVE::Vertex);
    std::shared_ptr<ShaderSourceUVE> fragmentSource = MakeSourceUVE(m_impl->renderDevice, ShaderStageUVE::Fragment);

    SubmitSourceCompileJobUVE(*m_impl, vertexSource, request.vertexSource);
    SubmitSourceCompileJobUVE(*m_impl, fragmentSource, request.fragmentSource);

    {
        std::lock_guard<std::mutex> lock(m_impl->mutex);
        m_impl->pendingProgramLinks.push_back(
            ImplUVE::PendingProgramLinkUVE{program, vertexSource, fragmentSource, request});
    }
    return program;
}

std::shared_ptr<ShaderProgramUVE> ShaderManagerUVE::CreateProgramFromStagesUVE(
    const ShaderProgramStagesDescUVE& desc) {
    const ImplUVE::ProgramRequestDescUVE request = BuildProgramRequestDescUVE(desc);
    std::shared_ptr<ShaderProgramUVE> program = MakeProgramUVE(m_impl->renderDevice);
    std::shared_ptr<ShaderSourceUVE> vertexSource = MakeSourceUVE(m_impl->renderDevice, ShaderStageUVE::Vertex);
    std::shared_ptr<ShaderSourceUVE> fragmentSource = MakeSourceUVE(m_impl->renderDevice, ShaderStageUVE::Fragment);

    SubmitSourceCompileJobUVE(*m_impl, vertexSource, request.vertexSource);
    SubmitSourceCompileJobUVE(*m_impl, fragmentSource, request.fragmentSource);

    {
        std::lock_guard<std::mutex> lock(m_impl->mutex);
        m_impl->pendingProgramLinks.push_back(
            ImplUVE::PendingProgramLinkUVE{program, vertexSource, fragmentSource, request});
    }
    return program;
}

void ShaderManagerUVE::UpdateUVE(double deltaTimeSeconds) {
    DrainCompletedSourceJobsUVE(*m_impl);
    ApplyPendingProgramLinksUVE(*m_impl);

    if (!m_impl->config.hotReloadEnabledUVE) {
        return;
    }
    m_impl->hotReloadAccumulatorSeconds += deltaTimeSeconds;
    if (m_impl->hotReloadAccumulatorSeconds < m_impl->config.hotReloadPollIntervalSecondsUVE) {
        return;
    }
    m_impl->hotReloadAccumulatorSeconds = 0.0;
    PollHotReloadUVE(*m_impl);
}

std::size_t ShaderManagerUVE::GetPendingJobCountUVE() const noexcept {
    std::lock_guard<std::mutex> lock(m_impl->mutex);
    return m_impl->pendingJobCount;
}

bool ShaderManagerUVE::GetLastCompileUsedCacheUVE() const noexcept {
    return m_impl->lastCompileUsedCache;
}

} // namespace UVE::Render::Shader
