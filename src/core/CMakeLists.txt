# Core library - Foundation systems
add_library(StonePotatoesCore STATIC
    io/FileSystem.cpp
    memory/MemoryPool.cpp
    memory/StackAllocator.cpp
    memory/ArenaAllocator.cpp
    time/Time.cpp
    config/Config.cpp
    logging/Logger.cpp
    plugin/Plugin.cpp
    profiling/Profiler.cpp
)

target_include_directories(StonePotatoesCore
    PUBLIC
        $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}>
        $<BUILD_INTERFACE:${PROJECT_SOURCE_DIR}/src>
        $<INSTALL_INTERFACE:${CMAKE_INSTALL_INCLUDEDIR}/StonePotatoes>
)

target_link_libraries(StonePotatoesCore
    PUBLIC
        Threads::Threads
        ${CMAKE_DL_LIBS}  # dlopen for SharedLibraryPluginLoader | 外掛動態載入
)
target_compile_features(StonePotatoesCore PUBLIC cxx_std_20)

add_library(StonePotatoesPlatform STATIC
    platform/Platform.cpp
    platform/Window.cpp
    platform/Input.cpp
)
target_include_directories(StonePotatoesPlatform PUBLIC ${PROJECT_SOURCE_DIR}/src)
target_link_libraries(StonePotatoesPlatform PUBLIC StonePotatoesCore glfw)
target_compile_features(StonePotatoesPlatform PUBLIC cxx_std_20)

if(STONEPOTATOES_ENABLE_PROFILING)
    target_compile_definitions(StonePotatoesCore PRIVATE STONEPOTATOES_PROFILING_ENABLED)
endif()

if(STONEPOTATOES_ENABLE_ASSERTIONS)
    target_compile_definitions(StonePotatoesCore PRIVATE STONEPOTATOES_ASSERTIONS_ENABLED)
endif()
