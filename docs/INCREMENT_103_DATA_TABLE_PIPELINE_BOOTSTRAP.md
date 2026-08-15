// Copyright (c) 2026 UniVex Studios. All Rights Reserved.

# Increment 103 — Data Table Pipeline Bootstrap v1

## Purpose

Increment 103 adds one explicit composition helper, `RegisterDataTablePipelineUVE`, that registers the existing schema-driven `.csv`/`.tsv`/`.json` Data Table importers and the existing typed `DataTableUVE` asset-manager loader on caller-owned services.

## Contract

| Concern | Contract |
|---|---|
| Entry point | `RegisterDataTablePipelineUVE(IAssetImporterUVE&, IAssetManagerUVE&)`. |
| Registration order | Source importer registration occurs first; typed loader registration occurs second. |
| Ownership | The helper owns no importer, manager, database, queue, cache, registry, path, worker, or loaded table. |
| Import behavior | Callers still supply `DataTableImportSettingsUVE`; the existing importer writes validated `.uvetable` assets and generic database registration remains explicit. |
| Load behavior | Callers still resolve a GUID and use `AssetManagerUVE::LoadUVE<DataTableUVE>`; async worker, handle, failure, and garbage-collection semantics remain generic. |
| Repeat calls | Repeated registration is intentionally delegated to the existing replace-on-register contracts and does not create duplicate service state. |

## Boundaries

This helper is a wiring convenience, not a new pipeline authority. It does not construct or retain services, enqueue jobs, manage import caches, scan directories, infer schemas, watch files, hot reload tables, mutate `DataTableRegistryUVE`, update the editor bridge, bind runtime/ECS objects, or transfer managed pointers. Applications remain responsible for service lifetime and for choosing when to import or load.

## Verification target

The increment requires one composition test proving that a single bootstrap call can import CSV to a validated `.uvetable` and load it through a typed asynchronous handle. Full GCC/Clang/CI validation, managed regression tests, and a signed Green PR are required before roadmap promotion.
