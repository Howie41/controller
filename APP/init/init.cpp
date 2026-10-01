#include "init.h"
#include "adc.h"
#include "dma_buffer.hpp"
#include "joystick.hpp"
#include "main.h"
#include "memory_map.h"
#include "rtos_queue.hpp"
#include "rtos_task.hpp"

DMA_BUFFER_BSS DmaBuffer<std::uint16_t, 4> joystick_adc_buffer{};
AdcDma<4> joystick_adc(hadc1, joystick_adc_buffer);
JoyStick left_joystick(joystick_adc, 1, 2);
JoyStick right_joystick(joystick_adc, 3, 4);

/**
 * @brief 初始化各种硬件软件
 * 在 RTOS 内核启动后执行
 */
extern "C" void init() {
    HAL_GPIO_WritePin(RF_EN_GPIO_Port, RF_EN_Pin, GPIO_PIN_RESET);

    joystick_adc.start();

    rtos::QueueManager::init();
    rtos::TaskManager::init();
}
