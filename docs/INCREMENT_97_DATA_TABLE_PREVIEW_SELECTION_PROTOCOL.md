// Copyright (c) 2026 UniVex Studios. All Rights Reserved.

# Increment 97 — Read-Only Data Table Preview Selection Protocol v1

## Purpose

Increment 97 completes the user-facing selection path for the registry-backed data-table preview. The managed Avalonia host can select a copied catalog entry, send a named native bridge request through stdio, and receive a fresh snapshot containing the selected read-only preview. The native registry remains the only table authority.

## Protocol contract

| Field | Contract |
|---|---|
| Request kind | `selectDataTablePreview` maps to `EditorBridgeRequestKindUVE::SelectDataTablePreview`. |
| Payload | `dataTableName` is a string bounded by `kEditorBridgeMaximumPresentationTextBytesUVE`; an empty name clears selection. |
| Revision | `expectedRevision` is checked through the existing bridge mutation gate before selection is attempted. |
| Success | A valid registry-backed session returns `bridge.command.applied` and a snapshot with the copied selected preview. |
| Unknown name | The request returns `bridge.data_table.preview.invalid` and retains the prior preview. |
| Missing registry | A registry-free bridge returns `bridge.data_table.registry.unavailable`; legacy snapshot-injection sessions remain compatible but cannot select registry previews. |
| Malformed payload | Stdio parsing rejects invalid field types or unknown request kinds before native dispatch. |

## Managed presentation

The Data Tables catalog ListBox uses single selection. User selection emits only the named `BridgeCommand`; it does not mutate table rows, schema, assets, files, or native memory. When a response snapshot is rendered, the managed host reconciles the selected catalog item from the preview name in that same immutable snapshot, avoiding a second read from mutable session state.

## Ownership and non-goals

The native C++ registry owns validated table instances and returns copies. C# owns only presentation DTOs and process-local command objects. The selection protocol is not a table editor, asset importer, persistence mechanism, filesystem operation, runtime binding, ECS reference path, hot-reload path, or visual-scripting node provider. Any future table mutation must use a separately named command, authorization rule, validation path, and history policy.

## Verification target

The increment requires focused native dispatch and stdio round-trip coverage, managed command serialization coverage, full GCC/Clang/CI native validation, managed .NET 8 tests, Avalonia startup smoke, and a signed Green PR before roadmap promotion.
