#include "lve_renderer.hpp"
#include <stdexcept>
#include <array>
namespace lve {

    LveRenderer::LveRenderer(Window& windowRef, LveDevice& deviceRef)
        : window{ windowRef }, lveDevice{ deviceRef } {
        SIZE initialSize = window.getExtent();
        VkExtent2D extent{ static_cast<uint32_t>(initialSize.cx), static_cast<uint32_t>(initialSize.cy) };
        lveSwapChain = std::make_unique<LveSwapChain>(lveDevice, extent, window.hwnd);
    }

    LveRenderer::~LveRenderer() { lveSwapChain.reset(); }

    void LveRenderer::recreateSwapChain() {
        SIZE wSize = window.getExtent();
        VkExtent2D currentExtent{ static_cast<uint32_t>(wSize.cx), static_cast<uint32_t>(wSize.cy) };

        // Если окно минимизировано (размер 0x0), переходим в режим ожидания
        if (currentExtent.width == 0 || currentExtent.height == 0) {
            MSG msg;
            // Каноничный блокирующий цикл Windows: GetMessage усыпляет поток CPU!
            // Цикл прервется только тогда, когда окно получит новые размеры
            while (currentExtent.width == 0 || currentExtent.height == 0) {
                if (GetMessage(&msg, NULL, 0, 0)) {
                    TranslateMessage(&msg);
                    DispatchMessage(&msg);
                }
                wSize = window.getExtent();
                currentExtent.width = static_cast<uint32_t>(wSize.cx);
                currentExtent.height = static_cast<uint32_t>(wSize.cy);
            }
        }

        vkDeviceWaitIdle(lveDevice.device());
        lveSwapChain.reset();
        lveSwapChain = std::make_unique<LveSwapChain>(lveDevice, currentExtent, window.hwnd);
    }

    VkCommandBuffer LveRenderer::beginFrame() {
        assert(!isFrameStarted && "Нельзя вызвать beginFrame, пока прошлый кадр не завершен!");

        // Вызываем логику drawframe, но забираем только Acquire картинки
        VkDevice dev = lveDevice.device();
        vkWaitForFences(dev, 1, &lveSwapChain->inFlightFences[currentFrameIndex], VK_TRUE, UINT64_MAX);

        VkResult result = vkAcquireNextImageKHR(
            dev, lveSwapChain->getSwapChain(), UINT64_MAX,
            lveSwapChain->imageAvailableSemaphores[currentFrameIndex], VK_NULL_HANDLE, &currentImageIndex);

        if (result == VK_ERROR_OUT_OF_DATE_KHR) {
            recreateSwapChain();
            return nullptr;
        }

        if (result != VK_SUCCESS && result != VK_SUBOPTIMAL_KHR) {
            throw std::runtime_error("failed to acquire swap chain image!");
        }

        isFrameStarted = true;

        VkCommandBuffer commandBuffer = lveSwapChain->commandBuffers[currentFrameIndex];
        vkResetCommandBuffer(commandBuffer, 0);

        VkCommandBufferBeginInfo beginInfo{};
        beginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
        if (vkBeginCommandBuffer(commandBuffer, &beginInfo) != VK_SUCCESS) {
            throw std::runtime_error("failed to begin recording command buffer!");
        }

        return commandBuffer;
    }

