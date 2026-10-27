# Civilization library - economic, population, culture, diplomacy, and military slices
# 文明函式庫 — 經濟、人口、文化、外交與軍事模型切片（Phase 7 進行中）
add_library(StonePotatoesCivilization STATIC
    economic/SystemDynamics.cpp
    economic/MarketPrice.cpp
    economic/FiscalBudget.cpp
    economic/TradeNetwork.cpp
    population/PopulationDynamics.cpp
    culture/CultureDiffusion.cpp
    military/MilitaryLogistics.cpp
    military/BattleResolution.cpp
    diplomacy/Diplomacy.cpp
)
target_include_directories(StonePotatoesCivilization
    PUBLIC
        $<BUILD_INTERFACE:${PROJECT_SOURCE_DIR}/src>
        $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}>
        $<INSTALL_INTERFACE:${CMAKE_INSTALL_INCLUDEDIR}/StonePotatoes>
)
target_compile_features(StonePotatoesCivilization PUBLIC cxx_std_20)
