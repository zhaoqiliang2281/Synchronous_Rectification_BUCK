#include "oled.h"
#include <string.h>
#include <stdio.h>
#include "bsp_pwm.h"
#include "i2c.h"

/* ============================================================
 *   OLED DMA 发送超时（ms），防止总线挂死时永久阻塞
 * ============================================================ */
#define OLED_DMA_TIMEOUT_MS   50U

/* ============================================================
 *   DMA 一次性发送缓冲最大长度（数据 + 1 控制字节）
 *   如果只用分块 DMA（31+1），这里可以留 32；
 *   如果用 oled_WriteData_Async 一次发整屏，则要 >= 1024+1。
 * ============================================================ */
#define OLED_MAX_BUF          (1024 + 1)

static uint8_t oled_buffer[OLED_BUFFER_SIZE];

static void oled_ClearBuffer(void);
static uint8_t oled_UpdateScreen(void);
static uint8_t oled_DrawChar(uint8_t x, uint8_t y, char ch, uint8_t invert);
static void oled_DrawString(uint8_t x, uint8_t y, const char *str, uint8_t invert);

static const uint8_t font6x8[] = {
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00,	// sp
	0x00, 0x00, 0x00, 0x2f, 0x00, 0x00,	// !
	0x00, 0x03, 0x00, 0x03, 0x00, 0x00,	// "
	0x14, 0x7f, 0x14, 0x7f, 0x14, 0x00,	// #
	0x24, 0x2a, 0x7f, 0x2a, 0x12, 0x00,	// $
	0x62, 0x64, 0x08, 0x13, 0x23, 0x00,	// %
	0x36, 0x49, 0x55, 0x22, 0x50, 0x00,	// &
	0x00, 0x05, 0x03, 0x00, 0x00, 0x00,	// '
	0x00, 0x1c, 0x22, 0x41, 0x00, 0x00,	// (
	0x00, 0x41, 0x22, 0x1c, 0x00, 0x00,	// )
	0x14, 0x08, 0x3e, 0x08, 0x14, 0x00,	// *
	0x08, 0x08, 0x3e, 0x08, 0x08, 0x00,	// +
	0x00, 0x80, 0x70, 0x30, 0x00, 0x00,	// ,
	0x08, 0x08, 0x08, 0x08, 0x08, 0x00,	// -
	0x00, 0x00, 0x60, 0x60, 0x00, 0x00,	// .
	0x20, 0x10, 0x08, 0x04, 0x02, 0x00,	// /
	0x3e, 0x51, 0x49, 0x45, 0x3e, 0x00,	// 0
	0x00, 0x42, 0x7f, 0x40, 0x00, 0x00,	// 1
	0x42, 0x61, 0x51, 0x49, 0x46, 0x00,	// 2
	0x21, 0x41, 0x45, 0x4b, 0x31, 0x00,	// 3
	0x18, 0x14, 0x12, 0x7f, 0x10, 0x00,	// 4
	0x27, 0x45, 0x45, 0x45, 0x39, 0x00,	// 5
	0x3c, 0x4a, 0x49, 0x49, 0x30, 0x00,	// 6
	0x01, 0x71, 0x09, 0x05, 0x03, 0x00,	// 7
	0x36, 0x49, 0x49, 0x49, 0x36, 0x00,	// 8
	0x06, 0x49, 0x49, 0x29, 0x1e, 0x00,	// 9
	0x00, 0x00, 0x14, 0x00, 0x14, 0x00,	// :
	0x00, 0x80, 0x74, 0x30, 0x00, 0x00,	// ;
	0x08, 0x14, 0x22, 0x41, 0x00, 0x00,	// <
	0x14, 0x14, 0x14, 0x14, 0x14, 0x00,	// =
	0x00, 0x41, 0x22, 0x14, 0x08, 0x00,	// >
	0x02, 0x01, 0x51, 0x09, 0x06, 0x00,	// ?
	0x32, 0x49, 0x59, 0x51, 0x3e, 0x00,	// @
	0x7e, 0x11, 0x11, 0x11, 0x7e, 0x00,	// A
	0x7f, 0x49, 0x49, 0x49, 0x36, 0x00,	// B
	0x3e, 0x41, 0x41, 0x41, 0x22, 0x00,	// C
	0x7f, 0x41, 0x41, 0x22, 0x1c, 0x00,	// D
	0x7f, 0x49, 0x49, 0x49, 0x41, 0x00,	// E
	0x7f, 0x09, 0x09, 0x09, 0x01, 0x00,	// F
	0x3e, 0x41, 0x49, 0x49, 0x7a, 0x00,	// G
	0x7f, 0x08, 0x08, 0x08, 0x7f, 0x00,	// H
	0x00, 0x41, 0x7f, 0x41, 0x00, 0x00,	// I
	0x20, 0x40, 0x41, 0x3f, 0x01, 0x00,	// J
	0x7f, 0x08, 0x14, 0x22, 0x41, 0x00,	// K
	0x7f, 0x40, 0x40, 0x40, 0x40, 0x00,	// L
	0x7f, 0x02, 0x0c, 0x02, 0x7f, 0x00,	// M
	0x7f, 0x04, 0x08, 0x10, 0x7f, 0x00,	// N
	0x3e, 0x41, 0x41, 0x41, 0x3e, 0x00,	// O
	0x7f, 0x09, 0x09, 0x09, 0x06, 0x00,	// P
	0x3e, 0x41, 0x51, 0x21, 0x5e, 0x00,	// Q
	0x7f, 0x09, 0x19, 0x29, 0x46, 0x00,	// R
	0x46, 0x49, 0x49, 0x49, 0x31, 0x00,	// S
	0x01, 0x01, 0x7f, 0x01, 0x01, 0x00,	// T
	0x3f, 0x40, 0x40, 0x40, 0x3f, 0x00,	// U
	0x1f, 0x20, 0x40, 0x20, 0x1f, 0x00,	// V
	0x3f, 0x40, 0x30, 0x40, 0x3f, 0x00,	// W
	0x63, 0x14, 0x08, 0x14, 0x63, 0x00,	// X
	0x07, 0x08, 0x70, 0x08, 0x07, 0x00,	// Y
	0x61, 0x51, 0x49, 0x45, 0x43, 0x00,	// Z
	0x00, 0x7f, 0x41, 0x41, 0x00, 0x00,	// [
	0x02, 0x04, 0x08, 0x10, 0x20, 0x00,	// '\'
	0x00, 0x41, 0x41, 0x7f, 0x00, 0x00,	// ]
	0x04, 0x02, 0x01, 0x02, 0x04, 0x00,	// ^
	0x40, 0x40, 0x40, 0x40, 0x40, 0x00,	// _
	0x00, 0x03, 0x05, 0x00, 0x00, 0x00,	// `
	0x20, 0x54, 0x54, 0x54, 0x78, 0x00,	// a
	0x7f, 0x48, 0x44, 0x44, 0x38, 0x00,	// b
	0x38, 0x44, 0x44, 0x44, 0x20, 0x00,	// c
	0x38, 0x44, 0x44, 0x48, 0x7f, 0x00,	// d
	0x38, 0x54, 0x54, 0x54, 0x18, 0x00,	// e
	0x08, 0x7e, 0x09, 0x01, 0x02, 0x00,	// f
	0x18, 0xa4, 0xa4, 0xa4, 0x7c, 0x00,	// g
	0x7f, 0x08, 0x04, 0x04, 0x78, 0x00,	// h
	0x00, 0x44, 0x7d, 0x40, 0x00, 0x00,	// i
	0x40, 0x80, 0x84, 0x7d, 0x00, 0x00,	// j
	0x7f, 0x10, 0x28, 0x44, 0x00, 0x00,	// k
	0x00, 0x41, 0x7f, 0x40, 0x00, 0x00,	// l
	0x7c, 0x04, 0x78, 0x04, 0x78, 0x00,	// m
	0x7c, 0x08, 0x04, 0x04, 0x78, 0x00,	// n
	0x38, 0x44, 0x44, 0x44, 0x38, 0x00,	// o
	0xfc, 0x24, 0x24, 0x24, 0x18, 0x00,	// p
	0x18, 0x24, 0x24, 0x18, 0xfc, 0x00,	// q
	0x7c, 0x08, 0x04, 0x04, 0x08, 0x00,	// r
	0x48, 0x54, 0x54, 0x54, 0x20, 0x00,	// s
	0x04, 0x3f, 0x44, 0x40, 0x20, 0x00,	// t
	0x3c, 0x40, 0x40, 0x20, 0x7c, 0x00,	// u
	0x1c, 0x20, 0x40, 0x20, 0x1c, 0x00,	// v
	0x3c, 0x40, 0x30, 0x40, 0x3c, 0x00,	// w
	0x44, 0x28, 0x10, 0x28, 0x44, 0x00,	// x
	0x9c, 0xa0, 0x60, 0x3c, 0x00, 0x00,	// y
	0x44, 0x64, 0x54, 0x4c, 0x44, 0x00,	// z
	0x08, 0x36, 0x41, 0x41, 0x00, 0x00,	// {
	0x00, 0x00, 0x7f, 0x00, 0x00, 0x00,	// |
	0x00, 0x41, 0x41, 0x36, 0x08, 0x00,	// }
	0x02, 0x01, 0x01, 0x02, 0x01, 0x00,	// ~
};

