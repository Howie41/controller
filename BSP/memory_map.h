#pragma once

// DTCMRAM is not accessible by DMA1/DMA2 on STM32H7.
#if defined(__GNUC__)
// *_DATA keeps non-zero initial values in FLASH and copies them at reset.
// *_BSS occupies no FLASH load image and is zero-filled at reset.
#define RAM_D1_DATA __attribute__((section(".ram_d1_data"), aligned(8)))
#define RAM_D1_BSS __attribute__((section(".ram_d1_bss"), aligned(8)))
#define DMA_BUFFER_DATA __attribute__((section(".dma_buffer_data"), aligned(32)))
#define DMA_BUFFER_BSS __attribute__((section(".dma_buffer_bss"), aligned(32)))
#else
#error "Unsupported compiler"
#endif
