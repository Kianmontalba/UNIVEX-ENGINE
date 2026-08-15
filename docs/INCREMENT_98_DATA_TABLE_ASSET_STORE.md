// Copyright (c) 2026 UniVex Studios. All Rights Reserved.

# Increment 98 — Data Table `.uve*` Asset Store v1

## Purpose

Increment 98 gives the typed data-table serializer a filesystem envelope boundary without creating a scanner, database, or hot-reload service. A `DataTableUVE` is persisted as the existing deterministic JSON document inside the universal `.uve*` envelope under the new `AssetKindUVE::DataTable` value.

## Contract

| Operation | Contract |
|---|---|
| Save | `SaveDataTableAssetUVE` serializes the caller's validated table without mutating it and writes one envelope-tagged file. Invalid names or oversized serialized documents are rejected before writing. |
| Load | `LoadDataTableAssetUVE` reads the universal envelope, requires `AssetKindUVE::DataTable`, deserializes into a temporary `DataTableUVE`, and moves it into the destination only after complete validation. |
| Payload | The payload remains the existing `DataTableAssetSerializerUVE` JSON document. No second table format or duplicate schema/value encoding is introduced. |
| Failure behavior | Missing files, invalid envelopes, wrong asset kinds, malformed JSON, schema errors, and row/value errors return `false`; the destination table remains unchanged on load failure. |
| Ownership | The caller owns the `DataTableUVE` and filesystem path. The store is synchronous and does not retain paths, open handles, registry references, or background jobs. |

## Boundaries

This increment persists one explicitly requested table per file. It does not scan directories, register paths or GUIDs in `AssetDatabaseUVE`, schedule imports, perform hot reload, infer schemas, update `DataTableRegistryUVE`, mutate editor bridge state, bind runtime/ECS objects, or expose managed pointers. Those concerns remain separate milestones with their own ownership and failure contracts.

The universal envelope remains the only binary framing authority. Adding `AssetKindUVE::DataTable` prevents a table file from being accepted as a scene, material, bundle, or other asset kind while preserving the established `ReadUveFileUVE` and `WriteUveFileUVE` diagnostics behavior.

## Verification target

The increment requires focused round-trip, wrong-kind, malformed-payload, missing-file, invalid-source, and failure-atomic tests, full GCC/Clang/CI native validation, managed regression tests, and a signed Green PR before roadmap promotion.
