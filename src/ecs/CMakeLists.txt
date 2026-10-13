# ECS library - Entity-Component-System
add_library(StonePotatoesECS STATIC entity/Entity.cpp entity/EntityManager.cpp component/Component.cpp component/ComponentManager.cpp archetype/Archetype.cpp archetype/ArchetypeManager.cpp system/System.cpp system/SystemScheduler.cpp query/Query.cpp events/Event.cpp events/EventBus.cpp resources/Resource.cpp resources/ResourceManager.cpp World.cpp)

target_include_directories(StonePotatoesECS
    PUBLIC
        ${CMAKE_CURRENT_SOURCE_DIR}
)

target_link_libraries(StonePotatoesECS
    PUBLIC
        StonePotatoesCore
)
