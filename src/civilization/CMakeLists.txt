# Civilization library - economic systems (partial; TODO: Phase 7 full implementation)
# 文明函式庫 — 經濟系統（部分；完整實作於 Phase 7）
add_library(StonePotatoesCivilization STATIC
    economic/SystemDynamics.cpp
)
target_include_directories(StonePotatoesCivilization
    PUBLIC
        $<BUILD_INTERFACE:${PROJECT_SOURCE_DIR}/src>
        $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}>
        $<INSTALL_INTERFACE:${CMAKE_INSTALL_INCLUDEDIR}/StonePotatoes>
)
target_compile_features(StonePotatoesCivilization PUBLIC cxx_std_20)
