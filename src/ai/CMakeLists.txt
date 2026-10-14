# AI library — MVP NPC vertical slice (perception/decision/action).
# AI 函式庫 — MVP NPC 垂直切片（感知／決策／行動）。
add_library(StonePotatoesAI STATIC NPC.cpp)
target_include_directories(StonePotatoesAI PUBLIC
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}>
    $<BUILD_INTERFACE:${PROJECT_SOURCE_DIR}/src>
    $<INSTALL_INTERFACE:${CMAKE_INSTALL_INCLUDEDIR}/StonePotatoes>
)
target_link_libraries(StonePotatoesAI PUBLIC StonePotatoesCore StonePotatoesECS)
target_compile_features(StonePotatoesAI PUBLIC cxx_std_20)
