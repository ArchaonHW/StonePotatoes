# GPU library - NOT IMPLEMENTED YET
# TODO: Implement in Phase 8
add_library(StonePotatoesGPU STATIC stub.cpp)
target_include_directories(StonePotatoesGPU PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})
target_link_libraries(StonePotatoesGPU PUBLIC StonePotatoesCore StonePotatoesECS StonePotatoesAI StonePotatoesPhysics)
