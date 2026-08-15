# Increment 91 — Data Table TSV/JSON Import v1

Increment 91 extends the Increment 90 typed data-table core with deterministic TSV and JSON adapters. Both adapters reuse the caller-declared schema, preserve bounded native ownership, publish copied diagnostics, and replace rows only after a complete successful import.

## TSV contract

TSV uses the same bounded quoted-delimited behavior as CSV, with a tab delimiter. It accepts CRLF/LF endings, quoted fields, doubled quotes, and empty string fields. The first header field must be `id`; remaining headers must match the declared schema exactly and in order. Values are converted according to the declared Boolean, Integer, Number, or String column type.

## JSON contract

JSON input must be a top-level array of objects. Each object must contain exactly one string `id` and one member for every schema column. Object member order is irrelevant; output row values always follow schema order. Boolean, integer, finite-number, and string JSON kinds are checked directly. Extra members, missing members, null values, wrong kinds, duplicate identifiers, malformed JSON, and out-of-bounds strings are rejected.

| Limit | Value |
|---|---:|
| Input document | 4 MiB |
| Columns | 64 |
| Rows | 4,096 |
| Identifier bytes | 64 |
| String value bytes | 1,024 |
| Diagnostics | 128 |

A failed TSV or JSON import advances the table generation and publishes diagnostics but leaves the previously committed rows unchanged. A successful import atomically replaces the complete row set and clears prior diagnostics.

Filesystem access, asset-database registration, schema inference or migration, hot reload, export, references, XLSX parsing, editor bridge, managed UI, runtime bindings, and visual-scripting nodes remain deferred.
