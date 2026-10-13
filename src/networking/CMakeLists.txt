# Networking library - NOT IMPLEMENTED YET
# TODO: Implement in Phase 9
add_library(StonePotatoesNetworking STATIC stub.cpp)
target_include_directories(StonePotatoesNetworking PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})
target_link_libraries(StonePotatoesNetworking PUBLIC StonePotatoesCore StonePotatoesECS StonePotatoesWorld StonePotatoesCivilization)
