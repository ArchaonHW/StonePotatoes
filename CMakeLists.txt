cmake_minimum_required(VERSION 3.20)
project(StonePotatoes VERSION 0.1.0 LANGUAGES CXX)

# Set C++ standard
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

# Disable debug CRT in release builds to avoid linker errors
if(MSVC)
    set(CMAKE_CXX_FLAGS_RELEASE "${CMAKE_CXX_FLAGS_RELEASE} /MT")
    set(CMAKE_CXX_FLAGS_DEBUG "${CMAKE_CXX_FLAGS_DEBUG} /MTd")
    # Disable debug reporting
    add_definitions(-D_CRT_SECURE_NO_WARNINGS)
endif()

# Build options
option(STONEPOTATOES_BUILD_TESTS "Build tests" ON)
option(STONEPOTATOES_BUILD_EXAMPLES "Build examples" ON)
option(STONEPOTATOES_BUILD_EDITOR "Build editor" ON)
option(STONEPOTATOES_ENABLE_PROFILING "Enable profiling" ON)
option(STONEPOTATOES_ENABLE_ASSERTIONS "Enable assertions" OFF)

# Compiler flags
if(MSVC)
    add_compile_options(/W4 /WX- /utf-8)
    if(STONEPOTATOES_ENABLE_ASSERTIONS)
        add_compile_definitions(_DEBUG)
    endif()
else()
    add_compile_options(-Wall -Wextra -Wpedantic)
    if(STONEPOTATOES_ENABLE_ASSERTIONS)
        add_compile_definitions(DEBUG)
    endif()
endif()

# Find dependencies
find_package(Threads REQUIRED)

# External dependencies
add_subdirectory(external)

# Core library
add_subdirectory(src/core)

# ECS library
add_subdirectory(src/ecs)

# Rendering library
add_subdirectory(src/rendering)

# Physics library
add_subdirectory(src/physics)

# AI library
add_subdirectory(src/ai)

# World library
add_subdirectory(src/world)

# Civilization library
add_subdirectory(src/civilization)

# Agent library
add_subdirectory(src/agent)

# Networking library
add_subdirectory(src/networking)

# Data library
add_subdirectory(src/data)

# GPU library
add_subdirectory(src/gpu)

# Math library (Riemann zeta core + spectral tools)
add_subdirectory(src/math)

# IQGPU library (unified compute + quantum-inspired)
add_subdirectory(src/iqgpu)

# Game library
add_subdirectory(src/game)

# API libraries
add_subdirectory(src/api/cpp)
add_subdirectory(src/api/python)
add_subdirectory(src/api/lua)

# Self-Developing Agent library
add_subdirectory(src/self_developing_agent)

# Tests
if(STONEPOTATOES_BUILD_TESTS)
    enable_testing()
    add_subdirectory(tests)
endif()

# Examples
if(STONEPOTATOES_BUILD_EXAMPLES)
    add_subdirectory(examples)
endif()

# Editor
if(STONEPOTATOES_BUILD_EDITOR)
    add_subdirectory(src/editor)
endif()

# Main executable
add_executable(StonePotatoes src/main.cpp)
target_link_libraries(StonePotatoes
    PRIVATE
        StonePotatoesCore
        StonePotatoesECS
        StonePotatoesSelfDevelopingAgent
        StonePotatoesIQGPU
        StonePotatoesAI
        StonePotatoesWorld
        Threads::Threads
)

# Install configuration
include(GNUInstallDirs)
install(TARGETS StonePotatoesCore StonePotatoesECS StonePotatoesMath StonePotatoesRendering StonePotatoesPhysics StonePotatoesAI StonePotatoesWorld StonePotatoesCivilization StonePotatoesAgent StonePotatoesNetworking StonePotatoesData StonePotatoesGPU StonePotatoesMath StonePotatoesIQGPU StonePotatoesGame StonePotatoesSelfDevelopingAgent
    LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
    ARCHIVE DESTINATION ${CMAKE_INSTALL_LIBDIR}
    RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
)

install(DIRECTORY include/ DESTINATION ${CMAKE_INSTALL_INCLUDEDIR})
