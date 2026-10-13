# C++ API library - NOT IMPLEMENTED YET
# TODO: Implement in Phase 1
add_library(StonePotatoesAPI_CPP STATIC stub.cpp)
target_include_directories(StonePotatoesAPI_CPP PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})
target_link_libraries(StonePotatoesAPI_CPP PUBLIC StonePotatoesCore StonePotatoesECS StonePotatoesRendering StonePotatoesPhysics StonePotatoesAI StonePotatoesWorld StonePotatoesCivilization StonePotatoesGame)
