# Civilization library - NOT IMPLEMENTED YET
# TODO: Implement in Phase 7
add_library(StonePotatoesCivilization STATIC stub.cpp)
target_include_directories(StonePotatoesCivilization PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})
target_link_libraries(StonePotatoesCivilization PUBLIC StonePotatoesCore StonePotatoesECS StonePotatoesWorld)