/* ============================================================
 *   等待 I2C DMA 传输完成（轮询状态机 + 超时）
 *   返回 0 = 完成，1 = 超时/出错
 *   只有等回到 READY，才能复用发送缓冲区 / 发下一块
 * ============================================================ */
static uint8_t oled_WaitDmaDone(void)
{
    uint32_t tick_start = HAL_GetTick();

    while (HAL_I2C_GetState(&hi2c1) != HAL_I2C_STATE_READY)
    {
        if ((HAL_GetTick() - tick_start) > OLED_DMA_TIMEOUT_MS)
        {
            /* 超时：中止 DMA 传输，尝试恢复总线 */
            HAL_I2C_Master_Abort_IT(&hi2c1, OLED_I2C_ADDR);
            /* 如果 Abort_IT 后状态还没回到 READY，强制 DeInit/Init */
            HAL_I2C_DeInit(&hi2c1);
            HAL_I2C_Init(&hi2c1);
			oled_Init();
            return 1;
        }
    }
    return 0;
}

/* ============================================================
 *   写命令：改用 DMA 发送（2 字节：0x00 控制字节 + 命令）
 *   命令很小，DMA 和阻塞差别不大，但为了统一都走 DMA。
 * ============================================================ */
static uint8_t oled_WriteCommand(uint8_t cmd)
{
    /* 缓冲区必须是 static，DMA 传输期间不能被释放 */
    static uint8_t data_to_send[2];
    data_to_send[0] = 0x00;   /* Co=0, D/C#=0 -> 命令 */
    data_to_send[1] = cmd;

    /* HAL_I2C_Master_Transmit -> HAL_I2C_Master_Transmit_DMA */
    if (HAL_I2C_Master_Transmit_DMA(&hi2c1, OLED_I2C_ADDR,
                                    data_to_send, 2) != HAL_OK)
    {
        return 1;
    }

    /* 等待 DMA 完成，保证下一条命令能复用/覆盖静态缓冲 */
    return oled_WaitDmaDone();
}

