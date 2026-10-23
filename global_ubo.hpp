#pragma once
#include <glm/glm.hpp>

constexpr int MAX_LIGHTS = 4;

struct PointLightGpu {
    glm::vec4 position{ 0.f };
    glm::vec4 color{ 1.f };
};

struct GlobalUbo {
    glm::mat4 projectionView{ 1.f };
    glm::vec4 ambientLightColor{ 1.f, 1.f, 1.f, 0.02f };
    PointLightGpu pointLights[MAX_LIGHTS];
    glm::ivec4 numLights{ 0, 0, 0, 0 };
    glm::vec4 cameraPos{ 0.f, 0.f, 0.f, 1.f };
};

static_assert(offsetof(GlobalUbo, projectionView) == 0, "UBO layout");
static_assert(offsetof(GlobalUbo, ambientLightColor) == 64, "UBO layout");
static_assert(offsetof(GlobalUbo, pointLights) == 80, "UBO layout");
static_assert(offsetof(GlobalUbo, numLights) == 208, "UBO layout");
static_assert(offsetof(GlobalUbo, cameraPos) == 224, "UBO layout");
static_assert(sizeof(GlobalUbo) == 240, "UBO size");