// Copyright (c) 2026 UniVex Studios. All Rights Reserved.

# Increment 99 — Validated Data Table Asset-Database Registration v1

## Purpose

Increment 99 connects the envelope-backed Data Table asset to the existing generic asset database through an explicit validation gate. The helper accepts one path, requires the `.uvetable` extension, loads and validates the complete `AssetKindUVE::DataTable` payload, and only then registers the path in the caller-provided database.

## Contract

| Concern | Contract |
|---|---|
| Entry point | `RegisterDataTableAssetUVE(IAssetDatabaseUVE&, path)` returns `std::optional<AssetGuidUVE>`. |
| Extension | The path must be non-empty and end in `.uvetable`; other extensions reject before database access. |
| Validation | `LoadDataTableAssetUVE` validates the universal envelope, asset kind, deterministic table JSON, schema, and rows before registration. |
| Identity | The generic database retains its established lexical path normalization and returns the existing GUID for equivalent repeated registration. |
| Failure | Invalid extension, missing file, wrong kind, malformed payload, or invalid table returns `nullopt` and leaves the database unchanged. |
| Ownership | The database owns only the normalized path/GUID record. The helper retains no path, table, file handle, registry reference, or background task. |

## Boundaries

This helper is an explicit registration operation, not a directory scanner or importer. It does not enqueue work, infer schemas, watch files, hot reload sessions, mutate `DataTableRegistryUVE`, update the editor bridge, bind runtime/ECS objects, or transfer managed pointers. The generic database continues to expose registration as a path identity record; successful registration does not imply that the file remains loaded or that a table instance is resident.

The validate-before-register ordering is deliberate. A failed load cannot create a stale or non-table database record, and a repeated valid registration remains idempotent because the generic database owns lexical identity resolution.

## Verification target

The increment requires valid registration/idempotency coverage, lexical resolution checks, wrong-extension rejection, wrong-kind rejection, missing-file rejection, database non-mutation assertions, full GCC/Clang/CI validation, managed regression tests, and a signed Green PR before roadmap promotion.
