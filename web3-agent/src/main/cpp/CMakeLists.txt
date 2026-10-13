cmake_minimum_required(VERSION 3.15)
project(stonepotatoes_engine)

# Find JNI
find_package(JNI REQUIRED)

# Find Vulkan (optional, for actual GPU support)
set(VULKAN_SDK_PATH "D:/HWC/.tools/VulkanSDK/1.4.363.0")
if(EXISTS "${VULKAN_SDK_PATH}")
    include_directories(${VULKAN_SDK_PATH}/Include)
    link_directories(${VULKAN_SDK_PATH}/Lib)
    set(Vulkan_FOUND TRUE)
    message(STATUS "Vulkan SDK path: ${VULKAN_SDK_PATH}")
endif()
find_package(Vulkan QUIET)

# Include directories
include_directories(
    ${JNI_INCLUDE_DIRS}
    ${CMAKE_SOURCE_DIR}/../../../../src/gpu
    ${CMAKE_SOURCE_DIR}/../../../../src
)

# Source files
set(SOURCES
    stonepotatoes_engine.cpp
)

# Create shared library (DLL on Windows)
add_library(stonepotatoes_engine SHARED ${SOURCES})

# Add StonePotatoes C++ sources if available
# Note: VulkanBackend.cpp compilation has C++ compatibility issues with MSVC
# For now, we compile without VulkanBackend.cpp to ensure JNI bridge works
# if(EXISTS "${CMAKE_SOURCE_DIR}/../../../../src/gpu/VulkanBackend.cpp")
#     if(Vulkan_FOUND)
#         target_link_libraries(stonepotatoes_engine ${VULKAN_SDK_PATH}/Lib/vulkan-1.lib)
#         target_compile_definitions(stonepotatoes_engine PRIVATE SP_VULKAN)
#         message(STATUS "Vulkan found: ${VULKAN_SDK_PATH}")
#     else()
#         message(STATUS "Vulkan not found, compiling without GPU support")
#     endif()
#     target_sources(stonepotatoes_engine PRIVATE ../../../../src/gpu/VulkanBackend.cpp)
#     message(STATUS "Adding VulkanBackend.cpp")
# endif()

# Set output directory
set_target_properties(stonepotatoes_engine PROPERTIES
    LIBRARY_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/lib
    RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin
)

# Windows-specific settings
if(WIN32)
    set_target_properties(stonepotatoes_engine PROPERTIES
        SUFFIX ".dll"
        OUTPUT_NAME "stonepotatoes_engine"
    )
endif()
