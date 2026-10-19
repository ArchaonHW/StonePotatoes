# Networking library - deterministic lockstep command-synchronization core;
# transport, reliability, and reconnect policy are provided by the host.
# 網路函式庫 — 決定性 lockstep 指令同步核心；傳輸、可靠性與重連策略由宿主提供。
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
