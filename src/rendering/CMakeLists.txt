# Rendering library - NOT IMPLEMENTED YET
# TODO: Implement in Phase 3
add_library(StonePotatoesRendering STATIC stub.cpp)
target_include_directories(StonePotatoesRendering PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})
target_link_libraries(StonePotatoesRendering PUBLIC StonePotatoesCore StonePotatoesECS)
