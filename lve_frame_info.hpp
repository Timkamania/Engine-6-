#pragma once

#include "lve_camera.hpp"
#include <vulkan/vulkan.h>

namespace lve {
    struct FrameInfo {
        int frameIndex;
        float frameTime;
        VkCommandBuffer commandBuffer;
        const LveCamera& camera;
        VkDescriptorSet globalDescriptorSet;
    };
}