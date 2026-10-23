#include "keyboard_movement_controller.hpp"

namespace lve {

    // 1. Метод управляет только перемещением камеры (WASD / EQ)
    void KeyboardMovementController::moveCamera(HWND hwnd, float dt, TransformComponent& cameraTransform) {
        if (GetForegroundWindow() != hwnd) return;

        // Камера сохраняет плавное смещение по направлению взгляда
        float yaw = cameraTransform.rotation.y;
        const glm::vec3 forwardDir{ glm::sin(yaw), 0.f, glm::cos(yaw) };
        const glm::vec3 rightDir{ forwardDir.z, 0.f, -forwardDir.x };
        const glm::vec3 upDir{ 0.f, -1.f, 0.f }; // В Vulkan ось Y направлена вниз

        glm::vec3 moveDir{ 0.f };
        if (GetAsyncKeyState(keys.moveForward) & 0x8000)  moveDir += forwardDir;
        if (GetAsyncKeyState(keys.moveBackward) & 0x8000) moveDir -= forwardDir;
        if (GetAsyncKeyState(keys.moveRight) & 0x8000)    moveDir += rightDir;
        if (GetAsyncKeyState(keys.moveLeft) & 0x8000)     moveDir -= rightDir;
        if (GetAsyncKeyState(keys.moveUp) & 0x8000)       moveDir += upDir;
        if (GetAsyncKeyState(keys.moveDown) & 0x8000)     moveDir -= upDir;

        if (glm::dot(moveDir, moveDir) > std::numeric_limits<float>::epsilon()) {
            cameraTransform.translation += moveSpeed * dt * glm::normalize(moveDir);
        }
    }

    // 2. Метод управляет только вращением куба (Стрелочки)
    void KeyboardMovementController::rotateCube(HWND hwnd, float dt, TransformComponent& cubeTransform) {
        if (GetForegroundWindow() != hwnd) return;

        // Построчно наращиваем углы Эйлера для куба без нормализации векторов
        if (GetAsyncKeyState(keys.lookRight) & 0x8000) cubeTransform.rotation.y += turnSpeed * dt;
        if (GetAsyncKeyState(keys.lookLeft) & 0x8000)  cubeTransform.rotation.y -= turnSpeed * dt;
        if (GetAsyncKeyState(keys.lookUp) & 0x8000)    cubeTransform.rotation.x += turnSpeed * dt;
        if (GetAsyncKeyState(keys.lookDown) & 0x8000)  cubeTransform.rotation.x -= turnSpeed * dt;

        // Ограничиваем вращение по X, чтобы куб не переворачивался "вверх ногами"
       // cubeTransform.rotation.x = fmod(cubeTransform.rotation.x, -1.5f, 1.5f);

      //  cubeTransform.rotation.x = fmod(cubeTransform.rotation.x, glm::two_pi<float>());
        cubeTransform.rotation.x = glm::clamp(cubeTransform.rotation.x, -1.5f, 1.5f);
        // Предотвращаем переполнение переменной угла (сброс при 360 градусах / 2PI)
        cubeTransform.rotation.y = fmod(cubeTransform.rotation.y, glm::two_pi<float>());
    }
   

    void KeyboardMovementController::rotateCamera(HWND hwnd, float dt, TransformComponent& cameraTransform) {
        if (GetForegroundWindow() != hwnd) return;

        if (GetAsyncKeyState(keys.lookRight) & 0x8000) cameraTransform.rotation.y += turnSpeed * dt;
        if (GetAsyncKeyState(keys.lookLeft) & 0x8000) cameraTransform.rotation.y -= turnSpeed * dt;
        if (GetAsyncKeyState(keys.lookUp) & 0x8000) cameraTransform.rotation.x += turnSpeed * dt;
        if (GetAsyncKeyState(keys.lookDown) & 0x8000) cameraTransform.rotation.x -= turnSpeed * dt;

        // Ограничиваем pitch, чтобы камера не переворачивалась
        cameraTransform.rotation.x = glm::clamp(cameraTransform.rotation.x, -1.5f, 1.5f);
        cameraTransform.rotation.y = fmod(cameraTransform.rotation.y, glm::two_pi<float>());
    }
}