#include "Renderer.h"

#include "../core/logging/Logger.h"

#include <GLFW/glfw3.h>
#ifdef _WIN32
#include <GL/gl.h>
#else
#include <GL/gl.h>
#endif
#include <imgui.h>
#include <imgui_impl_glfw.h>
#include <imgui_impl_opengl2.h>

#include <cmath>

namespace StonePotatoes {
namespace Rendering {

struct Renderer::Impl {
    GLFWwindow* window = nullptr;
    glm::vec2 viewCenter{0.0f, 0.0f};
    float zoom = 1.0f;
    int width = 0, height = 0;
};

Renderer::Renderer() : m_Impl(std::make_unique<Impl>()) {}
Renderer::~Renderer() { Shutdown(); }

bool Renderer::Initialize(const Config& cfg) {
    m_Impl->width = cfg.width;
    m_Impl->height = cfg.height;

    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 2);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 1);
    m_Impl->window = glfwCreateWindow(cfg.width, cfg.height, cfg.title,
                                       nullptr, nullptr);
    if (!m_Impl->window) {
        STONEPOTATOES_LOG_ERROR("Failed to create GLFW window");
        return false;
    }
    glfwMakeContextCurrent(m_Impl->window);
    glfwSwapInterval(1);  // vsync

    // ImGui setup
    // ImGui 設定
    IMGUI_CHECKVERSION();
    ImGui::CreateContext();
    ImGuiIO& io = ImGui::GetIO();
    (void)io;
    ImGui::StyleColorsDark();
    ImGui_ImplGlfw_InitForOpenGL(m_Impl->window, true);
    ImGui_ImplOpenGL2_Init();

    // Enable basic OpenGL features
    // 啟用基本 OpenGL 功能
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    glEnable(GL_POINT_SMOOTH);

    STONEPOTATOES_LOG_INFO("Renderer initialized: %dx%d", cfg.width,
                           cfg.height);
    return true;
}

void Renderer::Shutdown() {
    ImGui_ImplOpenGL2_Shutdown();
    ImGui_ImplGlfw_Shutdown();
    ImGui::DestroyContext();
    if (m_Impl->window) glfwDestroyWindow(m_Impl->window);
    m_Impl->window = nullptr;
}

bool Renderer::ShouldClose() const {
    return glfwWindowShouldClose(m_Impl->window);
}

void Renderer::BeginFrame() {
    glfwPollEvents();
    ImGui_ImplOpenGL2_NewFrame();
    ImGui_ImplGlfw_NewFrame();
    ImGui::NewFrame();
    glClearColor(0.1f, 0.1f, 0.12f, 1.0f);
    glClear(GL_COLOR_BUFFER_BIT);
    glViewport(0, 0, m_Impl->width, m_Impl->height);

    // Set up orthographic projection
    // 設定正交投影
    glMatrixMode(GL_PROJECTION);
    glLoadIdentity();
    const float aspect =
        static_cast<float>(m_Impl->width) / m_Impl->height;
    const float h = 2.0f / m_Impl->zoom;
    const float w = h * aspect;
    glOrtho(m_Impl->viewCenter.x - w / 2, m_Impl->viewCenter.x + w / 2,
            m_Impl->viewCenter.y - h / 2, m_Impl->viewCenter.y + h / 2,
            -1.0, 1.0);
    glMatrixMode(GL_MODELVIEW);
    glLoadIdentity();
}

void Renderer::EndFrame() {
    ImGui::Render();
    ImGui_ImplOpenGL2_RenderDrawData(ImGui::GetDrawData());
    glfwSwapBuffers(m_Impl->window);
}

void Renderer::SetView(float centerX, float centerY, float zoom) {
    m_Impl->viewCenter = {centerX, centerY};
    m_Impl->zoom = zoom;
}

void Renderer::DrawPoint(float x, float y, float radius,
                         const glm::vec3& color) {
    glColor3f(color.r, color.g, color.b);
    glPointSize(radius * 10.0f);
    glBegin(GL_POINTS);
    glVertex2f(x, y);
    glEnd();
}

void Renderer::DrawCircle(float x, float y, float radius,
                         const glm::vec3& color) {
    glColor3f(color.r, color.g, color.b);
    glLineWidth(2.0f);
    glBegin(GL_LINE_LOOP);
    const int segs = 32;
    const float step = 6.28318530718f / segs;
    for (int i = 0; i < segs; ++i) {
        const float ang = i * step;
        glVertex2f(x + std::cos(ang) * radius,
                  y + std::sin(ang) * radius);
    }
    glEnd();
}

void Renderer::BeginImGui() { ImGui::Begin("StonePotatoes Stats"); }
void Renderer::EndImGui() { ImGui::End(); }

} // namespace Rendering
} // namespace StonePotatoes
