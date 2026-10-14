# Stable installed C++ entry point for headless engine applications.
add_library(StonePotatoesAPI_CPP INTERFACE)
set_target_properties(StonePotatoesAPI_CPP PROPERTIES EXPORT_NAME CppAPI)
target_include_directories(StonePotatoesAPI_CPP INTERFACE
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}>
    $<BUILD_INTERFACE:${PROJECT_SOURCE_DIR}/src>
    $<INSTALL_INTERFACE:${CMAKE_INSTALL_INCLUDEDIR}/StonePotatoes>
)
target_link_libraries(StonePotatoesAPI_CPP INTERFACE
    StonePotatoesECS
    StonePotatoesGame
)
target_compile_features(StonePotatoesAPI_CPP INTERFACE cxx_std_20)
