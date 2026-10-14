cmake_minimum_required(VERSION 3.20)
project(StonePotatoesInstallConsumer LANGUAGES CXX)

find_package(StonePotatoes CONFIG REQUIRED)

add_executable(StonePotatoesConsumer main.cpp)
target_link_libraries(StonePotatoesConsumer PRIVATE StonePotatoes::ECS StonePotatoes::Game)
target_compile_features(StonePotatoesConsumer PRIVATE cxx_std_20)