/* ============================================================
 * 一次发完的异步 DMA 版本（原代码已有，保留）
 *   需要 static 大缓冲 oled_tx_buf[OLED_MAX_BUF]，且 size+1 <= OLED_MAX_BUF
 *   不阻塞，靠 HAL_I2C_MasterTxCpltCallback 清 busy 标志。
 * ============================================================ */
static uint8_t oled_tx_buf[OLED_MAX_BUF];   /* 静态 DMA 发送缓冲 */
static uint16_t oled_tx_len;
static uint8_t  oled_busy = 0;

static uint8_t oled_WriteData_Async(uint8_t* data, uint16_t size)
{
    if (oled_busy) return 2;                       /* 上一次还没完成 */
    if (size + 1 > OLED_MAX_BUF) return 3;         /* 越界保护 */

    oled_tx_buf[0] = 0x40;                         /* 数据控制字节 */
    memcpy(&oled_tx_buf[1], data, size);
    oled_tx_len = size + 1;
    oled_busy = 1;

    if (HAL_I2C_Master_Transmit_DMA(&hi2c1, OLED_I2C_ADDR,
                                    oled_tx_buf, oled_tx_len) != HAL_OK)
    {
        oled_busy = 0;
        return 1;
    }
    return 0;
}

/* DMA 发送完成回调：清 busy 标志
 *   注意：如果工程里别处也重写了这个回调，请合并，不要重复定义！ */
