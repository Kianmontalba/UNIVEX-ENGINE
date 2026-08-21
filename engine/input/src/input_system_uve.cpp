// Copyright (c) 2026 UniVex Studios. All Rights Reserved.


#include "uve/input/input_system_uve.h"

#include <algorithm>
#include <cmath>
#include <cstddef>
#include <utility>

#include "uve/input/input_action_triggered_event_uve.h"

namespace UVE::Input {

InputSystemUVE::InputSystemUVE(Events::IEventSystemUVE& eventSystem,
                               IGamepadInputSystemUVE* gamepadInputSystem)
    : m_eventSystem(&eventSystem), m_gamepadInputSystem(gamepadInputSystem) {}

float InputSystemUVE::GetBindingValueUVE(const InputBindingUVE& binding,
                                         const std::array<bool, kKeyCodeCount>& keyState,
                                         const std::array<bool, kMouseButtonCount>& mouseState,
                                         const GamepadSnapshotArrayUVE& currentGamepadState,
                                         const GamepadSnapshotArrayUVE& previousGamepadState,
                                         const bool usePreviousGamepadSnapshot) noexcept {
    switch (binding.source) {
    case InputBindingSourceUVE::Keyboard: {
        const std::size_t keyIndex = static_cast<std::size_t>(binding.key);
        return keyIndex < kKeyCodeCount && keyState[keyIndex] ? 1.0F : 0.0F;
    }
    case InputBindingSourceUVE::Mouse: {
        const std::size_t buttonIndex = static_cast<std::size_t>(binding.mouseButton);
        return buttonIndex < kMouseButtonCount && mouseState[buttonIndex] ? 1.0F : 0.0F;
    }
    case InputBindingSourceUVE::GamepadButton: {
        if (binding.gamepadIndex >= kMaximumGamepadCountUVE) {
            return 0.0F;
        }
        const std::size_t buttonIndex = static_cast<std::size_t>(binding.gamepadButton);
        if (buttonIndex >= kGamepadButtonCountUVE) {
            return 0.0F;
        }
        const GamepadStateSnapshotUVE& snapshot =
            usePreviousGamepadSnapshot ? previousGamepadState[binding.gamepadIndex]
                                        : currentGamepadState[binding.gamepadIndex];
        return snapshot.buttons[buttonIndex] ? 1.0F : 0.0F;
    }
    case InputBindingSourceUVE::GamepadAxis: {
        if (binding.gamepadIndex >= kMaximumGamepadCountUVE) {
            return 0.0F;
        }
        const std::size_t axisIndex = static_cast<std::size_t>(binding.gamepadAxis);
        if (axisIndex >= kGamepadAxisCountUVE) {
            return 0.0F;
        }
        const GamepadStateSnapshotUVE& snapshot =
            usePreviousGamepadSnapshot ? previousGamepadState[binding.gamepadIndex]
                                        : currentGamepadState[binding.gamepadIndex];
        const float value = snapshot.axes[axisIndex] * binding.scale;
        return std::isfinite(value) ? std::clamp(value, -1.0F, 1.0F) : 0.0F;
    }
    }
    return 0.0F;
}

bool InputSystemUVE::AnyBindingDownUVE(const std::vector<InputBindingUVE>& bindings,
                                        const std::array<bool, kKeyCodeCount>& keyState,
                                        const std::array<bool, kMouseButtonCount>& mouseState,
                                        const GamepadSnapshotArrayUVE& currentGamepadState,
                                        const GamepadSnapshotArrayUVE& previousGamepadState,
                                        const bool usePreviousGamepadSnapshot) noexcept {
    for (const InputBindingUVE& binding : bindings) {
        if (std::fabs(GetBindingValueUVE(binding, keyState, mouseState, currentGamepadState,
                                         previousGamepadState, usePreviousGamepadSnapshot)) > 0.5F) {
            return true;
        }
    }
    return false;
}

void InputSystemUVE::SetKeyStateUVE(KeyCodeUVE key, bool isDown) {
    const std::size_t index = static_cast<std::size_t>(key);
    if (index >= kKeyCodeCount) {
        return;
    }
    const std::lock_guard<std::mutex> lock(m_liveStateMutex);
    m_liveKeyState[index] = isDown;
}

void InputSystemUVE::SetMouseButtonStateUVE(MouseButtonUVE button, bool isDown) {
    const std::size_t index = static_cast<std::size_t>(button);
    if (index >= kMouseButtonCount) {
        return;
    }
    const std::lock_guard<std::mutex> lock(m_liveStateMutex);
    m_liveMouseButtonState[index] = isDown;
}

void InputSystemUVE::SetMousePositionUVE(Math::Vector2UVE position) {
    const std::lock_guard<std::mutex> lock(m_liveStateMutex);
    m_liveMousePosition = position;
}

void InputSystemUVE::SetMouseScrollDeltaUVE(const float delta) {
    if (!std::isfinite(delta)) {
        return;
    }
    const std::lock_guard<std::mutex> lock(m_liveStateMutex);
    const float candidate = m_scrollDeltaAccumulator + delta;
    if (std::isfinite(candidate)) {
        m_scrollDeltaAccumulator = candidate;
    }
}

void InputSystemUVE::UpdateUVE() {
    {
        const std::lock_guard<std::mutex> lock(m_liveStateMutex);
        // Shift the old "current" (last frame's committed snapshot) into "previous" *before*
        // overwriting "current" with the live state — this ordering is what keeps
        // current/previous exactly one frame apart for the rest of this call and every query
        // made until the next UpdateUVE().
        m_previousKeyState = m_currentKeyState;
        m_currentKeyState = m_liveKeyState;
        m_previousMouseButtonState = m_currentMouseButtonState;
        m_currentMouseButtonState = m_liveMouseButtonState;
        m_previousMousePosition = m_currentMousePosition;
        m_currentMousePosition = m_liveMousePosition;
        m_previousGamepadState = m_currentGamepadState;
        if (m_gamepadInputSystem == nullptr) {
            m_currentGamepadState.fill({});
        } else {
            for (std::size_t gamepadIndex = 0U; gamepadIndex < kMaximumGamepadCountUVE; ++gamepadIndex) {
                m_currentGamepadState[gamepadIndex] = m_gamepadInputSystem->GetSnapshotUVE(gamepadIndex);
            }
        }
        m_mouseScrollDelta = m_scrollDeltaAccumulator;
        m_scrollDeltaAccumulator = 0.0F;
    }
    m_mouseDelta = m_currentMousePosition - m_previousMousePosition;

    const auto GetAxisSnapshotValueUVE = [&](const InputActionUVE& action, const bool previous) {
        const auto SumBindingValuesUVE = [&](const std::vector<InputBindingUVE>& bindings) {
            float sum = 0.0F;
            for (const InputBindingUVE& binding : bindings) {
                sum += GetBindingValueUVE(
                    binding, previous ? m_previousKeyState : m_currentKeyState,
                    previous ? m_previousMouseButtonState : m_currentMouseButtonState,
                    m_currentGamepadState, m_previousGamepadState, previous);
            }
            return sum;
        };
        return std::clamp(SumBindingValuesUVE(action.positiveBindings) -
                              SumBindingValuesUVE(action.negativeBindings),
                          -1.0F, 1.0F);
    };

    for (const auto& [name, action] : m_actions) {
        if (action.type == InputActionTypeUVE::Button) {
            const bool isDownNow = AnyBindingDownUVE(action.positiveBindings, m_currentKeyState,
                                                     m_currentMouseButtonState, m_currentGamepadState,
                                                     m_previousGamepadState, false);
            const bool wasDownBefore =
                AnyBindingDownUVE(action.positiveBindings, m_previousKeyState, m_previousMouseButtonState,
                                  m_currentGamepadState, m_previousGamepadState, true);
            if (isDownNow && !wasDownBefore) {
                m_eventSystem->QueueEvent(InputActionTriggeredEventUVE{name, InputActionTypeUVE::Button, 0.0F});
            }
        } else {
            const float currentAxisValue = GetAxisSnapshotValueUVE(action, false);
            const float previousAxisValue = GetAxisSnapshotValueUVE(action, true);
            if (std::fabs(currentAxisValue) > kInputActionAxisTriggerThresholdUVE &&
                std::fabs(previousAxisValue) <= kInputActionAxisTriggerThresholdUVE) {
                m_eventSystem->QueueEvent(
                    InputActionTriggeredEventUVE{name, InputActionTypeUVE::Axis1D, currentAxisValue});
            }
        }
    }
}

bool InputSystemUVE::IsKeyDownUVE(KeyCodeUVE key) const {
    const std::size_t index = static_cast<std::size_t>(key);
    return index < kKeyCodeCount && m_currentKeyState[index];
}

bool InputSystemUVE::WasKeyPressedThisFrameUVE(KeyCodeUVE key) const {
    const std::size_t index = static_cast<std::size_t>(key);
    return index < kKeyCodeCount && m_currentKeyState[index] && !m_previousKeyState[index];
}

bool InputSystemUVE::WasKeyReleasedThisFrameUVE(KeyCodeUVE key) const {
    const std::size_t index = static_cast<std::size_t>(key);
    return index < kKeyCodeCount && !m_currentKeyState[index] && m_previousKeyState[index];
}

bool InputSystemUVE::IsMouseButtonDownUVE(MouseButtonUVE button) const {
    const std::size_t index = static_cast<std::size_t>(button);
    return index < kMouseButtonCount && m_currentMouseButtonState[index];
}

bool InputSystemUVE::WasMouseButtonPressedThisFrameUVE(MouseButtonUVE button) const {
    const std::size_t index = static_cast<std::size_t>(button);
    return index < kMouseButtonCount && m_currentMouseButtonState[index] && !m_previousMouseButtonState[index];
}

bool InputSystemUVE::WasMouseButtonReleasedThisFrameUVE(MouseButtonUVE button) const {
    const std::size_t index = static_cast<std::size_t>(button);
    return index < kMouseButtonCount && !m_currentMouseButtonState[index] && m_previousMouseButtonState[index];
}

Math::Vector2UVE InputSystemUVE::GetMousePositionUVE() const {
    return m_currentMousePosition;
}

Math::Vector2UVE InputSystemUVE::GetMouseDeltaUVE() const {
    return m_mouseDelta;
}

float InputSystemUVE::GetMouseScrollDeltaUVE() const {
    return m_mouseScrollDelta;
}

void InputSystemUVE::RegisterActionUVE(InputActionUVE&& action) {
    if (action.name.empty() || action.name.size() > kMaximumInputActionNameBytesUVE ||
        action.name.find('\0') != std::string::npos) {
        return;
    }
    const std::string name = action.name;
    m_actions[name] = std::move(action);
}

bool InputSystemUVE::UnregisterActionUVE(std::string_view actionName) {
    return m_actions.erase(std::string(actionName)) > 0;
}

bool InputSystemUVE::AreBindingsValidForRemapUVE(
    const std::vector<InputBindingUVE>& bindings) noexcept {
    if (bindings.size() > kMaximumRemappedBindingsPerSideUVE) {
        return false;
    }
    for (const InputBindingUVE& binding : bindings) {
        switch (binding.source) {
        case InputBindingSourceUVE::Keyboard:
            if (static_cast<std::size_t>(binding.key) >= kKeyCodeCount) {
                return false;
            }
            break;
        case InputBindingSourceUVE::Mouse:
            if (static_cast<std::size_t>(binding.mouseButton) >= kMouseButtonCount) {
                return false;
            }
            break;
        case InputBindingSourceUVE::GamepadButton:
            if (binding.gamepadIndex >= kMaximumGamepadCountUVE ||
                static_cast<std::size_t>(binding.gamepadButton) >= kGamepadButtonCountUVE) {
                return false;
            }
            break;
        case InputBindingSourceUVE::GamepadAxis:
            if (binding.gamepadIndex >= kMaximumGamepadCountUVE ||
                static_cast<std::size_t>(binding.gamepadAxis) >= kGamepadAxisCountUVE ||
                !std::isfinite(binding.scale)) {
                return false;
            }
            break;
        default:
            return false;
        }
    }
    return true;
}

bool InputSystemUVE::RemapActionUVE(std::string_view actionName,
                                    std::vector<InputBindingUVE> positiveBindings,
                                    std::vector<InputBindingUVE> negativeBindings) {
    const auto it = m_actions.find(std::string(actionName));
    if (it == m_actions.end() || !AreBindingsValidForRemapUVE(positiveBindings) ||
        !AreBindingsValidForRemapUVE(negativeBindings)) {
        return false;
    }
    it->second.positiveBindings = std::move(positiveBindings);
    it->second.negativeBindings = std::move(negativeBindings);
    return true;
}

bool InputSystemUVE::IsActionTriggeredUVE(std::string_view actionName) const {
    const auto it = m_actions.find(std::string(actionName));
    if (it == m_actions.end()) {
        return false;
    }
    const bool isDownNow = AnyBindingDownUVE(it->second.positiveBindings, m_currentKeyState, m_currentMouseButtonState,
                           m_currentGamepadState, m_previousGamepadState, false);
    const bool wasDownBefore =
        AnyBindingDownUVE(it->second.positiveBindings, m_previousKeyState, m_previousMouseButtonState,
                           m_currentGamepadState, m_previousGamepadState, true);
    return isDownNow && !wasDownBefore;
}

bool InputSystemUVE::IsActionHeldUVE(std::string_view actionName) const {
    const auto it = m_actions.find(std::string(actionName));
    if (it == m_actions.end()) {
        return false;
    }
    return AnyBindingDownUVE(it->second.positiveBindings, m_currentKeyState, m_currentMouseButtonState,
                           m_currentGamepadState, m_previousGamepadState, false);
}

bool InputSystemUVE::IsActionReleasedUVE(std::string_view actionName) const {
    const auto it = m_actions.find(std::string(actionName));
    if (it == m_actions.end()) {
        return false;
    }
    const bool isDownNow = AnyBindingDownUVE(it->second.positiveBindings, m_currentKeyState, m_currentMouseButtonState,
                           m_currentGamepadState, m_previousGamepadState, false);
    const bool wasDownBefore =
        AnyBindingDownUVE(it->second.positiveBindings, m_previousKeyState, m_previousMouseButtonState,
                           m_currentGamepadState, m_previousGamepadState, true);
    return !isDownNow && wasDownBefore;
}

float InputSystemUVE::GetAxisValueUVE(std::string_view actionName) const {
    const auto it = m_actions.find(std::string(actionName));
    if (it == m_actions.end()) {
        return 0.0F;
    }
    const auto SumBindingValuesUVE = [&](const std::vector<InputBindingUVE>& bindings) {
        float sum = 0.0F;
        for (const InputBindingUVE& binding : bindings) {
            sum += GetBindingValueUVE(binding, m_currentKeyState, m_currentMouseButtonState,
                                      m_currentGamepadState, m_previousGamepadState, false);
        }
        return sum;
    };
    return std::clamp(SumBindingValuesUVE(it->second.positiveBindings) -
                          SumBindingValuesUVE(it->second.negativeBindings),
                      -1.0F, 1.0F);
}

} // namespace UVE::Input
