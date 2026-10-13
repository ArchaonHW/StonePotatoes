#include "Window.h"
#include "Platform.h"
#include "Input.h"
#include "../logging/Logger.h"
#include <GLFW/glfw3.h>

namespace StonePotatoes {
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
        STONEPOTATOES_LOG_ERROR("Failed to create GLFW window");
        return false;
    }

    // Make the window's context current
    glfwMakeContextCurrent(m_NativeWindow);

    // Store dimensions
    m_Width = config.width;
    m_Height = config.height;
    m_ShouldClose = false;

    // Set user pointer for callbacks
    glfwSetWindowUserPointer(m_NativeWindow, this);

    // Set close callback
    glfwSetWindowCloseCallback(m_NativeWindow, [](GLFWwindow* window) {
        Window* win = static_cast<Window*>(glfwGetWindowUserPointer(window));
        if (win) {
            win->m_ShouldClose = true;
        }
    });

    // Set input callbacks
    glfwSetKeyCallback(m_NativeWindow, [](GLFWwindow* window, int key, int scancode, int action, int mods) {
        Input::KeyCallback(window, key, scancode, action, mods);
    });

    glfwSetMouseButtonCallback(m_NativeWindow, [](GLFWwindow* window, int button, int action, int mods) {
        Input::MouseButtonCallback(window, button, action, mods);
    });

    glfwSetCursorPosCallback(m_NativeWindow, [](GLFWwindow* window, double xpos, double ypos) {
        Input::MousePositionCallback(window, xpos, ypos);
    });

    glfwSetScrollCallback(m_NativeWindow, [](GLFWwindow* window, double xoffset, double yoffset) {
        Input::ScrollCallback(window, xoffset, yoffset);
    });

    STONEPOTATOES_LOG_INFO("Window created: %dx%d", m_Width, m_Height);
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
        return glfwWindowShouldClose(m_NativeWindow) != 0;
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
        glfwSwapBuffers(m_NativeWindow);
    }
}

void* Window::GetNativeHandle() const {
    return m_NativeWindow;
}

int Window::GetWidth() const {
    if (m_NativeWindow) {
        int width, height;
        glfwGetWindowSize(m_NativeWindow, &width, &height);
        return width;
    }
    return m_Width;
}

int Window::GetHeight() const {
    if (m_NativeWindow) {
        int width, height;
        glfwGetWindowSize(m_NativeWindow, &width, &height);
        return height;
    }
    return m_Height;
}

} // namespace Platform
} // namespace StonePotatoes
