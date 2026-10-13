# Lua API library - NOT IMPLEMENTED YET
# TODO: Implement in Phase 1
add_library(StonePotatoesAPI_Lua STATIC stub.cpp)
target_include_directories(StonePotatoesAPI_Lua PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})
target_link_libraries(StonePotatoesAPI_Lua PUBLIC StonePotatoesCore StonePotatoesECS)
