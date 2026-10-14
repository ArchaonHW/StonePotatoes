#include "Renderer.h"

#include "../core/logging/Logger.h"

#ifdef SP_VULKAN

#include "VulkanPresentationSmoke.h"
#include "../gpu/VulkanContext.h"

#include <GLFW/glfw3.h>
#include <imgui.h>
#include <imgui_impl_glfw.h>
#include <imgui_impl_vulkan.h>

#include <algorithm>
#include <array>
#include <cmath>
#include <utility>

namespace StonePotatoes::Rendering {

struct Renderer::Impl {
    explicit Impl(std::shared_ptr<GPU::VulkanContext> sharedContext)
        : context(std::move(sharedContext)),
          presentation(std::make_unique<VulkanPresentationSmoke>(context)) {}

    std::shared_ptr<GPU::VulkanContext> context;
    std::unique_ptr<VulkanPresentationSmoke> presentation;
    VkDescriptorPool descriptorPool = VK_NULL_HANDLE;
    VkRenderPass imguiPipelineRenderPass = VK_NULL_HANDLE;
    glm::vec2 viewCenter{0.0f, 0.0f};
    float zoom = 1.0f;
    bool imguiContext = false;
    bool imguiGlfw = false;
    bool imguiVulkan = false;
    bool initialized = false;

    bool CreateImGuiDescriptorPool() {
        constexpr uint32_t descriptorsPerType = 256;
        const std::array<VkDescriptorPoolSize, 11> sizes{{
            {VK_DESCRIPTOR_TYPE_SAMPLER, descriptorsPerType},
            {VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, descriptorsPerType},
            {VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE, descriptorsPerType},
            {VK_DESCRIPTOR_TYPE_STORAGE_IMAGE, descriptorsPerType},
            {VK_DESCRIPTOR_TYPE_UNIFORM_TEXEL_BUFFER, descriptorsPerType},
            {VK_DESCRIPTOR_TYPE_STORAGE_TEXEL_BUFFER, descriptorsPerType},
            {VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, descriptorsPerType},
            {VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, descriptorsPerType},
            {VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER_DYNAMIC, descriptorsPerType},
            {VK_DESCRIPTOR_TYPE_STORAGE_BUFFER_DYNAMIC, descriptorsPerType},
            {VK_DESCRIPTOR_TYPE_INPUT_ATTACHMENT, descriptorsPerType},
        }};
        VkDescriptorPoolCreateInfo info{
            VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO};
        info.flags = VK_DESCRIPTOR_POOL_CREATE_FREE_DESCRIPTOR_SET_BIT;
        info.maxSets = descriptorsPerType *
                       static_cast<uint32_t>(sizes.size());
        info.poolSizeCount = static_cast<uint32_t>(sizes.size());
        info.pPoolSizes = sizes.data();
        return vkCreateDescriptorPool(context->GetDevice(), &info, nullptr,
                                      &descriptorPool) == VK_SUCCESS;
    }

    bool InitializeImGui() {
        IMGUI_CHECKVERSION();
        ImGui::CreateContext();
        imguiContext = true;
        ImGui::StyleColorsDark();
        if (!ImGui_ImplGlfw_InitForVulkan(presentation->GetWindow(), true))
            return false;
        imguiGlfw = true;
        if (!CreateImGuiDescriptorPool()) return false;

        const uint32_t imageCount = presentation->GetImageCount();
        if (imageCount < 2) return false;
        ImGui_ImplVulkan_InitInfo info{};
        info.ApiVersion = VK_API_VERSION_1_1;
        info.Instance = context->GetInstance();
        info.PhysicalDevice = context->GetPhysicalDevice();
        info.Device = context->GetDevice();
        info.QueueFamily = context->GetGraphicsQueueFamily();
        info.Queue = context->GetGraphicsQueue();
        info.DescriptorPool = descriptorPool;
        info.MinImageCount = imageCount;
        info.ImageCount = imageCount;
        info.PipelineInfoMain.RenderPass = presentation->GetRenderPass();
        info.PipelineInfoMain.Subpass = 0;
        info.PipelineInfoMain.MSAASamples = VK_SAMPLE_COUNT_1_BIT;
        if (!ImGui_ImplVulkan_Init(&info)) return false;
        imguiVulkan = true;
        imguiPipelineRenderPass = presentation->GetRenderPass();
        return true;
    }

