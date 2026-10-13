# Data library - NOT IMPLEMENTED YET
# TODO: Implement in Phase 1-2
add_library(StonePotatoesData STATIC stub.cpp)
target_include_directories(StonePotatoesData PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})
target_link_libraries(StonePotatoesData PUBLIC StonePotatoesCore StonePotatoesECS)
