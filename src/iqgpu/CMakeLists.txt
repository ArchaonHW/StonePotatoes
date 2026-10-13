# IQGPU library - unified compute abstraction + quantum-inspired algorithms
# IQGPU 函式庫 — 統一計算抽象 + 量子啟發演算法
add_library(StonePotatoesIQGPU STATIC
    IQGPURuntime.cpp
    QuantumInspired.cpp
    KernelOptimizer.cpp
    ZetaCompute.cpp
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
