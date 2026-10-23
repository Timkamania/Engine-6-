
   


#include "framework.h"
#include "Engine.h"
#include "lve_device.hpp"
#include "lve_swap_chain.hpp"
#include "lve_pipeline.hpp"
#include "lve_model.hpp"
#include "Window.h"
#include "lve_renderer.hpp"
#include "simple_render_system.hpp"
#include "lve_descriptors.hpp"
#include "keyboard_movement_controller.hpp"
#include "ecs/world.hpp"
#include "ecs/systems.hpp"
#include "ecs/scene_setup.hpp"
#include "ecs/global_ubo.hpp"
#include "ecs/simulation.hpp"
#include <chrono>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/constants.hpp>

int APIENTRY wWinMain(_In_ HINSTANCE hInstance,
    _In_opt_ HINSTANCE hPrevInstance,
    _In_ LPWSTR lpCmdLine,
    _In_ int nCmdShow) {
    SetConsoleCP(CP_UTF8);
    SetConsoleOutputCP(CP_UTF8);

    Window w(hInstance);

    AllocConsole();
    SetConsoleCP(CP_UTF8);
    SetConsoleOutputCP(CP_UTF8);
    FILE* fp;
    if (freopen_s(&fp, "CONOUT$", "w", stdout) != 0) {
        MessageBoxA(NULL, "Не удалось перенаправить stdout", "Ошибка", MB_OK | MB_ICONERROR);
    }

    lve::LveDevice lveDevice{ w.hwnd, w.hInst };
    lve::LveRenderer lveRenderer{ w, lveDevice };

    // === Descriptor layout / pool / UBO ===
    auto globalSetLayout = lve::LveDescriptorSetLayout::Builder(lveDevice)
        .addBinding(0, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER,
            VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT)
        .build();

    std::unique_ptr<lve::LveDescriptorPool> globalPool =
        lve::LveDescriptorPool::Builder(lveDevice)
        .setMaxSets(lve::LveSwapChain::MAX_FRAMES_IN_FLIGHT)
        .addPoolSize(VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, lve::LveSwapChain::MAX_FRAMES_IN_FLIGHT)
        .build();

    std::vector<std::unique_ptr<lve::LveBuffer>> uboBuffers(lve::LveSwapChain::MAX_FRAMES_IN_FLIGHT);
    for (int i = 0; i < uboBuffers.size(); i++) {
        uboBuffers[i] = std::make_unique<lve::LveBuffer>(
            lveDevice, sizeof(GlobalUbo), 1,
            VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT,
            VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT, 64);
        uboBuffers[i]->map();
    }

    std::vector<VkDescriptorSet> globalDescriptorSets(lve::LveSwapChain::MAX_FRAMES_IN_FLIGHT);
    for (int i = 0; i < globalDescriptorSets.size(); i++) {
        auto bufferInfo = uboBuffers[i]->descriptorInfo();
        lve::LveDescriptorWriter(*globalSetLayout, *globalPool)
            .writeBuffer(0, &bufferInfo)
            .build(globalDescriptorSets[i]);
    }

    // === Загрузка ассетов ===
    ecs::SceneAssets assets;
    assets.vase = lve::LveModel::createModelFromFile(lveDevice, "models/smooth_vase.obj");
    assets.floor = lve::LveModel::createModelFromFile(lveDevice, "models/quad.obj");
    assets.billboardQuad = lve::LveModel::createModelFromFile(lveDevice, "models/quad.obj");

    // === Создание ECS-мира ===
    ecs::World world;
    ecs::spawnCamera(world, w.hwnd);
    ecs::spawnVase(world, assets.vase.get());
    ecs::spawnFloor(world, assets.floor.get());
    ecs::spawnWalls(world, assets.floor.get());
    ecs::spawnLights(world, assets.billboardQuad.get());

    // === Система рендеринга и ECS-системы ===
    lve::SimpleRenderSystem simpleRenderSystem{
        lveDevice,
        lveRenderer.getSwapChainRenderPass(),
        globalSetLayout->getDescriptorSetLayout()
    };

    ecs::LightAnimationSystem lightAnimSys;
   
    ecs::PlayerControllerSystem playerCtrlSys;
    ecs::GlobalUboBuildSystem uboBuildSys;
  //  ecs::RotationSystem rotationSystem;
    ecs::RenderSystem renderSys{ simpleRenderSystem };

    // === Цикл ===
    using Clock = std::chrono::high_resolution_clock;
    auto currentTime = Clock::now();
    float totalTime = 0.0f;
    float accumulator = 0.0f;
    return w.Run([&]() {
        auto newTime = Clock::now();
        //auto newTime = std::chrono::high_resolution_clock::now();
        float frameTime = std::chrono::duration<float>(newTime - currentTime).count();
        currentTime = newTime;

        // === Защита от гигантских скачков (свёрнутое окно, отладчик) ===
        if (frameTime > ecs::MAX_FRAME_TIME) frameTime = ecs::MAX_FRAME_TIME;

        
        playerCtrlSys.update(world, frameTime, w);
         // ============================================================
    // ФИЗИКА / ЛОГИКА: фиксированный шаг (детерминизм для сети)
    // ============================================================
        accumulator += frameTime;
        int physicsSteps = 0;
        while (accumulator >= ecs::FIXED_DT && physicsSteps < ecs::MAX_PHYSICS_STEPS) {
            // Все системы, влияющие на состояние мира — ТОЛЬКО здесь, ТОЛЬКО FIXED_DT
           // rotationSystem.update(world, ecs::FIXED_DT);
            ecs::simulateStep(world, ecs::FIXED_DT);
            // movementSystem.update(world, ecs::FIXED_DT);   // когда появится
            // physicsSystem.update(world, ecs::FIXED_DT);    // когда появится

            accumulator -= ecs::FIXED_DT;
            totalTime += ecs::FIXED_DT;   // детерминированное время растёт шагами!
            ++physicsSteps;
        }


        // === ОБНОВЛЕНИЕ ECS ===
        lightAnimSys.update(world, totalTime);
        
    
       // rotationSystem.update(world, frameTime);

        // === РЕНДЕР ===
        if (auto commandBuffer = lveRenderer.beginFrame()) {
            int frameIndex = lveRenderer.getFrameIndex();

            lve::LveCamera* camera = ecs::getActiveCamera(world);
            if (!camera) return;

            lve::FrameInfo frameInfo{
                frameIndex,
                frameTime,
                commandBuffer,
                *camera,
                globalDescriptorSets[frameIndex]
            };

            // UBO
            GlobalUbo ubo{};
            uboBuildSys.update(world, ubo, *camera);
            uboBuffers[frameIndex]->writeToBuffer(&ubo);
            uboBuffers[frameIndex]->flush();

            lveRenderer.beginSwapChainRenderPass(commandBuffer);
            renderSys.update(world, frameInfo);
            lveRenderer.endSwapChainRenderPass(commandBuffer);
            lveRenderer.endFrame();
        }
        });
}



