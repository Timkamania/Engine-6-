#pragma once

#include "transform_component.hpp"
#include <windows.h>
#include <glm/glm.hpp>
#include <glm/common.hpp> 
#include <glm/gtc/constants.hpp>
namespace lve {
    class KeyboardMovementController {
    public:
        struct KeyMappings {
            int moveLeft = 'A';
            int moveRight = 'D';
            int moveForward = 'W';
            int moveBackward = 'S';
            int moveUp = 'E';
            int moveDown = 'Q';
            int lookLeft = VK_LEFT;
            int lookRight = VK_RIGHT;
            int lookUp = VK_UP;
            int lookDown = VK_DOWN;
        };

        KeyMappings keys{};
        float moveSpeed{ 3.f };
        float turnSpeed{ 1.5f };

       

        // Разделяем один старый метод на два независимых
        void moveCamera(HWND hwnd, float dt, TransformComponent& cameraTransform);
        void rotateCube(HWND hwnd, float dt, TransformComponent& cubeTransform);

        // Стало:
        void rotateCamera(HWND hwnd, float dt, TransformComponent& cameraTransform);
    };
}