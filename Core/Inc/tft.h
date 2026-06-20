#ifndef __TFT_H__
#define __TFT_H__

#include "tft_conf.h"
#include "font.h"

/*
 * ==================== 硬件连接 ====================
 *
 *  ILI9341 2.4" TFT LCD (240x320)  SPI 8-Pin
 *  引脚配置见 tft_conf.h, 换板子只需改那一个文件
 */

/* ==================== 颜色定义 (RGB565) ==================== */
#define TFT_BLACK       0x0000
#define TFT_WHITE       0xFFFF
#define TFT_RED         0xF800
#define TFT_GREEN       0x07E0
#define TFT_BLUE        0x001F
#define TFT_YELLOW      0xFFE0
#define TFT_CYAN        0x07FF
#define TFT_MAGENTA     0xF81F
#define TFT_GRAY        0x8410
#define TFT_ORANGE      0xFC00

/* ==================== 屏幕尺寸 ==================== */
#define TFT_WIDTH       240
#define TFT_HEIGHT      320

/* ==================== 旋转方向 ==================== */
typedef enum {
    TFT_ROTATION_0 = 0,   // 默认竖屏 (排针朝下)
    TFT_ROTATION_90,      // 横屏
    TFT_ROTATION_180,     // 竖屏倒转
    TFT_ROTATION_270      // 横屏倒转
} TFT_Rotation;

/* ==================== API ==================== */
void TFT_Init(void);
void TFT_SetRotation(TFT_Rotation rot);
void TFT_SetWindow(uint16_t x, uint16_t y, uint16_t w, uint16_t h);
void TFT_FillColor(uint16_t color);
void TFT_FillRect(uint16_t x, uint16_t y, uint16_t w, uint16_t h, uint16_t color);
void TFT_FillScreen(uint16_t color);
void TFT_DrawPixel(uint16_t x, uint16_t y, uint16_t color);
void TFT_DrawImage(uint16_t x, uint16_t y, uint16_t w, uint16_t h, const uint16_t *data);
void TFT_DrawChar(uint16_t x, uint16_t y, char ch, const ASCIIFont *font,
                  uint16_t fg_color, uint16_t bg_color);
void TFT_DrawString(uint16_t x, uint16_t y, const char *str, const ASCIIFont *font,
                    uint16_t fg_color, uint16_t bg_color);
void TFT_DrawBitmap1BPP(uint16_t x, uint16_t y, uint16_t w, uint16_t h,
                        const uint8_t *bitmap, uint16_t fg, uint16_t bg);
void TFT_BackLight(uint8_t on);

#endif /* __TFT_H__ */
