#pragma once

#include <glm/glm.hpp>

namespace StonePotatoes::Rendering {

// glTF-style metallic-roughness inputs. This flat-triangle slice does not
// include textures, normal maps, image-based lighting, or tone mapping.
struct PBRMaterial {
    glm::vec3 baseColor{1.0f};
    float metallic = 0.0f;
    float roughness = 0.5f;
};

} // namespace StonePotatoes::Rendering
