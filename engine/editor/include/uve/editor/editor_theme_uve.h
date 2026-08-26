#pragma once

namespace UVE::Editor {

// Applies the UVE editor's neutral charcoal visual system to the active ImGui context.
// This is presentation-only; editor/session and engine/runtime state remain external.
void ApplyEditorVisualThemeUVE() noexcept;

} // namespace UVE::Editor
