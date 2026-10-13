cmake_minimum_required(VERSION 3.15)
project(stonepotatoes_engine)

# Find JNI
find_package(JNI REQUIRED)

# Find Vulkan (optional, for actual GPU support)
find_package(Vulkan QUIET)

# Include StonePotatoes headers
include_directories(
    ${JNI_INCLUDE_DIRS}
    ${CMAKE_SOURCE_DIR}/../../../src/gpu
    ${CMAKE_SOURCE_DIR}/../../../src
)

# Source files
set(SOURCES
    stonepotatoes_engine.cpp
)

# Add StonePotatoes C++ sources if available
if(EXISTS "${CMAKE_SOURCE_DIR}/../../../src/gpu/VulkanBackend.cpp")
    list(APPEND SOURCES ../../../src/gpu/VulkanBackend.cpp)
endif()

# Create shared library (DLL on Windows)
add_library(stonepotatoes_engine SHARED ${SOURCES})

# Link libraries
if(Vulkan_FOUND)
    target_link_libraries(stonepotatoes_engine Vulkan::Vulkan)
endif()

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
