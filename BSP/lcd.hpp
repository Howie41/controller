#pragma once

#include "fmc.h"

#include <cstddef>
#include <cstdint>
#include <initializer_list>

/**
 * @brief 320x480、16 位 8080 接口 LCD 驱动（RGB565）。
 *
 * 依赖 CubeMX 先完成 MX_GPIO_Init() 和 MX_FMC_Init()。命令/数据地址由
 * NE1=0x60000000、DCX=FMC_A16、16 位总线确定。所有写入都是同步阻塞的。
 * 此类不拥有 FMC 句柄，也不管理 TIM3 背光 PWM。
 */
class Lcd {
public:
    static constexpr std::uint16_t width = 320;
    static constexpr std::uint16_t height = 480;

    explicit Lcd(SRAM_HandleTypeDef& fmc) : fmc_(fmc) {}

    /** @brief 复位并运行供应商例程的初始化序列；FMC 必须已初始化。 */
    bool init() {
        initialized_ = false;
        if (fmc_.Instance != FMC_NORSRAM_DEVICE ||
            fmc_.Init.NSBank != FMC_NORSRAM_BANK1 ||
            fmc_.Init.MemoryDataWidth != FMC_NORSRAM_MEM_BUS_WIDTH_16 ||
            HAL_SRAM_GetState(&fmc_) != HAL_SRAM_STATE_READY) {
            return false;
        }

        HAL_GPIO_WritePin(LCD_RESET_GPIO_Port, LCD_RESET_Pin, GPIO_PIN_SET);
        HAL_Delay(20);
        HAL_GPIO_WritePin(LCD_RESET_GPIO_Port, LCD_RESET_Pin, GPIO_PIN_RESET);
        HAL_Delay(20);
        HAL_GPIO_WritePin(LCD_RESET_GPIO_Port, LCD_RESET_Pin, GPIO_PIN_SET);
        HAL_Delay(200);

        command(0x11); // Sleep Out
        HAL_Delay(120);

        // 来源：随屏提供的 16 位并口例程 TFT_Init()。
        // 该例程目录名为 ST7365，但源码注释称 ST7796；参数仍需实物验证。
        // 原例程的 E8 参数不符合 ST7365P 手册第 9.3.23 节的固定字段，故不写 E8。
        command_data(0xF0, {0xC3});
        command_data(0xF0, {0x96});
        command_data(0x36, {0x48}); // MV=0（320x480）、MX=1、BGR=1；沿用屏幕例程
        command_data(0x3A, {0x05}); // 16 位 8080 控制接口的 RGB565
        command_data(0xC5, {0x27});
        command_data(0xC2, {0xA7});
        command_data(0xE0, {0xF0, 0x01, 0x06, 0x0F, 0x12, 0x1D, 0x36,
                            0x54, 0x44, 0x0C, 0x18, 0x16, 0x13, 0x15});
        command_data(0xE1, {0xF0, 0x01, 0x05, 0x0A, 0x0B, 0x07, 0x32,
                            0x44, 0x44, 0x0C, 0x18, 0x17, 0x13, 0x16});
        command_data(0xF0, {0x3C});
        command_data(0xF0, {0x69});
        command(0x20); // Display Inversion Off
        HAL_Delay(20);
        command(0x29); // Display On

        initialized_ = true;
        return true;
    }

    /**
     * @brief 设置闭区间窗口并发送 Memory Write 命令。
     * @return 坐标有效且已初始化时返回 true。
     */
    bool set_window(std::uint16_t x1, std::uint16_t y1,
                    std::uint16_t x2, std::uint16_t y2) {
        if (!initialized_ || x1 > x2 || y1 > y2 || x2 >= width || y2 >= height) {
            return false;
        }
        command_data(0x2A, {static_cast<std::uint8_t>(x1 >> 8),
                            static_cast<std::uint8_t>(x1),
                            static_cast<std::uint8_t>(x2 >> 8),
                            static_cast<std::uint8_t>(x2)});
        command_data(0x2B, {static_cast<std::uint8_t>(y1 >> 8),
                            static_cast<std::uint8_t>(y1),
                            static_cast<std::uint8_t>(y2 >> 8),
                            static_cast<std::uint8_t>(y2)});
        command(0x2C);
        return true;
    }

    /**
     * @brief 向当前窗口连续写入 RGB565 像素。
     * @note 调用前须先 set_window()，且 count 不得超过窗口剩余像素数。
     *       传输期间调用者应避免其他任务操作同一块 LCD。
     */
    void write_pixels(const std::uint16_t* pixels, std::size_t count) {
        if (pixels == nullptr) {
            return;
        }
        for (std::size_t i = 0; i < count; ++i) {
            data(pixels[i]);
        }
    }

    /** @brief 将连续、逐行排列的 RGB565 像素同步写入指定闭区间。 */
    bool write_area(std::uint16_t x1, std::uint16_t y1,
                    std::uint16_t x2, std::uint16_t y2,
                    const std::uint16_t* pixels) {
        if (pixels == nullptr || !set_window(x1, y1, x2, y2)) {
            return false;
        }
        const std::size_t count = static_cast<std::size_t>(x2 - x1 + 1U) *
                                  static_cast<std::size_t>(y2 - y1 + 1U);
        write_pixels(pixels, count);
        return true;
    }

    /** @brief 用单一 RGB565 颜色填充指定闭区间。 */
    bool fill_area(std::uint16_t x1, std::uint16_t y1,
                   std::uint16_t x2, std::uint16_t y2,
                   std::uint16_t color) {
        if (!set_window(x1, y1, x2, y2)) {
            return false;
        }
        const std::size_t count = static_cast<std::size_t>(x2 - x1 + 1U) *
                                  static_cast<std::size_t>(y2 - y1 + 1U);
        for (std::size_t i = 0; i < count; ++i) {
            data(color);
        }
        return true;
    }

    bool clear(std::uint16_t color) {
        return fill_area(0, 0, width - 1U, height - 1U, color);
    }

private:
    static constexpr std::uintptr_t command_address = 0x60000000U;
    static constexpr std::uintptr_t data_address = 0x60020000U;

    SRAM_HandleTypeDef& fmc_;
    bool initialized_ = false;

    static void command(std::uint16_t value) {
        *reinterpret_cast<volatile std::uint16_t*>(command_address) = value;
    }

    static void data(std::uint16_t value) {
        *reinterpret_cast<volatile std::uint16_t*>(data_address) = value;
    }

    static void command_data(std::uint8_t cmd,
                             std::initializer_list<std::uint8_t> values) {
        command(cmd);
        for (std::uint8_t value : values) {
            data(value);
        }
    }
};
