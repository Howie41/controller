#pragma once

#include "asserter.hpp"
#include "main.h"
#include <cstddef>
#include <cstdint>


/**
 * @brief ADC DMA
 * 
 * @tparam BufferLength 目标缓冲区长度
 *
 * @attention AdcDma 实例应使用 DMA_BUFFER_BSS 放入 DMA 可访问内存
 * @attention DMA 数据宽度应该为默认值 Half word（16位）
 */
template <std::size_t BufferLength>
class AdcDma {
    static_assert(BufferLength > 0, "An ADC DMA buffer cannot be empty");

private:
    ADC_HandleTypeDef& m_adc_handle;
    volatile std::uint16_t m_adc_value_buffer[BufferLength]{};

public:
    explicit AdcDma(ADC_HandleTypeDef& adc_handle) : m_adc_handle(adc_handle) {}

    bool start() {
        auto calibration_result = HAL_ADCEx_Calibration_Start(
            &m_adc_handle,
            ADC_CALIB_OFFSET,
            ADC_SINGLE_ENDED
        );
        if (calibration_result != HAL_OK) return false;

        auto start_result = HAL_ADC_Start_DMA(
            &m_adc_handle,
            reinterpret_cast<std::uint32_t*>(
                const_cast<std::uint16_t*>(m_adc_value_buffer)
            ),
            BufferLength
        );
        if (start_result != HAL_OK) return false;

        (void) __HAL_DMA_DISABLE_IT(m_adc_handle.DMA_Handle, DMA_IT_HT);
        (void) __HAL_DMA_DISABLE_IT(m_adc_handle.DMA_Handle, DMA_IT_TC);
        return true;
    }

    [[nodiscard]] const volatile std::uint16_t& get_value_ref_at(std::size_t index) const {
        RUNTIME_ASSERT(index < BufferLength);
        return m_adc_value_buffer[index];
    }
};

class JoyStick {
public:
    const volatile std::uint16_t& m_horizon_value;
    const volatile std::uint16_t& m_vertical_value;

public:
    template<std::size_t BufferLength>
    JoyStick(
        AdcDma<BufferLength>& joystick_adc, 
        std::size_t horizon_adc_rank,
        std::size_t vertical_adc_rank
    ) : m_horizon_value(joystick_adc.get_value_ref_at(horizon_adc_rank - 1)),
        m_vertical_value(joystick_adc.get_value_ref_at(vertical_adc_rank - 1)) {}
};
