
#include "lve_model.hpp"
#include <cassert>
#include <cstring>
#include <unordered_map>
#include <iostream>

#define TINYOBJLOADER_DISABLE_FAST_FLOAT
#define TINYOBJLOADER_IMPLEMENTATION

#include "tiny_obj_loader.h"

#define GLM_ENABLE_EXPERIMENTAL
#include <glm/gtx/hash.hpp>
#include "lve_utils.hpp"



// Внедряем хэширование структуры Vertex в пространство std
namespace std {
    template <>
    struct hash<lve::LveModel::Vertex> {
        size_t operator()(lve::LveModel::Vertex const& vertex) const {
            size_t seed = 0;
            lve::hashCombine(seed, vertex.position, vertex.color, vertex.normal, vertex.uv);
            return seed;
        }
    };
} // namespace std

namespace lve {
    // Реализация фабрики
    std::unique_ptr<LveModel> LveModel::createModelFromFile(LveDevice& device, const std::string& filepath) {
        Builder builder{};
        builder.loadModel(filepath);
        std::cout << "Успешно загружена модель: " << filepath << ", вершин: " << builder.vertices.size() << "\n";
        return std::make_unique<LveModel>(device, builder);
    }

    // Алгоритм чтения .obj файла
    void LveModel::Builder::loadModel(const std::string& filepath) {
        tinyobj::attrib_t attrib;
        std::vector<tinyobj::shape_t> shapes;
        std::vector<tinyobj::material_t> materials;
        std::string warn, err;

        if (!tinyobj::LoadObj(&attrib, &shapes, &materials, &warn, &err, filepath.c_str())) {
            throw std::runtime_error(warn + err);
        }

        vertices.clear();
        indices.clear();

        std::unordered_map<Vertex, uint32_t> uniqueVertices{};

        for (const auto& shape : shapes) {
            for (const auto& index : shape.mesh.indices) {
                Vertex vertex{};

                if (index.vertex_index >= 0) {
                    vertex.position = {
                        attrib.vertices[3 * index.vertex_index + 0],
                        attrib.vertices[3 * index.vertex_index + 1],
                        attrib.vertices[3 * index.vertex_index + 2]
                    };

                    // Поддержка неофициального расширения цветных вершин (.obj изначально их не хранит)
                   if (3 * index.vertex_index + 2 < attrib.colors.size()) {
                        vertex.color = {
                            attrib.colors[3 * index.vertex_index + 0],
                            attrib.colors[3 * index.vertex_index + 1],
                            attrib.colors[3 * index.vertex_index + 2]
                        };
                    }
                    else {
                       vertex.color = { 1.0f, 1.0f, 1.0f }; // По умолчанию белый цвет
                    }
                }

                if (index.normal_index >= 0) {
                    vertex.normal = {
                        attrib.normals[3 * index.normal_index + 0],
                        attrib.normals[3 * index.normal_index + 1],
                        attrib.normals[3 * index.normal_index + 2]
                    };
                }

                if (index.texcoord_index >= 0) {
                    vertex.uv = {
                        attrib.texcoords[2 * index.texcoord_index + 0],
                        attrib.texcoords[2 * index.texcoord_index + 1]
                    };
                }

                // Склеивание дубликатов с помощью хэш-карты
                if (uniqueVertices.count(vertex) == 0) {
                    uniqueVertices[vertex] = static_cast<uint32_t>(vertices.size());
                    vertices.push_back(vertex);
                }
                indices.push_back(uniqueVertices[vertex]);
            }
        }
    }
 

    LveModel::LveModel(LveDevice& device, const LveModel::Builder& builder) : lveDevice{ device } {
        createVertexBuffers(builder.vertices);
        createIndexBuffers(builder.indices);
    }

    LveModel::~LveModel() {
     
    }
    // 2. Обновленный метод создания вершинного буфера в lve_model.cpp
    void LveModel::createVertexBuffers(const std::vector<Vertex>& vertices) {
        vertexCount = static_cast<uint32_t>(vertices.size());
        assert(vertexCount >= 3 && "Модель должна содержать как минимум 3 вершины!");

        VkDeviceSize instanceSize = sizeof(Vertex); // Явно берем размер структуры Vertex
        VkDeviceSize bufferSize = instanceSize * vertexCount;

        LveBuffer stagingBuffer{
            lveDevice,
            instanceSize,
            vertexCount,
            VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
            VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT
        };

        stagingBuffer.map();
        stagingBuffer.writeToBuffer((void*)vertices.data());

        vertexBuffer = std::make_unique<LveBuffer>(
            lveDevice,
            instanceSize,
            vertexCount,
            VK_BUFFER_USAGE_VERTEX_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT,
            VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT
        );

        lveDevice.copyBuffer(stagingBuffer.getBuffer(), vertexBuffer->getBuffer(), bufferSize);
    }

