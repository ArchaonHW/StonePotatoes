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

# Scale tests: spatial grid, command buffer, 10k-agent stress
# 規模測試：空間格、指令緩衝區、一萬代理壓力
add_executable(StonePotatoesScaleTests unit/test_scale.cpp)
target_include_directories(StonePotatoesScaleTests PRIVATE ${CMAKE_SOURCE_DIR}/src)
target_link_libraries(StonePotatoesScaleTests
    PRIVATE
        StonePotatoesCore
        StonePotatoesECS
        StonePotatoesAI
        StonePotatoesGPU
        Threads::Threads
)

add_test(NAME scale_unit COMMAND StonePotatoesScaleTests)

# Ported PotatoEngine utilities tests
# 移植自 PotatoEngine 的工具測試
add_executable(StonePotatoesPortedUtilsTests unit/test_ported_utils.cpp)
target_include_directories(StonePotatoesPortedUtilsTests PRIVATE ${CMAKE_SOURCE_DIR}/src)
target_link_libraries(StonePotatoesPortedUtilsTests
    PRIVATE
        StonePotatoesCore
        StonePotatoesMath
        StonePotatoesCivilization
        StonePotatoesNetworking
        Threads::Threads
)

add_test(NAME ported_utils_unit COMMAND StonePotatoesPortedUtilsTests)
