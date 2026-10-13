#include "Input.h"
#include "Window.h"
#include "../logging/Logger.h"
#include <GLFW/glfw3.h>
#include <cstring>

namespace StonePotatoes {
namespace Platform {

Input::Input() {
    memset(m_KeyMap, 0, sizeof(m_KeyMap));
    memset(m_KeysPressed, 0, sizeof(m_KeysPressed));
    memset(m_KeysReleased, 0, sizeof(m_KeysReleased));
    memset(m_MouseButtonsPressed, 0, sizeof(m_MouseButtonsPressed));
    memset(m_MouseButtonsReleased, 0, sizeof(m_MouseButtonsReleased));
}

Input::~Input() {
}

Input& Input::GetInstance() {
    static Input instance;
    return instance;
}

void Input::Initialize() {
    STONEPOTATOES_LOG_INFO("Input: Initialized");
}

void Input::Update() {
    // Reset pressed/released states
    memset(m_KeysPressed, 0, sizeof(m_KeysPressed));
    memset(m_KeysReleased, 0, sizeof(m_KeysReleased));
    memset(m_MouseButtonsPressed, 0, sizeof(m_MouseButtonsPressed));
    memset(m_MouseButtonsReleased, 0, sizeof(m_MouseButtonsReleased));

    // Reset mouse delta
    m_MouseState.deltaX = 0.0f;
    m_MouseState.deltaY = 0.0f;
    m_MouseState.scrollX = 0.0f;
    m_MouseState.scrollY = 0.0f;
}

void Input::KeyCallback(void* window, int key, int scancode, int action, int mods) {
    (void)window;
    (void)scancode;
    (void)mods;
    
    Input& input = GetInstance();
    
    if (key >= 0 && key < 512) {
        if (action == GLFW_PRESS) {
            input.m_KeyMap[key].pressed = true;
            input.m_KeysPressed[key] = true;
        } else if (action == GLFW_RELEASE) {
            input.m_KeyMap[key].pressed = false;
            input.m_KeysReleased[key] = true;
        } else if (action == GLFW_REPEAT) {
            input.m_KeyMap[key].repeated = true;
        }
    }
}

void Input::MouseButtonCallback(void* window, int button, int action, int mods) {
    (void)window;
    (void)mods;
    
    Input& input = GetInstance();
    
    if (button >= 0 && button < 8) {
        if (action == GLFW_PRESS) {
            input.m_MouseState.buttons[button].pressed = true;
            input.m_MouseButtonsPressed[button] = true;
        } else if (action == GLFW_RELEASE) {
            input.m_MouseState.buttons[button].pressed = false;
            input.m_MouseButtonsReleased[button] = true;
        }
    }
}

void Input::MousePositionCallback(void* window, double xpos, double ypos) {
    (void)window;
    Input& input = GetInstance();
    
    float newX = static_cast<float>(xpos);
    float newY = static_cast<float>(ypos);
    
    input.m_MouseState.deltaX = newX - input.m_MouseState.x;
    input.m_MouseState.deltaY = newY - input.m_MouseState.y;
    
    input.m_MouseState.x = newX;
    input.m_MouseState.y = newY;
}

void Input::ScrollCallback(void* window, double xoffset, double yoffset) {
    (void)window;
    Input& input = GetInstance();
    
    input.m_MouseState.scrollX = static_cast<float>(xoffset);
    input.m_MouseState.scrollY = static_cast<float>(yoffset);
}

bool Input::IsKeyPressed(Key key) const {
    int keyInt = static_cast<int>(key);
    if (keyInt >= 0 && keyInt < 512) {
        return m_KeysPressed[keyInt];
    }
    return false;
}

bool Input::IsKeyReleased(Key key) const {
    int keyInt = static_cast<int>(key);
    if (keyInt >= 0 && keyInt < 512) {
        return m_KeysReleased[keyInt];
    }
    return false;
}

bool Input::IsKeyDown(Key key) const {
    int keyInt = static_cast<int>(key);
    if (keyInt >= 0 && keyInt < 512) {
        return m_KeyMap[keyInt].pressed;
    }
    return false;
}

bool Input::IsMouseButtonPressed(MouseButton button) const {
    int buttonInt = static_cast<int>(button);
    if (buttonInt >= 0 && buttonInt < 8) {
        return m_MouseButtonsPressed[buttonInt];
    }
    return false;
}

bool Input::IsMouseButtonReleased(MouseButton button) const {
    int buttonInt = static_cast<int>(button);
    if (buttonInt >= 0 && buttonInt < 8) {
        return m_MouseButtonsReleased[buttonInt];
    }
    return false;
}

bool Input::IsMouseButtonDown(MouseButton button) const {
    int buttonInt = static_cast<int>(button);
    if (buttonInt >= 0 && buttonInt < 8) {
        return m_MouseState.buttons[buttonInt].pressed;
    }
    return false;
}

float Input::GetMouseX() const {
    return m_MouseState.x;
}

float Input::GetMouseY() const {
    return m_MouseState.y;
}

float Input::GetMouseDeltaX() const {
    return m_MouseState.deltaX;
}

float Input::GetMouseDeltaY() const {
    return m_MouseState.deltaY;
}

float Input::GetScrollX() const {
    return m_MouseState.scrollX;
}

float Input::GetScrollY() const {
    return m_MouseState.scrollY;
}

} // namespace Platform
} // namespace StonePotatoes
