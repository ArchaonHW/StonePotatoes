# Python API library - NOT IMPLEMENTED YET
# TODO: Implement in Phase 1
add_library(StonePotatoesAPI_Python STATIC stub.cpp)
target_include_directories(StonePotatoesAPI_Python PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})
target_link_libraries(StonePotatoesAPI_Python PUBLIC StonePotatoesCore StonePotatoesECS)
