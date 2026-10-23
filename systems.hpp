#pragma once
#include "world.hpp"
#include "components.hpp"
#include "global_ubo.hpp"
#include "..\simple_render_system.hpp"
#include "..\lve_frame_info.hpp"
#include "..\keyboard_movement_controller.hpp"
#include <glm/gtc/constants.hpp>
#include <chrono>

namespace ecs {

    constexpr float FIXED_DT = 1.0f / 60.0f;   // 60 Гц — шаг симуляции
    constexpr int   MAX_PHYSICS_STEPS = 5;           // защита от "спирали смерти"
    constexpr float MAX_FRAME_TIME = 0.25f;

    // --- Анимация источников света ---
    class LightAnimationSystem {
    public:
        void update(World& world, float totalTime) {
            for (Entity e : world.getEntitiesWith<Transform, PointLightComponent>()) {
                auto& t = world.getComponent<Transform>(e);
                auto& l = world.getComponent<PointLightComponent>(e);
                if (!l.animated) continue;

                float angle = totalTime * l.speed + l.phase;
                t.translation.x = glm::cos(angle) * l.radius;
                t.translation.z = glm::sin(angle) * l.radius;
                t.translation.y = -1.5f + 0.5f * glm::sin(totalTime * 1.5f + l.phase);
            }
        }
    };
  
    // --- Управление камерой с клавиатуры ---
    class CameraControlSystem {
    public:
        void update(World& world, float dt) {
            lve::KeyboardMovementController controller;

            for (Entity e : world.getEntitiesWith<Transform, CameraComponent, InputContext, ActiveCameraTag>()) {
                auto& t = world.getComponent<Transform>(e);
                auto& cam = world.getComponent<CameraComponent>(e);
                auto& input = world.getComponent<InputContext>(e);

                lve::TransformComponent lveT;
                lveT.translation = t.translation;
                lveT.rotation = t.rotation;

                controller.moveCamera(input.hwnd, dt, lveT);
                controller.rotateCamera(input.hwnd, dt, lveT);

                t.translation = lveT.translation;
                t.rotation = lveT.rotation;

                cam.camera.setViewYXZ(t.translation, t.rotation);
            }
        }
    };

    // --- Обновление проекции камеры при ресайзе ---
    class CameraProjectionSystem {
    public:
        void update(World& world, float aspect) {
            for (Entity e : world.getEntitiesWith<CameraComponent>()) {
                auto& cam = world.getComponent<CameraComponent>(e);
                cam.aspect = aspect;
                cam.updateProjection();
            }
        }
    };
 

    class GlobalUboBuildSystem {
    public:
        void update(World& world, GlobalUbo& ubo, const lve::LveCamera& camera) {
            ubo.projectionView = camera.getProjection() * camera.getView();
            ubo.ambientLightColor = glm::vec4{ 1.f, 1.f, 1.f, 0.02f };

            // камера
            for (Entity e : world.getEntitiesWith<Transform, ActiveCameraTag>()) {
                auto& t = world.getComponent<Transform>(e);
                ubo.cameraPos = glm::vec4(t.translation, 1.0f);
            }

            // источники света
            int idx = 0;
            for (Entity e : world.getEntitiesWith<Transform, PointLightComponent>()) {
                if (idx >= MAX_LIGHTS) break;
                auto& t = world.getComponent<Transform>(e);
                auto& l = world.getComponent<PointLightComponent>(e);
                ubo.pointLights[idx].position = glm::vec4(t.translation, 1.0f);
                ubo.pointLights[idx].color = glm::vec4(l.color, l.intensity);
                ++idx;
            }
            ubo.numLights = glm::ivec4{ idx, 0, 0, 0 };
        }
    };

    // --- Рендер всех сущностей с Transform + Mesh ---
    class RenderSystem {
    public:
        explicit RenderSystem(lve::SimpleRenderSystem& renderSystem)
            : renderSystem_{ renderSystem } {
        }

