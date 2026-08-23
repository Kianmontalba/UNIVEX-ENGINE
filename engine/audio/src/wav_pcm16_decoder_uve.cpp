// Copyright (c) 2026 UniVex Studios. All Rights Reserved.
#include "uve/audio/wav_pcm16_decoder_uve.h"
#include "uve/audio/wav_metadata_uve.h"
#include "uve/audio/pcm_gain_effect_uve.h"
#include <algorithm>
#include <cmath>
#include <cstdint>
#include <limits>
#include <new>
#include <cstring>
#include <utility>
namespace UVE::Audio {
namespace {
[[nodiscard]] std::uint16_t ReadU16LE(const std::vector<std::byte>& bytes, const std::size_t offset) noexcept {
    return static_cast<std::uint16_t>(std::to_integer<std::uint8_t>(bytes[offset]) |
                                      (std::to_integer<std::uint8_t>(bytes[offset + 1U]) << 8U));
}
[[nodiscard]] std::uint32_t ReadU32LE(const std::vector<std::byte>& bytes, const std::size_t offset) noexcept {
    return static_cast<std::uint32_t>(std::to_integer<std::uint8_t>(bytes[offset]) |
                                      (std::to_integer<std::uint8_t>(bytes[offset + 1U]) << 8U) |
                                      (std::to_integer<std::uint8_t>(bytes[offset + 2U]) << 16U) |
                                      (std::to_integer<std::uint8_t>(bytes[offset + 3U]) << 24U));
}
[[nodiscard]] bool HasTag(const std::vector<std::byte>& bytes, std::size_t offset, const char* tag) noexcept {
    return offset + 4U <= bytes.size() && std::memcmp(bytes.data() + offset, tag, 4U) == 0;
}
} // namespace

bool ApplyPcmGainEffectChainUVE(const std::vector<float>& inputSamples,
                                  const std::vector<float>& gains,
                                  std::vector<float>& outputSamples) noexcept {
    if (inputSamples.size() > kMaximumPcmGainSamplesUVE ||
        gains.size() > kMaximumPcmGainChainEffectsUVE) {
        return false;
    }
    for (const float sample : inputSamples) {
        if (!std::isfinite(sample)) {
            return false;
        }
    }
    try {
        if (gains.empty()) {
            outputSamples = inputSamples;
            return true;
        }
        std::vector<float> working = inputSamples;
        std::vector<float> next;
        for (const float gain : gains) {
            if (!ApplyPcmGainEffectUVE(working, gain, next)) {
                return false;
            }
            working.swap(next);
            next.clear();
        }
        outputSamples = std::move(working);
        return true;
    } catch (...) {
        return false;
    }
}

bool Pcm16StreamCursorUVE::ResetUVE(const std::size_t totalSamples, const bool loop,
                                     const std::size_t cursorSample, const std::size_t maximumSamples) noexcept {
    if (totalSamples == 0U || maximumSamples == 0U || totalSamples > maximumSamples || cursorSample > totalSamples) {
        return false;
    }
    m_totalSamples = totalSamples;
    m_cursorSample = cursorSample;
    m_maximumSamples = maximumSamples;
    m_loop = loop;
    return true;
}

bool Pcm16StreamCursorUVE::ConsumeWindowUVE(const std::size_t requestedSamples,
                                             Pcm16StreamWindowPlanUVE& outPlan) noexcept {
    Pcm16StreamWindowPlanUVE candidatePlan;
    if (!PlanPcm16StreamWindowUVE(m_totalSamples, m_cursorSample, requestedSamples, m_loop, candidatePlan,
                                  m_maximumSamples)) {
        return false;
    }
    m_cursorSample = candidatePlan.nextCursorSample;
    outPlan = candidatePlan;
    return true;
}

bool Pcm16StreamCursorUVE::AdvanceUVE(const std::size_t advanceSamples, bool& outReachedEnd,
                                      bool& outWrapped) noexcept {
    std::size_t candidateCursor = m_cursorSample;
    bool candidateReachedEnd = outReachedEnd;
    bool candidateWrapped = outWrapped;
    if (!AdvancePcm16StreamCursorUVE(m_totalSamples, m_cursorSample, advanceSamples, m_loop, candidateCursor,
                                      candidateReachedEnd, candidateWrapped, m_maximumSamples)) {
        return false;
    }
    m_cursorSample = candidateCursor;
    outReachedEnd = candidateReachedEnd;
    outWrapped = candidateWrapped;
    return true;
}

bool Pcm16StreamRefillSchedulerUVE::ResetUVE(const std::size_t totalSamples, const bool loop,
                                               const std::size_t cursorSample,
                                               const std::size_t maximumSamples) noexcept {
    Pcm16StreamCursorUVE candidateCursor;
    if (!candidateCursor.ResetUVE(totalSamples, loop, cursorSample, maximumSamples)) {
        return false;
    }
    m_cursor = candidateCursor;
    m_pending.fill(Pcm16StreamWindowPlanUVE{});
    m_head = 0U;
    m_count = 0U;
    return true;
}

bool Pcm16StreamRefillSchedulerUVE::ScheduleWindowUVE(const std::size_t requestedSamples) noexcept {
    if (m_count >= kMaximumPcm16StreamRefillWindowsUVE) {
        return false;
    }
    Pcm16StreamWindowPlanUVE candidatePlan;
    Pcm16StreamCursorUVE candidateCursor = m_cursor;
    if (!candidateCursor.ConsumeWindowUVE(requestedSamples, candidatePlan) || candidatePlan.sampleCount == 0U) {
        return false;
    }
    const std::size_t writeIndex = (m_head + m_count) % kMaximumPcm16StreamRefillWindowsUVE;
    m_pending[writeIndex] = candidatePlan;
    m_cursor = candidateCursor;
    ++m_count;
    return true;
}

bool Pcm16StreamRefillSchedulerUVE::PopNextWindowUVE(Pcm16StreamWindowPlanUVE& outPlan) noexcept {
    if (m_count == 0U) {
        return false;
    }
    const Pcm16StreamWindowPlanUVE candidatePlan = m_pending[m_head];
    m_pending[m_head] = Pcm16StreamWindowPlanUVE{};
    m_head = (m_head + 1U) % kMaximumPcm16StreamRefillWindowsUVE;
    --m_count;
    outPlan = candidatePlan;
    return true;
}

bool PlanPcm16StreamWindowUVE(const std::size_t totalSamples, const std::size_t cursorSample,
                               const std::size_t requestedSamples, const bool loop,
                               Pcm16StreamWindowPlanUVE& outPlan, const std::size_t maximumSamples) noexcept {
    if (totalSamples == 0U || maximumSamples == 0U || totalSamples > maximumSamples || requestedSamples == 0U ||
        requestedSamples > maximumSamples || cursorSample > totalSamples) {
        return false;
    }
    if (cursorSample == totalSamples && !loop) {
        outPlan = Pcm16StreamWindowPlanUVE{totalSamples, 0U, totalSamples, true, false};
        return true;
    }
    const bool wrappedBeforeWindow = cursorSample == totalSamples;
    const std::size_t startSample = wrappedBeforeWindow ? 0U : cursorSample;
    const std::size_t availableSamples = totalSamples - startSample;
    const std::size_t sampleCount = std::min(requestedSamples, availableSamples);
    const bool reachedEnd = sampleCount == availableSamples;
    const bool wrapped = wrappedBeforeWindow || (loop && reachedEnd);
    const std::size_t nextCursorSample = reachedEnd ? (loop ? 0U : totalSamples) : startSample + sampleCount;
    outPlan = Pcm16StreamWindowPlanUVE{startSample, sampleCount, nextCursorSample, reachedEnd, wrapped};
    return true;
}

bool AdvancePcm16StreamCursorUVE(const std::size_t totalSamples, const std::size_t cursorSample,
                                    const std::size_t advanceSamples, const bool loop,
                                    std::size_t& outCursorSample, bool& outReachedEnd,
                                    bool& outWrapped, const std::size_t maximumSamples) noexcept {
    if (totalSamples == 0U || totalSamples > maximumSamples || maximumSamples == 0U ||
        cursorSample > totalSamples) {
        return false;
    }
    if (!loop && cursorSample == totalSamples) {
        outCursorSample = totalSamples;
        outReachedEnd = true;
        outWrapped = false;
        return true;
    }
    const std::size_t normalizedCursor = cursorSample == totalSamples ? 0U : cursorSample;
    const std::size_t remainingSamples = totalSamples - normalizedCursor;
    const bool reachedEnd = advanceSamples >= remainingSamples;
    if (!loop) {
        outCursorSample = reachedEnd ? totalSamples : normalizedCursor + advanceSamples;
        outReachedEnd = reachedEnd;
        outWrapped = false;
        return true;
    }
    if (!reachedEnd) {
        outCursorSample = normalizedCursor + advanceSamples;
    } else {
        outCursorSample = (advanceSamples - remainingSamples) % totalSamples;
    }
    outReachedEnd = reachedEnd;
    outWrapped = cursorSample == totalSamples || reachedEnd;
    return true;
}

bool ValidateWavPcm16SampleWindowUVE(const std::size_t totalSamples,
                                     const std::size_t startSample,
                                     const std::size_t requestedSamples,
                                     const std::size_t maximumSamples) noexcept {
    if (totalSamples == 0U || requestedSamples == 0U || maximumSamples == 0U ||
        requestedSamples > maximumSamples || startSample > totalSamples ||
        requestedSamples > totalSamples - startSample) {
        return false;
    }
    return true;
}

bool DecodeWavPcm16SampleWindowUVE(const std::vector<std::byte>& wavBytes,
                                      const std::size_t startSample, const std::size_t requestedSamples,
                                      std::vector<float>& outSamples, const std::size_t maximumSamples) {
    const auto metadata = ParseWavMetadataUVE(wavBytes);
    if (!metadata || metadata->audioFormat != 1U || metadata->bitsPerSample != 16U ||
        metadata->blockAlign == 0U || metadata->dataBytes % metadata->blockAlign != 0U) {
        return false;
    }
    const std::size_t totalSamples = metadata->dataBytes / 2U;
    if (!ValidateWavPcm16SampleWindowUVE(totalSamples, startSample, requestedSamples, maximumSamples)) {
        return false;
    }
    std::size_t dataOffset = 12U;
    while (dataOffset + 8U <= wavBytes.size()) {
        const std::uint32_t chunkSize = ReadU32LE(wavBytes, dataOffset + 4U);
        const std::size_t payloadOffset = dataOffset + 8U;
        if (payloadOffset > wavBytes.size() || chunkSize > wavBytes.size() - payloadOffset) {
            return false;
        }
        if (HasTag(wavBytes, dataOffset, "data")) {
            if (chunkSize != metadata->dataBytes) return false;
            const std::size_t firstByte = payloadOffset + startSample * 2U;
            const std::size_t byteCount = requestedSamples * 2U;
            if (firstByte > payloadOffset + chunkSize || byteCount > payloadOffset + chunkSize - firstByte) {
                return false;
            }
            try {
                std::vector<float> samples;
                samples.reserve(requestedSamples);
                for (std::size_t offset = firstByte; offset < firstByte + byteCount; offset += 2U) {
                    const std::int16_t value = static_cast<std::int16_t>(ReadU16LE(wavBytes, offset));
                    samples.push_back(std::clamp(static_cast<float>(value) / 32768.0F, -1.0F, 1.0F));
                }
                outSamples = std::move(samples);
                return true;
            } catch (const std::bad_alloc&) {
                return false;
            }
        }
        const std::size_t payloadEnd = payloadOffset + chunkSize;
        if ((chunkSize & 1U) != 0U && payloadEnd == std::numeric_limits<std::size_t>::max()) return false;
        dataOffset = payloadEnd + (chunkSize & 1U);
    }
    return false;
}

bool DecodeWavPcm16SamplesUVE(const std::vector<std::byte>& wavBytes,
                              std::vector<float>& outSamples) noexcept {
    const auto metadata = ParseWavMetadataUVE(wavBytes);
    if (!metadata || metadata->audioFormat != 1U || metadata->bitsPerSample != 16U ||
        metadata->blockAlign == 0U || metadata->dataBytes % metadata->blockAlign != 0U) {
        return false;
    }
    const std::size_t sampleCount = metadata->dataBytes / 2U;
    if (sampleCount > kMaximumWavPcm16SamplesUVE) {
        return false;
    }
    std::size_t dataOffset = 12U;
    while (dataOffset + 8U <= wavBytes.size()) {
        const std::uint32_t chunkSize = ReadU32LE(wavBytes, dataOffset + 4U);
        const std::size_t payloadOffset = dataOffset + 8U;
        if (payloadOffset > wavBytes.size() || chunkSize > wavBytes.size() - payloadOffset) {
            return false;
        }
        if (HasTag(wavBytes, dataOffset, "data")) {
            if (chunkSize != metadata->dataBytes) return false;
            try {
                std::vector<float> samples;
                samples.reserve(sampleCount);
                for (std::size_t offset = payloadOffset; offset < payloadOffset + chunkSize; offset += 2U) {
                    const std::int16_t value = static_cast<std::int16_t>(ReadU16LE(wavBytes, offset));
                    samples.push_back(std::clamp(static_cast<float>(value) / 32768.0F, -1.0F, 1.0F));
                }
                outSamples = std::move(samples);
                return true;
            } catch (const std::bad_alloc&) {
                return false;
            }
        }
        dataOffset = payloadOffset + chunkSize + (chunkSize & 1U);
    }
    return false;
}
} // namespace UVE::Audio
