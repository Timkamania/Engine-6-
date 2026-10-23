


#pragma once

#include "world.hpp"
#include "components.hpp"

#include <cmath>
#include <glm/glm.hpp>
#include <glm/gtc/constants.hpp>
#include <algorithm>

namespace ecs {

    // ============================================================
    // Утилиты
    // ============================================================

    // Нормализация угла в (-π, π] — стабильно для сети и float
    inline float wrapAngle(float a) {
        a = std::fmod(a + glm::pi<float>(), glm::two_pi<float>());
        if (a < 0.0f) a += glm::two_pi<float>();
        return a - glm::pi<float>();
    }

    // Мировая позиция центра коллайдера
    inline glm::vec3 colliderCenter(const Transform& t, const ColliderComponent& c) {
        return t.translation + c.offset;
    }

    // ============================================================
    // Разрешение коллизий
    // ============================================================

    // Сфера vs AABB. Возвращает true, если коллизия, и заполняет normal/depth.
    // normal указывает от AABB к сфере.
    inline bool sphereVsAABB(const glm::vec3& sphereCenter, float sphereRadius,
        const glm::vec3& boxCenter, const glm::vec3& boxHalf,
        glm::vec3& outNormal, float& outDepth)
    {
        // Ближайшая точка AABB к центру сферы
        glm::vec3 closest = glm::clamp(sphereCenter,
            boxCenter - boxHalf,
            boxCenter + boxHalf);

        glm::vec3 diff = sphereCenter - closest;
        float dist2 = glm::dot(diff, diff);

        if (dist2 > sphereRadius * sphereRadius) return false;

        float dist = std::sqrt(dist2);
        if (dist > 1e-6f) {
            outNormal = diff / dist;
            outDepth = sphereRadius - dist;
        }
        else {
            // Центр сферы внутри AABB — выталкиваем по МИНИМАЛЬНОЙ ПРОНИКАЮЩЕЙ оси
            glm::vec3 rel = sphereCenter - boxCenter;
            glm::vec3 absRel = glm::abs(rel);

            // Проникающая глубина по каждой оси (сколько до грани)
            float dx = boxHalf.x - absRel.x;
            float dy = boxHalf.y - absRel.y;
            float dz = boxHalf.z - absRel.z;

            // Выбираем минимальную — кратчайший выход
            if (dx < dy && dx < dz) {
                outNormal = glm::vec3(rel.x >= 0 ? 1.f : -1.f, 0.f, 0.f);
                outDepth = dx + sphereRadius;
            }
            else if (dy < dz) {
                outNormal = glm::vec3(0.f, rel.y >= 0 ? 1.f : -1.f, 0.f);
                outDepth = dy + sphereRadius;
            }
            else {
                outNormal = glm::vec3(0.f, 0.f, rel.z >= 0 ? 1.f : -1.f);
                outDepth = dz + sphereRadius;
            }
            return true;
        }
    }

    // Сфера vs Сфера
    inline bool sphereVsSphere(const glm::vec3& c1, float r1,
        const glm::vec3& c2, float r2,
        glm::vec3& outNormal, float& outDepth)
    {
        glm::vec3 diff = c1 - c2;
        float dist2 = glm::dot(diff, diff);
        float sum = r1 + r2;
        if (dist2 > sum * sum) return false;

        float dist = std::sqrt(dist2);
        if (dist > 1e-6f) {
            outNormal = diff / dist;
            outDepth = sum - dist;
        }
        else {
            outNormal = glm::vec3(0.f, 1.f, 0.f);
            outDepth = sum;
        }
        return true;
    }



 

    inline void resolveCollisions(World& world) {
        auto colliders = world.getEntitiesWith<Transform, ColliderComponent>();

        // --- A. Сфера vs AABB ---
        for (Entity a : colliders) {
            auto& tA = world.getComponent<Transform>(a);
            auto& cA = world.getComponent<ColliderComponent>(a);
            if (cA.shape != ColliderComponent::Shape::Sphere) continue;

            for (Entity b : colliders) {
                if (a == b) continue;
                auto& tB = world.getComponent<Transform>(b);
                auto& cB = world.getComponent<ColliderComponent>(b);
                if (cB.shape != ColliderComponent::Shape::AABB) continue;

                glm::vec3 n; float depth;
                if (sphereVsAABB(colliderCenter(tA, cA), cA.radius,
                    colliderCenter(tB, cB), cB.halfExtents, n, depth)) {
                    tA.translation += n * depth;

                    if (world.hasComponent<RigidbodyComponent>(a)) {
                        auto& rb = world.getComponent<RigidbodyComponent>(a);
                        float vn = glm::dot(rb.velocity, n);
                        if (vn < 0.f) {
                            rb.velocity -= n * vn;
                        }
                        if (n.y > 0.7f) {
                            if (rb.velocity.y <= 0.f) {
                                rb.velocity.y = 0.f;
                                rb.isGrounded = true;
                            }
                        }
                    }
                }
            }
        }

        // --- B. Сфера vs Сфера (ВЫНЕСЕНО ИЗ for(a)!) ---
        for (size_t i = 0; i < colliders.size(); ++i) {
            Entity a = colliders[i];
            auto& tA = world.getComponent<Transform>(a);
            auto& cA = world.getComponent<ColliderComponent>(a);
            if (cA.shape != ColliderComponent::Shape::Sphere) continue;

            for (size_t j = i + 1; j < colliders.size(); ++j) {
                Entity b = colliders[j];
                if (a == b) continue;
                auto& tB = world.getComponent<Transform>(b);
                auto& cB = world.getComponent<ColliderComponent>(b);
                if (cB.shape != ColliderComponent::Shape::Sphere) continue;

                glm::vec3 n; float depth;
                if (sphereVsSphere(colliderCenter(tA, cA), cA.radius,
                    colliderCenter(tB, cB), cB.radius, n, depth)) {
                    glm::vec3 push = n * (depth * 0.5f);
                    tA.translation += push;
                    tB.translation -= push;

                    if (world.hasComponent<RigidbodyComponent>(a) &&
                        world.hasComponent<RigidbodyComponent>(b)) {
                        auto& rbA = world.getComponent<RigidbodyComponent>(a);
                        auto& rbB = world.getComponent<RigidbodyComponent>(b);
                        float vnA = glm::dot(rbA.velocity, n);
                        float vnB = glm::dot(rbB.velocity, -n);
                        if (vnA < 0.f) rbA.velocity -= n * vnA;
                        if (vnB < 0.f) rbB.velocity += n * vnB;
                    }
                }
            }
        }
    }