    // 3. Обновленный метод создания индексного буфера в lve_model.cpp
    void LveModel::createIndexBuffers(const std::vector<uint32_t>& indices) {
        indexCount = static_cast<uint32_t>(indices.size());
        hasIndexBuffer = indexCount > 0;
        if (!hasIndexBuffer) return;

        VkDeviceSize instanceSize = sizeof(uint32_t); // Явно задаем 4 байта для каждого индекса
        VkDeviceSize bufferSize = instanceSize * indexCount;

        LveBuffer stagingBuffer{
            lveDevice,
            instanceSize,
            indexCount,
            VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
            VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT
        };

        stagingBuffer.map();
        stagingBuffer.writeToBuffer((void*)indices.data());

        indexBuffer = std::make_unique<LveBuffer>(
            lveDevice,
            instanceSize,
            indexCount,
            VK_BUFFER_USAGE_INDEX_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT,
            VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT
        );

        lveDevice.copyBuffer(stagingBuffer.getBuffer(), indexBuffer->getBuffer(), bufferSize);
    }
    


    void LveModel::bind(VkCommandBuffer commandBuffer) {
        VkBuffer buffers[] = { vertexBuffer->getBuffer() };
        VkDeviceSize offsets[] = { 0 };
        vkCmdBindVertexBuffers(commandBuffer, 0, 1, buffers, offsets);

        if (hasIndexBuffer) {
            vkCmdBindIndexBuffer(commandBuffer, indexBuffer->getBuffer(), 0, VK_INDEX_TYPE_UINT32);
        }
    }

    void LveModel::draw(VkCommandBuffer commandBuffer) {
        // Отрисовываем ровно столько вершин, сколько было загружено
       // vkCmdDraw(commandBuffer, vertexCount, 1, 0, 0);

        if (hasIndexBuffer) {
            // ИСПРАВЛЕНО: Рисуем геометрию с помощью таблицы индексов
            vkCmdDrawIndexed(commandBuffer, indexCount, 1, 0, 0, 0);
        }
        else {
            // Фаллбэк на обычную отрисовку, если индексов нет
            vkCmdDraw(commandBuffer, vertexCount, 1, 0, 0);
        }
    }

    // РАЗМЕТКА: Описываем шаг (шаблон) прохода по массиву вершин
    std::vector<VkVertexInputBindingDescription> LveModel::Vertex::getBindingDescriptions() {
        std::vector<VkVertexInputBindingDescription> bindingDescriptions(1);
        bindingDescriptions[0].binding = 0;                          // Индекс привязки
        bindingDescriptions[0].stride = sizeof(Vertex);              // Расстояние между вершинами в байтах
        bindingDescriptions[0].inputRate = VK_VERTEX_INPUT_RATE_VERTEX; // Переходить к следующему элементу по вершинам
        return bindingDescriptions;
    }




    // Расширяем разметку атрибутов под новые Normal и UV входы шейдера
    std::vector<VkVertexInputAttributeDescription> LveModel::Vertex::getAttributeDescriptions() {
        std::vector<VkVertexInputAttributeDescription> attributeDescriptions(4); // Размер увеличен до 4

        attributeDescriptions[0].binding = 0;
        attributeDescriptions[0].location = 0;
        attributeDescriptions[0].format = VK_FORMAT_R32G32B32_SFLOAT;
        attributeDescriptions[0].offset = offsetof(Vertex, position);

        attributeDescriptions[1].binding = 0;
        attributeDescriptions[1].location = 1;
        attributeDescriptions[1].format = VK_FORMAT_R32G32B32_SFLOAT;
        attributeDescriptions[1].offset = offsetof(Vertex, color);

        // Добавлено: Атрибут нормалей (location = 2 в вершинном шейдере)
        attributeDescriptions[2].binding = 0;
        attributeDescriptions[2].location = 2;
        attributeDescriptions[2].format = VK_FORMAT_R32G32B32_SFLOAT;
        attributeDescriptions[2].offset = offsetof(Vertex, normal);

        // Добавлено: Атрибут текстурных координат (location = 3 в вершинном шейдере)
        attributeDescriptions[3].binding = 0;
        attributeDescriptions[3].location = 3;
        attributeDescriptions[3].format = VK_FORMAT_R32G32_SFLOAT; // vec2 — два float
        attributeDescriptions[3].offset = offsetof(Vertex, uv);

        return attributeDescriptions;
    }

} // namespace lve