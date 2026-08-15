// Copyright (c) 2026 UniVex Studios. All Rights Reserved.

#pragma once

namespace UVE::Asset {

class IAssetManagerUVE;

/// Registers the existing envelope-backed DataTableUVE loader with the supplied asset manager.
/// Registration is synchronous; individual loads retain AssetManagerUVE's asynchronous and
/// reference-counted ownership semantics.
void RegisterDataTableAssetLoaderUVE(IAssetManagerUVE& assetManager);

} // namespace UVE::Asset
