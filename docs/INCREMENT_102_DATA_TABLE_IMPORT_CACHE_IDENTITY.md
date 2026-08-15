// Copyright (c) 2026 UniVex Studios. All Rights Reserved.

# Increment 102 — Deterministic Data Table Import-Cache Identity v1

## Purpose

Increment 102 makes Data Table import settings part of the generic derived-artifact cache identity. The `DataTableImportSettingsUVE::GetCacheVersionUVE()` value now contains a stable version prefix and a fixed-width FNV-1a digest over the table name and ordered typed column schema.

## Contract

| Concern | Contract |
|---|---|
| Inputs | Table name, column count, each ordered column name, and each ordered `DataTableColumnTypeUVE` value participate in the digest. |
| Encoding | Each string token is length-delimited before hashing, avoiding ambiguity from concatenated names or delimiters. |
| Output | `data-table-import-v2-` followed by 16 lowercase hexadecimal characters. |
| Determinism | Equal settings produce equal values across calls; a table-name, column-order, or column-type change produces a different identity in the tested cases. |
| Queue behavior | `AssetImportQueueUVE` copies the settings version at enqueue time; a changed version prevents a derived-artifact cache hit and causes explicit reimport. |
| Cache ownership | `DerivedArtifactCacheUVE` remains metadata-only and treats the settings version as opaque data. No cache schema migration is required. |

## Boundaries

This is an identity contract, not a hash-based content validator or security primitive. It does not change source fingerprints, destination fingerprints, cache record schema, importer scheduling, filesystem watching, schema inference, hot reload, registry sessions, editor bridge state, runtime binding, or managed ownership. The queue remains the authority for deciding cache hits, and the importer remains the authority for producing a validated `.uvetable`.

The version prefix is intentionally bumped from the previous fixed `data-table-import-v1` value. Existing records with the old version cannot satisfy a new Data Table settings identity and will be regenerated on the next explicit queue tick.

## Verification target

The increment requires direct deterministic/sensitivity tests plus a queue integration test proving that a schema type change forces a fresh import and updates the stored Data Table envelope. Full GCC/Clang/CI validation, managed regression tests, and a signed Green PR are required before roadmap promotion.
