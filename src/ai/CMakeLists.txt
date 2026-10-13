# AI library — MVP NPC vertical slice (perception/decision/action).
# AI 函式庫 — MVP NPC 垂直切片（感知／決策／行動）。
add_library(StonePotatoesAI STATIC NPC.cpp)
target_include_directories(StonePotatoesAI PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})
target_link_libraries(StonePotatoesAI PUBLIC StonePotatoesCore StonePotatoesECS)
