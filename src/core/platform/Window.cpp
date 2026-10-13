#include "Window.h"
#include "Platform.h"
#include <GLFW/glfw3.h>

namespace Genesis {
namespace Platform {

Window::Window() = default;
Window::~Window() {
    Destroy();
}

bool Window::Create(const WindowConfig& config) {
    // Configure GLFW
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);

    // Create window
    m_NativeWindow = glfwCreateWindow(config.width, config.height, config.title, nullptr, nullptr);
    if (!m_NativeWindow) {
        Platform::ReportError("Failed to create GLFW window");
        return false;
    }

    // Make the window's context current
    glfwMakeContextCurrent(m_NativeWindow);

    // Store dimensions
    m_Width = config.width;
    m_Height = config.height;
    m_ShouldClose = false;

    // Set user pointer for callbacks
    glfwSetWindowUserPointer(static_cast<GLFWwindow*>(m_NativeWindow), this);

    // Set close callback
    glfwSetWindowCloseCallback(static_cast<GLFWwindow*>(m_NativeWindow), [](GLFWwindow* window) {
        Window* win = static_cast<Window*>(glfwGetWindowUserPointer(window));
        if (win) {
            win->m_ShouldClose = true;
        }
    });

    GENESIS_LOG_INFO("Window created: %dx%d", m_Width, m_Height);
    return true;
}

void Window::Destroy() {
    if (m_NativeWindow) {
        glfwDestroyWindow(static_cast<GLFWwindow*>(m_NativeWindow));
        m_NativeWindow = nullptr;
    }
}

bool Window::ShouldClose() const {
    if (m_NativeWindow) {
        return glfwWindowShouldClose(static_cast<GLFWwindow*>(m_NativeWindow)) != 0;
    }
    return m_ShouldClose;
}

void Window::PollEvents() {
    if (m_NativeWindow) {
        glfwPollEvents();
    }
}

void Window::SwapBuffers() {
    if (m_NativeWindow) {
        glfwSwapBuffers(static_cast<GLFWwindow*>(m_NativeWindow));
    }
}

void* Window::GetNativeHandle() const {
    return m_NativeWindow;
}

int Window::GetWidth() const {
    if (m_NativeWindow) {
        int width, height;
        glfwGetWindowSize(static_cast<GLFWwindow*>(m_NativeWindow), &width, &height);
        return width;
    }
    return m_Width;
}

int Window::GetHeight() const {
    if (m_NativeWindow) {
        int width, height;
        glfwGetWindowSize(static_cast<GLFWwindow*>(m_NativeWindow), &width, &height);
        return height;
    }
    return m_Height;
}

} // namespace Platform
} // namespace Genesis
