// Copyright (c) 2026 UniVex Studios. All Rights Reserved.


#pragma once

#include <string_view>

#include "uve/input/input_action_uve.h"
#include "uve/input/key_code_uve.h"
#include "uve/input/mouse_button_uve.h"
#include "uve/math/vector2_uve.h"

namespace UVE::Input {

/// IInputSystemUVE is the spec's `InputSystemUVE` (Part 7.7): abstract keyboard/mouse input plus
/// a named action-mapping layer (`InputActionUVE`/`InputBindingUVE`). Gamepad/Touch/Gyroscope are
/// deferred as a group this increment — Touch/Gyroscope are mobile-only (no mobile target or
/// WindowManagerUVE exists to source events from) and Gamepad has no concrete consumer yet.
/// Unlike ICollisionSystemUVE/IRaycastSystemUVE, an implementation is expected to be stateful
/// (current/previous device state for edge detection) and driven once per frame via UpdateUVE() —
/// closer in shape to IEventSystemUVE's own "self-contained stateful service" than to
/// RaycastSystemUVE's pure-stateless-query shape. There is deliberately no second "Null"-style
/// implementation: no real backend (WindowManagerUVE) exists yet to abstract away, so the single
/// concrete InputSystemUVE — which never touches real hardware, only consumes explicitly pushed
/// state — already is the sandbox-appropriate implementation. A future real backend calls the
/// same Set*StateUVE() methods below directly, rather than implementing a second variant of this
/// interface.
/// Thread-safety: the injection methods (Set*StateUVE()) are safe to call from any thread,
/// matching IEventSystemUVE::QueueEvent()'s contract — a future real backend would call them from
/// a platform/window thread. UpdateUVE() and every query method are main-thread-only, matching
/// IEventSystemUVE::DispatchQueuedUVE()'s contract.
class IInputSystemUVE {
public:
    virtual ~IInputSystemUVE() = default;

    // --- Injection: how device state gets in. ---
    // Reflects "current state," not a queued event log: a key press-and-release faster than one
    // frame apart can be missed — a documented, standard polling-input simplification, not an
    // oversight (see docs/CODING_STANDARDS.md).
    virtual void SetKeyStateUVE(KeyCodeUVE key, bool isDown) = 0;
    virtual void SetMouseButtonStateUVE(MouseButtonUVE button, bool isDown) = 0;
    virtual void SetMousePositionUVE(Math::Vector2UVE position) = 0;
    /// Accumulates until the next UpdateUVE() call drains it — a real scroll wheel can report
    /// several small deltas between frames.
    virtual void SetMouseScrollDeltaUVE(float delta) = 0;

    /// Advances current->previous state for edge detection, computes mouse delta, drains the
    /// scroll accumulator, and queues an InputActionTriggeredEventUVE for every Button action
    /// that newly transitioned to triggered this frame. Must be called exactly once per frame,
    /// from the main thread, before anything reads this frame's input state.
    virtual void UpdateUVE() = 0;

    // --- Raw device-state queries. ---
    [[nodiscard]] virtual bool IsKeyDownUVE(KeyCodeUVE key) const = 0;
    [[nodiscard]] virtual bool WasKeyPressedThisFrameUVE(KeyCodeUVE key) const = 0;
    [[nodiscard]] virtual bool WasKeyReleasedThisFrameUVE(KeyCodeUVE key) const = 0;
    [[nodiscard]] virtual bool IsMouseButtonDownUVE(MouseButtonUVE button) const = 0;
    [[nodiscard]] virtual bool WasMouseButtonPressedThisFrameUVE(MouseButtonUVE button) const = 0;
    [[nodiscard]] virtual bool WasMouseButtonReleasedThisFrameUVE(MouseButtonUVE button) const = 0;
    [[nodiscard]] virtual Math::Vector2UVE GetMousePositionUVE() const = 0;
    [[nodiscard]] virtual Math::Vector2UVE GetMouseDeltaUVE() const = 0;
    [[nodiscard]] virtual float GetMouseScrollDeltaUVE() const = 0;

    // --- Action layer. ---
    /// Registers (or replaces, by name) an action. Takes an explicit-move sink parameter — the
    /// ownership transfer is visible at the call site
    /// (`inputSystem.RegisterActionUVE(std::move(action))`, or a temporary) — and the
    /// implementation moves it straight into its internal registry.
    virtual void RegisterActionUVE(InputActionUVE&& action) = 0;
    /// Returns true if an action with this name existed and was removed, false otherwise — no
    /// assertion for an unknown name. Needed for future editor workflows, hot reload, and
    /// runtime action rebuilding without forcing the registry to only ever grow.
    virtual bool UnregisterActionUVE(std::string_view actionName) = 0;
    // An unregistered/typo'd action name returns false/0.0F rather than asserting — matches
    // IConfigManagerUVE's "keyed lookup that may legitimately miss" precedent, not a programmer
    // precondition violation.
    [[nodiscard]] virtual bool IsActionTriggeredUVE(std::string_view actionName) const = 0;
    [[nodiscard]] virtual bool IsActionHeldUVE(std::string_view actionName) const = 0;
    [[nodiscard]] virtual bool IsActionReleasedUVE(std::string_view actionName) const = 0;
    [[nodiscard]] virtual float GetAxisValueUVE(std::string_view actionName) const = 0;
};

} // namespace UVE::Input
