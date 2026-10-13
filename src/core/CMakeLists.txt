# Core library - Foundation systems
add_library(StonePotatoesCore STATIC
    platform/Platform.cpp
    platform/Window.cpp
    platform/Input.cpp
    io/FileSystem.cpp
    memory/MemoryPool.cpp
    memory/StackAllocator.cpp
    memory/ArenaAllocator.cpp
    time/Time.cpp
    config/Config.cpp
    logging/Logger.cpp
    plugin/Plugin.cpp
)

target_include_directories(StonePotatoesCore
    PUBLIC
        ${CMAKE_CURRENT_SOURCE_DIR}
)

target_link_libraries(StonePotatoesCore
    PUBLIC
        Threads::Threads
        glfw
        ${CMAKE_DL_LIBS}  # dlopen for SharedLibraryPluginLoader | 外掛動態載入
)

if(STONEPOTATOES_ENABLE_PROFILING)
    target_compile_definitions(StonePotatoesCore PRIVATE STONEPOTATOES_PROFILING_ENABLED)
endif()

if(STONEPOTATOES_ENABLE_ASSERTIONS)
    target_compile_definitions(StonePotatoesCore PRIVATE STONEPOTATOES_ASSERTIONS_ENABLED)
endif()
