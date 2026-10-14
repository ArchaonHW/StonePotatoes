#include <ecs/World.h>

#include <cmath>
#include <memory>

struct Position {
    float x = 0.0f;
};

struct Velocity {
    float x = 0.0f;
};

class MovementSystem final : public StonePotatoes::ECS::System {
public:
    void Update(StonePotatoes::ECS::World* world, float deltaTime) override {
        world->ForEach<Position, Velocity>(
            [deltaTime](StonePotatoes::ECS::Entity, Position& position,
                        Velocity& velocity) {
                position.x += velocity.x * deltaTime;
            });
    }
};

int main(int argc, char** argv) {
    if (argc != 2) return 1;

    StonePotatoes::ECS::Entity savedEntity;
    {
        StonePotatoes::ECS::World world;
        world.RegisterComponent<Position>("Position");
        world.RegisterComponent<Velocity>("Velocity");
        world.GetSystemScheduler().RegisterSystem(
            std::make_unique<MovementSystem>());
        world.Initialize();

        savedEntity = world.CreateEntity();
        world.AddComponent(savedEntity, Position{1.0f});
        world.AddComponent(savedEntity, Velocity{2.0f});
        world.Update(0.5f);

        const auto* position = world.GetComponent<Position>(savedEntity);
        if (!position || std::fabs(position->x - 2.0f) > 0.0001f) return 2;
        if (!world.SaveToFile(argv[1])) return 3;
    }

    StonePotatoes::ECS::World restored;
    restored.RegisterComponent<Position>("Position");
    restored.RegisterComponent<Velocity>("Velocity");
    if (!restored.LoadFromFile(argv[1])) return 4;

    const auto* position = restored.GetComponent<Position>(savedEntity);
    const auto* velocity = restored.GetComponent<Velocity>(savedEntity);
    if (!position || !velocity) return 5;
    if (std::fabs(position->x - 2.0f) > 0.0001f ||
        std::fabs(velocity->x - 2.0f) > 0.0001f) {
        return 6;
    }
    return 0;
}
