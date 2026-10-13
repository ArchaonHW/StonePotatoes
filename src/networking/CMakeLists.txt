# Networking library - lockstep sync (partial; TODO: Phase 9 full implementation)
# 網路函式庫 — lockstep 同步（部分；完整實作於 Phase 9）
add_library(StonePotatoesNetworking STATIC
    core/LockstepSync.cpp
    stub.cpp
)
target_include_directories(StonePotatoesNetworking PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})
target_link_libraries(StonePotatoesNetworking PUBLIC StonePotatoesCore StonePotatoesECS StonePotatoesWorld StonePotatoesCivilization)
