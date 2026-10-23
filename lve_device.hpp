#pragma once


#define VK_USE_PLATFORM_WIN32_KHR
#include <vulkan/vulkan.h>
#include <vector>
#include <string>
#include <optional>
#include <iostream>
#include <set>
#include "Engine.h"
#ifdef NDEBUG





const bool enableValidationLayers = false;
#else
const bool enableValidationLayers = true;
#endif
namespace lve {
    struct QueueFamilyIndices {
        std::optional< uint32_t> graphicsFamily;
        std::optional<uint32_t> presentFamily;
        bool isComplete() {
            return graphicsFamily.has_value() && presentFamily.has_value();
        }
    };

    struct SwapChainSupportDetails {
        VkSurfaceCapabilitiesKHR capabilities;
        std::vector<VkSurfaceFormatKHR> formats;
        std::vector<VkPresentModeKHR> presentModes;
    };


    class LveDevice {

    public:
        
        LveDevice(HWND hwnd, HINSTANCE hInst);
        ~LveDevice();
        VkQueue getGraphicsQueue() { return graphicsQueue; }
        VkQueue getPresentQueue() { return presentQueue; }
        VkCommandPool getCommandPool() { return commandPool; }
        VkSurfaceKHR surface() { return surface_; };
        VkDevice device() { return device_; };
        VkPhysicalDevice getPhysicalDevice() { return physicalDevice; }
        SwapChainSupportDetails getSwapChainSupport() { return  querySwapChainSupport(physicalDevice); }
        QueueFamilyIndices findPhysicalQueueFamilies() { return findQueueFamilies(physicalDevice); }

       
        void createBuffer(
            VkDeviceSize size,
            VkBufferUsageFlags usage,
            VkMemoryPropertyFlags properties,
            VkBuffer& buffer,
            VkDeviceMemory& bufferMemory);

        void copyBuffer(VkBuffer srcBuffer, VkBuffer dstBuffer, VkDeviceSize size);
        uint32_t findMemoryType(uint32_t typeFilter, VkMemoryPropertyFlags properties);
    private:

        HINSTANCE m_hInst;
        HWND m_hwnd;
        VkInstance instance;
        VkDebugUtilsMessengerEXT debugMessenger;
        VkDebugUtilsMessengerCreateInfoEXT createInfo;
        VkSurfaceKHR surface_;
        VkPhysicalDevice physicalDevice = VK_NULL_HANDLE;
        VkDevice device_;
        VkQueue presentQueue;
        VkQueue graphicsQueue;
        VkCommandPool commandPool;



        bool checkValidationLayerSupport();
        void populateDebugMessengerCreateInfo(VkDebugUtilsMessengerCreateInfoEXT& createInfo);
        void createInstance();
        void setupDebugMessenger();
        void createSurface();
        void pickPhysicalDevice();
        void createLogicalDevice();
        void createCommandPool();

        QueueFamilyIndices findQueueFamilies(VkPhysicalDevice device);
        std::vector<const char*> getRequiredExtensions();
        bool isDeviceSuitable(VkPhysicalDevice device);
        bool checkDeviceExtensionSupport(VkPhysicalDevice device);
        SwapChainSupportDetails querySwapChainSupport(VkPhysicalDevice device);
        const std::vector<const char*> validationLayers = { "VK_LAYER_KHRONOS_validation" };



        /*
        {

            std::vector<const char*> extensionSS;
            extensionSS.push_back(VK_KHR_SURFACE_EXTENSION_NAME);
            // extensionSS.push_back("VK_EXT_debug_utils");
            extensionSS.push_back(VK_KHR_WIN32_SURFACE_EXTENSION_NAME);
            if (enableValidationLayers) {
                extensionSS.push_back(VK_EXT_DEBUG_UTILS_EXTENSION_NAME);
            }
            return extensionSS;
        }*/
    };

}