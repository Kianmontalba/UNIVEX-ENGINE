// Copyright (c) 2026 UniVex Studios. All Rights Reserved.
#pragma once
#include "uve/window/window_desc_uve.h"
namespace UVE::Window {
/// Validates the value-only construction contract for a desktop WindowDescUVE.
/// This does not create a window, select a backend, or inspect platform lifecycle state.
[[nodiscard]] bool ValidateWindowDescUVE(const WindowDescUVE& desc) noexcept;
} // namespace UVE::Window
