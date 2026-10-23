#pragma once
#include "lve_device.hpp"
#include <vulkan/vulkan.h>
#include <string>
#include <vector>
#include <glm/glm.hpp> 

namespace lve {



	struct SimplePushConstantData {
		glm::mat4 modelMatrix{ 1.0f };  // Смещение 0  (Занимает 64 байта)
		glm::mat4 normalMatrix{ 1.0f }; // Смещение 64 (Занимает 64 байта)
		glm::vec4 color{ 1.0f, 1.0f, 1.0f, 1.0f };
	};

	struct PipelineConfigInfo {

		PipelineConfigInfo() = default;
	

		VkPipelineViewportStateCreateInfo viewportInfo;
		VkPipelineInputAssemblyStateCreateInfo inputAssemblyInfo;
		VkPipelineRasterizationStateCreateInfo rasterizationInfo;
		VkPipelineMultisampleStateCreateInfo multisampleInfo;
		VkPipelineColorBlendAttachmentState colorBlendAttachment;
		VkPipelineColorBlendStateCreateInfo colorBlendInfo;
		VkPipelineDepthStencilStateCreateInfo depthStencilInfo;

		std::vector<VkDynamicState> dynamicStateEnables;
		VkPipelineDynamicStateCreateInfo dynamicStateInfo;

		VkPipelineLayout pipelineLayout = nullptr;
		VkRenderPass renderPass = nullptr;
		uint32_t subpass = 0;

		// КАНОНИЧНОЕ ДОБАВЛЕНИЕ ИЗ УРОКА: Теперь разметку вершин хранит сам Конфиг!
		std::vector<VkVertexInputBindingDescription> bindingDescriptions{};
		std::vector<VkVertexInputAttributeDescription> attributeDescriptions{};

		std::string vertShaderPath = "shaders/shader.vert.spv";
		std::string fragShaderPath = "shaders/shader.frag.spv";
	};

	class LvePipeline {
    
	public:
		LvePipeline(LveDevice& lveDevice, const PipelineConfigInfo& configInfo);
		~LvePipeline();
		LvePipeline(const LvePipeline&) = delete;
		LvePipeline& operator=(const LvePipeline&) = delete;


		static  void defaultPipelineConfigInfo(PipelineConfigInfo& confihInfo);


		VkPipeline getGraphicsPipeline() const { return graphicsPipeline; }

	


	private:
		LveDevice& lveDevice;
		VkPipeline graphicsPipeline;
	
		void createGraphicsPipeline(const PipelineConfigInfo& configInfo);
		VkShaderModule createShaderModule(const std::vector<char>& code);
		static std::vector<char> readFile(const std::string& filepath);


	};

}
