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
        StonePotatoesWorld
        Threads::Threads
)

add_test(NAME math_iqgpu_unit COMMAND StonePotatoesMathTests)

# RH CLI smoke: golden self-test + Turing-certified verification to g_20000
# RH 命令列冒煙測試：黃金自檢 + Turing 認證驗證至 g_20000
add_test(NAME rh_verify_cli COMMAND StonePotatoesRHVerify 20000 8 --selftest)

# NPC vertical-slice tests | NPC 垂直切片測試
add_executable(StonePotatoesNpcTests unit/test_npc.cpp)
target_include_directories(StonePotatoesNpcTests PRIVATE ${CMAKE_SOURCE_DIR}/src)
target_link_libraries(StonePotatoesNpcTests
    PRIVATE
        StonePotatoesCore
        StonePotatoesECS
        StonePotatoesAI
        StonePotatoesMath
        StonePotatoesWorld
        Threads::Threads
)

add_test(NAME npc_unit COMMAND StonePotatoesNpcTests)

# Parallel system scheduler tests | 平行系統排程測試
add_executable(StonePotatoesParallelTests unit/test_parallel.cpp)
target_include_directories(StonePotatoesParallelTests PRIVATE ${CMAKE_SOURCE_DIR}/src)
target_link_libraries(StonePotatoesParallelTests
    PRIVATE
        StonePotatoesCore
        StonePotatoesECS
        StonePotatoesGPU
        Threads::Threads
)

add_test(NAME parallel_unit COMMAND StonePotatoesParallelTests)

# Vulkan backend tests (skips cleanly without a loader/device)
# Vulkan 後端測試（無 loader/裝置時乾淨跳過）
add_executable(StonePotatoesVulkanTests unit/test_vulkan.cpp)
target_include_directories(StonePotatoesVulkanTests PRIVATE ${CMAKE_SOURCE_DIR}/src)
target_link_libraries(StonePotatoesVulkanTests
    PRIVATE
        StonePotatoesCore
        StonePotatoesGPU
        StonePotatoesIQGPU
        Threads::Threads
)

add_test(NAME vulkan_unit COMMAND StonePotatoesVulkanTests)
# No GPU / no loader → exit 77 → ctest reports Skipped (not Failed/Passed).
# 無 GPU / loader 時回傳 77，ctest 顯示為 Skipped。
set_tests_properties(vulkan_unit PROPERTIES SKIP_RETURN_CODE 77)
