#pragma once

#include <array>
#include <cstddef>
#include <type_traits>

/**
 * @brief DMA 缓冲区
 * @attention 使用时务必放在 DMA 可读写的内存段
 * - 在这个项目中，用 `DMA_BUFFER_BSS` 宏修饰
 */
template<typename T, std::size_t Length>
class alignas(32) DmaBuffer {
    static_assert(Length > 0, "A DMA buffer cannot be empty");
    static_assert(std::is_trivially_copyable_v<T>,
                  "DMA buffer elements must be trivially copyable");

public:
    using value_type = T;

    DmaBuffer() = default;
    DmaBuffer(const DmaBuffer&) = delete;
    DmaBuffer& operator=(const DmaBuffer&) = delete;

    [[nodiscard]] volatile T* data() noexcept {
        return m_data.data();
    }

    [[nodiscard]] const volatile T* data() const noexcept {
        return m_data.data();
    }

    [[nodiscard]] static constexpr std::size_t size() noexcept {
        return Length;
    }

    [[nodiscard]] volatile T& operator[](std::size_t index) noexcept {
        return m_data[index];
    }

    [[nodiscard]] const volatile T& operator[](std::size_t index) const noexcept {
        return m_data[index];
    }

private:
    std::array<volatile T, Length> m_data;
};
