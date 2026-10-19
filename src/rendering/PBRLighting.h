#pragma once

#include <glm/glm.hpp>

namespace StonePotatoes::Rendering::Detail {

// Project-authored Cook-Torrance direct-light reference implementation.
glm::vec3 ShadeMetallicRoughness(const glm::vec3& baseColor, float metallic,
                                 float roughness, const glm::vec3& normal,
                                 const glm::vec3& viewDirection,
                                 const glm::vec3& lightDirection,
                                 const glm::vec3& lightRadiance,
                                 const glm::vec3& ambientRadiance);

} // namespace StonePotatoes::Rendering::Detail
