# GPU library - compute abstraction (CPU reference backend implemented;
# real GPU backend is Planned)
# GPU 函式庫 — 計算抽象（CPU 參考後端已實作；真實 GPU 後端計畫中）
add_library(StonePotatoesGPU STATIC
    ComputeDevice.cpp
    ComputeBuffer.cpp
    CPUBackend.cpp
    ComputeGraph.cpp
    stub.cpp
)

target_include_directories(StonePotatoesGPU
    PUBLIC
        ${CMAKE_CURRENT_SOURCE_DIR}
)

target_link_libraries(StonePotatoesGPU
    PUBLIC
        StonePotatoesCore
        StonePotatoesECS
        StonePotatoesAI
        StonePotatoesPhysics
)
