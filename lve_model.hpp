#pragma once

#include "lve_device.hpp"

// Подключаем GLM для работы с векторами (если у вас его нет, можно заменить на свои структуры)
#define GLM_FORCE_RADIANS
#define GLM_FORCE_DEPTH_ZERO_TO_ONE
#include <glm/glm.hpp>
#include <vector>
#include "lve_buffer.hpp"

namespace lve {

    class LveModel {
    public:
        // Структура, описывающая одну вершину
        struct Vertex {
            glm::vec3 position; // 8 байт (X, Y)
            glm::vec3 color;    // 12 байт (R, G, B)
            glm::vec3 normal{}; // Добавлено из урока 17
            glm::vec2 uv{};

            // Функции, которые расскажут Vulkan о разметке нашей памяти
            static std::vector<VkVertexInputBindingDescription> getBindingDescriptions();
            static std::vector<VkVertexInputAttributeDescription> getAttributeDescriptions();

            // Оператор сравнения для unordered_map (Добавлено)
            bool operator==(const Vertex& other) const {
                return position == other.position && color == other.color &&
                    normal == other.normal && uv == other.uv;
            }
        };

        struct Builder {
            std::vector<Vertex> vertices{};
            std::vector<uint32_t> indices{};

            void loadModel(const std::string& filepath); // Метод загрузки (Добавлено)
        };

        // Конструктор принимает устройство и вектор вершин
        LveModel(LveDevice& device,  const LveModel::Builder& builder);
        ~LveModel();


        // Статический метод-фабрика для удобного создания моделей (Добавлено)
        static std::unique_ptr<LveModel> createModelFromFile(LveDevice& device, const std::string& filepath);

        // Запрещаем копирование объекта модели, чтобы случайно не размножить буферы в видеопамяти
        LveModel(const LveModel&) = delete;
        LveModel& operator=(const LveModel&) = delete;

        void bind(VkCommandBuffer commandBuffer); // Связать буфер с конвейером команд
        void draw(VkCommandBuffer commandBuffer); // Отправить команду отрисовки

    private:
        void createVertexBuffers(const std::vector<Vertex>& vertices);
        void createIndexBuffers(const std::vector<uint32_t>& indices);
        LveDevice& lveDevice;       // Ссылка на наше устройство Vulkan

        std::unique_ptr<LveBuffer> vertexBuffer;
       // VkBuffer vertexBuffer = VK_NULL_HANDLE;       // Дескриптор буфера
       // VkDeviceMemory vertexBufferMemory = VK_NULL_HANDLE; // Дескриптор выделенной под него памяти GPU
        uint32_t vertexCount = 0;   // Сколько всего вершин в модели


        bool hasIndexBuffer = false;
        std::unique_ptr<LveBuffer> indexBuffer;
        uint32_t indexCount = 0;

        
    };

} // namespace lve