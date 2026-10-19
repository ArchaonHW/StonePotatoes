# Civilization library - economic and population model slices (Phase 7 in progress)
# 文明函式庫 — 經濟與人口模型切片（Phase 7 進行中）
add_library(StonePotatoesCivilization STATIC
    economic/SystemDynamics.cpp
    economic/MarketPrice.cpp
    economic/TradeNetwork.cpp
    population/PopulationDynamics.cpp
)
target_include_directories(StonePotatoesCivilization
    PUBLIC
        $<BUILD_INTERFACE:${PROJECT_SOURCE_DIR}/src>
        $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}>
        $<INSTALL_INTERFACE:${CMAKE_INSTALL_INCLUDEDIR}/StonePotatoes>
)
target_compile_features(StonePotatoesCivilization PUBLIC cxx_std_20)
