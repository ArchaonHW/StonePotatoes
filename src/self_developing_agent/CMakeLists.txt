# Self-Developing Agent library
add_library(StonePotatoesSelfDevelopingAgent STATIC MetaProgrammingAgent.cpp)

target_include_directories(StonePotatoesSelfDevelopingAgent
    PUBLIC
        ${CMAKE_CURRENT_SOURCE_DIR}
)

target_link_libraries(StonePotatoesSelfDevelopingAgent
    PUBLIC
        StonePotatoesCore
)