        void update(World& world, lve::FrameInfo& frameInfo) {
            // 1. Обычные модели
            for (Entity e : world.getEntitiesWith<Transform, Mesh>()) {
                if (world.hasComponent<Billboard>(e)) continue;
                auto& t = world.getComponent<Transform>(e);
                auto& m = world.getComponent<Mesh>(e);
                if (m.model == nullptr) continue;

                lve::TransformComponent lveT;
                lveT.translation = t.translation;
                lveT.scale = t.scale;
                lveT.rotation = t.rotation;

                renderSystem_.renderModel(frameInfo, lveT, m.model);
            }

            // 2. Биллборды
            for (Entity e : world.getEntitiesWith<Transform, Mesh, Billboard>()) {
                auto& t = world.getComponent<Transform>(e);
                auto& m = world.getComponent<Mesh>(e);
                auto& b = world.getComponent<Billboard>(e);
                if (m.model == nullptr) continue;

                lve::TransformComponent lveT;
                lveT.translation = t.translation;
                lveT.scale = glm::vec3(b.scale);
                lveT.rotation = t.rotation;

                renderSystem_.renderBillboard(frameInfo, lveT, m.model, b.color);
            }
        }


  
     

    private:
        lve::SimpleRenderSystem& renderSystem_;
    };



    class PlayerControllerSystem {
    public:
        void update(World& world, float frameTime, Window& window) {
            lve::KeyboardMovementController controller;

            // --- 1. InputCommand (WASD + Space) ---
            for (Entity e : world.getEntitiesWith<InputCommand, InputContext,
                ActiveCameraTag>()) {
                auto& cmd = world.getComponent<InputCommand>(e);
                auto& input = world.getComponent<InputContext>(e);

                if (GetForegroundWindow() != input.hwnd) {
                    cmd = InputCommand{};
                    continue;
                }

                cmd.forward = 0.f; cmd.right = 0.f; cmd.jump = false;
                if (GetAsyncKeyState(controller.keys.moveForward) & 0x8000) cmd.forward += 1.f;
                if (GetAsyncKeyState(controller.keys.moveBackward) & 0x8000) cmd.forward -= 1.f;
                if (GetAsyncKeyState(controller.keys.moveRight) & 0x8000) cmd.right += 1.f;
                if (GetAsyncKeyState(controller.keys.moveLeft) & 0x8000) cmd.right -= 1.f;
                if (GetAsyncKeyState(VK_SPACE) & 0x8000) cmd.jump = true;
            }

            // --- 2. Камера: поворот + зум ---
            for (Entity e : world.getEntitiesWith<Transform, CameraComponent,
                InputContext, ActiveCameraTag>()) {
                auto& t = world.getComponent<Transform>(e);
                auto& cam = world.getComponent<CameraComponent>(e);
                auto& input = world.getComponent<InputContext>(e);

                if (GetForegroundWindow() != input.hwnd) continue;

                // Поворот (стрелки)
                lve::TransformComponent lveT;
                lveT.translation = t.translation;
                lveT.rotation = t.rotation;
                controller.rotateCamera(input.hwnd, frameTime, lveT);
                t.rotation = lveT.rotation;

                // Зум: колесо мыши (из Window)
                int wheel = window.getMouseWheelDelta();
                if (wheel != 0) {
                    cam.distance -= (wheel / 120.0f) * cam.zoomWheelSensitivity;
                    cam.distance = glm::clamp(cam.distance, cam.minDistance, cam.maxDistance);
                }

                // Зум: клавиши (E = приблизить, Q = отдалить)
                if (GetAsyncKeyState('E') & 0x8000) cam.distance -= cam.zoomStep * frameTime * 10.f;
                if (GetAsyncKeyState('Q') & 0x8000) cam.distance += cam.zoomStep * frameTime * 10.f;
                cam.distance = glm::clamp(cam.distance, cam.minDistance, cam.maxDistance);
            }

            // Сброс колеса (после обработки)
            window.resetMouseWheel();
        }
    };
}