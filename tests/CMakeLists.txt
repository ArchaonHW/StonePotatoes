# Tests | 測試
enable_testing()

add_executable(StonePotatoesTests unit/test_phase1.cpp)
target_include_directories(StonePotatoesTests PRIVATE ${CMAKE_SOURCE_DIR}/src)
target_link_libraries(StonePotatoesTests
    PRIVATE
        StonePotatoesCore
        StonePotatoesECS
        StonePotatoesMath
        StonePotatoesIQGPU
        Threads::Threads
)

add_test(NAME phase1_unit COMMAND StonePotatoesTests)

# Math + IQGPU module tests | 數學 + IQGPU 模組測試
add_executable(StonePotatoesMathTests unit/test_math_iqgpu.cpp)
target_include_directories(StonePotatoesMathTests PRIVATE ${CMAKE_SOURCE_DIR}/src)
target_link_libraries(StonePotatoesMathTests
    PRIVATE
        StonePotatoesCore
        StonePotatoesMath
        StonePotatoesGPU
        StonePotatoesIQGPU
        Threads::Threads
)

add_test(NAME math_iqgpu_unit COMMAND StonePotatoesMathTests)

# NPC vertical-slice tests | NPC 垂直切片測試
add_executable(StonePotatoesNpcTests unit/test_npc.cpp)
target_include_directories(StonePotatoesNpcTests PRIVATE ${CMAKE_SOURCE_DIR}/src)
target_link_libraries(StonePotatoesNpcTests
    PRIVATE
        StonePotatoesCore
        StonePotatoesECS
        StonePotatoesAI
        Threads::Threads
)

add_test(NAME npc_unit COMMAND StonePotatoesNpcTests)
