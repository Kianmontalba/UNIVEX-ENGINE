// Copyright (c) 2026 UniVex Studios. All Rights Reserved.
#pragma once
#include <array>
#include <cstddef>
#include <vector>
namespace UVE::Audio {
inline constexpr std::size_t kMaximumWavPcm16SamplesUVE = 1U << 20U;
inline constexpr std::size_t kMaximumPcm16StreamRefillWindowsUVE = 8U;

struct Pcm16StreamWindowPlanUVE final {
    std::size_t startSample = 0U;
    std::size_t sampleCount = 0U;
    std::size_t nextCursorSample = 0U;
    bool reachedEnd = false;
    bool wrapped = false;
    [[nodiscard]] bool operator==(const Pcm16StreamWindowPlanUVE&) const noexcept = default;
};

/// Owns only persistent bounded PCM16 cursor state. It delegates window and advance semantics to the
/// stateless helpers below, but owns no sample bytes, decoder, refill schedule, voice, device, or backend.
class Pcm16StreamCursorUVE final {
public:
    [[nodiscard]] bool ResetUVE(std::size_t totalSamples, bool loop, std::size_t cursorSample = 0U,
                                std::size_t maximumSamples = kMaximumWavPcm16SamplesUVE) noexcept;
    [[nodiscard]] bool ConsumeWindowUVE(std::size_t requestedSamples,
                                         Pcm16StreamWindowPlanUVE& outPlan) noexcept;
    [[nodiscard]] bool AdvanceUVE(std::size_t advanceSamples, bool& outReachedEnd,
                                  bool& outWrapped) noexcept;

    [[nodiscard]] std::size_t GetTotalSamplesUVE() const noexcept { return m_totalSamples; }
    [[nodiscard]] std::size_t GetCursorSampleUVE() const noexcept { return m_cursorSample; }
    [[nodiscard]] bool IsLoopingUVE() const noexcept { return m_loop; }
    [[nodiscard]] std::size_t GetMaximumSamplesUVE() const noexcept { return m_maximumSamples; }

private:
    std::size_t m_totalSamples = 0U;
    std::size_t m_cursorSample = 0U;
    std::size_t m_maximumSamples = kMaximumWavPcm16SamplesUVE;
    bool m_loop = false;
};

/// Owns a fixed-capacity FIFO of planned PCM16 refill windows and a persistent cursor. It owns no sample
/// bytes, decoder, refill thread, voice, device, or backend; callers pop plans and perform refill work.
class Pcm16StreamRefillSchedulerUVE final {
public:
    [[nodiscard]] bool ResetUVE(std::size_t totalSamples, bool loop, std::size_t cursorSample = 0U,
                                std::size_t maximumSamples = kMaximumWavPcm16SamplesUVE) noexcept;
    [[nodiscard]] bool ScheduleWindowUVE(std::size_t requestedSamples) noexcept;
    [[nodiscard]] bool PopNextWindowUVE(Pcm16StreamWindowPlanUVE& outPlan) noexcept;

    [[nodiscard]] std::size_t GetTotalSamplesUVE() const noexcept { return m_cursor.GetTotalSamplesUVE(); }
    [[nodiscard]] std::size_t GetCursorSampleUVE() const noexcept { return m_cursor.GetCursorSampleUVE(); }
    [[nodiscard]] bool IsLoopingUVE() const noexcept { return m_cursor.IsLoopingUVE(); }
    [[nodiscard]] std::size_t GetPendingWindowCountUVE() const noexcept { return m_count; }

private:
    Pcm16StreamCursorUVE m_cursor;
    std::array<Pcm16StreamWindowPlanUVE, kMaximumPcm16StreamRefillWindowsUVE> m_pending{};
    std::size_t m_head = 0U;
    std::size_t m_count = 0U;
};

/// Plans one contiguous bounded PCM16 refill window from a caller-owned cursor. Both totalSamples and
/// requestedSamples must remain within maximumSamples. The planner does not mutate a cursor, decode bytes,
/// schedule refills, own voices, or select a stream backend.
[[nodiscard]] bool PlanPcm16StreamWindowUVE(
    std::size_t totalSamples, std::size_t cursorSample, std::size_t requestedSamples,
    bool loop, Pcm16StreamWindowPlanUVE& outPlan,
    std::size_t maximumSamples = kMaximumWavPcm16SamplesUVE) noexcept;
/// Advances a caller-owned PCM16 sample cursor without decoding, scheduling refills, or owning a stream.
/// Non-looping cursors clamp at totalSamples; looping cursors wrap to the beginning after the end.
[[nodiscard]] bool AdvancePcm16StreamCursorUVE(
    std::size_t totalSamples, std::size_t cursorSample, std::size_t advanceSamples, bool loop,
    std::size_t& outCursorSample, bool& outReachedEnd, bool& outWrapped,
    std::size_t maximumSamples = kMaximumWavPcm16SamplesUVE) noexcept;
/// Validates one bounded PCM16 sample window for caller-owned streaming/chunk preparation.
[[nodiscard]] bool ValidateWavPcm16SampleWindowUVE(std::size_t totalSamples,
                                                    std::size_t startSample,
                                                    std::size_t requestedSamples,
                                                    std::size_t maximumSamples = kMaximumWavPcm16SamplesUVE) noexcept;
/// Decodes validated PCM16 WAV bytes into copied normalized float samples.
/// Supports only RIFF/WAVE PCM16; owns no decoder, stream, device, or voice lifetime.
[[nodiscard]] bool DecodeWavPcm16SamplesUVE(const std::vector<std::byte>& wavBytes,
                                            std::vector<float>& outSamples) noexcept;

/// Decodes one bounded PCM16 sample window directly from caller-owned WAV bytes with atomic output
/// publication. It owns no stream cursor, refill schedule, decoder, device, or voice lifetime.
[[nodiscard]] bool DecodeWavPcm16SampleWindowUVE(
    const std::vector<std::byte>& wavBytes, std::size_t startSample, std::size_t requestedSamples,
    std::vector<float>& outSamples, std::size_t maximumSamples = kMaximumWavPcm16SamplesUVE);
} // namespace UVE::Audio
