#include "tft.h"
#include "spi.h"

/* ==================== ILI9341 寄存器 ==================== */
#define ILI9341_SWRESET   0x01
#define ILI9341_SLEEPOUT  0x11
#define ILI9341_DISPOFF   0x28
#define ILI9341_DISPON    0x29
#define ILI9341_CASET     0x2A  // 列地址
#define ILI9341_PASET     0x2B  // 页地址
#define ILI9341_RAMWR     0x2C  // 内存写
#define ILI9341_MADCTL    0x36  // 内存访问控制
#define ILI9341_PIXFMT    0x3A  // 像素格式
#define ILI9341_PWCTRLA   0xCB
#define ILI9341_PWCTRLB   0xCF
#define ILI9341_TIMCTRLA  0xE8
#define ILI9341_TIMCTRLB  0xEA
#define ILI9341_PUMPCTRL  0xF7

/* ==================== 内部变量 ==================== */
static uint16_t tft_width;   // 当前方向屏幕宽
static uint16_t tft_height;  // 当前方向屏幕高

/* ==================== 底层 SPI 操作 ==================== */

static void TFT_WriteCmd(uint8_t cmd)
{
    TFT_DC_LOW();
    TFT_SPI_TX(&cmd, 1);
}

static void TFT_WriteData(uint8_t data)
{
    TFT_DC_HIGH();
    TFT_SPI_TX(&data, 1);
}

static void TFT_WriteData16(uint16_t data)
{
    uint8_t buf[2];
    buf[0] = data >> 8;
    buf[1] = data & 0xFF;
    TFT_DC_HIGH();
    TFT_SPI_TX(buf, 2);
}

/**
 * @brief 批量发送 16 位颜色数据 (用于填充)
 * @note  DMA 行缓冲: 一行 240 像素(480 字节)一次 DMA 发送,
 *        将 HAL 调用次数从 N×2 降到 ~N/120, 帧率从 <1FPS 拉到 30+FPS
 */
static void TFT_WriteColorBurst(uint16_t color, uint32_t len)
{
    if (len == 0) return;

    /* 行缓冲: 240 像素 = 480 字节, 一次 DMA 发一行 */
    #define BURST_LINE 240
    static uint8_t  line_buf[BURST_LINE * 2];
    static uint16_t line_color;

    if (line_color != color)
    {
        uint8_t hi = color >> 8;
        uint8_t lo = color & 0xFF;
        for (uint16_t i = 0; i < BURST_LINE; i++)
        {
            line_buf[i * 2]     = hi;
            line_buf[i * 2 + 1] = lo;
        }
        line_color = color;
    }

    TFT_DC_HIGH();

    uint32_t lines = len / BURST_LINE;
    while (lines--)
    {
        TFT_SPI_TX_DMA(line_buf, BURST_LINE * 2);
        TFT_SPI_DMA_WAIT();
    }

    uint32_t remain = len % BURST_LINE;
    if (remain > 0)
    {
        TFT_SPI_TX(line_buf, remain * 2);
    }
}

/* ==================== 初始化序列 ==================== */

void TFT_Init(void)
{
    /* 硬件复位 */
    TFT_RES_LOW();
    TFT_DELAY_MS(10);
    TFT_RES_HIGH();
    TFT_DELAY_MS(120);

    /* ==== ILI9341 初始化寄存器序列 ==== */

    TFT_WriteCmd(ILI9341_SWRESET);
    TFT_DELAY_MS(120);

    TFT_WriteCmd(ILI9341_PWCTRLA);
    TFT_WriteData(0x39);
    TFT_WriteData(0x2C);
    TFT_WriteData(0x00);
    TFT_WriteData(0x34);
    TFT_WriteData(0x02);

    TFT_WriteCmd(ILI9341_PWCTRLB);
    TFT_WriteData(0x00);
    TFT_WriteData(0xC1);
    TFT_WriteData(0x30);

    TFT_WriteCmd(ILI9341_TIMCTRLA);
    TFT_WriteData(0x85);
    TFT_WriteData(0x00);
    TFT_WriteData(0x78);

    TFT_WriteCmd(ILI9341_TIMCTRLB);
    TFT_WriteData(0x00);
    TFT_WriteData(0x00);

    /* 内存访问控制 — 将在 TFT_SetRotation 中设置 */
    TFT_WriteCmd(ILI9341_PWCTRLA);  // 再次写 PWCTRLA (某些初始化序列要求)
    TFT_WriteData(0x39);
    TFT_WriteData(0x2C);
    TFT_WriteData(0x00);
    TFT_WriteData(0x34);
    TFT_WriteData(0x02);

    TFT_WriteCmd(ILI9341_PUMPCTRL);
    TFT_WriteData(0x20);

    TFT_WriteCmd(ILI9341_TIMCTRLA);
    TFT_WriteData(0x00);
    TFT_WriteData(0x00);

    /* 像素格式: 16bit RGB565 */
    TFT_WriteCmd(ILI9341_PIXFMT);
    TFT_WriteData(0x55);

    /* 设置默认旋转方向 (竖屏) */
    TFT_SetRotation(TFT_ROTATION_0);

    /* 退出休眠 */
    TFT_WriteCmd(ILI9341_SLEEPOUT);
    TFT_DELAY_MS(120);

    /* 开启显示 */
    TFT_WriteCmd(ILI9341_DISPON);
    TFT_DELAY_MS(20);

    /* 开启背光 */
    TFT_BackLight(1);
}

