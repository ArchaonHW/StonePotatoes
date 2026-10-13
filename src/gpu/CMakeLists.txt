# GPU library — compute abstraction. CPU reference backend + Vulkan backend
# (when the Vulkan SDK is present). GLSL compute shaders are compiled to
# SPIR-V by glslc at build time into ${CMAKE_BINARY_DIR}/shaders.
# GPU 函式庫 — 計算抽象。CPU 參考後端 + Vulkan 後端（有 SDK 時）。
# GLSL 計算 shader 由 glslc 建置期編譯為 SPIR-V 至 build/shaders。
add_library(StonePotatoesGPU STATIC
    ComputeDevice.cpp
    ComputeBuffer.cpp
    CPUBackend.cpp
    ComputeGraph.cpp
    VulkanBackend.cpp
    stub.cpp
)

target_include_directories(StonePotatoesGPU
    PUBLIC
        ${CMAKE_CURRENT_SOURCE_DIR}
)

target_link_libraries(StonePotatoesGPU
    PUBLIC
        StonePotatoesCore
        StonePotatoesECS
        StonePotatoesAI
        StonePotatoesPhysics
)

# --- Optional Vulkan backend | 可選 Vulkan 後端 ---
find_package(Vulkan QUIET)
if(Vulkan_FOUND)
    target_compile_definitions(StonePotatoesGPU PUBLIC SP_VULKAN=1)
    target_link_libraries(StonePotatoesGPU PUBLIC Vulkan::Vulkan)

    # Compile every shaders/*.comp → ${CMAKE_BINARY_DIR}/shaders/*.spv
    # 將 shaders/*.comp 編譯為 ${CMAKE_BINARY_DIR}/shaders/*.spv
    set(SP_SHADER_DIR "${CMAKE_BINARY_DIR}/shaders")
    file(MAKE_DIRECTORY ${SP_SHADER_DIR})
    file(GLOB SP_SHADER_SOURCES "${CMAKE_CURRENT_SOURCE_DIR}/shaders/*.comp")
    set(SP_SHADER_OUTPUTS)
    foreach(src ${SP_SHADER_SOURCES})
        get_filename_component(name ${src} NAME_WE)
        set(spv ${SP_SHADER_DIR}/${name}.spv)
        add_custom_command(
            OUTPUT ${spv}
            COMMAND ${Vulkan_GLSLC_EXECUTABLE} -O ${src} -o ${spv}
            DEPENDS ${src}
            COMMENT "glslc ${name}.comp -> ${name}.spv"
        )
        list(APPEND SP_SHADER_OUTPUTS ${spv})
    endforeach()
    add_custom_target(StonePotatoesShaders ALL DEPENDS ${SP_SHADER_OUTPUTS})
    add_dependencies(StonePotatoesGPU StonePotatoesShaders)
    target_compile_definitions(StonePotatoesGPU PUBLIC
        SP_SHADER_DIR="${SP_SHADER_DIR}")
else()
    message(STATUS "Vulkan SDK not found — building CPU backend only | 未找到 Vulkan SDK — 僅建置 CPU 後端")
endif()
