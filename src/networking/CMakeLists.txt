# Networking library - deterministic lockstep command synchronization (partial)
# 網路函式庫 — 決定性 lockstep 指令同步（部分）
add_library(StonePotatoesNetworking STATIC
    core/LockstepSync.cpp
)
target_include_directories(StonePotatoesNetworking
    PUBLIC
        $<BUILD_INTERFACE:${PROJECT_SOURCE_DIR}/src>
        $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}>
        $<INSTALL_INTERFACE:${CMAKE_INSTALL_INCLUDEDIR}/StonePotatoes>
)
target_compile_features(StonePotatoesNetworking PUBLIC cxx_std_20)
