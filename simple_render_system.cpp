#include "simple_render_system.hpp"
#include <glm/gtc/matrix_transform.hpp>
#include <stdexcept>

namespace lve {

    SimpleRenderSystem::SimpleRenderSystem(LveDevice& device, VkRenderPass renderPass, VkDescriptorSetLayout globalSetLayout) : lveDevice{ device } {
        createPipelineLayout(globalSetLayout);
        createPipeline(renderPass);
        

    }

    SimpleRenderSystem::~SimpleRenderSystem() {
        if (pipelineLayout != VK_NULL_HANDLE) {
            vkDestroyPipelineLayout(lveDevice.device(), pipelineLayout, nullptr);
        }
      
    }

    void SimpleRenderSystem::createPipelineLayout(VkDescriptorSetLayout globalSetLayout) {

       
     
        // 2. Обновляем создание PipelineLayout, передавая туда наш Layout дескриптора
        VkPushConstantRange pushConstantRange{};
        pushConstantRange.stageFlags = VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT;
        pushConstantRange.offset = 0;
        pushConstantRange.size = sizeof(SimplePushConstantData);

        VkPipelineLayoutCreateInfo pipelineLayoutInfo{};
        pipelineLayoutInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
        pipelineLayoutInfo.setLayoutCount = 1; // ТЕПЕРЬ У НАС 1 LAYOUT
        pipelineLayoutInfo.pSetLayouts = &globalSetLayout; // Передаем его
        pipelineLayoutInfo.pushConstantRangeCount = 1;
        pipelineLayoutInfo.pPushConstantRanges = &pushConstantRange;

        if (vkCreatePipelineLayout(lveDevice.device(), &pipelineLayoutInfo, nullptr, &pipelineLayout) != VK_SUCCESS) {
            throw std::runtime_error("failed to create pipeline layout!");
        }
    }

    void SimpleRenderSystem::createPipeline(VkRenderPass renderPass) {
        assert(pipelineLayout != VK_NULL_HANDLE && "Нельзя создать конвейер до PipelineLayout!");

        PipelineConfigInfo pipelineConfig{};
        LvePipeline::defaultPipelineConfigInfo(pipelineConfig);
        pipelineConfig.renderPass = renderPass;
        pipelineConfig.pipelineLayout = pipelineLayout; // Передаем лейаут

        // КАНОНИЧНОЕ ЗАПОЛНЕНИЕ: Система рендеринга связывает разметку вершин LveModel с конфигом пайплайна
        pipelineConfig.bindingDescriptions = LveModel::Vertex::getBindingDescriptions();
        pipelineConfig.attributeDescriptions = LveModel::Vertex::getAttributeDescriptions();
        lvePipeline = std::make_unique<LvePipeline>(lveDevice, pipelineConfig);
       

        // --- Пайплайн для биллбордов (2D-спрайты) ---
        PipelineConfigInfo billboardConfig{};
       
        LvePipeline::defaultPipelineConfigInfo(billboardConfig);
        billboardConfig.fragShaderPath = "shaders/billboard.frag.spv";
        billboardConfig.renderPass = renderPass;
        billboardConfig.pipelineLayout = pipelineLayout;
        billboardConfig.bindingDescriptions = LveModel::Vertex::getBindingDescriptions();
        billboardConfig.attributeDescriptions = LveModel::Vertex::getAttributeDescriptions();

        // Не писать в depth (чтобы не перекрывать другие объекты)
        billboardConfig.depthStencilInfo.depthWriteEnable = VK_FALSE;
        // Но проверять depth (чтобы не рисоваться сквозь вазу, если биллборд за ней)
        billboardConfig.depthStencilInfo.depthTestEnable = VK_TRUE;

        // Alpha-blending (для будущих текстур с прозрачностью)
        billboardConfig.colorBlendAttachment.blendEnable = VK_TRUE;
        billboardConfig.colorBlendAttachment.srcColorBlendFactor = VK_BLEND_FACTOR_SRC_ALPHA;
        billboardConfig.colorBlendAttachment.dstColorBlendFactor = VK_BLEND_FACTOR_ONE_MINUS_SRC_ALPHA;
        billboardConfig.colorBlendAttachment.colorBlendOp = VK_BLEND_OP_ADD;
        billboardConfig.colorBlendAttachment.srcAlphaBlendFactor = VK_BLEND_FACTOR_ONE;
        billboardConfig.colorBlendAttachment.dstAlphaBlendFactor = VK_BLEND_FACTOR_ZERO;
        billboardConfig.colorBlendAttachment.alphaBlendOp = VK_BLEND_OP_ADD;
        billboardConfig.rasterizationInfo.cullMode = VK_CULL_MODE_NONE;
        lveBillboardPipeline = std::make_unique<LvePipeline>(lveDevice, billboardConfig);
    }

