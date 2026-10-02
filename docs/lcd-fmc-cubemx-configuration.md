# ST7365 屏幕的 FMC / CubeMX 配置记录

本记录对应 STM32H750VBTx 工程 [`controller.ioc`](../controller.ioc) 和 TX01 主板 40 Pin 屏幕接口。屏幕采用 **16 位 8080 并口**；CubeMX 配置的是 MCU 的 FMC 总线，屏幕控制器的复位、上电初始化和绘图命令仍需应用代码实现。

## CubeMX 中的 FMC 配置

在 **Connectivity → FMC → NOR Flash/PSRAM/SRAM/ROM/LCD 1** 中，选择 NE1 片选、LCD Interface、A16 寄存器选择、16-bit 数据宽度。使用异步访问；数据/地址复用、外部时钟和 WAIT 信号均未启用。生成代码使用 `FMC_NORSRAM_BANK1`、`FMC_NORSRAM_MEM_BUS_WIDTH_16` 和 `HAL_SRAM_Init()`；这里的 SRAM 句柄是 CubeMX 对 FMC LCD 接口的代码实现，并不表示屏幕是 SRAM。

为读写设置独立时序（Extended Mode）：

| 参数 | 读时序 | 写时序 |
| --- | ---: | ---: |
| Address Setup Time | 5 个 FMC 时钟周期 | 5 个 FMC 时钟周期 |
| Data Setup Time | 110 个 FMC 时钟周期 | 20 个 FMC 时钟周期 |
| Access Mode | A | A |

生成代码中的 Address Hold Time 和 Bus Turn Around Duration 均为 15，Write FIFO 为 ENABLE，突发访问和 WAIT 信号为 DISABLE。FMC 时钟源为 D1HCLK；当前 [`controller.ioc`](../controller.ioc) 显示 FMC 时钟为 240 MHz。上述时序是保守的起始值，尚未通过示波器或实物屏验证。完整初始化值见 [`Core/Src/fmc.c`](../Core/Src/fmc.c)。

## 引脚与屏幕信号

下表按 TX01 原理图的 J3 屏幕接口与 CubeMX 生成结果整理：

| 屏幕信号 | STM32 引脚 | FMC 功能 |
| --- | --- | --- |
| DB0–DB3 | PD14、PD15、PD0、PD1 | D0–D3 |
| DB4–DB12 | PE7–PE15 | D4–D12 |
| DB13–DB15 | PD8–PD10 | D13–D15 |
| RDX | PD4 | NOE |
| WRX | PD5 | NWE |
| DCX | PD11 | A16 |
| CSX | PC7 | NE1 |

PC7 被 CubeMX 配置为 `GPIO_AF9_FMC`，由 **FMC_NE1 硬件片选**驱动；PD、PE 上的 FMC 引脚使用 `GPIO_AF12_FMC`。不需要把 PC7 当作普通 GPIO 手动拉低。屏幕接口模式引脚由硬件连接决定，本次 CubeMX 配置没有修改它们。

## MPU 地址区

工程原有 MPU Region 0 对包含 FMC 的地址范围设置为禁止访问。为使 CPU 能访问 LCD，在 **System Core → CORTEX_M7 → MPU Region 1** 中增加了高优先级区域：

| 属性 | 配置 |
| --- | --- |
| Base Address | `0x60000000` |
| Size | 256 KB |
| Access Permission | Full Access |
| Instruction Access | Disable（禁止执行） |
| Shareable | Enable |
| Cacheable / Bufferable | Disable / Disable |

Region 1 覆盖 `0x60000000` 至 `0x6003FFFF`，并覆盖 Region 0 在该范围内的权限。生成结果见 [`Core/Src/main.c`](../Core/Src/main.c) 的 `MPU_Config()`。由于数据总线宽度为 16 位、DCX 接在 FMC_A16，后续驱动可使用 `0x60000000` 作为命令地址、`0x60020000` 作为数据地址；访问类型应为 `volatile uint16_t`。

## 生成文件与验证

CubeMX 已生成 [`Core/Inc/fmc.h`](../Core/Inc/fmc.h)、[`Core/Src/fmc.c`](../Core/Src/fmc.c)，并在 `main()` 中调用 `MX_FMC_Init()`；同时更新了 HAL 配置、FMC 所需 HAL 文件和 CubeMX 管理的 CMake 源文件。顶层用户维护的 `CMakeLists.txt` 没有为此修改。

配置完成后，CubeMX 报告代码生成成功；使用 CMake/Ninja 对 `controller.elf` 的 Debug 构建通过，`git diff --check` 通过。**这些检查只证明配置能生成并编译，不证明屏幕已经点亮。** LCD 驱动负责上电初始化、复位控制和按窗口写入像素；背光控制及 FMC 时序仍需在实物上验证。