    void ShutdownImGui() {
        if (context && context->GetDevice()) vkDeviceWaitIdle(context->GetDevice());
        if (imguiVulkan) ImGui_ImplVulkan_Shutdown();
        imguiVulkan = false;
        if (imguiGlfw) ImGui_ImplGlfw_Shutdown();
        imguiGlfw = false;
        if (imguiContext) ImGui::DestroyContext();
        imguiContext = false;
        if (descriptorPool && context && context->GetDevice())
            vkDestroyDescriptorPool(context->GetDevice(), descriptorPool,
                                    nullptr);
        descriptorPool = VK_NULL_HANDLE;
        imguiPipelineRenderPass = VK_NULL_HANDLE;
    }
};

Renderer::Renderer() : Renderer(std::shared_ptr<GPU::VulkanContext>{}) {}
Renderer::Renderer(std::shared_ptr<GPU::VulkanContext> context)
    : m_Impl(std::make_unique<Impl>(std::move(context))) {}
Renderer::~Renderer() { Shutdown(); }

bool Renderer::Initialize(const Config& cfg) {
    Impl& im = *m_Impl;
    if (im.initialized) return true;
    VulkanPresentationSmoke::Config presentationConfig;
    presentationConfig.width = cfg.width;
    presentationConfig.height = cfg.height;
    presentationConfig.title = cfg.title;
    presentationConfig.terminateGlfwOnShutdown = false;
    if (!im.presentation->Initialize(presentationConfig)) {
        STONEPOTATOES_LOG_ERROR("Vulkan renderer initialization failed: %s",
                                im.presentation->LastError().c_str());
        return false;
    }
    im.context = im.presentation->GetContext();
    if (!im.InitializeImGui()) {
        STONEPOTATOES_LOG_ERROR("Vulkan ImGui initialization failed");
        Shutdown();
        return false;
    }
    im.initialized = true;
    STONEPOTATOES_LOG_INFO("Vulkan renderer initialized: %dx%d", cfg.width,
                           cfg.height);
    return true;
}

void Renderer::Shutdown() {
    if (!m_Impl) return;
    Impl& im = *m_Impl;
    im.ShutdownImGui();
    if (im.presentation) im.presentation->Shutdown();
    im.initialized = false;
}

bool Renderer::ShouldClose() const {
    return !m_Impl->initialized || m_Impl->presentation->ShouldClose();
}

void Renderer::BeginFrame() {
    Impl& im = *m_Impl;
    if (!im.initialized) return;
    im.presentation->PollEvents();
    ImGui_ImplVulkan_NewFrame();
    ImGui_ImplGlfw_NewFrame();
    ImGui::NewFrame();
}

bool Renderer::EndFrame() {
    Impl& im = *m_Impl;
    if (!im.initialized) return false;
    ImGui::Render();
    VkClearColorValue clear{};
    clear.float32[0] = 0.1f;
    clear.float32[1] = 0.1f;
    clear.float32[2] = 0.12f;
    clear.float32[3] = 1.0f;
    ImDrawData* drawData = ImGui::GetDrawData();
    const bool presented = im.presentation->PresentFrame(
        clear, [&im, drawData](VkCommandBuffer commandBuffer) {
            const VkRenderPass renderPass = im.presentation->GetRenderPass();
            if (renderPass != im.imguiPipelineRenderPass) {
                ImGui_ImplVulkan_PipelineInfo pipelineInfo{};
                pipelineInfo.RenderPass = renderPass;
                pipelineInfo.Subpass = 0;
                pipelineInfo.MSAASamples = VK_SAMPLE_COUNT_1_BIT;
                ImGui_ImplVulkan_CreateMainPipeline(&pipelineInfo);
                im.imguiPipelineRenderPass = renderPass;
            }
            ImGui_ImplVulkan_RenderDrawData(drawData, commandBuffer);
        });
    if (!presented) {
        STONEPOTATOES_LOG_ERROR("Vulkan renderer frame failed: %s",
                                im.presentation->LastError().c_str());
    }
    return presented;
}

void Renderer::SetView(float centerX, float centerY, float zoom) {
    m_Impl->viewCenter = {centerX, centerY};
    m_Impl->zoom = zoom;
}

void Renderer::DrawPoint(float x, float y, float radius,
                         const glm::vec3& color) {
    Impl& im = *m_Impl;
    if (!im.initialized || im.zoom <= 0.0f) return;
    int width = 0, height = 0;
    glfwGetFramebufferSize(im.presentation->GetWindow(), &width, &height);
    if (width <= 0 || height <= 0) return;
    const float aspect = static_cast<float>(width) / height;
    const ImVec2 screen(
        (x - im.viewCenter.x) * im.zoom / aspect * width * 0.5f + width * 0.5f,
        height * 0.5f - (y - im.viewCenter.y) * im.zoom * height * 0.5f);
    const ImU32 packedColor = ImGui::ColorConvertFloat4ToU32(
        ImVec4(color.r, color.g, color.b, 1.0f));
    ImGui::GetBackgroundDrawList()->AddCircleFilled(
        screen, std::abs(radius) * 5.0f, packedColor, 16);
}

void Renderer::DrawCircle(float x, float y, float radius,
                          const glm::vec3& color) {
    Impl& im = *m_Impl;
    if (!im.initialized || im.zoom <= 0.0f) return;
    int width = 0, height = 0;
    glfwGetFramebufferSize(im.presentation->GetWindow(), &width, &height);
    if (width <= 0 || height <= 0) return;
    const float aspect = static_cast<float>(width) / height;
    const ImVec2 screen(
        (x - im.viewCenter.x) * im.zoom / aspect * width * 0.5f + width * 0.5f,
        height * 0.5f - (y - im.viewCenter.y) * im.zoom * height * 0.5f);
    const ImU32 packedColor = ImGui::ColorConvertFloat4ToU32(
        ImVec4(color.r, color.g, color.b, 1.0f));
    ImGui::GetBackgroundDrawList()->AddCircle(
        screen, std::abs(radius) * im.zoom * height * 0.5f, packedColor, 32,
        2.0f);
}

void Renderer::BeginImGui() {
    if (m_Impl->initialized) ImGui::Begin("StonePotatoes Stats");
}
void Renderer::EndImGui() {
    if (m_Impl->initialized) ImGui::End();
}

std::shared_ptr<GPU::VulkanContext> Renderer::GetVulkanContext() const {
    return m_Impl->context;
}

} // namespace StonePotatoes::Rendering

