cmake_minimum_required(VERSION 3.20)
project(StonePotatoes VERSION 0.1.0 LANGUAGES CXX)

include(GNUInstallDirs)
include(CMakePackageConfigHelpers)

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
option(STONEPOTATOES_ENABLE_COVERAGE "Instrument for gcov/lcov coverage (GCC/Clang)" OFF)

# Coverage | 覆蓋率（gcov/lcov）
if(STONEPOTATOES_ENABLE_COVERAGE)
    if(MSVC)
        message(WARNING "STONEPOTATOES_ENABLE_COVERAGE is ignored on MSVC")
    else()
        message(STATUS "Coverage instrumentation enabled | 已啟用覆蓋率插樁")
        add_compile_options(--coverage -O0 -g)
        add_link_options(--coverage)
    endif()
endif()

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
find_package(Vulkan QUIET)

# External dependencies
add_subdirectory(external)

# Core library
add_subdirectory(src/core)

# ECS library
add_subdirectory(src/ecs)

# Math library (Riemann zeta core + spectral tools)
add_subdirectory(src/math)

# Physics library
add_subdirectory(src/physics)

# AI library
add_subdirectory(src/ai)

# World library
add_subdirectory(src/world)

# Civilization library
add_subdirectory(src/civilization)

# Networking library
add_subdirectory(src/networking)

# Data library
add_subdirectory(src/data)

# GPU library
add_subdirectory(src/gpu)

# Rendering library (uses the shared GPU/Vulkan context)
add_subdirectory(src/rendering)

# IQGPU library (unified compute + quantum-inspired)
add_subdirectory(src/iqgpu)

# Game library
add_subdirectory(src/game)

# API libraries
add_subdirectory(src/api/cpp)

# Editor target is developer-only and excluded from installation.
if(STONEPOTATOES_BUILD_EDITOR)
    add_subdirectory(src/editor)
endif()

# Standalone self-developing development tool
add_subdirectory(tools/self_developing_agent)

# Tests
if(STONEPOTATOES_BUILD_TESTS)
    enable_testing()
    add_subdirectory(tests)
endif()

# Examples
if(STONEPOTATOES_BUILD_EXAMPLES)
    add_subdirectory(examples)
endif()

# Main executable
add_executable(StonePotatoes src/main.cpp)
set_target_properties(StonePotatoes PROPERTIES
    RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/bin"
)
target_link_libraries(StonePotatoes
    PRIVATE
        StonePotatoesCore
        StonePotatoesPlatform
        StonePotatoesECS
        StonePotatoesGame
        StonePotatoesIQGPU
        StonePotatoesAI
        StonePotatoesWorld
        Threads::Threads
)

install(TARGETS StonePotatoes
    RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
)
install(FILES LICENSE README.md docs/market-settlement-model.md
    docs/currency-supply-model.md
    DESTINATION ${CMAKE_INSTALL_DATADIR}/doc/StonePotatoes
)

# Installable headless engine package. Windowing remains a build-tree target
# until GLFW is distributed as part of the installed package.
set_target_properties(StonePotatoesCore PROPERTIES EXPORT_NAME Core)
set_target_properties(StonePotatoesECS PROPERTIES EXPORT_NAME ECS)
set_target_properties(StonePotatoesPhysics PROPERTIES EXPORT_NAME Physics)
set_target_properties(StonePotatoesCivilization PROPERTIES EXPORT_NAME Civilization)
set_target_properties(StonePotatoesNetworking PROPERTIES EXPORT_NAME Networking)
set_target_properties(StonePotatoesMath PROPERTIES EXPORT_NAME Math)
set_target_properties(StonePotatoesAI PROPERTIES EXPORT_NAME AI)
set_target_properties(StonePotatoesWorld PROPERTIES EXPORT_NAME World)
set_target_properties(StonePotatoesGame PROPERTIES EXPORT_NAME Game)
set_target_properties(StonePotatoesData PROPERTIES EXPORT_NAME Data)
set_target_properties(StonePotatoesAPI_CPP PROPERTIES EXPORT_NAME CppAPI)
foreach(_target StonePotatoesCore StonePotatoesECS StonePotatoesPhysics StonePotatoesCivilization StonePotatoesNetworking StonePotatoesMath StonePotatoesAI StonePotatoesWorld StonePotatoesGame StonePotatoesData)
    target_compile_options(${_target} PUBLIC "$<$<CXX_COMPILER_ID:MSVC>:/utf-8>")
