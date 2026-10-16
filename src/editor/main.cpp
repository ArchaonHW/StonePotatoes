#include "EditorSession.h"
#include "../rendering/Renderer.h"

#include <imgui.h>

#include <charconv>
#include <cstdint>
#include <iostream>
#include <string>
#include <string_view>

namespace {

bool ParseArguments(int argc, char** argv, std::uint64_t& frameLimit,
                    std::string& scenePath, std::string& savePath) {
    frameLimit = UINT64_MAX;
    for (int i = 1; i < argc; ++i) {
        const std::string_view arg(argv[i]);
        if (arg == "--help" || arg == "-h") {
            std::cout << "StonePotatoesEditor [--frames N]\n"
                         "                       [--scene FILE] [--save FILE]\n"
                         "  --frames N  Exit cleanly after N rendered frames.\n";
            frameLimit = 0;
            return true;
        }
        if ((arg == "--scene" || arg == "--save") && i + 1 < argc) {
            std::string& destination = arg == "--scene" ? scenePath : savePath;
            if (!destination.empty()) return false;
            destination = argv[++i];
            if (destination.empty()) return false;
            continue;
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
                StonePotatoes::ECS::World& world,
                char* entityLabel, const std::string& savePath,
                std::string& saveStatus) {
    using StonePotatoes::ECS::Entity;

    if (ImGui::Begin("Entity Hierarchy")) {
        ImGui::InputText("Entity label", entityLabel, 64);
        if (ImGui::Button("Create Entity")) {
            session.CreateEntity(entityLabel);
            entityLabel[0] = '\0';
        }
        ImGui::Separator();

        const Entity selected = session.SelectedEntity();
        for (const Entity entity : session.Entities()) {
            ImGui::PushID(static_cast<int>(entity.GetIndex()));
            const std::string_view name = session.GetEntityLabel(entity);
            std::string label(name.empty() ? "Entity" : name);
            label += "  [" + std::to_string(entity.GetIndex()) + ":" +
                     std::to_string(entity.GetGeneration()) + "]";
            if (ImGui::Selectable(label.c_str(), entity == selected)) {
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
        if (!savePath.empty() && ImGui::Button("Save Scene")) {
            if (world.SaveToFile(savePath.c_str())) {
                saveStatus = "Scene saved.";
            } else {
                saveStatus = "Scene save failed.";
            }
        }
        if (savePath.empty()) {
            ImGui::TextDisabled("Start with --save FILE to enable saving.");
        }
        if (!saveStatus.empty()) ImGui::TextUnformatted(saveStatus.c_str());
        ImGui::Separator();
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
    std::string scenePath;
    std::string savePath;
    if (!ParseArguments(argc, argv, frameLimit, scenePath, savePath)) {
        std::cerr << "Usage: StonePotatoesEditor [--frames N] [--scene FILE] [--save FILE]\n";
        return 2;
    }
    if (frameLimit == 0) {
        return 0;
    }

    StonePotatoes::ECS::World world;
    world.Initialize();
    StonePotatoes::Editor::RegisterEditorComponents(world);
    if (!scenePath.empty() && !world.LoadFromFile(scenePath.c_str())) {
        std::cerr << "Failed to load scene: " << scenePath << '\n';
        world.Shutdown();
        return 1;
    }
    StonePotatoes::Editor::EditorSession session(world);
    StonePotatoes::Rendering::Renderer renderer;
    StonePotatoes::Rendering::Renderer::Config config;
    config.title = "StonePotatoes Editor";
    if (!renderer.Initialize(config)) {
        world.Shutdown();
        return 1;
    }

    std::uint64_t renderedFrames = 0;
    char entityLabel[64]{};
    std::string saveStatus;
    int result = 0;
    while (!renderer.ShouldClose() && renderedFrames < frameLimit) {
        renderer.BeginFrame();
        DrawEditor(session, world, entityLabel, savePath, saveStatus);
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
