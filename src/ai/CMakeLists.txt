# AI library - NOT IMPLEMENTED YET
# TODO: Implement in Phase 5
add_library(StonePotatoesAI STATIC stub.cpp)
target_include_directories(StonePotatoesAI PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})
target_link_libraries(StonePotatoesAI PUBLIC StonePotatoesCore StonePotatoesECS)
