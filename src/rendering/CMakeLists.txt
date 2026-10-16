# Rendering library - Vulkan 2D/3D slice + ImGui (OpenGL fallback without Vulkan)
# 渲染庫 — Vulkan 2D／3D 切片 + ImGui（無 Vulkan 時使用 OpenGL 後備）
add_library(StonePotatoesRendering STATIC
    Renderer.cpp
    Renderer2D.cpp
    Renderer3D.cpp
    VulkanPresentationSmoke.cpp
    stub.cpp
)
target_include_directories(StonePotatoesRendering PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}
    ${imgui_SOURCE_DIR}
    ${imgui_SOURCE_DIR}/backends
    ${glm_SOURCE_DIR}
)
target_link_libraries(StonePotatoesRendering PUBLIC
    StonePotatoesCore
    StonePotatoesECS
    StonePotatoesGPU
    glfw
    imgui
)
if(Vulkan_FOUND)
    add_executable(StonePotatoesVulkanPresentationSmoke
        ${PROJECT_SOURCE_DIR}/examples/vulkan_presentation_smoke.cpp)
    target_link_libraries(StonePotatoesVulkanPresentationSmoke
        PRIVATE StonePotatoesRendering)

    add_executable(StonePotatoesVulkanComputeRendererDemo
        ${PROJECT_SOURCE_DIR}/examples/vulkan_compute_renderer_demo.cpp)
    target_include_directories(StonePotatoesVulkanComputeRendererDemo
        PRIVATE ${PROJECT_SOURCE_DIR}/src)
    target_link_libraries(StonePotatoesVulkanComputeRendererDemo
        PRIVATE StonePotatoesRendering StonePotatoesPlatform)
endif()
if(NOT Vulkan_FOUND)
    if(WIN32)
        target_link_libraries(StonePotatoesRendering PUBLIC opengl32)
    else()
        find_package(OpenGL REQUIRED)
        target_link_libraries(StonePotatoesRendering PUBLIC OpenGL::GL)
    endif()
endif()
