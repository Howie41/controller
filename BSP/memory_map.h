#pragma once

// DTCMRAM is not accessible by DMA1/DMA2 on STM32H7.
#if defined(__GNUC__)
// *_DATA keeps non-zero initial values in FLASH and copies them at reset.
// *_BSS occupies no FLASH load image and is zero-filled at reset.
#define RAM_D1_DATA __attribute__((section(".ram_d1_data"), aligned(8)))
#define RAM_D1_BSS __attribute__((section(".ram_d1_bss"), aligned(8)))
// DMA_BUFFER_* 段位于 D2 SRAM。移植时，编译 system_stm32h7xx.c 必须定义
// DATA_IN_D2_SRAM（当前在 CMakeLists.txt 中设置），使 SystemInit 在
// Reset_Handler 复制或清零这些段之前开启 D2 SRAM 时钟。
// 仅将对象放入这些段不会自动开启时钟。
#define DMA_BUFFER_DATA __attribute__((section(".dma_buffer_data"), aligned(32)))
#define DMA_BUFFER_BSS __attribute__((section(".dma_buffer_bss"), aligned(32)))
#else
#error "Unsupported compiler"
#endif
