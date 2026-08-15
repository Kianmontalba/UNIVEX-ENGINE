// Copyright (c) 2026 UniVex Studios. All Rights Reserved.

# Increment 101 — Schema-Driven Data Table Source Importers v1

## Purpose

Increment 101 adds explicit source importers for `.csv`, `.tsv`, and `.json` files. Each importer receives caller-supplied table name and typed column settings, validates the source through the existing `DataTableUVE` importer, writes one `.uvetable` envelope via `SaveDataTableAssetUVE`, and lets the generic importer register the destination path in the asset database.

## Contract

| Concern | Contract |
|---|---|
| Registration | `RegisterDataTableImportersUVE(IAssetImporterUVE&)` registers `.csv`, `.tsv`, and `.json` handlers. |
| Schema | `DataTableImportSettingsUVE` supplies a non-empty table name and ordered typed columns; automatic inference is intentionally unsupported. |
| Source bound | Source text is read synchronously and rejected above `DataTableUVE::kMaximumDocumentBytesUVE`. |
| Destination | The importer requires a `.uvetable` destination and writes the existing universal DataTable envelope. |
| Database | `AssetImporterUVE::ImportUVE` registers the successful destination using its established generic path/GUID behavior. |
| Failure | Missing settings, malformed source, typed value errors, invalid schema, wrong destination extension, and write failures return `kInvalidAssetGuidUVE` without registering a destination. |

## Ownership and non-goals

The importer owns no persistent schema, path, thread, cache, registry session, or editor state. It does not scan directories, infer schemas, enqueue work, watch files, hot reload tables, mutate `DataTableRegistryUVE`, update bridge snapshots, bind runtime/ECS objects, or transfer managed pointers. Future importer settings must extend the typed settings contract and update its cache discriminator when output changes.

## Verification target

The increment requires CSV/TSV/JSON end-to-end conversion tests, typed destination load checks, malformed and missing-settings rejection, database non-registration assertions, full GCC/Clang/CI validation, managed regression tests, and a signed Green PR before roadmap promotion.
