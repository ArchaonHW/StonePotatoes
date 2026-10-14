# Tests | 測試
enable_testing()

add_executable(StonePotatoesTests unit/test_phase1.cpp)
target_include_directories(StonePotatoesTests PRIVATE ${CMAKE_SOURCE_DIR}/src)
target_link_libraries(StonePotatoesTests
    PRIVATE
        StonePotatoesCore
        StonePotatoesPlatform
        StonePotatoesECS
        StonePotatoesData
        StonePotatoesMath
        StonePotatoesIQGPU
        Threads::Threads
)

add_test(NAME phase1_unit COMMAND StonePotatoesTests)

add_executable(StonePotatoesGameTests unit/test_game.cpp)
target_include_directories(StonePotatoesGameTests PRIVATE ${CMAKE_SOURCE_DIR}/src)
target_link_libraries(StonePotatoesGameTests PRIVATE StonePotatoesGame)
add_test(NAME game_lifecycle_unit COMMAND StonePotatoesGameTests)

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
# No GPU / no loader → exit 77 → ctest reports Skipped (not Failed/Passed).
# 無 GPU / loader 時回傳 77，ctest 顯示為 Skipped。
set_tests_properties(vulkan_unit PROPERTIES SKIP_RETURN_CODE 77)

# Exercise the integrated Vulkan renderer and compute path without a physical
# display. CI provides Xvfb and Mesa's software Vulkan driver.
if(Vulkan_FOUND AND TARGET StonePotatoesVulkanComputeRendererDemo AND
   CMAKE_SYSTEM_NAME STREQUAL "Linux")
    find_program(STONEPOTATOES_XVFB_RUN xvfb-run)
    if(STONEPOTATOES_XVFB_RUN)
        add_test(NAME vulkan_compute_renderer_runtime
            COMMAND ${STONEPOTATOES_XVFB_RUN} -a
                $<TARGET_FILE:StonePotatoesVulkanComputeRendererDemo>
                --frames 3
        )
        set_tests_properties(vulkan_compute_renderer_runtime PROPERTIES
            TIMEOUT 90
        )
    endif()
endif()

# Install the headless public package, then build and run a separate consumer
# that creates, updates, saves, and reloads an ECS world.
if(CMAKE_CONFIGURATION_TYPES)
    set(_sp_multi_config TRUE)
else()
    set(_sp_multi_config FALSE)
endif()
set(_sp_consumer_linker_flags "")
if(STONEPOTATOES_ENABLE_COVERAGE AND CMAKE_CXX_COMPILER_ID MATCHES "GNU|Clang")
    # The separately configured consumer must link the gcov runtime required
    # by the instrumented static libraries installed from this build.
    set(_sp_consumer_linker_flags "--coverage")
endif()
add_test(NAME install_consumer
    COMMAND ${CMAKE_COMMAND}
        -DSP_SOURCE_DIR=${CMAKE_SOURCE_DIR}
        -DSP_BUILD_DIR=${CMAKE_BINARY_DIR}
        -DSP_CONSUMER_SOURCE=${CMAKE_CURRENT_SOURCE_DIR}/install_consumer
        -DSP_GENERATOR=${CMAKE_GENERATOR}
        -DSP_MULTI_CONFIG=${_sp_multi_config}
        -DSP_CONSUMER_LINKER_FLAGS=${_sp_consumer_linker_flags}
        -DSP_CONFIG=$<CONFIG>
        -P ${CMAKE_CURRENT_SOURCE_DIR}/install_consumer/run.cmake
)

# Plugin system tests (incl. a real shared-library plugin) | 外掛系統測試（含實際共享函式庫外掛）
add_library(StonePotatoesTestPlugin MODULE unit/test_plugin_module.cpp)
target_include_directories(StonePotatoesTestPlugin PRIVATE ${CMAKE_SOURCE_DIR}/src)
add_executable(StonePotatoesPluginTests unit/test_plugin.cpp)
target_include_directories(StonePotatoesPluginTests PRIVATE ${CMAKE_SOURCE_DIR}/src)
target_link_libraries(StonePotatoesPluginTests PRIVATE StonePotatoesCore)
target_compile_definitions(StonePotatoesPluginTests PRIVATE
    SP_TEST_PLUGIN_PATH="$<TARGET_FILE:StonePotatoesTestPlugin>")
add_dependencies(StonePotatoesPluginTests StonePotatoesTestPlugin)
add_test(NAME plugin_unit COMMAND StonePotatoesPluginTests)
