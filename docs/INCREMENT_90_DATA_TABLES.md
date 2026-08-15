# Increment 90 — Typed Data Table Core v1

## Purpose

Increment 90 introduces the first bounded native data-table foundation for UniVex. It is intentionally a **typed value model and deterministic CSV ingestion seam**, not yet a spreadsheet editor, asset-database importer, hot-reload service, or visual-scripting runtime binding.

## Contract

`DataTableUVE` owns an ordered schema, unique bounded row identifiers, typed value rows, a monotonic generation, and copied diagnostics. The supported column types are `Boolean`, signed 64-bit `Integer`, finite `Number`, and bounded `String`. A row must contain exactly one value for each declared column, and failed mutations do not replace existing rows.

`DataTableCsvImporterUVE` accepts one header row followed by records. The first header field is `id`; remaining headers must match the declared schema exactly and in order. CSV supports comma delimiters, CRLF/LF endings, quoted fields, and doubled quotes. The caller supplies the schema; v1 deliberately performs no type inference.

## Safety bounds

| Fact | Bound |
|---|---:|
| Columns | 64 |
| Rows | 4,096 |
| Identifier bytes | 64 |
| String value bytes | 1,024 |
| CSV document bytes | 4 MiB |
| Diagnostics | 128 |

Boolean values accept only lowercase `true` and `false`. Integer parsing uses a complete signed 64-bit conversion. Number parsing rejects overflow, trailing characters, locale-dependent forms, and non-finite values. CSV errors report a stable diagnostic code with line and column context.

## Ownership and deferrals

The native asset module owns parsing and table values. The table exposes copied snapshots and a read-only row lookup; it does not access the filesystem, asset database, ECS, renderer, process, network, reflection, or managed runtime. JSON/TSV/XLSX adapters, asset references, editor bridge DTOs, hot reload, export, runtime bindings, and visual-scripting nodes remain separate reviewed increments.

## Verification

The regression suite covers deterministic snapshots, duplicate and invalid schema rejection, typed value compatibility, quoted CSV and CRLF handling, invalid numeric/boolean values, header mismatch, duplicate rows, diagnostic generation, and failure atomicity.
