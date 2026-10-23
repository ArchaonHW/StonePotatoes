# Examples - NOT IMPLEMENTED YET
# TODO: Implement in Phase 1
# add_subdirectory(hello_world)
# add_subdirectory(basic_rendering)
# add_subdirectory(physics_demo)
# add_subdirectory(ai_demo)
# add_subdirectory(world_demo)
# add_subdirectory(civilization_demo)
# add_subdirectory(multiplayer_demo)
# add_subdirectory(full_game)

# World visualization demo | 世界可視化 demo
add_executable(StonePotatoesVisDemo visualization_demo.cpp)
target_include_directories(StonePotatoesVisDemo PRIVATE ${CMAKE_SOURCE_DIR}/src)
target_link_libraries(StonePotatoesVisDemo
    PRIVATE
        StonePotatoesCore
        StonePotatoesPlatform
        StonePotatoesECS
        StonePotatoesAI
        StonePotatoesRendering
        StonePotatoesWorld
        Threads::Threads
        glfw
        imgui
)

# Game-engine-backed visualization of Hardy Z and sign-change zero candidates.
# This is an exploratory view; rigorous finite verification remains in
# StonePotatoesRHVerify and neither tool claims a proof of the Riemann Hypothesis.
add_executable(StonePotatoesRiemannExplorer riemann_explorer.cpp)
target_include_directories(StonePotatoesRiemannExplorer PRIVATE ${CMAKE_SOURCE_DIR}/src)
target_link_libraries(StonePotatoesRiemannExplorer
    PRIVATE
        StonePotatoesPlatform
        StonePotatoesRendering
        StonePotatoesMath
)
