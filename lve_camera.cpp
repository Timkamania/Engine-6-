#include "lve_camera.hpp"
#include <cassert>
#include <limits>
#include <glm/glm.hpp>                  // Основные типы (vec3, mat4)
#include <glm/gtc/matrix_transform.hpp>


namespace lve {

    void LveCamera::setOrthographicProjection(
        float left, float right, float top, float bottom, float near, float far) {

        projectionMatrix = glm::mat4{ 1.0f };
        // Матрица ортографии, адаптированная под систему координат Vulkan (Z: 0 to 1)
        projectionMatrix[0][0] = 2.f / (right - left);
        projectionMatrix[1][1] = 2.f / (bottom - top);
        projectionMatrix[2][2] = 1.f / (far - near);
        projectionMatrix[3][0] = -(right + left) / (right - left);
        projectionMatrix[3][1] = -(bottom + top) / (bottom - top);
        projectionMatrix[3][2] = -near / (far - near);
   
       
        
    }
    

    void LveCamera::setPerspectiveProjection(float fovy, float aspect, float near, float far) {
        assert(glm::abs(aspect - std::numeric_limits<float>::epsilon()) > 0.0f);

        const float c = 1.f / glm::tan(fovy / 2.f);

        projectionMatrix = glm::mat4{ 0.0f };
        // Математика вывода перспективной матрицы из урока 13 для Vulkan
        projectionMatrix[0][0] = c / aspect;
        projectionMatrix[1][1] = c;
        projectionMatrix[2][2] = far / (far - near);
        projectionMatrix[2][3] = 1.f;
        projectionMatrix[3][2] = -(far * near) / (far - near);

        // ✅ Vulkan Y-флип: NDC Y идёт вниз, а у нас мир Y идёт вверх
        projectionMatrix[1][1] *= -1.0f;   // ← ДОБАВИТЬ
    }

    // 1. Позиционирование по вектору направления взгляда
    void LveCamera::setViewDirection(glm::vec3 position, glm::vec3 direction, glm::vec3 up) {
        /*
        // Формируем ортонормальный базис камеры (оси пространства камеры)
        const glm::vec3 w{ glm::normalize(direction) };
        const glm::vec3 u{ glm::normalize(glm::cross(w, up)) };
        const glm::vec3 v{ glm::cross(w, u) };

        viewMatrix = glm::mat4{ 1.f };
        viewMatrix[0][0] = u.x; viewMatrix[1][0] = u.y; viewMatrix[2][0] = u.z;
        viewMatrix[0][1] = v.x; viewMatrix[1][1] = v.y; viewMatrix[2][1] = v.z;
        viewMatrix[0][2] = w.x; viewMatrix[1][2] = w.y; viewMatrix[2][2] = w.z;
        viewMatrix[3][0] = -glm::dot(u, position);
        viewMatrix[3][1] = -glm::dot(v, position);
        viewMatrix[3][2] = -glm::dot(w, position);
        */

        viewMatrix = glm::lookAt(position, position + direction, up);
    }


    // 2. Позиционирование с жесткой фиксацией на 3D-точке/объекте
    void LveCamera::setViewTarget(glm::vec3 position, glm::vec3 target, glm::vec3 up) {
        /*
        setViewDirection(position, target - position, up); // Направление — это вектор от камеры к цели
        */

        viewMatrix = glm::lookAt(position, target, up);
    }

    
    // 3. Позиционирование через углы Эйлера (вращение мыши в шутерах)
    void LveCamera::setViewYXZ(glm::vec3 position, glm::vec3 rotation) {
        /*
        const float c3 = glm::cos(rotation.z);
        const float s3 = glm::sin(rotation.z);
        const float c2 = glm::cos(rotation.x);
        const float s2 = glm::sin(rotation.x);
        const float c1 = glm::cos(rotation.y);
        const float s1 = glm::sin(rotation.y);

        const glm::vec3 u{ (c1 * c3 + s1 * s2 * s3), (c3 * s1 * s2 - c1 * s3), (c2 * s1) };
        const glm::vec3 v{ (c2 * s3), (c2 * c3), (-s2) };
        const glm::vec3 w{ (c1 * s2 * s3 - c3 * s1), (c1 * c3 * s2 + s1 * s3), (c1 * c2) };

        viewMatrix = glm::mat4{ 1.f };
        viewMatrix[0][0] = u.x; viewMatrix[1][0] = u.y; viewMatrix[2][0] = u.z;
        viewMatrix[0][1] = v.x; viewMatrix[1][1] = v.y; viewMatrix[2][1] = v.z;
        viewMatrix[0][2] = w.x; viewMatrix[1][2] = w.y; viewMatrix[2][2] = w.z;
        viewMatrix[3][0] = -glm::dot(u, position);
        viewMatrix[3][1] = -glm::dot(v, position);
        viewMatrix[3][2] = -glm::dot(w, position);

        */


        // Создаем единичную матрицу
        glm::mat4 R = glm::mat4(1.0f);

        // Вращаем ее строго в порядке Y -> X -> Z (как заложено в архитектуре LVE)
        R = glm::rotate(R, rotation.z, glm::vec3(0.0f, 0.0f, 1.0f)); // Roll
        R = glm::rotate(R, rotation.x, glm::vec3(1.0f, 0.0f, 0.0f)); // Pitch
        R = glm::rotate(R, rotation.y, glm::vec3(0.0f, 1.0f, 0.0f)); // Yaw

        // Сдвигаем мир в противоположную от камеры сторону (Инвертированный сдвиг)
        glm::mat4 T = glm::translate(glm::mat4(1.0f), -position);

        // Итоговая матрица вида — это комбинация вращения и сдвига
        viewMatrix = R * T;
    }

    
    /*
    void LveCamera::setViewYXZ(glm::vec3 position, glm::vec3 rotation) {
        glm::vec3 forward = {
            glm::cos(rotation.x) * glm::sin(rotation.y),
            glm::sin(rotation.x),
            glm::cos(rotation.x) * glm::cos(rotation.y)
        };
        viewMatrix = glm::lookAt(position, position + forward, glm::vec3(0.f, 1.f, 0.f));
    }
    */

} // namespace lve