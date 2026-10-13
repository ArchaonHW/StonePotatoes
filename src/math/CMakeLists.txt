# Math library - Riemann ζ numerical core + procedural/util tools
# (RH core ported from PotatoEngine MathUtils/RiemannZeta.h;
#  Pathfinding/QuasiRandom/CurlNoise ported from PotatoEngine MathUtils)
# Math 函式庫 — 黎曼 ζ 數值核心 + 程序化/工具
# （RH 核心移植自 PotatoEngine MathUtils/RiemannZeta.h；
#  Pathfinding/QuasiRandom/CurlNoise 移植自 PotatoEngine MathUtils）
add_library(StonePotatoesMath STATIC
    RiemannZeta.cpp
    ZetaSpectral.cpp
    HighPrecisionZ.cpp
)

target_include_directories(StonePotatoesMath
    PUBLIC
        ${CMAKE_CURRENT_SOURCE_DIR}
)
