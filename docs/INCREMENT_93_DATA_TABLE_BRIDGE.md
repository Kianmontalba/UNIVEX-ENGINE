# Increment 93 — Read-Only Data Table Catalog Bridge v1

Increment 93 exposes bounded native data-table catalog descriptors to the managed editor through the existing value-only bridge snapshot. The bridge is deliberately read-only: it carries presentation facts but introduces no catalog mutation request, file operation, or runtime binding.

## Native and transport contract

The native bridge owns a copied `DataTableCatalogSnapshotUVE` injection seam and converts it to a bridge DTO limited to the existing 128-entry panel bound. Each record contains a bounded table name, source generation, column count, row count, and validity derived from native diagnostics. Catalog changes participate in observed-state comparison and increment the bridge revision.

The stdio response adds an additive `dataTableCatalog` object containing `generation`, `entriesTruncated`, and `entries`. Existing protocol consumers may ignore the new object, while newer managed hosts parse it with strict types and bounds.

## Managed presentation

The managed parser falls back to an empty catalog when the property is absent, preserving compatibility with older native backends. The Avalonia host renders copied descriptor display text in the lower-dock `Data Tables` tab and shows generation/truncation status. There are no edit fields, mutation buttons, or command kinds in this increment.

## Ownership and deferred scope

C++ remains authoritative for table schema, rows, diagnostics, asset envelopes, files, and future asset-database integration. C# stores copied DTOs only. Filesystem scanning, asset registration, content hashing, hot reload, editor-side table editing, managed asset mutation, runtime bindings, references, XLSX, and visual-scripting nodes remain deferred.
