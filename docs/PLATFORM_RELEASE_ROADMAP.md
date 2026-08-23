<div align="center">

<h1><strong>UNIVEX ENGINE — PLATFORM & RELEASE ROADMAP</strong></h1>

<strong>Build systems, supported platforms, cooking, packaging, sample delivery, and release documentation</strong>

</div>

> A platform is not supported because it appears in a design document. It becomes supported only when its toolchain, executable path, relevant tests, and delivery workflow are continuously validated.

| Status | Completed foundation | Delivered capability | Ongoing boundary |
|---|---|---|---|
| **COMPLETED** | **CMake foundation** | Native CMake project composition and C++20 build configuration. | New modules must preserve target ownership and dependency direction. |
| **COMPLETED** | **GCC / Clang validation** | Debug builds and full CTest execution under both supported Linux compilers. | Cross-compiler validation remains required for engine increments. |
| **COMPLETED** | **Increment 37 CI** | Pull-request CI build/test verification with GLFW/X11 prerequisites. | CI does not yet prove release packaging or non-Linux delivery. |
| **COMPLETED** | **Increment 20 desktop windowing** | GLFW/OpenGL desktop execution path. | Desktop runtime is a foundation, not a full Windows/macOS/Linux release matrix. |
| **COMPLETED** | **Editor smoke discipline** | Headless and Xvfb/OpenGL smoke paths for editor lifecycle verification. | A smoke test is necessary but does not replace visual/presentation-specific assertions. |

<div align="center">

<h2><strong>PARTIAL — BUILD AND PLATFORM EXPANSION</strong></h2>

</div>

| Status | Area | Intended capability | Completion proof / boundary |
|---|---|---|---|
| **PARTIAL** | Windows support | MSVC toolchain, desktop window/input behavior, graphics-backend policy, and CI coverage. | Native Windows CI build/test and a supported runtime smoke path. |
| **PARTIAL** | Linux release support | Reproducible package format, runtime dependency policy, and install/uninstall flow. | Clean-machine installation and release artifact verification. |
| **PARTIAL** | Android support | NDK/Gradle build, ARM64, touch lifecycle/input, packaging, and graphics backend choice. | Device/emulator CI plus real-device validation; no “mobile supported” claim before this. |
| **PARTIAL** | iOS support | Xcode generation, Metal backend policy, app lifecycle, signing, and deployment. | macOS/Xcode toolchain validation and device delivery evidence. |
| **PARTIAL** | Build configurations | Debug, Development, Release, and Shipping policy. | Defined optimization, assertion, logging, asset, and symbol behavior for each configuration. Increment 523 adds reproducible CMake configure/build presets for all four profiles, maps them to Debug/RelWithDebInfo/Release/MinSizeRel, applies explicit UVE_BUILD_CONFIGURATION and UVE_SHIPPING markers, and makes the Development, Release, and Shipping editor/test targets build successfully; Increment 524 adds profile-aware assertion policy (Debug/Development enabled, Release/Shipping disabled), profile-specific default log thresholds (Trace/Debug/Info/Warning), and a compiled EngineConfig regression for those markers; asset-tool and symbol-policy behavior remain to be integrated before completion. |
| **PARTIAL** | Asset cooking | Platform-specific conversion, texture/shader preparation, and derived-data/caching strategy. | Deterministic output tests and selected real import formats. |
| **PARTIAL** | Packaging | Executable, asset bundle, metadata, versioning, and installable distribution artifacts. | Reproducible build artifact validated on a clean environment. |
| **PARTIAL** | Release automation | Versioning, changelog, signed artifacts, release checks, and delivery workflow. | Dedicated secure CI/release design; no credentials embedded in tooling. |

<div align="center">

<h2><strong>PARTIAL — SAMPLE PROJECT AND DOCUMENTATION</strong></h2>

</div>

| Status | Deliverable | Intended capability | Entry condition |
|---|---|---|---|
| **PARTIAL** | Sample project | Demonstrate a real 3D scene, camera/light/mesh, physics, player, audio, UI, gameplay loop, and platform build. | Runtime/content/visual-scripting/asset/pipeline systems are actually implemented. |
| **PARTIAL** | Engine API reference | Accurate public C++ API documentation. | Public APIs have complete documentation comments and stable contracts. |
| **PARTIAL** | Editor user manual | Real layout, shortcut, workflow, and scene-authoring documentation. | Scene Editor completion gate is met; no mock UI manual. |
| **PARTIAL** | Visual scripting reference | Node reference, type/pin rules, compiler behavior, and debugging guide. | Native visual scripting runtime/editor exists. |
| **PARTIAL** | Plugin guide | Manifest, ABI, lifecycle, registration, compatibility, and security guidance. | Plugin architecture is implemented and versioned. |
| **PARTIAL** | Build/deployment guide | Platform prerequisites, CMake/build configurations, cooking, packaging, and troubleshooting. | At least one reproducible release path exists. |

<div align="center">

<h2><strong>PLATFORM RELEASE RULES</strong></h2>

</div>

| Rule | Requirement |
|---|---|
| **Evidence per target** | Every target needs its own toolchain and runtime evidence; Linux results do not prove Windows, Android, or iOS. |
| **No premature packaging** | Cooking and installers wait for stable asset/import/project-health contracts. |
| **Secure automation** | Release credentials and signing are handled through approved protected automation, never source-controlled scripts. |
| **Sample as proof** | The sample project is an integrated validation artifact, not a replacement for subsystem tests. |
