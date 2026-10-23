#include "lve_descriptors.hpp"
#include <cassert>
#include <stdexcept>

namespace lve {

    // ====== LveDescriptorSetLayout::Builder ======
    LveDescriptorSetLayout::Builder& LveDescriptorSetLayout::Builder::addBinding(
        uint32_t binding, VkDescriptorType descriptorType, VkShaderStageFlags stageFlags, uint32_t count) {
        assert(bindings.count(binding) == 0 && "Данный слот привязки (binding) уже занят");
        VkDescriptorSetLayoutBinding layoutBinding{};
        layoutBinding.binding = binding;
        layoutBinding.descriptorType = descriptorType;
        layoutBinding.descriptorCount = count;
        layoutBinding.stageFlags = stageFlags;
        bindings[binding] = layoutBinding;
        return *this;
    }

    std::unique_ptr<LveDescriptorSetLayout> LveDescriptorSetLayout::Builder::build() {
        return std::make_unique<LveDescriptorSetLayout>(lveDevice, bindings);
    }

    // ====== LveDescriptorSetLayout ======
    LveDescriptorSetLayout::LveDescriptorSetLayout(LveDevice& lveDevice, std::unordered_map<uint32_t, VkDescriptorSetLayoutBinding> bindings)
        : lveDevice{ lveDevice }, bindings{ bindings } {
        std::vector<VkDescriptorSetLayoutBinding> setLayoutBindings{};
        for (auto kv : bindings) {
            setLayoutBindings.push_back(kv.second);
        }

        VkDescriptorSetLayoutCreateInfo descriptorLayoutInfo{};
        descriptorLayoutInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
        descriptorLayoutInfo.bindingCount = static_cast<uint32_t>(setLayoutBindings.size());
        descriptorLayoutInfo.pBindings = setLayoutBindings.data();

        if (vkCreateDescriptorSetLayout(lveDevice.device(), &descriptorLayoutInfo, nullptr, &descriptorSetLayout) != VK_SUCCESS) {
            throw std::runtime_error("failed to create descriptor set layout!");
        }
    }

    LveDescriptorSetLayout::~LveDescriptorSetLayout() {
        vkDestroyDescriptorSetLayout(lveDevice.device(), descriptorSetLayout, nullptr);
    }

    // ====== LveDescriptorPool::Builder ======
    LveDescriptorPool::Builder& LveDescriptorPool::Builder::addPoolSize(VkDescriptorType descriptorType, uint32_t count) {
        poolSizes.push_back({ descriptorType, count });
        return *this;
    }

    LveDescriptorPool::Builder& LveDescriptorPool::Builder::setPoolFlags(VkDescriptorPoolCreateFlags flags) {
        poolFlags = flags;
        return *this;
    }

    LveDescriptorPool::Builder& LveDescriptorPool::Builder::setMaxSets(uint32_t count) {
        maxSets = count;
        return *this;
    }

    std::unique_ptr<LveDescriptorPool> LveDescriptorPool::Builder::build() {
        return std::make_unique<LveDescriptorPool>(lveDevice, maxSets, poolFlags, poolSizes);
    }

    // ====== LveDescriptorPool ======
    LveDescriptorPool::LveDescriptorPool(LveDevice& lveDevice, uint32_t maxSets, VkDescriptorPoolCreateFlags poolFlags, const std::vector<VkDescriptorPoolSize>& poolSizes)
        : lveDevice{ lveDevice } {
        VkDescriptorPoolCreateInfo descriptorPoolInfo{};
        descriptorPoolInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
        descriptorPoolInfo.poolSizeCount = static_cast<uint32_t>(poolSizes.size());
        descriptorPoolInfo.pPoolSizes = poolSizes.data();
        descriptorPoolInfo.maxSets = maxSets;
        descriptorPoolInfo.flags = poolFlags;

        if (vkCreateDescriptorPool(lveDevice.device(), &descriptorPoolInfo, nullptr, &descriptorPool) != VK_SUCCESS) {
            throw std::runtime_error("failed to create descriptor pool!");
        }
    }

    LveDescriptorPool::~LveDescriptorPool() {
        vkDestroyDescriptorPool(lveDevice.device(), descriptorPool, nullptr);
    }

    bool LveDescriptorPool::allocateDescriptorSet(const VkDescriptorSetLayout descriptorSetLayout, VkDescriptorSet& descriptorSet) {
        VkDescriptorSetAllocateInfo allocInfo{};
        allocInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
        allocInfo.descriptorPool = descriptorPool;
        allocInfo.pSetLayouts = &descriptorSetLayout;
        allocInfo.descriptorSetCount = 1;

        if (vkAllocateDescriptorSets(lveDevice.device(), &allocInfo, &descriptorSet) != VK_SUCCESS) {
            return false;
        }
        return true;
    }

    void LveDescriptorPool::freeDescriptors(std::vector<VkDescriptorSet>& descriptors) {
        vkFreeDescriptorSets(lveDevice.device(), descriptorPool, static_cast<uint32_t>(descriptors.size()), descriptors.data());
    }

    void LveDescriptorPool::resetPool() {
        vkResetDescriptorPool(lveDevice.device(), descriptorPool, 0);
    }

    // ====== LveDescriptorWriter ======
    LveDescriptorWriter::LveDescriptorWriter(LveDescriptorSetLayout& setLayout, LveDescriptorPool& pool)
        : setLayout{ setLayout }, pool{ pool } {
    }

    LveDescriptorWriter& LveDescriptorWriter::writeBuffer(uint32_t binding, VkDescriptorBufferInfo* bufferInfo) {
        assert(setLayout.bindings.count(binding) == 1 && "Layout не содержит привязки под данным индексом");
        auto& bindingDescription = setLayout.bindings[binding];
        assert(bindingDescription.descriptorCount == 1 && "Метод writeBuffer поддерживает только одиночные дескрипторы");

        VkWriteDescriptorSet write{};
        write.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
        write.descriptorType = bindingDescription.descriptorType;
        write.dstBinding = binding;
        write.pBufferInfo = bufferInfo;
        write.descriptorCount = 1;
        writes.push_back(write);
        return *this;
    }

    LveDescriptorWriter& LveDescriptorWriter::writeImage(uint32_t binding, VkDescriptorImageInfo* imageInfo) {
        assert(setLayout.bindings.count(binding) == 1 && "Layout не содержит привязки под данным индексом");
        auto& bindingDescription = setLayout.bindings[binding];
        assert(bindingDescription.descriptorCount == 1 && "Метод writeImage поддерживает только одиночные дескрипторы");

        VkWriteDescriptorSet write{};
        write.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
        write.descriptorType = bindingDescription.descriptorType;
        write.dstBinding = binding;
        write.pImageInfo = imageInfo;
        write.descriptorCount = 1;
        writes.push_back(write);
        return *this;
    }

    bool LveDescriptorWriter::build(VkDescriptorSet& set) {
        bool success = pool.allocateDescriptorSet(setLayout.getDescriptorSetLayout(), set);
        if (!success) return false;
        overwrite(set);
        return true;
    }

    void LveDescriptorWriter::overwrite(VkDescriptorSet& set) {
        for (auto& write : writes) {
            write.dstSet = set;
        }
        vkUpdateDescriptorSets(pool.lveDevice.device(), static_cast<uint32_t>(writes.size()), writes.data(), 0, nullptr);
    }

} // namespace lve