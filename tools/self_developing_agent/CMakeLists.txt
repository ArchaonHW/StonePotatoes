add_executable(StonePotatoesSelfDevelopingAgent
    main.cpp
    MetaProgrammingAgent.cpp
)
target_include_directories(StonePotatoesSelfDevelopingAgent PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}
    ${PROJECT_SOURCE_DIR}/src
)
target_link_libraries(StonePotatoesSelfDevelopingAgent PRIVATE StonePotatoesCore)
target_compile_features(StonePotatoesSelfDevelopingAgent PRIVATE cxx_std_20)
