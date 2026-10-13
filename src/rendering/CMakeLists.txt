# Rendering library - simple 2D OpenGL 2.1 renderer + ImGui
# 渲染庫 — 簡單 2D OpenGL 2.1 渲染器 + ImGui
add_library(StonePotatoesRendering STATIC
    Renderer.cpp
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
    glfw
    imgui
)
if(WIN32)
    target_link_libraries(StonePotatoesRendering PUBLIC opengl32)
else()
    find_package(OpenGL REQUIRED)
    target_link_libraries(StonePotatoesRendering PUBLIC OpenGL::GL)
endif()
