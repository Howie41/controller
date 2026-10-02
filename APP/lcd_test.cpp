#include "main.h"
#include "lcd.hpp"
#include "memory_map.h"
#include "rtos_task.hpp"

extern Lcd lcd;

std::uint32_t db_screen_switch_times = 0;

void lcd_test_entry(void *arg) {
    (void) arg;
    for (;;) {
        lcd.clear(0xFFFFU);
        rtos::delay(1000);
        db_screen_switch_times++;
        lcd.clear(0x0000U);
        rtos::delay(1000);
        db_screen_switch_times++;
    }
}

RAM_D1_BSS rtos::Task<&lcd_test_entry, 1024 * 4 * 4, rtos::TaskPriority::Normal>
lcd_test("lcd_test");