

#pragma once
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include "../lve_model.hpp"

namespace ecs {

    // Трансформация в мире (замена lve::TransformComponent)
    struct Transform {
        glm::vec3 translation{ 0.f };
        glm::vec3 scale{ 1.f, 1.f, 1.f };
        glm::vec3 rotation{ 0.f };

        glm::mat4 mat4() const {
            glm::mat4 m = glm::translate(glm::mat4{ 1.0f }, translation);
            m = glm::rotate(m, rotation.y, glm::vec3{ 0.f, 1.f, 0.f });
            m = glm::rotate(m, rotation.x, glm::vec3{ 1.f, 0.f, 0.f });
            m = glm::rotate(m, rotation.z, glm::vec3{ 0.f, 0.f, 1.f });
            return glm::scale(m, scale);
        }

        glm::mat3 normalMatrix() const {
            const float c3 = glm::cos(rotation.z);
            const float s3 = glm::sin(rotation.z);
            const float c2 = glm::cos(rotation.x);
            const float s2 = glm::sin(rotation.x);
            const float c1 = glm::cos(rotation.y);
            const float s1 = glm::sin(rotation.y);
            const glm::vec3 invScale = 1.0f / scale;
            return glm::mat3{
                glm::vec3{ invScale.x * (c1 * c3 + s1 * s2 * s3), invScale.x * (c3 * s1 * s2 - c1 * s3), invScale.x * (c2 * s1) },
                glm::vec3{ invScale.y * (c2 * s3),              invScale.y * (c2 * c3),              invScale.y * (-s2) },
                glm::vec3{ invScale.z * (c1 * s2 * s3 - c3 * s1), invScale.z * (c1 * c3 * s2 + s1 * s3), invScale.z * (c1 * c2) }
            };
        }
    };

    // Ссылка на модель (не владеет — модель живёт в LveModel)
    struct Mesh {
        lve::LveModel* model = nullptr;
    };

    // Источник света (данные для UBO + анимация)
    struct PointLightComponent {
        glm::vec3 color{ 1.f, 1.f, 1.f };
        float intensity = 2.f;
        float radius = 1.5f;
        float speed = 0.8f;
        float phase = 0.f;
        bool animated = true;
    };

    // Маркер «это биллборд» (рисуется отдельным пайплайном)
    struct Billboard {
        glm::vec3 color{ 1.f, 1.f, 1.f };
        float scale = 0.15f;
    };

    // Скорость (пока не используется, но пригодится)
    struct Velocity {
        glm::vec3 value{ 0.f };
    };
    // Угловая скорость (радиан/сек)
    struct AngularVelocity {
        glm::vec3 value{ 0.f };
    };

    // НОВОЕ: Камера как компонент ECS
    struct CameraComponent {
        lve::LveCamera camera;
        float fovRadians = glm::radians(50.f);
        float aspect = 16.f / 9.f;
        float nearZ = 0.1f;
        float farZ = 10.f;
        
        // === Зум (3rd person) ===
        float distance = 0.0f;         // 0 = 1st person (камера "в глазах")
        float minDistance = 0.0f;
        float maxDistance = 15.0f;
        float zoomStep = 0.5f;          // шаг для клавиш
        float zoomWheelSensitivity = 0.5f;  // для колеса мыши
        float cameraHeightOffset = 0.5f;    // подъём камеры над "глазами"

        bool freeCamera = false;

        void updateProjection() {
            camera.setPerspectiveProjection(fovRadians, aspect, nearZ, farZ);
        }
    };

    // НОВОЕ: Маркер "это активная камера"
    struct ActiveCameraTag {};

    // НОВОЕ: Компонент для управления вводом (хранит хэндл окна)
    struct InputContext {
        HWND hwnd = nullptr;
    };

    // НОВОЕ: Маркер "это управляемая сущность (игрок)"
    struct PlayerTag {};

    // Твёрдое тело: скорость, масса, гравитация
    struct RigidbodyComponent {
        glm::vec3 velocity{ 0.f };      // м/с (мировая скорость)
        float    mass = 1.0f;
        bool     useGravity = true;
        bool     isKinematic = false;   // true = управляется вручную (игрок)
        bool     isGrounded = false;    // касается ли пола в этом тике
    };

    // Коллайдер: сфера или AABB
    struct ColliderComponent {
        enum class Shape : uint8_t { Sphere, AABB };

        Shape shape = Shape::Sphere;
        float radius = 0.5f;              // для Sphere
        glm::vec3 halfExtents{ 0.5f };    // для AABB (половина размера)
        // Смещение коллайдера относительно Transform.translation
        // (например, для игрока — по центру тела)
        glm::vec3 offset{ 0.f };
    };

    // Игрок: кто управляется вводом
    // (у вас уже есть PlayerTag — используем его)
    // Но добавим компонент для скорости ходьбы/прыжка:
    struct PlayerMovement {
        float walkSpeed = 5.0f;   // м/с
        float jumpSpeed = 5.0f;   // м/с (импульс при прыжке)
        float gravity = 18.0f;  // м/с² (сильнее реальной — «игровая»)
    };

    struct InputCommand {
        float forward = 0.f;   // -1..1
        float right = 0.f;   // -1..1
        bool  jump = false;
    };
}