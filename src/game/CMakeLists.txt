# Headless game/scene lifecycle, independent of the platform window layer.
add_library(StonePotatoesGame STATIC Game.cpp)
target_include_directories(StonePotatoesGame PUBLIC
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}>
    $<BUILD_INTERFACE:${PROJECT_SOURCE_DIR}/src>
    $<INSTALL_INTERFACE:${CMAKE_INSTALL_INCLUDEDIR}/StonePotatoes>
)
target_link_libraries(StonePotatoesGame PUBLIC StonePotatoesECS)
target_compile_features(StonePotatoesGame PUBLIC cxx_std_20)
