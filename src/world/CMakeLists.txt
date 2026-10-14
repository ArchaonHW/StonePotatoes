# World library — procedural generation (more subsystems planned)
# World 函式庫 — 程序化生成（其他子系統：Phase 6）
add_library(StonePotatoesWorld STATIC
    procedural/ResourceScatter.cpp
    procedural/WorldStreamer.cpp
)
target_include_directories(StonePotatoesWorld PUBLIC
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}>
    $<BUILD_INTERFACE:${PROJECT_SOURCE_DIR}/src>
    $<INSTALL_INTERFACE:${CMAKE_INSTALL_INCLUDEDIR}/StonePotatoes>
)
target_link_libraries(StonePotatoesWorld PUBLIC StonePotatoesCore StonePotatoesECS StonePotatoesMath StonePotatoesAI)
target_compile_features(StonePotatoesWorld PUBLIC cxx_std_20)
