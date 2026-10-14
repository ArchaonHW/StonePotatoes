# Editor library - NOT IMPLEMENTED YET
# TODO: Implement in Phase 10
add_library(StonePotatoesEditor STATIC stub.cpp)
target_include_directories(StonePotatoesEditor PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})
target_link_libraries(StonePotatoesEditor PUBLIC StonePotatoesCore StonePotatoesECS StonePotatoesRendering StonePotatoesPhysics StonePotatoesAI StonePotatoesWorld StonePotatoesCivilization StonePotatoesGame)
