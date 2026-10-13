# Math library - Riemann ζ numerical core + zero-spectrum engine tools
# (ported from PotatoEngine MathUtils/RiemannZeta.h RH verification work)
# Math 函式庫 — 黎曼 ζ 數值核心 + 零點頻譜引擎工具
# （移植自 PotatoEngine MathUtils/RiemannZeta.h 的 RH 驗證成果）
add_library(StonePotatoesMath STATIC
    RiemannZeta.cpp
    ZetaSpectral.cpp
)

target_include_directories(StonePotatoesMath
    PUBLIC
        ${CMAKE_CURRENT_SOURCE_DIR}
)
