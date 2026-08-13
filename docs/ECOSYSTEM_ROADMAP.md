<div align="center">

<h1><strong>UNIVEX ENGINE — ECOSYSTEM ROADMAP</strong></h1>

<strong>UniVex Hub, public web/news, account services, backend, and community delivery</strong>

</div>

> The ecosystem serves the engine; it does not replace unfinished engine foundations. These items are intentionally tracked separately so launch/dashboard work cannot silently displace core Scene Editor, runtime, renderer, and project-health priorities.

| Status | Area | Intended capability | Entry condition / boundary |
|---|---|---|---|
| **PARTIAL** | UniVex Hub | Desktop launcher, engine installation/version management, project discovery/opening, update/repair flow, and local project status. | Requires an installable engine release, stable CLI/project-open contract, version metadata, and protected update design. |
| **PARTIAL** | Project management | Create/open/clone/recent project workflows and engine-version compatibility checks. | Project format and release/install paths must be stable; project creation must not fabricate unsupported assets. |
| **PARTIAL** | Engine-to-Hub integration | `--project` open, build/export invocation, progress output, and crash-log discovery. | Engine CLI/build/crash contracts must be documented, tested, and versioned. |
| **PARTIAL** | Account service | Shared authentication, profile, entitlement, and account recovery for Hub/community services. | Separate privacy/security architecture, data model, and compliance review. |
| **PARTIAL** | Public website and news | Product pages, documentation portal, release/news posts, tutorials, and download handoff. | Public content/release process and a maintained engine/documentation baseline. |
| **PARTIAL** | Cloud and collaboration | Optional project sync, backup, team access, and later source-control integration. | Explicit identity, access-control, conflict-resolution, cost, privacy, and data-loss recovery design. |
| **PARTIAL** | Marketplace/community | Plugin, asset, tutorial, and creator distribution workflows. | Mature plugin/asset/versioning/safety review contracts; no ungoverned executable distribution. |
| **PARTIAL** | Closed beta and public launch | Invite-only validation, support process, feedback loop, release quality gates, and public launch. | Stable editor/renderer/project workflow, release artifacts, documentation, and operational ownership. |

<div align="center">

<h2><strong>ECOSYSTEM PRIORITY RULES</strong></h2>

</div>

| Rule | Requirement |
|---|---|
| **Engine-first** | Core engine and active Scene Editor milestones remain the primary execution path. |
| **Separate security boundary** | Accounts, authentication, payments, analytics, and cloud files require their own security/privacy design; none is implied by an engine commit. |
| **No feature theatre** | A Hub/dashboard must invoke real engine functionality and expose truthful release state. |
| **Release-dependent work** | Installer/update/download features begin only once reproducible signed engine artifacts and version metadata exist. |
| **Independent delivery** | Ecosystem work may be staffed/delivered separately, but integration points with the engine must remain documented and versioned. |
