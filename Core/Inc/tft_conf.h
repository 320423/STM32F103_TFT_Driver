/**
 * @file tft_conf.h
 * @brief TFT 硬件配置 (引脚 / SPI / 芯片适配)
 *
 * 换 MCU 或开发板时, 只需修改此文件即可, 驱动代码无需改动。
 *
 * ============ F1 / F4 适配说明 ============
 *
 *   F1 和 F4 的 HAL GPIO / SPI / Delay API 完全一致,
 *   唯一区别是 HAL 头文件不同。以下通过 STM32 芯片宏自动包含。
 *
 *   如需手动指定, 在编译选项中添加 -DTFT_MCU_F1 或 -DTFT_MCU_F4 即可,
 *   或直接在下方 #include 你的 HAL 头文件。
 *
 * ============ 引脚接线 (ILI9341 8-Pin SPI) ============
 *
 *   屏幕引脚  |  MCU 引脚  |  宏定义
 *   ----------|-----------|------------------
 *   RES      |  PA3       |  TFT_RES_PORT / TFT_RES_PIN
 *   DC       |  PA4       |  TFT_DC_PORT  / TFT_DC_PIN
 *   BLK      |  PA2       |  TFT_BLK_PORT / TFT_BLK_PIN
 *   CLK      |  PA5       |  SPI1_SCK (CubeMX 配置)
 *   MOSI     |  PA7       |  SPI1_MOSI (CubeMX 配置)
 */

#ifndef __TFT_CONF_H__
#define __TFT_CONF_H__

/* ==================== 1. 芯片识别 & HAL 头文件 ==================== */

#if defined(TFT_MCU_F1) || defined(STM32F1) || defined(STM32F1xx)  \
    || defined(STM32F100xB) || defined(STM32F100xE)                \
    || defined(STM32F101x6) || defined(STM32F101xB)                \
    || defined(STM32F101xE) || defined(STM32F101xG)                \
    || defined(STM32F102x6) || defined(STM32F102xB)                \
    || defined(STM32F103x6) || defined(STM32F103xB)                \
    || defined(STM32F103xE) || defined(STM32F103xG)                \
    || defined(STM32F105xC) || defined(STM32F107xC)
  #include "stm32f1xx_hal.h"

#elif defined(TFT_MCU_F4) || defined(STM32F4) || defined(STM32F4xx) \
    || defined(STM32F401xE) || defined(STM32F405xx)                \
    || defined(STM32F407xx) || defined(STM32F410xx)                \
    || defined(STM32F411xE) || defined(STM32F412xx)                \
    || defined(STM32F413xx) || defined(STM32F415xx)                \
    || defined(STM32F417xx) || defined(STM32F427xx)                \
    || defined(STM32F429xx) || defined(STM32F437xx)                \
    || defined(STM32F439xx) || defined(STM32F446xx)                \
    || defined(STM32F469xx) || defined(STM32F479xx)
  #include "stm32f4xx_hal.h"

#else
  /* 未识别的芯片 — 请在编译选项中定义 TFT_MCU_F1 或 TFT_MCU_F4,
     或在包含 tft_conf.h 之前手动 #include 对应的 HAL 头文件 */
  #warning "TFT_CONF: Unknown MCU, please #include the correct HAL header before tft_conf.h"
#endif

/* ==================== 2. 引脚配置 ==================== */

#define TFT_RES_PORT    GPIOA
#define TFT_RES_PIN     GPIO_PIN_3

#define TFT_DC_PORT     GPIOA
#define TFT_DC_PIN      GPIO_PIN_4

#define TFT_BLK_PORT    GPIOA
#define TFT_BLK_PIN     GPIO_PIN_2

/* ==================== 3. SPI + DMA 配置 ==================== */

#define TFT_SPI_HANDLE  hspi1

/*  SPI 时钟 = APB2 / prescaler
 *  F1: APB2=72M  → /4=18MHz  /8=9MHz
 *  F4: APB2=80M  → /4=20MHz  /8=10MHz
 *  ILI9341 标称上限 10MHz, 实际多数板子可跑 18~20MHz */
#define TFT_SPI_BAUD   SPI_BAUDRATEPRESCALER_4

/* ==================== 4. 底层操作宏 ==================== */

/* 阻塞 SPI (小数据: 寄存器/命令) */
#define TFT_SPI_TX(buf, n)        HAL_SPI_Transmit(&TFT_SPI_HANDLE, (buf), (n), HAL_MAX_DELAY)

/* DMA SPI (大批量像素数据) */
#define TFT_SPI_TX_DMA(buf, n)    HAL_SPI_Transmit_DMA(&TFT_SPI_HANDLE, (buf), (n))
#define TFT_SPI_DMA_WAIT()        while (HAL_SPI_GetState(&TFT_SPI_HANDLE) != HAL_SPI_STATE_READY) {}

/* 延时 (ms) */
#define TFT_DELAY_MS(ms)    HAL_Delay(ms)

/* RES 复位 */
#define TFT_RES_LOW()       HAL_GPIO_WritePin(TFT_RES_PORT, TFT_RES_PIN, GPIO_PIN_RESET)
#define TFT_RES_HIGH()      HAL_GPIO_WritePin(TFT_RES_PORT, TFT_RES_PIN, GPIO_PIN_SET)

/* DC 数据/命令 */
#define TFT_DC_LOW()        HAL_GPIO_WritePin(TFT_DC_PORT,  TFT_DC_PIN,  GPIO_PIN_RESET)
#define TFT_DC_HIGH()       HAL_GPIO_WritePin(TFT_DC_PORT,  TFT_DC_PIN,  GPIO_PIN_SET)

/* BLK 背光 */
#define TFT_BLK_OFF()       HAL_GPIO_WritePin(TFT_BLK_PORT, TFT_BLK_PIN, GPIO_PIN_RESET)
#define TFT_BLK_ON()        HAL_GPIO_WritePin(TFT_BLK_PORT, TFT_BLK_PIN, GPIO_PIN_SET)

/* SPI 发送 */
#define TFT_SPI_TX(buf, n)  HAL_SPI_Transmit(&TFT_SPI_HANDLE, (buf), (n), HAL_MAX_DELAY)

#endif /* __TFT_CONF_H__ */
