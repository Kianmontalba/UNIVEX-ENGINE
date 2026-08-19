// Copyright (c) 2026 UniVex Studios. All Rights Reserved.


#pragma once

#include <array>
#include <cstddef>
#include <mutex>
#include <string>
#include <unordered_map>

#include "uve/events/i_event_system_uve.h"
#include "uve/input/i_gamepad_input_system_uve.h"
#include "uve/input/i_input_system_uve.h"

namespace UVE::Input {

/// InputSystemUVE is the concrete, engine-standard implementation of IInputSystemUVE. Composes an
/// Events::IEventSystemUVE& (dependency injection, matching Renderer3DUVE/PhysicsSystemUVE's
/// precedent for a service that needs a constructor-injected collaborator) to queue
/// InputActionTriggeredEventUVE.
///
/// Uses a triple-buffered state model per device field: a "live" value Set*StateUVE() writes to
/// immediately, and a "current"/"previous" pair that only UpdateUVE() ever touches. UpdateUVE()
/// shifts current->previous *before* snapshotting live->current, so current/previous always
/// differ by exactly one full frame throughout the frame that follows — the frame-stability
/// WasKeyPressedThisFrameUVE()-style edge queries need (comparing live directly against a
/// same-call snapshot would make "pressed this frame" false immediately after the very UpdateUVE()
/// call that should report it true).
///
/// Thread-safety: an internal mutex guards only the "live" fields Set*StateUVE() writes and
/// UpdateUVE() reads from — matching EventSystemUVE's own mutex-guarded-state precedent, and
/// backing the documented "Set*StateUVE() safe from any thread" contract. The "current"/
/// "previous" frame-snapshot fields (and the action registry) are written and read exclusively by
/// UpdateUVE()/the query methods/RegisterActionUVE()/UnregisterActionUVE() — all main-thread-only
/// by contract — so they need no synchronization of their own.
class InputSystemUVE final : public IInputSystemUVE {
public:
    /// `eventSystem` must outlive this InputSystemUVE.
    explicit InputSystemUVE(Events::IEventSystemUVE& eventSystem,
                            IGamepadInputSystemUVE* gamepadInputSystem = nullptr);

    void SetKeyStateUVE(KeyCodeUVE key, bool isDown) override;
    void SetMouseButtonStateUVE(MouseButtonUVE button, bool isDown) override;
    void SetMousePositionUVE(Math::Vector2UVE position) override;
    void SetMouseScrollDeltaUVE(float delta) override;
    void UpdateUVE() override;

    [[nodiscard]] bool IsKeyDownUVE(KeyCodeUVE key) const override;
    [[nodiscard]] bool WasKeyPressedThisFrameUVE(KeyCodeUVE key) const override;
    [[nodiscard]] bool WasKeyReleasedThisFrameUVE(KeyCodeUVE key) const override;
    [[nodiscard]] bool IsMouseButtonDownUVE(MouseButtonUVE button) const override;
    [[nodiscard]] bool WasMouseButtonPressedThisFrameUVE(MouseButtonUVE button) const override;
    [[nodiscard]] bool WasMouseButtonReleasedThisFrameUVE(MouseButtonUVE button) const override;
    [[nodiscard]] Math::Vector2UVE GetMousePositionUVE() const override;
    [[nodiscard]] Math::Vector2UVE GetMouseDeltaUVE() const override;
    [[nodiscard]] float GetMouseScrollDeltaUVE() const override;

    void RegisterActionUVE(InputActionUVE&& action) override;
    bool UnregisterActionUVE(std::string_view actionName) override;
    bool RemapActionUVE(std::string_view actionName, std::vector<InputBindingUVE> positiveBindings,
                        std::vector<InputBindingUVE> negativeBindings) override;
    [[nodiscard]] bool IsActionTriggeredUVE(std::string_view actionName) const override;
    [[nodiscard]] bool IsActionHeldUVE(std::string_view actionName) const override;
    [[nodiscard]] bool IsActionReleasedUVE(std::string_view actionName) const override;
    [[nodiscard]] float GetAxisValueUVE(std::string_view actionName) const override;

private:
    static constexpr std::size_t kKeyCodeCount = static_cast<std::size_t>(KeyCodeUVE::Count);
    static constexpr std::size_t kMouseButtonCount = static_cast<std::size_t>(MouseButtonUVE::Count);
    using GamepadSnapshotArrayUVE = std::array<GamepadStateSnapshotUVE, kMaximumGamepadCountUVE>;
    static constexpr std::size_t kMaximumRemappedBindingsPerSideUVE = 32U;

    [[nodiscard]] static bool AreBindingsValidForRemapUVE(
        const std::vector<InputBindingUVE>& bindings) noexcept;

    /// True iff any binding in `bindings` is down in the given state arrays.
    [[nodiscard]] static bool AnyBindingDownUVE(const std::vector<InputBindingUVE>& bindings,
                                                 const std::array<bool, kKeyCodeCount>& keyState,
                                                 const std::array<bool, kMouseButtonCount>& mouseState,
                                                 const GamepadSnapshotArrayUVE& currentGamepadState,
                                                 const GamepadSnapshotArrayUVE& previousGamepadState,
                                                 bool usePreviousGamepadSnapshot) noexcept;
    [[nodiscard]] static float GetBindingValueUVE(const InputBindingUVE& binding,
                                                   const std::array<bool, kKeyCodeCount>& keyState,
                                                   const std::array<bool, kMouseButtonCount>& mouseState,
                                                   const GamepadSnapshotArrayUVE& currentGamepadState,
                                                   const GamepadSnapshotArrayUVE& previousGamepadState,
                                                   bool usePreviousGamepadSnapshot) noexcept;

    Events::IEventSystemUVE* m_eventSystem;
    IGamepadInputSystemUVE* m_gamepadInputSystem;

    // "Live" state — written by Set*StateUVE() (any thread), read only by UpdateUVE() under
    // m_liveStateMutex.
    mutable std::mutex m_liveStateMutex;
    std::array<bool, kKeyCodeCount> m_liveKeyState{};
    std::array<bool, kMouseButtonCount> m_liveMouseButtonState{};
    Math::Vector2UVE m_liveMousePosition{};
    float m_scrollDeltaAccumulator = 0.0F;

    // Frame-snapshot state — written only by UpdateUVE(), read only by the query methods; both
    // main-thread-only by contract, so no synchronization is needed for these fields.
    std::array<bool, kKeyCodeCount> m_currentKeyState{};
    std::array<bool, kKeyCodeCount> m_previousKeyState{};
    std::array<bool, kMouseButtonCount> m_currentMouseButtonState{};
    std::array<bool, kMouseButtonCount> m_previousMouseButtonState{};
    GamepadSnapshotArrayUVE m_currentGamepadState{};
    GamepadSnapshotArrayUVE m_previousGamepadState{};
    Math::Vector2UVE m_currentMousePosition{};
    Math::Vector2UVE m_previousMousePosition{};
    Math::Vector2UVE m_mouseDelta{};
    float m_mouseScrollDelta = 0.0F;

    std::unordered_map<std::string, InputActionUVE> m_actions;
};

} // namespace UVE::Input
