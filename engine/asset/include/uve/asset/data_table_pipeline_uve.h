// Copyright (c) 2026 UniVex Studios. All Rights Reserved.

#pragma once

namespace UVE::Asset {

class IAssetImporterUVE;
class IAssetManagerUVE;

/// Registers the Data Table source importers and typed asset-manager loader on existing services.
/// This helper owns no service, queue, cache, registry, path, worker, or loaded table state.
void RegisterDataTablePipelineUVE(IAssetImporterUVE& importer, IAssetManagerUVE& assetManager);

} // namespace UVE::Asset
