# Agent library - NOT IMPLEMENTED YET
# TODO: Implement in Phase 5-7
add_library(StonePotatoesAgent STATIC stub.cpp)
target_include_directories(StonePotatoesAgent PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})
target_link_libraries(StonePotatoesAgent PUBLIC StonePotatoesCore StonePotatoesECS StonePotatoesAI StonePotatoesCivilization)
