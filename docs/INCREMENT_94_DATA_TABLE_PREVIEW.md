# Increment 94 — Read-Only Data Table Detail Preview Bridge v1

Increment 94 extends the Increment 93 data-table catalog bridge with one bounded native detail preview. The managed editor receives copied column metadata and native-formatted cell display values while C++ retains all schema, row, diagnostics, persistence, and mutation authority.

## Preview contract

The preview contains availability, table name, source generation, total column and row counts, bounded typed column descriptors, bounded row identifiers, bounded display values, truncation facts, and a reason string. The native bridge limits visible columns and rows to the existing 128-entry panel bound and limits names, identifiers, reasons, and cell display text to the existing presentation-text bound.

Boolean, integer, number, and string values are formatted by native code. The managed host never infers types or reads raw native values. Non-finite numbers are represented by an explicit native diagnostic display string rather than being converted through managed parsing.

## Transport and managed presentation

The stdio response adds an additive `dataTablePreview` object. The managed parser validates types, bounds, and non-negative total counts, and uses an unavailable preview fallback when older backends omit the field. The Avalonia Data Tables tab renders copied column labels/types, row display strings, generation, availability, and truncation status. It exposes no edit controls or bridge commands.

## Ownership boundary

C++ owns the complete table schema, rows, diagnostics, serialization, catalog, preview selection, and future asset persistence. C# owns copied DTOs and transient presentation state only. Filesystem scanning, asset-database registration, schema/row editing, references, hot reload, runtime bindings, XLSX, and visual-scripting nodes remain deferred.
