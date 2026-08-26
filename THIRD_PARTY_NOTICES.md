# UniVex Engine Third-Party Notices and Integration Audit

This file records third-party repositories evaluated or integrated by UniVex Engine. It is maintained alongside `ThirdParty/manifest.json`. A repository marked **audit-only** is not bundled, linked, or distributed by UniVex; its entry documents why no source was integrated. A **candidate-adapter** entry has a permissive top-level license and a defined UVE boundary, but still requires the adapter implementation and integration tests before it becomes a shipped dependency.

## Distribution rule

UniVex's own source and product terms do not replace the licenses below. When a dependency is shipped, its applicable copyright, license, conditions, disclaimers, and any additional notices must remain available in source distributions and binary documentation as required by that dependency. This file is a central index; exact license files must also be kept with vendored source or generated into the release notice bundle.

## JoltPhysics — candidate adapter

Repository: <https://github.com/jrouwe/JoltPhysics.git>

Audited commit: `78d483dc3d375581203cf070ea2790e8045e0879`

License: MIT. The repository license grants use, modification, distribution, sublicensing, and sale subject to retaining the copyright and permission notice and the standard warranty/liability disclaimer.

Proposed UVE boundary: `engine/physics` owns the UVE physics interfaces and lifecycle. A future Jolt backend may implement those interfaces without exposing Jolt types to ECS, gameplay code, or `EngineServicesUVE`.

Selected scope: the Jolt library target only. Samples, viewer, test applications, and assets are excluded.

Current status: **candidate-adapter; not linked into the default runtime**. The current increment records the license and boundary but does not claim that Jolt is already the active physics implementation. A real backend still requires ECS-to-physics shape/body mapping, lifecycle ownership, query parity, and integration/regression tests.

## imGuIZMO.quat — integrated editor-private adapter

Repository: <https://github.com/BrutPitt/imGuIZMO.quat.git>

Audited commit: `23c8e689ec6b7d9bbfe5231a8d33ae946e2e65be`

License: BSD 2-Clause. Redistributions of source and binary forms, with or without modification, are permitted when the copyright notice, conditions, and disclaimer are retained in the required locations.

Proposed UVE boundary: `engine/editor` owns the editor-private viewport adapter. The widget may be used behind UVE's native editor state and existing pinned Dear ImGui target; it must not define the UVE editor layout, terminology, scene ownership, or runtime API.

The repository references an `libs/imgui` submodule at commit `5f371da49a4f530c2f16c279b7613ba1d39b1581`. That submodule has its own license boundary and must be audited independently; UVE must not accidentally bundle a second untracked ImGui copy.

Current status: **integrated-editor-private**. The pinned widget source is compiled as `uve_editor_imguizmo` against UVE's existing Dear ImGui target and is used only by the camera-orientation overlay adapter. The native UVE object transform gizmo remains authoritative for scene Move/Rotate/Scale interaction. Its BSD-2-Clause notice remains required in source and binary documentation.

## SpartanEngine — audit-only

Repository: <https://github.com/PanosK92/SpartanEngine.git>

Audited commit: `cda0f7b6176fad55aa2f06d1ffd78c16b9ecb3aa`

Repository-root license: MIT, copyright Panos Karabelas. This root license does not automatically relicense separately bundled components.

Blocking audit findings include a bundled NVIDIA RTX SDK license for DLSS/NRD-related code. That license contains restrictions around distribution, sublicensing, hardware/platform use, notifications, trademarks, open-source contamination, export, and termination. The repository also contains additional separately licensed graphics, compression, and platform components.

Current status: **audit-only; no SpartanEngine source, binary, asset, logo, or proprietary SDK is integrated or distributed by UVE**. A future integration must select a narrowly scoped component, audit its exact license and dependencies, and place it behind the native renderer interfaces. The full engine is explicitly excluded.

## Stride — audit-only

Repository: <https://github.com/stride3d/stride.git>

Audited commit: `cc3f993de7a8263ebbc14d12d5dcbbe93e31f6e9`

Repository-root license: MIT, with copyright notices for the .NET Foundation, Stride contributors, and Silicon Studio Corp. The repository has a substantial separate dependency inventory.

Blocking audit findings include FFmpeg under GPL-3 and Gettext under LGPL-2.1 in the repository dependency inventory, along with additional independently licensed runtime/editor dependencies. The repository is also a C# engine/editor architecture and cannot be linked wholesale into the native C++ UVE runtime without violating the native ownership boundary.

Current status: **audit-only; no Stride source, binary, asset, logo, or engine subsystem is integrated or distributed by UVE**. Future use is limited to component-level technical study followed by a separate license and architecture review.

## Existing editor dependencies

Dear ImGui is already an editor-private pinned dependency managed by `engine/editor/CMakeLists.txt`. Its MIT license and current version/commit are documented in `docs/CODING_STANDARDS.md`. This notice file does not change that existing integration or imply that unrelated Dear ImGui projects share the same license.

## Audit conclusion

The authorized repositories do not share one license. The safe commercialization model is one central notice index containing the exact notices for each shipped component, while each third-party license remains legally distinct. Native UVE APIs, scene/runtime ownership, editor identity, and replaceable adapter boundaries remain authoritative.
