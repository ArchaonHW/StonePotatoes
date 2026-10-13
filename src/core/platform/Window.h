#pragma once

#include <cstdint>

namespace Genesis {
namespace Platform {

struct WindowConfig {
    int width = 1280;
    int height = 720;
    const char* title = "Genesis Engine";
    bool fullscreen = false;
    bool resizable = true;
    bool vsync = true;
};

class Window {
public:
    Window();
    ~Window();

    bool Create(const WindowConfig& config);
    void Destroy();

    bool ShouldClose() const;
    void PollEvents();
    void SwapBuffers();

    void* GetNativeHandle() const;
    int GetWidth() const;
    int GetHeight() const;

private:
    void* m_NativeWindow = nullptr;
    void* m_NativeContext = nullptr;
    int m_Width = 0;
    int m_Height = 0;
    bool m_ShouldClose = false;
};

} // namespace Platform
} // namespace Genesis
