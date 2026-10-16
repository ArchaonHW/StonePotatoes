#include "editor/EditorSession.h"

#define CHECK(condition) do { if (!(condition)) return __LINE__; } while (false)

int main() {
    StonePotatoes::ECS::World world;
    world.Initialize();
    StonePotatoes::Editor::EditorSession session(world);

    CHECK(session.Entities().empty());
    CHECK(session.SelectedEntity() == StonePotatoes::ECS::INVALID_ENTITY);

    const auto first = session.CreateEntity();
    const auto second = session.CreateEntity();
    CHECK(session.Entities().size() == 2);
    CHECK(world.IsEntityValid(first));
    CHECK(world.IsEntityValid(second));
    CHECK(session.SelectedEntity() == second);

    CHECK(session.SelectEntity(first));
    CHECK(!session.SelectEntity(StonePotatoes::ECS::INVALID_ENTITY));
    CHECK(session.SelectedEntity() == first);

    CHECK(session.DeleteEntity(second));
    CHECK(!world.IsEntityValid(second));
    CHECK(session.Entities().size() == 1);
    CHECK(session.SelectedEntity() == first);
    CHECK(!session.DeleteEntity(second));

    CHECK(session.DeleteEntity(first));
    CHECK(session.SelectedEntity() == StonePotatoes::ECS::INVALID_ENTITY);
    CHECK(session.Entities().empty());

    const auto reused = session.CreateEntity();
    CHECK(reused.GetIndex() == first.GetIndex());
    CHECK(reused.GetGeneration() != first.GetGeneration());
    CHECK(!session.SelectEntity(first));
    CHECK(!session.DeleteEntity(first));
    CHECK(world.IsEntityValid(reused));

    world.Shutdown();
    return 0;
}
