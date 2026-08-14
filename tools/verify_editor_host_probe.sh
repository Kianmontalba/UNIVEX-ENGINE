#!/usr/bin/env bash
# Copyright (c) 2026 UniVex Studios. All Rights Reserved.

set -euo pipefail

repository_root="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
backend_executable="${1:-${repository_root}/build/ci-gcc/engine/app/uve_editor}"
host_project="${repository_root}/editor/managed/UniVex.EditorHost/UniVex.EditorHost.csproj"

if [[ ! -x "${backend_executable}" ]]; then
    printf 'Missing executable bridge backend: %s\n' "${backend_executable}" >&2
    exit 64
fi

dotnet restore "${host_project}" --locked-mode
dotnet build "${host_project}" --no-restore
dotnet run --project "${host_project}" --no-build -- --probe "${backend_executable}"