    void LveRenderer::endFrame() {
        assert(isFrameStarted && "Нельзя вызвать endFrame, пока кадр не запущен через beginFrame!");
        VkCommandBuffer commandBuffer = lveSwapChain->commandBuffers[currentFrameIndex];
        if (vkEndCommandBuffer(commandBuffer) != VK_SUCCESS) {
            throw std::runtime_error("failed to record command buffer!");
        }

        // Логика Submit очереди
        if (lveSwapChain->imagesInFlight[currentImageIndex] != VK_NULL_HANDLE) {
            vkWaitForFences(lveDevice.device(), 1, &lveSwapChain->imagesInFlight[currentImageIndex], VK_TRUE, UINT64_MAX);
        }
        lveSwapChain->imagesInFlight[currentImageIndex] = lveSwapChain->inFlightFences[currentFrameIndex];
        vkResetFences(lveDevice.device(), 1, &lveSwapChain->inFlightFences[currentFrameIndex]);

        VkSubmitInfo submitInfo{};
        submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
        submitInfo.waitSemaphoreCount = 1;

        VkSemaphore waitSemaphores[] = { lveSwapChain->imageAvailableSemaphores[currentFrameIndex] };
        submitInfo.pWaitSemaphores = waitSemaphores;
        VkPipelineStageFlags waitStages[] = { VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT };
        
        submitInfo.pWaitDstStageMask = waitStages;
        submitInfo.commandBufferCount = 1;
        submitInfo.pCommandBuffers = &commandBuffer;

        VkSemaphore signalSemaphores[] = { lveSwapChain->renderFinishedSemaphores[currentImageIndex] };
        submitInfo.signalSemaphoreCount = 1;
        submitInfo.pSignalSemaphores = signalSemaphores;

        if (vkQueueSubmit(lveDevice.getGraphicsQueue(), 1, &submitInfo, lveSwapChain->inFlightFences[currentFrameIndex]) != VK_SUCCESS) {
            throw std::runtime_error("failed to submit draw command buffer!");
        }

        VkPresentInfoKHR presentInfo{};
        presentInfo.sType = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR;
        presentInfo.waitSemaphoreCount = 1;
        presentInfo.pWaitSemaphores = signalSemaphores;
        VkSwapchainKHR swapChains[] = { lveSwapChain->getSwapChain() };
        presentInfo.swapchainCount = 1;
        presentInfo.pSwapchains = swapChains;
        presentInfo.pImageIndices = &currentImageIndex;

        VkResult result = vkQueuePresentKHR(lveDevice.getPresentQueue(), &presentInfo);

        if (result == VK_ERROR_OUT_OF_DATE_KHR || result == VK_SUBOPTIMAL_KHR || window.wasWindowResized()) {
            window.resetWindowResizedFlag();
            recreateSwapChain();
        }
        else if (result != VK_SUCCESS) {
            throw std::runtime_error("failed to present swap chain image!");
        }

        isFrameStarted = false;
        currentFrameIndex = (currentFrameIndex + 1) % LveSwapChain::MAX_FRAMES_IN_FLIGHT;
    }

    void LveRenderer::beginSwapChainRenderPass(VkCommandBuffer commandBuffer) {
        assert(isFrameStarted && "Обязательно вызовите beginFrame перед запуском Render Pass!");

        VkRenderPassBeginInfo renderPassInfo{};
        renderPassInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
        renderPassInfo.renderPass = lveSwapChain->getRenderPass();
        renderPassInfo.framebuffer = lveSwapChain->getFrameBuffer(currentImageIndex);
        renderPassInfo.renderArea.offset = { 0, 0 };
        renderPassInfo.renderArea.extent = lveSwapChain->getSwapChainExtent();

       // VkClearValue clearColor = { { { 1.0f, 0.0f, 1.0f, 1.0f } } }; // розовый

        VkClearValue clearColor = { { { 0.2f, 0.2f, 0.2f, 1.0f } } };// серый

       // VkClearValue clearColor = { { { 0.0f, 0.0f, 0.0f, 1.0f } } }; черный 



        std::array<VkClearValue, 2> clearValues{};
        clearValues[0].color = { { 0.2f, 0.2f, 0.2f, 1.0f } }; 
        clearValues[1].depthStencil = { 1.0f, 0 };            

        renderPassInfo.clearValueCount = static_cast<uint32_t>(clearValues.size());
        renderPassInfo.pClearValues = clearValues.data();

        vkCmdBeginRenderPass(commandBuffer, &renderPassInfo, VK_SUBPASS_CONTENTS_INLINE);

        VkViewport viewport{};
        viewport.x = 0.0f;
        viewport.y = 0.0f;
        viewport.width = static_cast<float>(lveSwapChain->getSwapChainExtent().width);
        viewport.height = static_cast<float>(lveSwapChain->getSwapChainExtent().height);
        viewport.minDepth = 0.0f;
        viewport.maxDepth = 1.0f;
        vkCmdSetViewport(commandBuffer, 0, 1, &viewport);

        VkRect2D scissor{};
        scissor.offset = { 0, 0 };
        scissor.extent = lveSwapChain->getSwapChainExtent();
        vkCmdSetScissor(commandBuffer, 0, 1, &scissor);
    }

    void LveRenderer::endSwapChainRenderPass(VkCommandBuffer commandBuffer) {
        assert(isFrameStarted && "Нельзя закрыть Render Pass, если кадр не запущен!");
        vkCmdEndRenderPass(commandBuffer);
    }
}