/* ==================== 旋转设置 ==================== */

void TFT_SetRotation(TFT_Rotation rot)
{
    TFT_WriteCmd(ILI9341_MADCTL);
    switch (rot)
    {
    case TFT_ROTATION_0:
        TFT_WriteData(0x48);  // 竖屏, 排针朝下
        tft_width  = 240;
        tft_height = 320;
        break;
    case TFT_ROTATION_90:
        TFT_WriteData(0x28);  // 横屏
        tft_width  = 320;
        tft_height = 240;
        break;
    case TFT_ROTATION_180:
        TFT_WriteData(0x88);  // 竖屏倒转
        tft_width  = 240;
        tft_height = 320;
        break;
    case TFT_ROTATION_270:
        TFT_WriteData(0xE8);  // 横屏倒转
        tft_width  = 320;
        tft_height = 240;
        break;
    }
}

/* ==================== 绘制窗口 ==================== */

void TFT_SetWindow(uint16_t x, uint16_t y, uint16_t w, uint16_t h)
{
    uint16_t x2 = x + w - 1;
    uint16_t y2 = y + h - 1;

    /* 列地址 */
    TFT_WriteCmd(ILI9341_CASET);
    TFT_WriteData16(x);
    TFT_WriteData16(x2);

    /* 页地址 */
    TFT_WriteCmd(ILI9341_PASET);
    TFT_WriteData16(y);
    TFT_WriteData16(y2);

    /* 准备写入像素数据 */
    TFT_WriteCmd(ILI9341_RAMWR);
}

/* ==================== 像素 / 填充 ==================== */

void TFT_DrawPixel(uint16_t x, uint16_t y, uint16_t color)
{
    TFT_SetWindow(x, y, 1, 1);
    TFT_WriteData16(color);
}

void TFT_FillColor(uint16_t color)
{
    /* 假设 TFT_SetWindow 已调用, 直接持续写数据 */
    /* ILI9341 支持自增, RAMWR 之后可连续写入 */
}

/**
 * @brief 填充指定区域
 * @note  内部使用 TFT_SetWindow + 批量 SPI 写入
 */
void TFT_FillRect(uint16_t x, uint16_t y, uint16_t w, uint16_t h, uint16_t color)
{
    TFT_SetWindow(x, y, w, h);
    TFT_WriteColorBurst(color, (uint32_t)w * h);
}

void TFT_FillScreen(uint16_t color)
{
    TFT_FillRect(0, 0, tft_width, tft_height, color);
}

/* ==================== 图片 ==================== */

void TFT_DrawImage(uint16_t x, uint16_t y, uint16_t w, uint16_t h, const uint16_t *data)
{
    TFT_SetWindow(x, y, w, h);
    TFT_DC_HIGH();

    uint32_t count = (uint32_t)w * h;

    /* 小图 (<512 像素) 直接阻塞发送, 避免 DMA 开销 */
    if (count < 512)
    {
        while (count--)
        {
            uint8_t buf[2];
            buf[0] = (*data) >> 8;
            buf[1] = (*data) & 0xFF;
            TFT_SPI_TX(buf, 2);
            data++;
        }
        return;
    }

    /* 大图: DMA 分块发送, 256 像素一块 (512 字节) */
    #define DMA_BLOCK_PIXELS 256
    uint8_t dma_buf[DMA_BLOCK_PIXELS * 2];
    uint16_t buf_idx = 0;

    while (count--)
    {
        dma_buf[buf_idx++] = (*data) >> 8;
        dma_buf[buf_idx++] = (*data) & 0xFF;
        data++;

        if (buf_idx >= DMA_BLOCK_PIXELS * 2)
        {
            TFT_SPI_TX_DMA(dma_buf, buf_idx);
            TFT_SPI_DMA_WAIT();
            buf_idx = 0;
        }
    }

    /* 尾部 */
    if (buf_idx > 0)
    {
        TFT_SPI_TX_DMA(dma_buf, buf_idx);
        TFT_SPI_DMA_WAIT();
    }
}

/* ==================== 1-Bit 位图 / 视频 ==================== */

