cmake_minimum_required(VERSION 3.15)
project(stonepotatoes_engine CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# JNI bridge between web3-agent (Java) and the StonePotatoes GPU backend.
# JNI 橋接：web3-agent（Java）↔ StonePotatoes GPU 後端。

find_package(JNI REQUIRED)

# Root of the StonePotatoes repo (web3-agent/src/main/cpp → ../../../..)
get_filename_component(SP_ROOT "${CMAKE_CURRENT_SOURCE_DIR}/../../../.." ABSOLUTE)
set(SP_GPU_DIR "${SP_ROOT}/src/gpu")

# --- Optional Vulkan backend | 可選 Vulkan 後端 ---
# Disable with -DSP_ENGINE_ENABLE_VULKAN=OFF. The SDK is located via the
# VULKAN_SDK env var or -DVULKAN_SDK_PATH=...; without it, the DLL still
# builds and every JNI call reports "unavailable".
# 以 -DSP_ENGINE_ENABLE_VULKAN=OFF 停用。SDK 由 VULKAN_SDK 環境變數或
# -DVULKAN_SDK_PATH 指定；找不到時 DLL 仍可建置，JNI 呼叫皆回報不可用。
option(SP_ENGINE_ENABLE_VULKAN "Build the JNI bridge with the Vulkan backend" ON)
set(VULKAN_SDK_PATH "" CACHE PATH "Vulkan SDK root (optional; falls back to $ENV{VULKAN_SDK})")
if(NOT VULKAN_SDK_PATH AND EXISTS "D:/HWC/.tools/VulkanSDK/1.4.363.0")
    # Owner's local default layout | 擁有者本機預設路徑
    set(VULKAN_SDK_PATH "D:/HWC/.tools/VulkanSDK/1.4.363.0")
endif()
if(VULKAN_SDK_PATH)
    set(ENV{VULKAN_SDK} "${VULKAN_SDK_PATH}")
endif()

set(SP_ENGINE_HAS_VULKAN OFF)
if(SP_ENGINE_ENABLE_VULKAN AND EXISTS "${SP_GPU_DIR}/VulkanBackend.cpp")
    find_package(Vulkan QUIET)
    if(Vulkan_FOUND)
        set(SP_ENGINE_HAS_VULKAN ON)
    endif()
endif()

# Sources are collected BEFORE add_library | 先收集來源再 add_library
set(SOURCES stonepotatoes_engine.cpp)
if(SP_ENGINE_HAS_VULKAN)
    list(APPEND SOURCES "${SP_GPU_DIR}/VulkanBackend.cpp")
endif()

add_library(stonepotatoes_engine SHARED ${SOURCES})

target_include_directories(stonepotatoes_engine PRIVATE
    ${JNI_INCLUDE_DIRS}
    "${SP_GPU_DIR}"
    "${SP_ROOT}/src"
)

if(SP_ENGINE_HAS_VULKAN)
    target_compile_definitions(stonepotatoes_engine PRIVATE SP_VULKAN=1)
    target_link_libraries(stonepotatoes_engine PRIVATE Vulkan::Vulkan)

    # Compile the GPU shaders to SPIR-V next to the DLL (same as root build).
    # 將 GPU shader 編譯為 SPIR-V（與根專案相同）。
    if(Vulkan_GLSLC_EXECUTABLE)
        set(SP_SHADER_OUT "${CMAKE_BINARY_DIR}/shaders")
        file(MAKE_DIRECTORY "${SP_SHADER_OUT}")
        file(GLOB SP_SHADER_SOURCES "${SP_GPU_DIR}/shaders/*.comp")
        set(SP_SHADER_OUTPUTS)
        foreach(src ${SP_SHADER_SOURCES})
            get_filename_component(name ${src} NAME_WE)
            set(spv "${SP_SHADER_OUT}/${name}.spv")
            add_custom_command(OUTPUT ${spv}
                COMMAND ${Vulkan_GLSLC_EXECUTABLE} -O ${src} -o ${spv}
                DEPENDS ${src}
                COMMENT "glslc ${name}.comp -> ${name}.spv")
            list(APPEND SP_SHADER_OUTPUTS ${spv})
        endforeach()
        add_custom_target(stonepotatoes_engine_shaders ALL DEPENDS ${SP_SHADER_OUTPUTS})
        add_dependencies(stonepotatoes_engine stonepotatoes_engine_shaders)
        target_compile_definitions(stonepotatoes_engine PRIVATE SP_SHADER_DIR="${SP_SHADER_OUT}")
    endif()
    message(STATUS "stonepotatoes_engine: Vulkan backend ON (${Vulkan_LIBRARIES})")
else()
    message(STATUS "stonepotatoes_engine: Vulkan backend OFF — JNI stubs only | 僅 JNI 存根")
endif()

if(MSVC)
    target_compile_options(stonepotatoes_engine PRIVATE /utf-8)
endif()

set_target_properties(stonepotatoes_engine PROPERTIES
    OUTPUT_NAME "stonepotatoes_engine"
    LIBRARY_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/lib
    RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin
)