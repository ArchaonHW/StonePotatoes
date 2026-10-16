#include "editor/EditorSession.h"

#include <filesystem>
#include <string>

#define CHECK(condition) do { if (!(condition)) return __LINE__; } while (false)

int main() {
    StonePotatoes::ECS::World world;
    world.Initialize();
    StonePotatoes::Editor::RegisterEditorComponents(world);
    StonePotatoes::Editor::EditorSession session(world);

    CHECK(session.Entities().empty());
    CHECK(session.SelectedEntity() == StonePotatoes::ECS::INVALID_ENTITY);

    const auto first = session.CreateEntity("Player");
    const auto second = session.CreateEntity("Enemy");
    CHECK(session.Entities().size() == 2);
    CHECK(world.IsEntityValid(first));
    CHECK(world.IsEntityValid(second));
    CHECK(session.SelectedEntity() == second);
    CHECK(session.GetEntityLabel(first) == "Player");
    CHECK(session.GetEntityLabel(second) == "Enemy");
    CHECK(!session.SetEntityLabel(first, ""));
    CHECK(session.GetEntityLabel(first) == "Player");

    const std::string longLabel(64, 'x');
    CHECK(!session.SetEntityLabel(first, longLabel));
    CHECK(session.SetEntityLabel(first, "Hero"));
    CHECK(session.GetEntityLabel(first) == "Hero");
    CHECK(session.SetEntityLabel(first, "Hero Prime"));
    CHECK(session.GetEntityLabel(first) == "Hero Prime");

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

    const auto path = std::filesystem::temp_directory_path() /
                      "stonepotatoes-editor-scene-roundtrip.bin";
    std::error_code error;
    std::filesystem::remove(path, error);
    {
        StonePotatoes::ECS::World source;
        source.Initialize();
        StonePotatoes::Editor::RegisterEditorComponents(source);
        StonePotatoes::Editor::EditorSession sourceSession(source);
        sourceSession.CreateEntity("Saved player");
        sourceSession.CreateEntity("Saved enemy");
        CHECK(source.SaveToFile(path.string().c_str()));
        source.Shutdown();
    }
    {
        StonePotatoes::ECS::World loaded;
        loaded.Initialize();
        StonePotatoes::Editor::RegisterEditorComponents(loaded);
        CHECK(loaded.LoadFromFile(path.string().c_str()));
        StonePotatoes::Editor::EditorSession loadedSession(loaded);
        CHECK(loadedSession.Entities().size() == 2);
        CHECK(loadedSession.GetEntityLabel(loadedSession.Entities()[0]) ==
              "Saved player");
        CHECK(loadedSession.GetEntityLabel(loadedSession.Entities()[1]) ==
              "Saved enemy");
        loaded.Shutdown();
    }
    std::filesystem::remove(path, error);
    return 0;
}
