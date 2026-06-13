/**
 * @file font.h
 * @brief TFT ASCII 字库
 *
 * 字库数据来源于波特律动 OLED 字库。
 * 字模为列主序: 每列 ceil(height/8) 字节, bit 0 = 顶部像素, bit 7 = 底部像素。
 * 先存所有列的第1字节, 再存所有列的第2字节……
 * 每字符占用 width * ceil(height/8) 字节。
 */

#ifndef __FONT_H__
#define __FONT_H__

#include <stdint.h>

/* ASCII 字符数 (space 0x20 ~ '~' 0x7E, 共 95 个) */
#define ASCII_CHAR_COUNT 95

typedef struct {
    uint8_t width;           /* 字符宽度 (像素) — 即 OLED 驱动中的 w 字段 */
    uint8_t height;          /* 字符高度 (像素) — 即 OLED 驱动中的 h 字段 */
    const uint8_t *chars;    /* 字模数据, 每字符 width * ceil(height/8) 字节 */
} ASCIIFont;

extern const ASCIIFont afont8x6;   /* 宽6 高8  — 注意命名是 8x6 指 h×w */
extern const ASCIIFont afont12x6;  /* 宽6 高12 */
extern const ASCIIFont afont16x8;  /* 宽8 高16 */

#endif /* __FONT_H__ */
