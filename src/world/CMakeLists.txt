# World library - NOT IMPLEMENTED YET
# TODO: Implement in Phase 6
add_library(StonePotatoesWorld STATIC stub.cpp)
target_include_directories(StonePotatoesWorld PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})
target_link_libraries(StonePotatoesWorld PUBLIC StonePotatoesCore StonePotatoesECS)
