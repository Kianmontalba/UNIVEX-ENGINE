// Copyright (c) 2026 UniVex Studios. All Rights Reserved.
#pragma once

#include "uve/scene/components/audio_source_component_uve.h"

namespace UVE::Scene::Nodes {

/// User-facing AudioSource3D façade. Audio playback remains owned by the audio source system.
using AudioSource3DNodeUVE = AudioSourceComponentUVE;
using AudioSource3DNodeFacadeUVE = AudioSource3DNodeUVE;
using AudioAttenuationCurveNodeUVE = AudioAttenuationCurveUVE;

} // namespace UVE::Scene::Nodes
