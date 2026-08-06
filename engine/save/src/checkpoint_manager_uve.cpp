// Copyright (c) 2026 UniVex Studios. All Rights Reserved.


#include "uve/save/checkpoint_manager_uve.h"

namespace UVE::Save {

CheckpointManagerUVE::CheckpointManagerUVE(ISaveGameSystemUVE& saveGameSystem, double autoSaveIntervalSeconds)
    : m_saveGameSystem(&saveGameSystem), m_autoSaveIntervalSeconds(autoSaveIntervalSeconds) {}

void CheckpointManagerUVE::UpdateUVE(double deltaTimeSeconds, Scene::IEntityManagerUVE& entityManager,
                                      const std::vector<Scene::EntityUVE>& rootEntities) {
    m_totalPlaytimeSeconds += deltaTimeSeconds;
    m_elapsedSinceLastSaveSeconds += deltaTimeSeconds;

    if (m_elapsedSinceLastSaveSeconds >= m_autoSaveIntervalSeconds) {
        static_cast<void>(SaveCheckpointUVE(entityManager, rootEntities));
    }
}

bool CheckpointManagerUVE::CheckpointUVE(Scene::IEntityManagerUVE& entityManager,
                                          const std::vector<Scene::EntityUVE>& rootEntities) {
    return SaveCheckpointUVE(entityManager, rootEntities);
}

void CheckpointManagerUVE::SetAutoSaveIntervalSecondsUVE(double intervalSeconds) noexcept {
    m_autoSaveIntervalSeconds = intervalSeconds;
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

bool CheckpointManagerUVE::SaveCheckpointUVE(Scene::IEntityManagerUVE& entityManager,
                                              const std::vector<Scene::EntityUVE>& rootEntities) {
    GameStateMetadataUVE metadata;
    metadata.playtimeSeconds = m_totalPlaytimeSeconds;

    const bool saved = m_saveGameSystem->SaveUVE(kAutoSaveSlotIndexUVE, entityManager, rootEntities, metadata);
    m_elapsedSinceLastSaveSeconds = 0.0;
    return saved;
}

} // namespace UVE::Save
