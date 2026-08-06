// Copyright (c) 2026 UniVex Studios. All Rights Reserved.


#include "uve/input/input_system_uve.h"

#include <cstddef>
#include <utility>

#include "uve/input/input_action_triggered_event_uve.h"

namespace UVE::Input {

InputSystemUVE::InputSystemUVE(Events::IEventSystemUVE& eventSystem) : m_eventSystem(&eventSystem) {}

bool InputSystemUVE::AnyBindingDownUVE(const std::vector<InputBindingUVE>& bindings,
                                        const std::array<bool, kKeyCodeCount>& keyState,
                                        const std::array<bool, kMouseButtonCount>& mouseState) noexcept {
    for (const InputBindingUVE& binding : bindings) {
        const bool isDown = binding.source == InputBindingSourceUVE::Keyboard
                                 ? keyState[static_cast<std::size_t>(binding.key)]
                                 : mouseState[static_cast<std::size_t>(binding.mouseButton)];
        if (isDown) {
            return true;
        }
    }
    return false;
}

void InputSystemUVE::SetKeyStateUVE(KeyCodeUVE key, bool isDown) {
    const std::lock_guard<std::mutex> lock(m_liveStateMutex);
    m_liveKeyState[static_cast<std::size_t>(key)] = isDown;
}

void InputSystemUVE::SetMouseButtonStateUVE(MouseButtonUVE button, bool isDown) {
    const std::lock_guard<std::mutex> lock(m_liveStateMutex);
    m_liveMouseButtonState[static_cast<std::size_t>(button)] = isDown;
}

void InputSystemUVE::SetMousePositionUVE(Math::Vector2UVE position) {
    const std::lock_guard<std::mutex> lock(m_liveStateMutex);
    m_liveMousePosition = position;
}

void InputSystemUVE::SetMouseScrollDeltaUVE(float delta) {
    const std::lock_guard<std::mutex> lock(m_liveStateMutex);
    m_scrollDeltaAccumulator += delta;
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
        m_mouseScrollDelta = m_scrollDeltaAccumulator;
        m_scrollDeltaAccumulator = 0.0F;
    }
    m_mouseDelta = m_currentMousePosition - m_previousMousePosition;

    for (const auto& [name, action] : m_actions) {
        if (action.type != InputActionTypeUVE::Button) {
            continue;
        }
        const bool isDownNow = AnyBindingDownUVE(action.positiveBindings, m_currentKeyState, m_currentMouseButtonState);
        const bool wasDownBefore =
            AnyBindingDownUVE(action.positiveBindings, m_previousKeyState, m_previousMouseButtonState);
        if (isDownNow && !wasDownBefore) {
            m_eventSystem->QueueEvent(InputActionTriggeredEventUVE{name, InputActionTypeUVE::Button, 0.0F});
        }
    }
}

bool InputSystemUVE::IsKeyDownUVE(KeyCodeUVE key) const {
    return m_currentKeyState[static_cast<std::size_t>(key)];
}

bool InputSystemUVE::WasKeyPressedThisFrameUVE(KeyCodeUVE key) const {
    const std::size_t index = static_cast<std::size_t>(key);
    return m_currentKeyState[index] && !m_previousKeyState[index];
}

bool InputSystemUVE::WasKeyReleasedThisFrameUVE(KeyCodeUVE key) const {
    const std::size_t index = static_cast<std::size_t>(key);
    return !m_currentKeyState[index] && m_previousKeyState[index];
}

bool InputSystemUVE::IsMouseButtonDownUVE(MouseButtonUVE button) const {
    return m_currentMouseButtonState[static_cast<std::size_t>(button)];
}

bool InputSystemUVE::WasMouseButtonPressedThisFrameUVE(MouseButtonUVE button) const {
    const std::size_t index = static_cast<std::size_t>(button);
    return m_currentMouseButtonState[index] && !m_previousMouseButtonState[index];
}

bool InputSystemUVE::WasMouseButtonReleasedThisFrameUVE(MouseButtonUVE button) const {
    const std::size_t index = static_cast<std::size_t>(button);
    return !m_currentMouseButtonState[index] && m_previousMouseButtonState[index];
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
    const std::string name = action.name;
    m_actions[name] = std::move(action);
}

bool InputSystemUVE::UnregisterActionUVE(std::string_view actionName) {
    return m_actions.erase(std::string(actionName)) > 0;
}

bool InputSystemUVE::IsActionTriggeredUVE(std::string_view actionName) const {
    const auto it = m_actions.find(std::string(actionName));
    if (it == m_actions.end()) {
        return false;
    }
    const bool isDownNow = AnyBindingDownUVE(it->second.positiveBindings, m_currentKeyState, m_currentMouseButtonState);
    const bool wasDownBefore =
        AnyBindingDownUVE(it->second.positiveBindings, m_previousKeyState, m_previousMouseButtonState);
    return isDownNow && !wasDownBefore;
}

bool InputSystemUVE::IsActionHeldUVE(std::string_view actionName) const {
    const auto it = m_actions.find(std::string(actionName));
    if (it == m_actions.end()) {
        return false;
    }
    return AnyBindingDownUVE(it->second.positiveBindings, m_currentKeyState, m_currentMouseButtonState);
}

bool InputSystemUVE::IsActionReleasedUVE(std::string_view actionName) const {
    const auto it = m_actions.find(std::string(actionName));
    if (it == m_actions.end()) {
        return false;
    }
    const bool isDownNow = AnyBindingDownUVE(it->second.positiveBindings, m_currentKeyState, m_currentMouseButtonState);
    const bool wasDownBefore =
        AnyBindingDownUVE(it->second.positiveBindings, m_previousKeyState, m_previousMouseButtonState);
    return !isDownNow && wasDownBefore;
}

float InputSystemUVE::GetAxisValueUVE(std::string_view actionName) const {
    const auto it = m_actions.find(std::string(actionName));
    if (it == m_actions.end()) {
        return 0.0F;
    }
    const float positive =
        AnyBindingDownUVE(it->second.positiveBindings, m_currentKeyState, m_currentMouseButtonState) ? 1.0F : 0.0F;
    const float negative =
        AnyBindingDownUVE(it->second.negativeBindings, m_currentKeyState, m_currentMouseButtonState) ? 1.0F : 0.0F;
    return positive - negative;
}

} // namespace UVE::Input
