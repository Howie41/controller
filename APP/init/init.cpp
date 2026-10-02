#include "init.h"
#include "adc.h"
#include "fmc.h"
#include "lcd.hpp"
#include "joystick.hpp"
#include "main.h"
#include "memory_map.h"
#include "pwm.hpp"
#include "rtos_queue.hpp"
#include "rtos_task.hpp"
#include "tim.h"

DMA_BUFFER_BSS AdcDma<4> joystick_adc(hadc1);
JoyStick left_joystick(joystick_adc, 1, 2);
JoyStick right_joystick(joystick_adc, 3, 4);

Lcd lcd(hsram1);

/**
 * @brief 初始化各种硬件软件
 * - 在 RTOS 内核初始化后执行
 *
 * @attention RTOS内核还没有启动，延时应该使用 HAL_Delay
 */
extern "C" void init() {
    HAL_GPIO_WritePin(RF_EN_GPIO_Port, RF_EN_Pin, GPIO_PIN_RESET);

    [[maybe_unused]] auto joystick_result = joystick_adc.start();
    if (!lcd.init() || !lcd.clear(0xFFFFU)) {
        Error_Handler();
    }

    // APB1 分频为 2，TIM3 输入时钟为 PCLK1 的两倍。
    // Pwm lcd_backlight(htim3, Pwm::channel_4, HAL_RCC_GetPCLK1Freq() * 2U);
    // if (!lcd_backlight.set_duty_cycle(0.9F) || lcd_backlight.start() != HAL_OK) {
    //     Error_Handler();
    // }

    rtos::QueueManager::init();
    rtos::TaskManager::init();
}
