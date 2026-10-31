# Civilization library - economic, population, culture, diplomacy, politics, and military slices
# 文明函式庫 — 經濟、人口、文化、外交、政治與軍事模型切片（Phase 7 進行中）
add_library(StonePotatoesCivilization STATIC
    economic/SystemDynamics.cpp
    economic/MarketPrice.cpp
    economic/FiscalBudget.cpp
    economic/TradeNetwork.cpp
    economic/MarketSettlement.cpp
    economic/CurrencySupply.cpp
    population/PopulationDynamics.cpp
    population/PopulationCalibration.cpp
    culture/CultureDiffusion.cpp
    culture/CultureFormation.cpp
    military/MilitaryLogistics.cpp
    military/BattleResolution.cpp
    diplomacy/Diplomacy.cpp
    diplomacy/TreatyLedger.cpp
    politics/PoliticalLegitimacy.cpp
    politics/PluralityElection.cpp
    politics/LeadershipSuccession.cpp
    politics/ElectionTerm.cpp
)
target_include_directories(StonePotatoesCivilization
    PUBLIC
        $<BUILD_INTERFACE:${PROJECT_SOURCE_DIR}/src>
        $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}>
        $<INSTALL_INTERFACE:${CMAKE_INSTALL_INCLUDEDIR}/StonePotatoes>
)
target_compile_features(StonePotatoesCivilization PUBLIC cxx_std_20)
