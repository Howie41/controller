#include "init.h"
#include "adc.h"
#include "joystick.hpp"
#include "main.h"
#include "memory_map.h"
#include "rtos_queue.hpp"
#include "rtos_task.hpp"

DMA_BUFFER_BSS AdcDma<4> joystick_adc(hadc1);
JoyStick left_joystick(joystick_adc, 1, 2);
JoyStick right_joystick(joystick_adc, 3, 4);

/**
 * @brief 初始化各种硬件软件
 * 在 RTOS 内核启动后执行
 */
extern "C" void init() {
    HAL_GPIO_WritePin(RF_EN_GPIO_Port, RF_EN_Pin, GPIO_PIN_RESET);

    [[maybe_unused]] auto joystick_result = joystick_adc.start();

    rtos::QueueManager::init();
    rtos::TaskManager::init();
}
