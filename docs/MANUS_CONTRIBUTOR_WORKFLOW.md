# Manus Contributor Workflow

This guide defines how Manus contributes changes to UniVex Engine as a disciplined repository contributor. It supplements, rather than replaces, [CONTRIBUTING.md](../CONTRIBUTING.md) and [the coding standards](CODING_STANDARDS.md). The workflow applies to all Manus-authored feature, bug-fix, documentation, and maintenance branches.

| Principle | Contributor commitment |
|---|---|
| Scope before code | Confirm the intended outcome, acceptance criteria, affected systems, and non-goals before editing files. |
| Main stays stable | Never develop directly on `main`; every change begins on an isolated feature branch. |
| Vertical completeness | Include the necessary source, tests, documentation, and validation for a reviewable change. |
| Evidence over claims | Report the exact build and test commands run, their outcome, and known environmental limits. |
| Review remains human-controlled | Push a branch and open a pull request only after validation; never merge without explicit approval. |

## Sequential Contribution Workflow

### 1. Intake and scope gate

Start by locating the proposed work in the engine roadmap and reading the affected module's interfaces, implementation, tests, and relevant coding-standard sections. State the problem, intended behavior, out-of-scope work, compatibility constraints, and acceptance criteria. Ask for clarification whenever a request changes a public API, format, project architecture, or the intended roadmap priority.

### 2. Branch and baseline gate

Refresh `origin/main`, confirm a clean working tree, and create a descriptive branch using `manus/<work-item>`. Start from current `main` unless the user explicitly directs a different base branch. Do not combine unrelated fixes or refactors in the same branch.

### 3. Change-map gate

Before editing, create a concise change map that identifies affected public headers, source files, tests, CMake targets, assets, and documentation. Preserve module directionality, UVE naming, RAII, thread-safety, error handling, and OpenGL symbol-confinement rules already established by the repository.

### 4. Implementation gate

Implement the smallest complete vertical slice. A behavior change must be paired with appropriate tests; a public API change must have XML-style documentation and a documented thread-safety contract; a new source file must use the repository copyright header. Do not leave stubs, placeholder-only paths, unrelated formatting churn, or TODOs in place of delivered behavior.

### 5. Validation gate

Use the narrowest relevant tests during iteration, then run the complete required validation before handoff. Build with GCC and Clang under warnings-as-errors. Run CTest with failure output. Rendering changes must retain NullRenderDevice coverage and add or update a real OpenGL validation when the public RHI and test environment support it; if a real display is unavailable, report the skipped condition precisely.

| Change type | Minimum validation |
|---|---|
| Core C++ or module behavior | Focused test target, full GCC build, full CTest, and full Clang build. |
| Public API or serialization format | All core validation plus backward-compatibility or error-path tests. |
| Rendering or shader behavior | All core validation plus NullRenderDevice command tests and real OpenGL validation where supported. |
| Documentation-only change | Markdown link and path review, then repository status review. |

### 6. Contribution handoff gate

Review `git diff`, confirm that only intended files changed, then create focused commits. Push the branch and prepare a pull request with the problem statement, implementation summary, tests, compatibility notes, and any deliberate follow-up work. Do not merge, force-push shared branches, rewrite `main`, or change repository protections without explicit approval.

## Branch and Pull Request Conventions

Use lower-case, hyphenated names such as `manus/increment-27-glsl-shadow-sampling`, `manus/fix-scene-serializer-validation`, or `manus/docs-contributor-workflow`. Prefer one logical change per branch and one purpose per pull request.

Every pull request should start with a compact implementation status block. This provides an instant review signal without adding process overhead.

| Status field | Expected value |
|---|---|
| Roadmap or issue | The relevant increment, issue, or explicit user request. |
| Scope | What the branch changes and what it intentionally does not change. |
| Validation | Exact commands and whether they passed, failed, or were skipped with a reason. |
| Risk and compatibility | Public API, asset-format, threading, rendering, or migration considerations. |
| Review request | The focused question the reviewer should evaluate. |

The [pull-request template](../.github/PULL_REQUEST_TEMPLATE.md) captures this information consistently. It intentionally avoids duplicating the repository's coding standards and does not require contributors to claim validation they did not perform.
