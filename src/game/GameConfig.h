#pragma once

#include <string>

namespace StonePotatoes::Game {

// Engine-level defaults shared by config loading and the platform bootstrap.
struct GameConfig {
    int windowWidth = 1280;
    int windowHeight = 720;
    std::string windowTitle = "StonePotatoes";
};

} // namespace StonePotatoes::Game
