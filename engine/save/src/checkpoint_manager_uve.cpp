// Copyright (c) 2026 UniVex Studios. All Rights Reserved.


#include "uve/save/checkpoint_manager_uve.h"

#include <cmath>

namespace UVE::Save {
namespace {

constexpr double kDefaultAutoSaveIntervalSecondsUVE = 300.0;

[[nodiscard]] bool IsValidAutoSaveIntervalUVE(const double intervalSeconds) noexcept {
    return std::isfinite(intervalSeconds) && intervalSeconds > 0.0;
}

} // namespace

CheckpointManagerUVE::CheckpointManagerUVE(ISaveGameSystemUVE& saveGameSystem,
                                             double autoSaveIntervalSeconds)
    : m_saveGameSystem(&saveGameSystem),
      m_autoSaveIntervalSeconds(IsValidAutoSaveIntervalUVE(autoSaveIntervalSeconds)
                                    ? autoSaveIntervalSeconds
                                    : kDefaultAutoSaveIntervalSecondsUVE) {}

void CheckpointManagerUVE::UpdateUVE(double deltaTimeSeconds, Scene::IEntityManagerUVE& entityManager,
                                      const std::vector<Scene::EntityUVE>& rootEntities) {
    if (!std::isfinite(deltaTimeSeconds) || deltaTimeSeconds < 0.0) {
        return;
    }
    m_totalPlaytimeSeconds += deltaTimeSeconds;
    m_elapsedSinceLastSaveSeconds += deltaTimeSeconds;

    if (m_elapsedSinceLastSaveSeconds >= m_autoSaveIntervalSeconds) {
        static_cast<void>(SaveCheckpointUVE(kAutoSaveSlotIndexUVE, entityManager, rootEntities));
    }
}

bool CheckpointManagerUVE::CheckpointUVE(Scene::IEntityManagerUVE& entityManager,
                                          const std::vector<Scene::EntityUVE>& rootEntities) {
    return SaveCheckpointUVE(kManualCheckpointSlotIndexUVE, entityManager, rootEntities);
}

void CheckpointManagerUVE::SetAutoSaveIntervalSecondsUVE(const double intervalSeconds) noexcept {
    if (IsValidAutoSaveIntervalUVE(intervalSeconds)) {
        m_autoSaveIntervalSeconds = intervalSeconds;
    }
}

double CheckpointManagerUVE::GetAutoSaveIntervalSecondsUVE() const noexcept {
    return m_autoSaveIntervalSeconds;
}

double CheckpointManagerUVE::GetElapsedSinceLastSaveSecondsUVE() const noexcept {
    return m_elapsedSinceLastSaveSeconds;
}

double CheckpointManagerUVE::GetTotalPlaytimeSecondsUVE() const noexcept {
    return m_totalPlaytimeSeconds;
}

bool CheckpointManagerUVE::SaveCheckpointUVE(const int slotIndex, Scene::IEntityManagerUVE& entityManager,
                                              const std::vector<Scene::EntityUVE>& rootEntities) {
    GameStateMetadataUVE metadata;
    metadata.playtimeSeconds = m_totalPlaytimeSeconds;

    const bool saved = m_saveGameSystem->SaveUVE(slotIndex, entityManager, rootEntities, metadata);
    if (saved) {
        m_elapsedSinceLastSaveSeconds = 0.0;
    }
    return saved;
}

} // namespace UVE::Save
