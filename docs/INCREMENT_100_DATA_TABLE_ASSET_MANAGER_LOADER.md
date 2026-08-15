// Copyright (c) 2026 UniVex Studios. All Rights Reserved.

# Increment 100 — Typed Data Table Asset-Manager Loader v1

## Purpose

Increment 100 connects the validated `.uvetable` asset to the existing generic `AssetManagerUVE`. A single explicit registration helper binds `DataTableUVE` to `LoadDataTableAssetUVE`; callers then use the manager's ordinary asynchronous, reference-counted `AssetHandleUVE<DataTableUVE>` path.

## Contract

| Concern | Contract |
|---|---|
| Registration | `RegisterDataTableAssetLoaderUVE(IAssetManagerUVE&)` synchronously replaces/registers the loader for the `DataTableUVE` C++ type. |
| Load | `AssetManagerUVE::LoadUVE<DataTableUVE>(guid, database)` resolves the registered path and performs the load on the manager's worker thread. |
| Success | A valid `.uvetable` produces a ready typed handle whose table is an owned manager record and whose snapshot is safe to copy. |
| Failure | Missing paths, wrong envelope kinds, malformed documents, and invalid table payloads follow generic manager failed-handle semantics. No invalid table is retained. |
| Lifecycle | Reference counting and garbage collection remain `AssetManagerUVE` responsibilities; the helper retains no manager, database, path, thread, or table state. |

## Boundaries

The typed loader is an adapter, not a second asset manager. It does not scan directories, register database paths, mutate `DataTableRegistryUVE`, update editor bridge snapshots, perform hot reload, infer schemas, bind ECS/runtime objects, or transfer managed pointers. Future hot-reload support must reuse the manager's explicit `ReloadUVE` contract and separately define registry/editor synchronization behavior.

## Verification target

The increment requires successful typed async handle loading, wrong-kind failure coverage, existing asset-store and registration regression coverage, full GCC/Clang/CI validation, managed regression tests, and a signed Green PR before roadmap promotion.
