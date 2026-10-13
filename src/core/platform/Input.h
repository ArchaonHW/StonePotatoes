#pragma once

#include <cstdint>

namespace StonePotatoes {
namespace Platform {

enum class Key : int {
    Unknown = -1,
    Space = 32,
    Apostrophe = 39,
    Comma = 44,
    Minus = 45,
    Period = 46,
    Slash = 47,
    Key0 = 48,
    Key1 = 49,
    Key2 = 50,
    Key3 = 51,
    Key4 = 52,
    Key5 = 53,
    Key6 = 54,
    Key7 = 55,
    Key8 = 56,
    Key9 = 57,
    Semicolon = 59,
    Equal = 61,
    A = 65,
    B = 66,
    C = 67,
    D = 68,
    E = 69,
    F = 70,
    G = 71,
    H = 72,
    I = 73,
    J = 74,
    K = 75,
    L = 76,
    M = 77,
    N = 78,
    O = 79,
    P = 80,
    Q = 81,
    R = 82,
    S = 83,
    T = 84,
    U = 85,
    V = 86,
    W = 87,
    X = 88,
    Y = 89,
    Z = 90,
    LeftBracket = 91,
    Backslash = 92,
    RightBracket = 93,
    GraveAccent = 96,
    World1 = 161,
    World2 = 162,
    Escape = 256,
    Enter = 257,
    Tab = 258,
    Backspace = 259,
    Insert = 260,
    Delete = 261,
    Right = 262,
    Left = 263,
    Down = 264,
    Up = 265,
    PageUp = 266,
    PageDown = 267,
    Home = 268,
    End = 269,
    CapsLock = 280,
    ScrollLock = 281,
    NumLock = 282,
    PrintScreen = 283,
    Pause = 284,
    F1 = 290,
    F2 = 291,
    F3 = 292,
    F4 = 293,
    F5 = 294,
    F6 = 295,
    F7 = 296,
    F8 = 297,
    F9 = 298,
    F10 = 299,
    F11 = 300,
    F12 = 301,
    F13 = 302,
    F14 = 303,
    F15 = 304,
    F16 = 305,
    F17 = 306,
    F18 = 307,
    F19 = 308,
    F20 = 309,
    F21 = 310,
    F22 = 311,
    F23 = 312,
    F24 = 313,
    F25 = 314,
    KP0 = 320,
    KP1 = 321,
    KP2 = 322,
    KP3 = 323,
    KP4 = 324,
    KP5 = 325,
    KP6 = 326,
    KP7 = 327,
    KP8 = 328,
    KP9 = 329,
    KPDecimal = 330,
    KPDivide = 331,
    KPMultiply = 332,
    KPSubtract = 333,
    KPAdd = 334,
    KPEnter = 335,
    KPEqual = 336,
    KPLeftShift = 340,
    KPRightShift = 341,
    KPLeftControl = 342,
    KPRightControl = 343,
    KPLeftAlt = 344,
    KPRightAlt = 345,
    LeftShift = 340,
    LeftControl = 341,
    LeftAlt = 342,
    LeftSuper = 343,
    RightShift = 344,
    RightControl = 345,
    RightAlt = 346,
    RightSuper = 347,
    Menu = 348
};

enum class MouseButton : int {
    Left = 0,
    Right = 1,
    Middle = 2,
    Button4 = 3,
    Button5 = 4,
    Button6 = 5,
    Button7 = 6,
    Button8 = 7
};

enum class KeyAction : int {
    Release = 0,
    Press = 1,
    Repeat = 2
};

struct KeyState {
    bool pressed = false;
    bool repeated = false;
};

struct MouseState {
    float x = 0.0f;
    float y = 0.0f;
    float deltaX = 0.0f;
    float deltaY = 0.0f;
    float scrollX = 0.0f;
    float scrollY = 0.0f;
    KeyState buttons[8];
};

class Input {
public:
    static Input& GetInstance();

    void Initialize();
    void Update();

    bool IsKeyPressed(Key key) const;
    bool IsKeyReleased(Key key) const;
    bool IsKeyDown(Key key) const;

    bool IsMouseButtonPressed(MouseButton button) const;
    bool IsMouseButtonReleased(MouseButton button) const;
    bool IsMouseButtonDown(MouseButton button) const;

    float GetMouseX() const;
    float GetMouseY() const;
    float GetMouseDeltaX() const;
    float GetMouseDeltaY() const;
    float GetScrollX() const;
    float GetScrollY() const;

    // GLFW callbacks (public for Window class to access)
    static void KeyCallback(void* window, int key, int scancode, int action, int mods);
    static void MouseButtonCallback(void* window, int button, int action, int mods);
    static void MousePositionCallback(void* window, double xpos, double ypos);
    static void ScrollCallback(void* window, double xoffset, double yoffset);

private:
    Input();
    ~Input();

    KeyState m_KeyMap[512];
    MouseState m_MouseState;
    bool m_KeysPressed[512];
    bool m_KeysReleased[512];
    bool m_MouseButtonsPressed[8];
    bool m_MouseButtonsReleased[8];
};

} // namespace Platform
} // namespace StonePotatoes
