# Increment 95 — Native Data Table Session Registry v1

Increment 95 introduces a bounded in-memory registry for validated native `DataTableUVE` instances. The registry is an explicit session seam for future editor and asset workflows; it is not a filesystem database, serializer scheduler, hot-reload service, or runtime binding layer.

## Ownership and lifecycle

`DataTableRegistryUVE` takes ownership by move, retains tables through RAII, and exposes no mutable table references or raw pointers. Registration accepts only a non-empty table name with no diagnostics. Duplicate names, invalid tables, and capacity overflow are rejected without changing the registry. The maximum is 128 registered tables.

Successful registration, removal, and clearing advance the registry generation. Failed removal and clearing an already-empty registry do not advance it. Queries return copied table snapshots and copied sorted catalog descriptors; callers may mutate their copies without changing the registry.

## Deferred authority

The registry does not access the filesystem, asset database, process environment, ECS, renderer, OpenGL, network, runtime bindings, managed editor state, or visual-scripting graphs. Persistence, directory discovery, import scheduling, hot reload, schema/row editing, and cross-session identity require separately reviewed contracts.
