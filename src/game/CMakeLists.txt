# Game library - NOT IMPLEMENTED YET
# TODO: Implement in Phase 1
add_library(StonePotatoesGame STATIC stub.cpp)
target_include_directories(StonePotatoesGame PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})
target_link_libraries(StonePotatoesGame PUBLIC StonePotatoesCore StonePotatoesECS StonePotatoesRendering StonePotatoesPhysics StonePotatoesAI)
