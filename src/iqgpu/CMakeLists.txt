# IQGPU library - unified compute abstraction + quantum-inspired algorithms
# IQGPU 函式庫 — 統一計算抽象 + 量子啟發演算法
add_library(StonePotatoesIQGPU STATIC
    IQGPURuntime.cpp
    QuantumInspired.cpp
    KernelOptimizer.cpp
    ZetaCompute.cpp
    ZetaVerificationJob.cpp
)

target_include_directories(StonePotatoesIQGPU
    PUBLIC
        ${CMAKE_CURRENT_SOURCE_DIR}
)

target_link_libraries(StonePotatoesIQGPU
    PUBLIC
        StonePotatoesCore
        StonePotatoesGPU
        StonePotatoesMath
)

# RH large-scale finite-verification CLI | RH 大規模有限驗證命令列工具
add_executable(StonePotatoesRHVerify ${CMAKE_SOURCE_DIR}/tools/benchmark/rh_verify.cpp)
target_include_directories(StonePotatoesRHVerify PRIVATE ${CMAKE_SOURCE_DIR}/src)
target_link_libraries(StonePotatoesRHVerify PRIVATE StonePotatoesIQGPU Threads::Threads)
