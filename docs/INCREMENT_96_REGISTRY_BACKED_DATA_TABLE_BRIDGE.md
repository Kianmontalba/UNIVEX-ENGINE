// Copyright (c) 2026 UniVex Studios. All Rights Reserved.

# Increment 96 — Registry-Backed Data Table Bridge v1

## Purpose

Increment 96 closes the native data-table session arc by making `DataTableRegistryUVE` the authoritative source for editor bridge catalog and selected preview snapshots. The bridge receives a **non-owning pointer** to a registry that is created and owned by the native editor session. Every catalog and preview fact crossing the bridge remains a copied value DTO.

## Contract

| Concern | Contract |
|---|---|
| Registry lifetime | The caller owns `DataTableRegistryUVE`; it must outlive `EditorBridgeUVE`. The bridge stores no owning handle and does not destroy or move the registry. |
| Catalog authority | With a registry dependency, `GetCatalogSnapshotUVE()` is captured on every bridge synchronization. Registry generation and sorted entries therefore become observable bridge state automatically. |
| Preview authority | `SetPreviewTableUVE(name)` accepts only a bounded name that exists in the registry. `CaptureDataTablePreviewUVE()` obtains a copied `DataTableSnapshotUVE` through `TryGetSnapshotUVE()`. |
| Selection clearing | Passing an empty name clears the selected preview. If a selected table is removed, the selection intent remains but the preview becomes unavailable until the name is cleared or the table is registered again. |
| Legacy seams | `SetDataTableCatalogSnapshotUVE()` and `SetDataTablePreviewSnapshotUVE()` remain usable for bridge sessions constructed without a registry. They are ignored when a registry is authoritative, preventing split-brain snapshots. |
| Revision behavior | Registry register/remove/clear operations and preview selection changes advance the bridge revision only when their copied observable facts change. Rejected selection and no-op registry operations do not advance it. |

## Ownership and boundary rules

The registry is a **native, main-thread session service**. It is not a filesystem scanner, asset database, persistence service, hot-reload coordinator, runtime binding, ECS reference provider, or managed mutation path. The managed editor receives only the already-bounded `dataTableCatalog` and `dataTablePreview` DTOs that the existing stdio protocol serializes.

The bridge integration deliberately does not add a data-table request enum or a managed-side table mutation command. Preview selection is a native seam for the current read-only workflow; a future command surface must be separately named, revision-checked, and authorized before it can mutate any table or asset.

## Verification target

The increment requires focused registry-backed bridge coverage, existing legacy injection coverage, full GCC and Clang CTest runs under Xvfb, CI-equivalent GCC/Ninja validation, managed .NET 8 tests, and bridge smoke/probe checks. The roadmap is updated only after these validations pass and the signed PR is Green.