void HAL_I2C_MasterTxCpltCallback(I2C_HandleTypeDef *hi2c)
{
    if (hi2c == &hi2c1)
        oled_busy = 0;
}

/* ============================================================
 * 写数据：分块 DMA 发送 static
 *   每块最多 31 字节数据 + 1 控制字节 = 32 字节
 *   每块发完必须等 DMA 完成才能复用栈上 transfer_buffer
 * ============================================================ */
uint8_t oled_WriteData(uint8_t* data, uint16_t size)//暂时不用
{
    const uint16_t max_payload_size = 31;

    /* 栈上缓冲即可：因为我们每块都等 DMA 完成才复用 */
    uint8_t transfer_buffer[max_payload_size + 1];
    transfer_buffer[0] = 0x40;   /* 数据控制字节 */

    uint16_t bytes_sent = 0;

    while (bytes_sent < size)
    {
        uint16_t current_chunk_size = size - bytes_sent;
        if (current_chunk_size > max_payload_size)
        {
            current_chunk_size = max_payload_size;
        }

        memcpy(&transfer_buffer[1], data + bytes_sent, current_chunk_size);

        /* 阻塞发送 -> DMA 发送 */
        if (HAL_I2C_Master_Transmit_DMA(&hi2c1, OLED_I2C_ADDR,
                                        transfer_buffer,
                                        current_chunk_size + 1) != HAL_OK)
        {
            return 1;
        }

        /* 等待这一块 DMA 发完，才能覆盖 transfer_buffer */
        if (oled_WaitDmaDone() != 0)
        {
            return 1;
        }

        bytes_sent += current_chunk_size;
    }

    return 0;
}

uint8_t oled_Init(void)
{
    HAL_Delay(100);

    if(oled_WriteCommand(0xAE)) return 1;
    if(oled_WriteCommand(0xD5)) return 1;
    if(oled_WriteCommand(0x80)) return 1;
    if(oled_WriteCommand(0xA8)) return 1;
    if(oled_WriteCommand(0x3F)) return 1;
    if(oled_WriteCommand(0xD3)) return 1;
    if(oled_WriteCommand(0x00)) return 1;

    if(oled_WriteCommand(0x40)) return 1;

    if(oled_WriteCommand(0x8D)) return 1;
    if(oled_WriteCommand(0x14)) return 1;

    if(oled_WriteCommand(0x20)) return 1;
    if(oled_WriteCommand(0x00)) return 1;

    if(oled_WriteCommand(0xA1)) return 1;
    if(oled_WriteCommand(0xC8)) return 1;

    if(oled_WriteCommand(0xDA)) return 1;
    if(oled_WriteCommand(0x12)) return 1;

    if(oled_WriteCommand(0x81)) return 1;
    if(oled_WriteCommand(0xCF)) return 1;

    if(oled_WriteCommand(0xD9)) return 1;
    if(oled_WriteCommand(0xF1)) return 1;

    if(oled_WriteCommand(0xDB)) return 1;
    if(oled_WriteCommand(0x40)) return 1;

    if(oled_WriteCommand(0xA4)) return 1;
    if(oled_WriteCommand(0xA6)) return 1;

    if(oled_WriteCommand(0x2E)) return 1;

    if(oled_WriteCommand(0xAF)) return 1;

    oled_ClearBuffer();
    return oled_UpdateScreen();
}

static void oled_ClearBuffer(void)
{
    memset(oled_buffer, 0x00, OLED_BUFFER_SIZE);
}

