#pragma once

#include "lve_device.hpp"
#include "lve_swap_chain.hpp"
#include "Window.h"
#include <memory>
#include <cassert>

namespace lve {

    class LveRenderer {
    public:
        LveRenderer(Window& window, LveDevice& device);
        ~LveRenderer();

        LveRenderer(const LveRenderer&) = delete;
        LveRenderer& operator=(const LveRenderer&) = delete;

        VkRenderPass getSwapChainRenderPass() const { return lveSwapChain->getRenderPass(); }
        VkExtent2D getSwapChainExtent() const { return lveSwapChain->getSwapChainExtent(); }
        int getFrameIndex() const { return currentFrameIndex; }
        VkCommandBuffer beginFrame();
        void endFrame();
        void beginSwapChainRenderPass(VkCommandBuffer commandBuffer);
        void endSwapChainRenderPass(VkCommandBuffer commandBuffer);
       
    private:
        void recreateSwapChain();

        Window& window;
        LveDevice& lveDevice;
        std::unique_ptr<LveSwapChain> lveSwapChain;

        uint32_t currentImageIndex;
        int currentFrameIndex{ 0 };
        bool isFrameStarted{ false };
    };
}