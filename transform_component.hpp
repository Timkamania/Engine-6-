#pragma once
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
namespace lve {
    struct TransformComponent {
        glm::vec3 translation{}; // Позиция (X, Y, Z)
        glm::vec3 scale{ 1.f, 1.f, 1.f };
        glm::vec3 rotation{};    // Углы поворота (Pitch, Yaw, Roll)




        // Матрица модели 4x4 (Каноничный расчет)
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

            const glm::vec3 invScale = 1.0f / scale; // Взаимно-обратный масштаб

            return glm::mat3{
                glm::vec3{ invScale.x * (c1 * c3 + s1 * s2 * s3), invScale.x * (c3 * s1 * s2 - c1 * s3), invScale.x * (c2 * s1) },
                glm::vec3{ invScale.y * (c2 * s3), invScale.y * (c2 * c3), invScale.y * (-s2) },
                glm::vec3{ invScale.z * (c1 * s2 * s3 - c3 * s1), invScale.z * (c1 * c3 * s2 + s1 * s3), invScale.z * (c1 * c2) }
            };
        }

    };



}