static uint8_t oled_UpdateScreen(void)
{
    if(oled_WriteCommand(0x21)) return 1;               
    if(oled_WriteCommand(0))    return 1;              
    if(oled_WriteCommand(OLED_WIDTH - 1)) return 1;     

    if(oled_WriteCommand(0x22)) return 1;               
    if(oled_WriteCommand(0))    return 1;              
    if(oled_WriteCommand((OLED_HEIGHT / 8) - 1)) return 1; 

//    return oled_WriteData(oled_buffer, OLED_BUFFER_SIZE);
	
	return oled_WriteData_Async(oled_buffer, OLED_BUFFER_SIZE); 
}

static void oled_DrawPixel(uint8_t x, uint8_t y, uint8_t color)
{
    if (x >= OLED_WIDTH || y >= OLED_HEIGHT)
    {
        return;
    }

    uint16_t byte_index = x + (y / 8) * OLED_WIDTH;
    uint8_t bit_mask = 1 << (y % 8);

    if (color == 1)
    {
        oled_buffer[byte_index] |= bit_mask;
    }
    else
    {
        oled_buffer[byte_index] &= ~bit_mask;
    }
}

static uint8_t oled_DrawChar(uint8_t x, uint8_t y, char ch, uint8_t invert)
{
    if (ch < ' ' || ch > '~')
    {
        ch = '?';
    }

    const uint8_t* font_char_ptr = &font6x8[(ch - ' ') * 6];

    for (uint8_t col = 0; col < 6; col++)
    {
        uint8_t col_data = font_char_ptr[col];
        if (invert)
        {
            col_data = ~col_data;
        }

        for (uint8_t row = 0; row < 8; row++)
        {
            if ((col_data >> row) & 0x01)
            {
                oled_DrawPixel(x + col, y + row, 1);
            }
            else
            {
                oled_DrawPixel(x + col, y + row, 0);
            }
        }
    }
    return x + 6;
}

static void oled_DrawString(uint8_t x, uint8_t y, const char *str, uint8_t invert)
{
    uint8_t current_x = x;
    while (*str)
    {
        if (current_x + 6 > OLED_WIDTH)
        {
            break;
        }
        current_x = oled_DrawChar(current_x, y, *str, invert);
        str++;
    }
}

void oled_DisplayData_WithMode(float v_in, float v_out, float i_out, float Power,uint32_t duty_raw, WorkMode_t mode, uint8_t enabled)
{
    char display_str_buffer[22];
    extern float target_voltage;
    extern float target_current;

    oled_ClearBuffer();

    // ========== 左侧竖排（0-63像素宽）==========
    sprintf(display_str_buffer, "VIN:%.2fV", v_in);
    oled_DrawString(0, 0, display_str_buffer, 0);

    sprintf(display_str_buffer, "VO:%.2fV", v_out);
    oled_DrawString(0, 10, display_str_buffer, 0);

    sprintf(display_str_buffer, "IO:%.2fA", i_out);
    oled_DrawString(0, 20, display_str_buffer, 0);

    sprintf(display_str_buffer, "P:%.2fW", Power);
    oled_DrawString(0, 30, display_str_buffer, 0);

    // ========== 右侧竖排（70-127像素宽）==========
    float duty_percent = (float)duty_raw * 100.0f / 840.0f;
    sprintf(display_str_buffer, "PWM:%.0f%%", duty_percent);
    oled_DrawString(70, 0, display_str_buffer, 0);

    if (mode == MODE_CV)
    {
        sprintf(display_str_buffer, "CV MODE");
    }
    else
    {
        sprintf(display_str_buffer, "CC MODE");
    }
    oled_DrawString(70, 10, display_str_buffer, 0);

    if (mode == MODE_CV)
    {
        sprintf(display_str_buffer, "SET:%.1fV", target_voltage);
    }
    else
    {
        sprintf(display_str_buffer, "SET:%.1fA", target_current);
    }
    oled_DrawString(70, 20, display_str_buffer, 0);

    sprintf(display_str_buffer, "%s", enabled ? "ON" : "OFF");
    oled_DrawString(70, 30, display_str_buffer, 1);

    oled_UpdateScreen();
}
