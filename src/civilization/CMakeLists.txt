# Civilization library - economic systems (partial; TODO: Phase 7 full implementation)
# 文明函式庫 — 經濟系統（部分；完整實作於 Phase 7）
add_library(StonePotatoesCivilization STATIC
    economic/SystemDynamics.cpp
    stub.cpp
)
target_include_directories(StonePotatoesCivilization PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})
target_link_libraries(StonePotatoesCivilization PUBLIC StonePotatoesCore StonePotatoesECS StonePotatoesWorld)