        // ============================================================
       // Симуляция одного шага
       // ============================================================

        inline void simulateStep(World & world, float fixedDt) {


            static int tick = 0;
            if (++tick % 60 == 0) {
                for (Entity e : world.getEntitiesWith<Transform, RigidbodyComponent, PlayerTag>()) {
                    auto& t = world.getComponent<Transform>(e);
                    auto& rb = world.getComponent<RigidbodyComponent>(e);
                    std::cout << "[tick=" << tick << "] "
                        << "pos=(" << t.translation.x << ", " << t.translation.y << ", " << t.translation.z << ") "
                        << "vel=(" << rb.velocity.x << ", " << rb.velocity.y << ", " << rb.velocity.z << ") "
                        << "grounded=" << rb.isGrounded << "\n";
                }
            }


           











            // --- 1. Гравитация + интеграция скорости ---
            for (Entity e : world.getEntitiesWith<Transform, RigidbodyComponent, InputCommand, PlayerMovement>()) {
                auto& t = world.getComponent<Transform>(e);
                auto& rb = world.getComponent<RigidbodyComponent>(e);
                auto& cmd = world.getComponent<InputCommand>(e);
                auto& pm = world.getComponent<PlayerMovement>(e);


                float yaw = t.rotation.y;
                glm::vec3 forward{ std::sin(yaw), 0.f, std::cos(yaw) };
                glm::vec3 right{ forward.z, 0.f, -forward.x };

                glm::vec3 wish = forward * cmd.forward + right * cmd.right;
                if (glm::dot(wish, wish) > 1e-6f) {
                    wish = glm::normalize(wish) * pm.walkSpeed;
                }

                rb.velocity.x = wish.x;
                rb.velocity.z = wish.z;

                if (cmd.jump && rb.isGrounded) {
                    rb.velocity.y = pm.jumpSpeed;
                    rb.isGrounded = false;
                }
                cmd.jump = false;   // съедаем прыжок (один тик)
            }


            // --- 1. Гравитация + интеграция ---
            for (Entity e : world.getEntitiesWith<Transform, RigidbodyComponent>()) {
                auto& t = world.getComponent<Transform>(e);
                auto& rb = world.getComponent<RigidbodyComponent>(e);
                if (rb.isKinematic) continue;
                if (rb.useGravity) {
                    float g = 18.0f;
                    if (world.hasComponent<PlayerMovement>(e))
                        g = world.getComponent<PlayerMovement>(e).gravity;
                    rb.velocity.y -= g * fixedDt;
                }
                t.translation += rb.velocity * fixedDt;
                rb.isGrounded = false;   // сбрасываем перед проверкой коллизий
            }
            // --- 2. Разрешение коллизий (итеративно, 2 прохода) ---
            // 2 прохода нужны: после выталкивания из пола игрок может
            // оказаться в стене — второй проход поправит.
            constexpr int COLLISION_ITERATIONS = 2;
            for (int iter = 0; iter < COLLISION_ITERATIONS; ++iter) {
                resolveCollisions(world);
            }

            // --- 3. Вращение (как раньше) ---
            for (Entity e : world.getEntitiesWith<Transform, AngularVelocity>()) {
                auto& t = world.getComponent<Transform>(e);
                auto& av = world.getComponent<AngularVelocity>(e);
                t.rotation += av.value * fixedDt;
                t.rotation.x = wrapAngle(t.rotation.x);
                t.rotation.y = wrapAngle(t.rotation.y);
                t.rotation.z = wrapAngle(t.rotation.z);
            }

            // --- 4. Синхронизация камеры с физикой ---
            for (Entity e : world.getEntitiesWith<Transform, CameraComponent, ActiveCameraTag>()) {
                auto& t = world.getComponent<Transform>(e);
                auto& c = world.getComponent<CameraComponent>(e);
              
                // 3rd person: камера сзади и выше
                glm::vec3 forward{
                    std::sin(t.rotation.y) * std::cos(t.rotation.x),
                    std::sin(t.rotation.x),
                    std::cos(t.rotation.y) * std::cos(t.rotation.x)
                };
 
               glm::vec3 eyePos = t.translation + glm::vec3(0, c.cameraHeightOffset, 0);
               glm::vec3 camPos = eyePos - forward * c.distance;
              c.camera.setViewYXZ(camPos, t.rotation);
              // c.camera.setViewYXZ(t.translation, t.rotation);

              



            }
           
        }



       
    
}