#include "PBRLighting.h"

#include <algorithm>
#include <cmath>

namespace StonePotatoes::Rendering::Detail {
namespace {

constexpr float kPi = 3.14159265358979323846f;

bool IsFinite(const glm::vec3& v) {
    return std::isfinite(v.x) && std::isfinite(v.y) && std::isfinite(v.z);
}

glm::vec3 SafeNormalize(const glm::vec3& v) {
    const float lengthSquared = glm::dot(v, v);
    if (!std::isfinite(lengthSquared) || lengthSquared <= 1.0e-12f)
        return {};
    return v * glm::inversesqrt(lengthSquared);
}

} // namespace

glm::vec3 ShadeMetallicRoughness(const glm::vec3& baseColor, float metallic,
                                 float roughness, const glm::vec3& normal,
                                 const glm::vec3& viewDirection,
                                 const glm::vec3& lightDirection,
                                 const glm::vec3& lightRadiance,
                                 const glm::vec3& ambientRadiance) {
    if (!IsFinite(baseColor) || !IsFinite(normal) || !IsFinite(viewDirection) ||
        !IsFinite(lightDirection) || !IsFinite(lightRadiance) ||
        !IsFinite(ambientRadiance) || !std::isfinite(metallic) ||
        !std::isfinite(roughness)) return {};

    const glm::vec3 n = SafeNormalize(normal);
    const glm::vec3 v = SafeNormalize(viewDirection);
    const glm::vec3 l = SafeNormalize(lightDirection);
    const float nDotV = std::max(glm::dot(n, v), 0.0f);
    const float nDotL = std::max(glm::dot(n, l), 0.0f);
    const glm::vec3 albedo = glm::clamp(
        baseColor, glm::vec3(0.0f), glm::vec3(1.0f));
    const float metal = std::clamp(metallic, 0.0f, 1.0f);
    const float r = std::clamp(roughness, 0.045f, 1.0f);
    const glm::vec3 ambient = glm::max(ambientRadiance, glm::vec3(0.0f));
    if (nDotV <= 0.0f || nDotL <= 0.0f)
        return glm::clamp(albedo * ambient * (1.0f - metal),
                          glm::vec3(0.0f), glm::vec3(1.0f));

    const glm::vec3 h = SafeNormalize(v + l);
    const float nDotH = std::max(glm::dot(n, h), 0.0f);
    const float vDotH = std::max(glm::dot(v, h), 0.0f);
    const float alpha = r * r;
    const float alphaSquared = alpha * alpha;
    const float denominator = nDotH * nDotH * (alphaSquared - 1.0f) + 1.0f;
    const float distribution = alphaSquared /
        std::max(kPi * denominator * denominator, 1.0e-7f);

    const auto smithG1 = [alphaSquared](float nDotX) {
        return (2.0f * nDotX) /
            std::max(nDotX + std::sqrt(alphaSquared +
                (1.0f - alphaSquared) * nDotX * nDotX), 1.0e-7f);
    };
    const float geometry = smithG1(nDotV) * smithG1(nDotL);
    const glm::vec3 f0 = glm::mix(glm::vec3(0.04f), albedo, metal);
    const float fresnelFactor = std::pow(1.0f - vDotH, 5.0f);
    const glm::vec3 fresnel =
        f0 + (glm::vec3(1.0f) - f0) * fresnelFactor;
    const glm::vec3 specular = distribution * geometry * fresnel /
        std::max(4.0f * nDotV * nDotL, 1.0e-6f);
    const glm::vec3 diffuse = (glm::vec3(1.0f) - fresnel) * (1.0f - metal) *
        albedo / kPi;
    const glm::vec3 direct = (diffuse + specular) *
        glm::max(lightRadiance, glm::vec3(0.0f)) * nDotL;
    return glm::clamp(albedo * ambient * (1.0f - metal) + direct,
                      glm::vec3(0.0f), glm::vec3(1.0f));
}

} // namespace StonePotatoes::Rendering::Detail
