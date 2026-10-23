#pragma once

#include <functional>

namespace lve {

    // Шаблонная функция объединения хэш-значений (C++17 fold expressions)
    template <typename T, typename... Rest>
    void hashCombine(std::size_t& seed, const T& v, const Rest&... rest) {
        std::hash<T> hasher;
        seed ^= hasher(v) + 0x9e3779b9 + (seed << 6) + (seed >> 2);
        (hashCombine(seed, rest), ...);
    }

} // namespace lve