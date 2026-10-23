
#pragma once
#include"lve_model.hpp"
#include "lve_device.hpp"
#include "lve_pipeline.hpp"
//#include "Engine.h"
#include <memory>
#include <string>
#include <vector>
#include <algorithm>
//#include "Engine.h"
namespace lve {

    class LveSwapChain {

    public:
        
        LveSwapChain(LveDevice& deviceRef, VkExtent2D windowExtent, HWND hwnd);
        LveSwapChain(LveDevice& deviceRef, VkExtent2D windowExtent, std::shared_ptr<LveSwapChain> previous);
        ~LveSwapChain();

        LveSwapChain(const LveSwapChain&) = delete;
        LveSwapChain& operator=(const LveSwapChain&) = delete;
       
        
        VkFramebuffer getFrameBuffer(int index) { return swapChainFramebuffers[index]; }
        VkRenderPass getRenderPass() const { return renderPass; }
        VkImageView getImageView(int index) { return swapChainImageViews[index]; }
        size_t imageCount() { return swapChainImages.size(); }
        VkFormat getSwapChainImageFormat() const { return swapChainImageFormat; }
        VkExtent2D getSwapChainExtent() const { return windowExtent; }
       // uint32_t width() const { return swapChainExtent.width; }
      //  uint32_t height()  const { return swapChainExtent.height; }
        VkSwapchainKHR getSwapChain() { return swapChain; }
       // VkSemaphore getImageAvailableSemaphore() { return imageAvailableSemaphores;}

        //VkResult drawframe(lve::LveModel* model, const SimplePushConstantData& pushData, VkPipelineLayout pipelineLayout);
        
        void recordCommandBuffer(VkCommandBuffer commandBuffer, uint32_t imageIndex, lve::LveModel* model, const SimplePushConstantData& pushData, VkPipelineLayout pipelineLayout);
        void setPipeline(VkPipeline pipeline) { graphicsPipeline = pipeline; }
        VkResult drawframe();
        std::vector<VkFramebuffer> swapChainFramebuffers;
        std::vector<VkImageView> swapChainImageViews;

        static constexpr int MAX_FRAMES_IN_FLIGHT = 2;

        // Вместо одиночных объектов создаем массивы (векторы)
        std::vector<VkSemaphore> imageAvailableSemaphores;
        std::vector<VkSemaphore> renderFinishedSemaphores;

        std::vector<VkFence> inFlightFences;
        std::vector<VkFence> imagesInFlight;

        std::vector<VkCommandBuffer> commandBuffers;


    private:

        void createDepthResources();
        VkFormat findDepthFormat();

        VkFormat swapChainDepthFormat;
        std::vector<VkImage> depthImages;
        std::vector<VkDeviceMemory> depthImageMemorys;
        std::vector<VkImageView> depthImageViews;


        
        HWND m_hwnd;
  
        size_t currentFrame = 0;

        VkPipeline graphicsPipeline;
        VkSwapchainKHR swapChain;
        std::vector<VkImage> swapChainImages;
        VkFormat swapChainImageFormat;
        VkExtent2D windowExtent;
        LveDevice& device;
        VkRenderPass renderPass;
        VkQueue graphicsQueue;
        VkQueue presentQueue;
   
        VkSurfaceFormatKHR chooseSwapSurfaceFormat(const std::vector<VkSurfaceFormatKHR>& availableFormats);
        VkPresentModeKHR chooseSwapPresentMode(const std::vector<VkPresentModeKHR>& availablePresentModes);
        VkExtent2D chooseSwapExtent(const VkSurfaceCapabilitiesKHR& capabilities);


        void createSwapChain();
        void createImageViews();
        void createRenderPass();
        void createFramebuffers();
        void createSemaphores();
        void allocateCommandBuffer();

    };
}