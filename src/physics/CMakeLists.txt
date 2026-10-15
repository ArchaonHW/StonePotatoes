add_library(StonePotatoesPhysics STATIC Physics2D.cpp)
target_include_directories(StonePotatoesPhysics
    PUBLIC
        $<BUILD_INTERFACE:${PROJECT_SOURCE_DIR}/src>
        $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}>
        $<INSTALL_INTERFACE:${CMAKE_INSTALL_INCLUDEDIR}/StonePotatoes>
)
target_link_libraries(StonePotatoesPhysics
    PUBLIC
        StonePotatoesCore
        StonePotatoesECS
        StonePotatoesMath
)
target_compile_features(StonePotatoesPhysics PUBLIC cxx_std_20)