endforeach()

install(TARGETS StonePotatoesCore StonePotatoesECS StonePotatoesPhysics StonePotatoesCivilization StonePotatoesNetworking StonePotatoesMath StonePotatoesAI StonePotatoesWorld StonePotatoesGame StonePotatoesData StonePotatoesAPI_CPP
    EXPORT StonePotatoesTargets
    LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
    ARCHIVE DESTINATION ${CMAKE_INSTALL_LIBDIR}
    RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
    INCLUDES DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}/StonePotatoes
)

# Install headers only for modules represented by exported package targets.
install(DIRECTORY src/core/ DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}/StonePotatoes/core
    FILES_MATCHING PATTERN "*.h" PATTERN "*.hpp" PATTERN "platform" EXCLUDE)
foreach(_module ecs physics civilization networking math ai world data)
    install(DIRECTORY src/${_module}/
        DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}/StonePotatoes/${_module}
        FILES_MATCHING PATTERN "*.h" PATTERN "*.hpp")
endforeach()
install(DIRECTORY src/game/ DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}/StonePotatoes/game
    FILES_MATCHING PATTERN "*.h" PATTERN "*.hpp")
install(FILES src/api/cpp/StonePotatoes.h
    DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}/StonePotatoes/api/cpp)

install(EXPORT StonePotatoesTargets
    FILE StonePotatoesTargets.cmake
    NAMESPACE StonePotatoes::
    DESTINATION ${CMAKE_INSTALL_LIBDIR}/cmake/StonePotatoes
)
configure_package_config_file(
    cmake/StonePotatoesConfig.cmake.in
    ${CMAKE_CURRENT_BINARY_DIR}/StonePotatoesConfig.cmake
    INSTALL_DESTINATION ${CMAKE_INSTALL_LIBDIR}/cmake/StonePotatoes
)
write_basic_package_version_file(
    ${CMAKE_CURRENT_BINARY_DIR}/StonePotatoesConfigVersion.cmake
    VERSION ${PROJECT_VERSION}
    COMPATIBILITY SameMajorVersion
)
install(FILES
    ${CMAKE_CURRENT_BINARY_DIR}/StonePotatoesConfig.cmake
    ${CMAKE_CURRENT_BINARY_DIR}/StonePotatoesConfigVersion.cmake
    DESTINATION ${CMAKE_INSTALL_LIBDIR}/cmake/StonePotatoes
)

# Create portable ZIP (Windows) and TGZ (Linux/macOS) packages from install rules.
set(CPACK_PACKAGE_NAME "StonePotatoes")
set(CPACK_PACKAGE_VENDOR "StonePotatoes contributors")
set(CPACK_PACKAGE_DESCRIPTION_SUMMARY "StonePotatoes engine runtime and C++ package")
set(CPACK_PACKAGE_VERSION ${PROJECT_VERSION})
set(CPACK_PACKAGE_FILE_NAME "StonePotatoes-${PROJECT_VERSION}-${CMAKE_SYSTEM_NAME}-${CMAKE_SYSTEM_PROCESSOR}")
set(CPACK_PACKAGE_DIRECTORY "${CMAKE_BINARY_DIR}/packages")
if(WIN32)
    set(CPACK_GENERATOR ZIP)
else()
    set(CPACK_GENERATOR TGZ)
endif()
include(CPack)
