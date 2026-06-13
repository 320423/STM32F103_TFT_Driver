# F103_TFT_DEMO

STM32F103ZET6 + ILI9341 2.4" TFT LCD (240×320) SPI 驱动演示项目。

## 硬件连接

| 屏幕引脚 | STM32 引脚 |
|----------|------------|
| GND      | GND        |
| VCC      | 3.3V       |
| CLK      | PA5 (SPI1_SCK) |
| MOSI     | PA7 (SPI1_MOSI) |
| MISO     | PA6 (SPI1_MISO) |
| RES      | PA3        |
| DC       | PA4        |
| BLK      | PA2        |

## 构建

STM32CubeIDE 工程，直接导入编译烧录。

## 驱动文件

- [Core/Inc/tft.h](Core/Inc/tft.h) — TFT API 头文件
- [Core/Src/tft.c](Core/Src/tft.c) — TFT 驱动实现