#else

#include <GLFW/glfw3.h>
#include <GL/gl.h>
#include <imgui.h>
#include <imgui_impl_glfw.h>
#include <imgui_impl_opengl2.h>

#include <cmath>

namespace StonePotatoes::Rendering {

struct Renderer::Impl {
    GLFWwindow* window = nullptr;
    glm::vec2 viewCenter{0.0f, 0.0f};
    float zoom = 1.0f;
    int width = 0, height = 0;
    bool imguiContext = false;
    bool imguiGlfw = false;
    bool imguiOpenGL = false;
};

Renderer::Renderer() : m_Impl(std::make_unique<Impl>()) {}
Renderer::Renderer(std::shared_ptr<GPU::VulkanContext>)
    : Renderer() {}
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
    glfwSwapInterval(1);

    IMGUI_CHECKVERSION();
    ImGui::CreateContext();
    m_Impl->imguiContext = true;
    ImGui::GetIO();
    ImGui::StyleColorsDark();
    m_Impl->imguiGlfw = ImGui_ImplGlfw_InitForOpenGL(m_Impl->window, true);
    m_Impl->imguiOpenGL = ImGui_ImplOpenGL2_Init();
    if (!m_Impl->imguiGlfw || !m_Impl->imguiOpenGL) {
        Shutdown();
        return false;
    }

    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    glEnable(GL_POINT_SMOOTH);
    STONEPOTATOES_LOG_INFO("OpenGL fallback renderer initialized: %dx%d",
                           cfg.width, cfg.height);
    return true;
}

void Renderer::Shutdown() {
    if (!m_Impl) return;
    if (m_Impl->imguiOpenGL) ImGui_ImplOpenGL2_Shutdown();
    m_Impl->imguiOpenGL = false;
    if (m_Impl->imguiGlfw) ImGui_ImplGlfw_Shutdown();
    m_Impl->imguiGlfw = false;
    if (m_Impl->imguiContext) ImGui::DestroyContext();
    m_Impl->imguiContext = false;
    if (m_Impl->window) glfwDestroyWindow(m_Impl->window);
    m_Impl->window = nullptr;
}

bool Renderer::ShouldClose() const {
    return !m_Impl->window || glfwWindowShouldClose(m_Impl->window);
}

void Renderer::BeginFrame() {
    if (!m_Impl->window) return;
    glfwPollEvents();
    ImGui_ImplOpenGL2_NewFrame();
    ImGui_ImplGlfw_NewFrame();
    ImGui::NewFrame();
    glClearColor(0.1f, 0.1f, 0.12f, 1.0f);
    glClear(GL_COLOR_BUFFER_BIT);
    glViewport(0, 0, m_Impl->width, m_Impl->height);
    glMatrixMode(GL_PROJECTION);
    glLoadIdentity();
    const float aspect = static_cast<float>(m_Impl->width) / m_Impl->height;
    const float h = 2.0f / m_Impl->zoom;
    const float w = h * aspect;
    glOrtho(m_Impl->viewCenter.x - w / 2, m_Impl->viewCenter.x + w / 2,
            m_Impl->viewCenter.y - h / 2, m_Impl->viewCenter.y + h / 2,
            -1.0, 1.0);
    glMatrixMode(GL_MODELVIEW);
    glLoadIdentity();
}

bool Renderer::EndFrame() {
    if (!m_Impl->window) return false;
    ImGui::Render();
    ImGui_ImplOpenGL2_RenderDrawData(ImGui::GetDrawData());
    glfwSwapBuffers(m_Impl->window);
    return true;
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
    constexpr int segments = 32;
    constexpr float tau = 6.28318530718f;
    for (int i = 0; i < segments; ++i) {
        const float angle = i * tau / segments;
        glVertex2f(x + std::cos(angle) * radius,
                   y + std::sin(angle) * radius);
    }
    glEnd();
}

void Renderer::BeginImGui() {
    if (m_Impl->window) ImGui::Begin("StonePotatoes Stats");
}
void Renderer::EndImGui() {
    if (m_Impl->window) ImGui::End();
}
std::shared_ptr<GPU::VulkanContext> Renderer::GetVulkanContext() const {
    return {};
}

} // namespace StonePotatoes::Rendering

#endif
