# World library — procedural generation (more subsystems planned)
# World 函式庫 — 程序化生成（其他子系統：Phase 6）
add_library(StonePotatoesWorld STATIC
    procedural/ResourceScatter.cpp
    procedural/WorldStreamer.cpp
)
target_include_directories(StonePotatoesWorld PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})
target_link_libraries(StonePotatoesWorld PUBLIC StonePotatoesCore StonePotatoesECS StonePotatoesMath StonePotatoesAI)
