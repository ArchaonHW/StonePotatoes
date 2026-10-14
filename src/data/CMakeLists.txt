add_library(StonePotatoesData STATIC AssetCache.cpp)
set_target_properties(StonePotatoesData PROPERTIES EXPORT_NAME Data)
target_include_directories(StonePotatoesData
    PUBLIC
        $<BUILD_INTERFACE:${PROJECT_SOURCE_DIR}/src>
        $<INSTALL_INTERFACE:${CMAKE_INSTALL_INCLUDEDIR}/StonePotatoes>
)
target_link_libraries(StonePotatoesData PUBLIC StonePotatoesCore)
target_compile_features(StonePotatoesData PUBLIC cxx_std_20)
