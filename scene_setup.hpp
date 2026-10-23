#pragma once
#include "world.hpp"
#include "components.hpp"
#include "global_ubo.hpp"
#include <memory>
#include "..\lve_camera.hpp"
namespace ecs {

    struct SceneAssets {
        std::unique_ptr<lve::LveModel> vase;
        std::unique_ptr<lve::LveModel> floor;
        std::unique_ptr<lve::LveModel> billboardQuad;
    };

    inline Entity spawnCamera(World& world, HWND hwnd) {
        Entity cam = world.createEntity();

        Transform t;
     
      

        // Ставим камеру сзади и сверху от вазы (ваза на 0,0,0)
        t.translation = { 0.f, 4.0f, 3.0f };
        t.rotation = { -9.f, 0.f, 0.f };

        CameraComponent camComp;
        camComp.updateProjection();
        camComp.distance = 5.0f;        // ← 3rd person по умолчанию
        camComp.minDistance = 0.5f;
        camComp.maxDistance = 15.0f;
        camComp.zoomWheelSensitivity = 0.5f;
        camComp.cameraHeightOffset = 0.5f;
        bool freeCamera = false;

        world.addComponent<Transform>(cam, t);
        world.addComponent<CameraComponent>(cam, camComp);
        world.addComponent<InputContext>(cam, InputContext{ hwnd });
        world.addComponent<ActiveCameraTag>(cam, ActiveCameraTag{});


        

        // === ЭТАП 6: физика игрока ===
        RigidbodyComponent rb;
        rb.mass = 70.f;
        rb.useGravity = false ;
        rb.isKinematic = false;   // ← игрок управляется вводом, не интегрируется
        world.addComponent<RigidbodyComponent>(cam, rb);

        ColliderComponent col;
        col.shape = ColliderComponent::Shape::Sphere;
        col.radius = 0.4f;
        // Коллайдер на уровне «ног» — сдвинем вниз
        col.offset = glm::vec3(0.f, -0.8f, 0.f);
        world.addComponent<ColliderComponent>(cam, col);

        world.addComponent<PlayerMovement>(cam, PlayerMovement{});
        world.addComponent<PlayerTag>(cam, PlayerTag{});
        

        world.addComponent<InputCommand>(cam, InputCommand{});

        return cam;
    }

    inline Entity spawnVase(World& world, lve::LveModel* model) {
        Entity e = world.createEntity();
        Transform t;
        t.translation = { 0.f, 0.f, 0.f };
        t.scale = { 0.1f, 0.1f, 0.1f };
        world.addComponent<Transform>(e, t);
        world.addComponent<Mesh>(e, Mesh{ model });

        // Вращение вокруг оси Y, 1 радиан/сек ≈ 57°/сек
        world.addComponent<AngularVelocity>(e, AngularVelocity{
            .value = { 0.f, 1.0f, 0.f }
            });
        return e;
    }

    inline Entity spawnFloor(World& world, lve::LveModel* model) {
        Entity e = world.createEntity();
        Transform t;
   
        t.translation = { 0.f, 0.f, 0.f };
       
        t.scale = { 6.f, 1.f, 6.f };

        world.addComponent<Transform>(e, t);
        world.addComponent<Mesh>(e, Mesh{ model });

        // == = ЭТАП 6: AABB - коллайдер пола == =
            ColliderComponent col;
        col.shape = ColliderComponent::Shape::AABB;
        // Квад масштабирован 3x3 — но модель quad.obj = 1x1, повёрнут...
        // halfExtents должны покрыть визуальную геометрию пола.
        // Если quad.obj — плоский 1x1 в плоскости XZ, то:
       // col.halfExtents = glm::vec3(1.5f, 0.05f, 1.5f);
       // col.halfExtents = glm::vec3(1.5f, 0.5f, 1.5f);
        col.halfExtents = glm::vec3(3.f, 0.5f, 3.f);
        world.addComponent<ColliderComponent>(e, col);
        return e;
    }

    // === НОВОЕ: стены (4 AABB) ===
    inline void spawnWalls(World& world, lve::LveModel* quadModel) {
        struct WallDef { glm::vec3 pos; glm::vec3 scale; glm::vec3 half; };
        const WallDef walls[] = {
            // Север
            { { 0.f, -1.f,  3.f },{ 6.f, 3.f, 0.1f },{ 3.f, 1.5f, 0.05f } },
            // Юг
            { { 0.f, -1.f, -3.f },{ 6.f, 3.f, 0.1f },{ 3.f, 1.5f, 0.05f } },
            // Запад
            { { -3.f, -1.f, 0.f },{ 0.1f, 3.f, 6.f },{ 0.05f, 1.5f, 3.f } },
            // Восток
            { { 3.f, -1.f, 0.f },{ 0.1f, 3.f, 6.f },{ 0.05f, 1.5f, 3.f } },
        };

        for (const auto& w : walls) {
            Entity e = world.createEntity();
            Transform t;
            t.translation = w.pos;
            t.scale = w.scale;

            world.addComponent<Transform>(e, t);
            world.addComponent<Mesh>(e, Mesh{ quadModel });

            ColliderComponent col;
            col.shape = ColliderComponent::Shape::AABB;
            col.halfExtents = w.half;
            world.addComponent<ColliderComponent>(e, col);
        }
    }

    inline void spawnLights(World& world, lve::LveModel* billboardQuad) {
        struct LightDef {
            glm::vec3 pos;
            glm::vec3 color;
            float phase;
        };

        const LightDef defs[] = {
            { { 2.0f, -2.0f, -2.0f },{ 1.0f, 1.0f, 1.0f }, 0.0f },
            { { -2.0f, -1.0f, -1.0f },{ 1.0f, 0.2f, 0.2f }, glm::half_pi<float>() },
            { { 0.0f, -2.0f,  2.0f },{ 0.2f, 0.2f, 1.0f }, glm::pi<float>() },
            { { 2.0f, -1.0f,  2.0f },{ 0.2f, 1.0f, 0.2f }, 3.0f * glm::half_pi<float>() },
        };

        for (int i = 0; i < MAX_LIGHTS; ++i) {
            Entity light = world.createEntity();

            Transform t;
            t.translation = defs[i].pos;

            PointLightComponent lc;
            lc.color = defs[i].color;
            lc.intensity = 2.f;
            lc.radius = 1.5f;
            lc.speed = 0.8f;
            lc.phase = defs[i].phase;
            lc.animated = true;

            Billboard bb;
            bb.color = defs[i].color;
            bb.scale = 0.15f;

            Mesh mesh{ billboardQuad };

            world.addComponent<Transform>(light, t);
            world.addComponent<PointLightComponent>(light, lc);
            world.addComponent<Mesh>(light, mesh);
            world.addComponent<Billboard>(light, bb);
        }
    }

    // Утилита: получить активную камеру из мира
    inline lve::LveCamera* getActiveCamera(World& world) {
        for (Entity e : world.getEntitiesWith<CameraComponent, ActiveCameraTag>()) {
            return &world.getComponent<CameraComponent>(e).camera;
        }
        return nullptr;
    }
}