# ECS library - Entity-Component-System
add_library(StonePotatoesECS STATIC entity/Entity.cpp entity/EntityManager.cpp component/Component.cpp component/ComponentManager.cpp archetype/Archetype.cpp archetype/ArchetypeManager.cpp system/System.cpp system/SystemScheduler.cpp query/Query.cpp events/Event.cpp events/EventBus.cpp resources/Resource.cpp resources/ResourceManager.cpp commands/CommandBuffer.cpp World.cpp)

target_include_directories(StonePotatoesECS
    PUBLIC
        $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}>
        $<BUILD_INTERFACE:${PROJECT_SOURCE_DIR}/src>
        $<INSTALL_INTERFACE:${CMAKE_INSTALL_INCLUDEDIR}/StonePotatoes>
)

target_link_libraries(StonePotatoesECS
    PUBLIC
        StonePotatoesCore
)
target_compile_features(StonePotatoesECS PUBLIC cxx_std_20)
