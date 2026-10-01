# 内存段宏与 DMA 缓冲区

本项目基于 STM32H750。默认的 `.data` 和 `.bss` 位于 DTCMRAM（`0x20000000`），而 DMA1/DMA2 不能访问 DTCMRAM。需要让 DMA 访问的对象，必须放到 DMA 可访问的 SRAM 中。段宏定义在 [`BSP/memory_map.h`](../BSP/memory_map.h)，`DmaBuffer` 定义在 [`BSP/dma_buffer.hpp`](../BSP/dma_buffer.hpp)。

## 四个段宏

| 宏 | 链接段 | 运行地址 | 启动时的处理 | 对齐 |
| --- | --- | --- | --- | --- |
| `RAM_D1_DATA` | `.ram_d1_data` | D1 AXI SRAM，`0x24000000` 起 | 从 Flash 复制初值 | 8 字节 |
| `RAM_D1_BSS` | `.ram_d1_bss` | D1 AXI SRAM，`0x24000000` 起 | 清零，不占用 Flash 初值镜像 | 8 字节 |
| `DMA_BUFFER_DATA` | `.dma_buffer_data` | D2 SRAM，`0x30000000` 起 | 从 Flash 复制初值 | 32 字节 |
| `DMA_BUFFER_BSS` | `.dma_buffer_bss` | D2 SRAM，`0x30000000` 起 | 清零，不占用 Flash 初值镜像 | 32 字节 |

`DATA` 用于需要在启动时恢复非零初值的静态存储期对象；`BSS` 用于从零开始的对象。宏是 GCC 的 `section` 和 `aligned` 属性，应写在**变量定义**上，而不是只写在类型或指针上。例如：

```cpp
#include <cstdint>
#include "memory_map.h"
#include "dma_buffer.hpp"

RAM_D1_DATA std::uint32_t saved_default = 42;
RAM_D1_BSS std::uint32_t counter{};
DMA_BUFFER_BSS DmaBuffer<std::uint16_t, 4> adc_samples{};
```

这些示例变量须具有静态存储期（全局变量或 `static` 变量）。`DMA_BUFFER_DATA` 也提供给需要预置内容的 DMA 数据；当前项目的 ADC 接收缓冲区使用 `DMA_BUFFER_BSS`。

## `DmaBuffer<T, Length>` 的作用

`DmaBuffer` 是固定长度的缓冲区封装：`T` 必须可平凡复制，`Length` 必须大于零；类型本身按 32 字节对齐，不允许复制。`data()` 返回 `volatile T*`，`operator[]` 返回 `volatile` 元素引用，`size()` 返回元素个数。`volatile` 使 CPU 对缓冲区的读取保持为实际内存访问，但它本身不负责选择 SRAM，也不解决启用 D-Cache 后的缓存一致性问题。

**要把对象放到 D2 SRAM，仍须在变量定义处标注 `DMA_BUFFER_BSS` 或 `DMA_BUFFER_DATA`。** 单独声明 `DmaBuffer` 类型不会改变链接段。当前用法见 [`APP/init/init.cpp`](../APP/init/init.cpp)：

```cpp
DMA_BUFFER_BSS DmaBuffer<std::uint16_t, 4> joystick_adc_buffer{};
```

该对象由 [`MODULE/joystick/joystick.hpp`](../MODULE/joystick/joystick.hpp) 中的 `AdcDma` 传给 `HAL_ADC_Start_DMA`。DMA 的数据宽度还需要与元素类型和外设配置一致；当前 ADC 路径使用 16 位元素。32 字节对齐不等于自动完成缓存清理或失效。如果以后启用 D-Cache，应按 DMA 方向处理缓存一致性，并核对缓冲区的缓存行边界。

## 配套的链接与启动改动

[`STM32H750XX_FLASH.ld`](../STM32H750XX_FLASH.ld) 增加了上述四个输出段：D1 两段放入 `RAM_D1`，DMA 两段放入 `RAM_D2`。`*_DATA` 段使用 `AT> FLASH` 保存加载镜像，`*_BSS` 段使用 `NOLOAD`。链接脚本同时提供每段的起止符号，以及 `*_DATA` 段的 Flash 加载地址符号。

[`startup_stm32h750xx.s`](../startup_stm32h750xx.s) 在进入 `main()` 前，使用这些符号将两个新增的 `DATA` 段从 Flash 复制到 SRAM，并将两个新增的 `BSS` 段清零。这些步骤与链接脚本配套：只改段位置、不增加相应启动初始化，变量初值就不能得到保证。

D1 的 AXI SRAM（`0x24000000` 起）也需要上述链接段和启动初始化处理；项目已经为 `RAM_D1_DATA` 和 `RAM_D1_BSS` 实现了这些步骤。与 D2 SRAM 不同，D1 AXI SRAM 复位后即可访问，因此这里不需要仿照 `DATA_IN_D2_SRAM` 再增加一个 D1 开时钟宏。

D2 SRAM 还必须在启动代码访问它之前开启时钟。[`CMakeLists.txt`](../CMakeLists.txt) 对编译 [`Core/Src/system_stm32h7xx.c`](../Core/Src/system_stm32h7xx.c) 的 `STM32_Drivers` 目标定义了 `DATA_IN_D2_SRAM`。因此 `SystemInit()` 会开启 D2 SRAM 时钟；随后 `Reset_Handler` 才复制和清零位于 D2 的 DMA 段。这个编译定义与上述链接脚本、启动代码改动须一起保留。