/**
 * @brief 绘制 1-bit 位图 (横屏视频用)
 * @param bitmap 位图数据, 每字节 8 像素, OLED 页寻址模式
 * @note  页寻址: h/8 页 × w 列, bitmap[page*w + col], bit=row%8
 *        LSB=顶行, 与波特律动 OLED 字库同格式
 *        多行缓冲: 4 行一批 DMA, 减少 HAL 调用 240→60 次/帧
 */
void TFT_DrawBitmap1BPP(uint16_t x, uint16_t y, uint16_t w, uint16_t h,
                        const uint8_t *bitmap, uint16_t fg, uint16_t bg)
{
    TFT_SetWindow(x, y, w, h);
    TFT_DC_HIGH();

    uint16_t fg16 = fg, bg16 = bg;

    #define ROWS_PER_BURST 12
    static uint8_t buf[ROWS_PER_BURST * 640];
    uint16_t *p16 = (uint16_t *)buf;

    for (uint16_t row = 0; row < h; row++)
    {
        uint8_t page = row / 8;
        uint8_t bit  = row % 8;
        const uint8_t *page_data = &bitmap[(uint32_t)page * w];

        for (uint16_t col = 0; col < w; col++)
        {
            *p16++ = (page_data[col] >> bit) & 1 ? fg16 : bg16;
        }

        if (p16 - (uint16_t *)buf >= ROWS_PER_BURST * w)
        {
            uint16_t bytes = (uint8_t *)p16 - buf;
            TFT_SPI_TX_DMA(buf, bytes);
            TFT_SPI_DMA_WAIT();
            p16 = (uint16_t *)buf;
        }
    }

    uint16_t bytes = (uint8_t *)p16 - buf;
    if (bytes > 0)
    {
        TFT_SPI_TX_DMA(buf, bytes);
        TFT_SPI_DMA_WAIT();
    }
}

/* ==================== 文字 ==================== */

/**
 * @brief 绘制单个 ASCII 字符
 * @param x,y   字符左上角坐标
 * @param ch    要绘制的字符 (0x20 ~ 0x7E)
 * @param font  字体指针 (afont8x6 / afont12x6 / afont16x8)
 * @param fg_color 前景色 (RGB565)
 * @param bg_color 背景色 (RGB565)
 * @note  字模为列主序: 每列 ceil(height/8) 字节, bit 0 = 顶部像素。
 *        TFT 需要行主序输出, 此处逐行扫描转换。
 */
void TFT_DrawChar(uint16_t x, uint16_t y, char ch, const ASCIIFont *font,
                  uint16_t fg_color, uint16_t bg_color)
{
    uint8_t idx;
    if (ch < 32 || ch > 126)
        idx = 0;  /* 不可打印字符用空格替代 */
    else
        idx = ch - 32;

    uint8_t w = font->width;
    uint8_t h = font->height;
    uint8_t bpc = (h + 7) / 8;  /* 每列字节数 */
    const uint8_t *char_data = &font->chars[(uint32_t)idx * w * bpc];

    TFT_SetWindow(x, y, w, h);
    TFT_DC_HIGH();

    /* 行缓冲: 最大 8 像素宽 (16x8 字体) × 2 字节 = 16 */
    uint8_t row_buf[16];
    for (uint8_t row = 0; row < h; row++)
    {
        for (uint8_t col = 0; col < w; col++)
        {
            /* 列主序交叉排布: 先所有列的 byte0, 再所有列的 byte1...
               OLED 驱动用 data[col + page * width] 索引, page = row/8 */
            uint8_t byte_idx = col + (row / 8) * w;
            uint8_t bit_pos  = row % 8;
            uint16_t color = (char_data[byte_idx] & (1 << bit_pos))
                           ? fg_color : bg_color;
            row_buf[col * 2]     = color >> 8;
            row_buf[col * 2 + 1] = color & 0xFF;
        }
        TFT_SPI_TX(row_buf, (uint16_t)w * 2);
    }
}

/**
 * @brief 绘制字符串 (不支持中文)
 * @param x,y    字符串左上角坐标
 * @param str    要显示的字符串 (以 '\0' 结尾)
 * @param font   字体指针
 * @param fg_color 前景色
 * @param bg_color 背景色
 * @note  支持 '\n' 换行, 超出屏幕不会自动裁剪
 */
void TFT_DrawString(uint16_t x, uint16_t y, const char *str, const ASCIIFont *font,
                    uint16_t fg_color, uint16_t bg_color)
{
    uint16_t cx = x;
    uint16_t cy = y;
    while (*str)
    {
        if (*str == '\n')
        {
            cx = x;
            cy += font->height;
        }
        else
        {
            TFT_DrawChar(cx, cy, *str, font, fg_color, bg_color);
            cx += font->width;
        }
        str++;
    }
}

/* ==================== 背光 ==================== */

void TFT_BackLight(uint8_t on)
{
    if (on)
        TFT_BLK_ON();
    else
        TFT_BLK_OFF();
}
