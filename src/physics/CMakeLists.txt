# Physics library - NOT IMPLEMENTED YET
# TODO: Implement in Phase 4
add_library(StonePotatoesPhysics STATIC stub.cpp)
target_include_directories(StonePotatoesPhysics PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})
target_link_libraries(StonePotatoesPhysics PUBLIC StonePotatoesCore StonePotatoesECS)
