#include "EditorSession.h"
#include "../rendering/Renderer.h"

#include <imgui.h>

#include <charconv>
#include <cstdio>
#include <cstdint>
#include <iostream>
#include <string_view>

namespace {

bool ParseFrameLimit(int argc, char** argv, std::uint64_t& frameLimit) {
    frameLimit = UINT64_MAX;
    for (int i = 1; i < argc; ++i) {
        const std::string_view arg(argv[i]);
        if (arg == "--help" || arg == "-h") {
            std::cout << "StonePotatoesEditor [--frames N]\n"
                         "  --frames N  Exit cleanly after N rendered frames.\n";
            frameLimit = 0;
            return true;
        }
        if (arg != "--frames" || i + 1 >= argc) {
            return false;
        }

        const std::string_view value(argv[++i]);
        std::uint64_t parsed = 0;
        const auto result = std::from_chars(value.data(),
                                            value.data() + value.size(), parsed);
        if (result.ec != std::errc{} ||
            result.ptr != value.data() + value.size() || parsed == 0) {
            return false;
        }
        frameLimit = parsed;
    }
    return true;
}

void DrawEditor(StonePotatoes::Editor::EditorSession& session,
                StonePotatoes::ECS::World& world) {
    using StonePotatoes::ECS::Entity;

    if (ImGui::Begin("Entity Hierarchy")) {
        if (ImGui::Button("Create Entity")) {
            session.CreateEntity();
        }
        ImGui::Separator();

        const Entity selected = session.SelectedEntity();
        int row = 0;
        for (const Entity entity : session.Entities()) {
            ImGui::PushID(row++);
            char label[48];
            std::snprintf(label, sizeof(label), "Entity %u",
                          entity.GetIndex());
            if (ImGui::Selectable(label, entity == selected)) {
                session.SelectEntity(entity);
            }
            ImGui::PopID();
        }
        if (session.Entities().empty()) {
            ImGui::TextDisabled("No entities. Create one to begin.");
        }
    }
    ImGui::End();

    if (ImGui::Begin("Inspector")) {
        const Entity selected = session.SelectedEntity();
        if (world.IsEntityValid(selected)) {
            ImGui::Text("Entity identity");
            ImGui::Separator();
            ImGui::Text("Index: %u", selected.GetIndex());
            ImGui::Text("Generation: %u", selected.GetGeneration());
            if (ImGui::Button("Delete Entity")) {
                session.DeleteEntity(selected);
            }
        } else {
            ImGui::TextDisabled("Select an entity to inspect it.");
        }
    }
    ImGui::End();
}

} // namespace

int main(int argc, char** argv) {
    std::uint64_t frameLimit = UINT64_MAX;
    if (!ParseFrameLimit(argc, argv, frameLimit)) {
        std::cerr << "Usage: StonePotatoesEditor [--frames N]\n";
        return 2;
    }
    if (frameLimit == 0) {
        return 0;
    }

    StonePotatoes::ECS::World world;
    world.Initialize();
    StonePotatoes::Editor::EditorSession session(world);
    StonePotatoes::Rendering::Renderer renderer;
    StonePotatoes::Rendering::Renderer::Config config;
    config.title = "StonePotatoes Editor";
    if (!renderer.Initialize(config)) {
        world.Shutdown();
        return 1;
    }

    std::uint64_t renderedFrames = 0;
    int result = 0;
    while (!renderer.ShouldClose() && renderedFrames < frameLimit) {
        renderer.BeginFrame();
        DrawEditor(session, world);
        if (!renderer.EndFrame()) {
            result = 1;
            break;
        }
        ++renderedFrames;
    }

    renderer.Shutdown();
    world.Shutdown();
    return result;
}
