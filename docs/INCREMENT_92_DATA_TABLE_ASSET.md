# Increment 92 — Data Table Asset Envelope and Catalog v1

Increment 92 adds a versioned in-memory asset envelope and a read-only catalog descriptor seam on top of the typed data-table core. The envelope is JSON-shaped for portability but is not a general-purpose import format; its purpose is deterministic persistence of an already validated native table.

## Envelope contract

The fixed format identifier is `uve.data_table` and the current schema version is `1`. An envelope contains a table name, ordered column descriptors, and ordered rows. Values are explicitly tagged as `boolean`, `integer`, `number`, or `string`; no inference is performed. Serialization preserves schema and row order and emits stable compact JSON. Deserialization validates into a temporary table and replaces the destination only after the complete envelope succeeds.

Unknown versions, wrong format identifiers, malformed JSON, duplicate columns or rows, missing or extra envelope members, wrong tagged value kinds, non-finite numbers, and bounds violations are rejected without changing the destination table.

## Catalog contract

`DataTableCatalogUVE` stores copied descriptors rather than rows or pointers. Each descriptor contains the bounded table name, source generation, column count, row count, and a validity flag derived from diagnostics. Catalog snapshots are copied and sorted lexicographically by name, making them suitable for future read-only editor presentation.

The catalog has a maximum of 1,024 entries. Upsert and removal are explicit mutations with generation tracking. Repeating an identical upsert is a no-op, and removing a missing entry is rejected without mutation.

## Ownership boundary

The serializer and catalog do not read files, scan directories, register with the asset database, hash content, hot-reload tables, or cross the managed/editor bridge. They do not own ECS, renderer, process, network, runtime-binding, reference, or visual-scripting authority. These capabilities remain separate reviewed increments.
