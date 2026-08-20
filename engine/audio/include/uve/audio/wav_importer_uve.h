// Copyright (c) 2026 UniVex Studios. All Rights Reserved.


#pragma once

#include "uve/asset/i_asset_importer_uve.h"

namespace UVE::Audio {

/// Registers the bounded raw PCM16 WAV importer for `.wav`. The importer decodes through the
/// decoder-independent WAV/PCM16 contracts, persists one normalized interleaved AudioAssetUVE as a
/// `.uveaudio` envelope, and owns no stream cursor, mixer, voice, device, or platform backend.
void RegisterWavImporterUVE(UVE::Asset::IAssetImporterUVE& importer);

} // namespace UVE::Audio