    void SimpleRenderSystem::renderModel(FrameInfo& frameInfo, const TransformComponent& cubeTransform, LveModel* model) {
        vkCmdBindPipeline(frameInfo.commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, lvePipeline->getGraphicsPipeline());

       
  


    

        // ДОБАВЛЕНО ИЗ УРОКА 20: Привязываем Uniform-буфер к конвейеру!
        vkCmdBindDescriptorSets(
            frameInfo.commandBuffer,
            VK_PIPELINE_BIND_POINT_GRAPHICS,
            pipelineLayout,
            0, 1,
            &frameInfo.globalDescriptorSet,
            0, nullptr
        );

       

        // Математика ручного разделения сдвига и вращения для вазы
        glm::mat4 modelMatrix = cubeTransform.mat4();


     

        SimplePushConstantData push{};
        push.modelMatrix = modelMatrix;
        push.normalMatrix = glm::mat4(cubeTransform.normalMatrix());
        push.color = glm::vec4(1.0f);

        vkCmdPushConstants(
            frameInfo.commandBuffer,
            pipelineLayout,
            VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT,
            0,
            sizeof(SimplePushConstantData),
            &push
        );

        if (model != nullptr) {
            model->bind(frameInfo.commandBuffer);
            model->draw(frameInfo.commandBuffer);
        }
    }

   
    void SimpleRenderSystem::renderBillboard(FrameInfo& frameInfo, const TransformComponent& transform, LveModel* model, const glm::vec3& color) {
     
        // Используем ОТДЕЛЬНЫЙ пайплайн для биллбордов
        vkCmdBindPipeline(frameInfo.commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, lveBillboardPipeline->getGraphicsPipeline());

        vkCmdBindDescriptorSets(
            frameInfo.commandBuffer,
            VK_PIPELINE_BIND_POINT_GRAPHICS,
            pipelineLayout,
            0, 1,
            &frameInfo.globalDescriptorSet,
            0, nullptr
        );
       
        // --- Биллборд: модель ориентируется по камере ---
        glm::mat4 viewInv = glm::inverse(frameInfo.camera.getView());
        viewInv[3] = glm::vec4(0, 0, 0, 1); // убираем трансляцию, оставляем только вращение

        glm::mat4 standUp = glm::rotate(glm::mat4(1.0f), glm::radians(90.0f), glm::vec3(1, 0, 0));

        glm::mat4 billboardModel =
            glm::translate(glm::mat4(1.0f), transform.translation)
            * viewInv
            * standUp
            * glm::scale(glm::mat4(1.0f), transform.scale);


        SimplePushConstantData push{};
        push.modelMatrix = billboardModel;
        push.normalMatrix = glm::mat4(1.0f);
        push.color = glm::vec4(color, 1.0f);

        vkCmdPushConstants(
            frameInfo.commandBuffer,
            pipelineLayout,
            VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT,
            0,
            sizeof(SimplePushConstantData),
            &push
        );
        if (model != nullptr) {
            model->bind(frameInfo.commandBuffer);
            model->draw(frameInfo.commandBuffer);
        }

    }

}   
