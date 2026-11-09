/**
 * @file vofa.c
 * @brief 遥测结构体发送（FireWater 文本帧） + 调试输出
 * @date 2026-06-14
 */
#include "vofa.h"
#include "usart.h"
#include "main.h"
#include <stdarg.h>
#include <stdio.h>
#include <string.h>

/* 等 USART1 空闲（上一帧 DMA / 阻塞发送结束）的最长等待时间 */
#define UART1_TX_WAIT_MS   50U

#define TELEMETRY_TX_BUF_SIZE 256U
#define VOFA_TX_BUF_SIZE      256U

static char    g_telemetry_tx_buf[TELEMETRY_TX_BUF_SIZE];
static uint8_t g_vofa_tx_buf[VOFA_TX_BUF_SIZE];

/* USART1 只有一个发送通道：任何输出前都要等它空闲，
   否则 HAL_UART_Transmit/Transmit_DMA 直接返回 HAL_BUSY，字节被静默丢弃 */
static bool_t uart1_wait_ready(uint32_t timeout_ms)
{
  uint32_t start = HAL_GetTick();

  while (huart1.gState != HAL_UART_STATE_READY)
  {
    if ((uint32_t)(HAL_GetTick() - start) >= timeout_ms)
      return 0;
  }
  return 1;
}

/* float -> "123.45"（保留 2 位小数）
   不用 printf("%f")：本工程链接 MicroLIB，标准库的浮点格式化不可用 */
static uint16_t format_float(char *dst, float value)
{
  char     digits[12];
  uint16_t n = 0;
  uint16_t k = 0;
  uint32_t int_part;
  uint32_t frac_part;

  if (!(value > -1.0e9f && value < 1.0e9f))   /* NaN / ±Inf / 超范围 */
  {
    dst[0] = '0'; dst[1] = '.'; dst[2] = '0'; dst[3] = '0';
    return 4;
  }

  if (value < 0.0f)
  {
    dst[n++] = '-';
    value = -value;
  }

  int_part  = (uint32_t)value;
  frac_part = (uint32_t)((value - (float)int_part) * 100.0f + 0.5f);
  if (frac_part >= 100U)
  {
    int_part  += 1U;
    frac_part -= 100U;
  }

  do
  {
    digits[k++] = (char)('0' + (int_part % 10U));
    int_part /= 10U;
  } while (int_part != 0U);

  while (k > 0U)
    dst[n++] = digits[--k];

  dst[n++] = '.';
  dst[n++] = (char)('0' + (frac_part / 10U));
  dst[n++] = (char)('0' + (frac_part % 10U));
  return n;
}

/* ================= 遥测实现 ================= */

void telemetry_init(void)
{
  /* 无状态 */
}

void telemetry_send(telemetry_frame_t *frame)
{
  uint16_t len = 0;
  uint32_t i;

  if (frame == 0)
    return;

  /* 上一帧还在 DMA 发送中就丢掉这一帧，保证固定发送周期 */
  if (huart1.gState != HAL_UART_STATE_READY)
    return;

  for (i = 0; i < TELEMETRY_CHANNEL_COUNT; i++)
  {
    if ((uint32_t)len + 16U > TELEMETRY_TX_BUF_SIZE)
      return;

    len += format_float(&g_telemetry_tx_buf[len], frame->data[i]);
    g_telemetry_tx_buf[len++] =
        (i + 1U < TELEMETRY_CHANNEL_COUNT) ? ',' : '\n';
  }

  (void)HAL_UART_Transmit_DMA(&huart1, (uint8_t *)g_telemetry_tx_buf, len);
}

/* ================= 调试输出 ================= */

void vofa_init(void) {}

void vofa_putc(char ch)
{
  uint8_t byte = (uint8_t)ch;

  if (!uart1_wait_ready(UART1_TX_WAIT_MS))
    return;

  (void)HAL_UART_Transmit(&huart1, &byte, 1U, 10U);
}

bool_t vofa_send_raw(const uint8_t *data, uint16_t len)
{
  if ((data == 0) || (len == 0) || (len > VOFA_TX_BUF_SIZE)) return 0;
  if (!uart1_wait_ready(UART1_TX_WAIT_MS)) return 0;
  memcpy(g_vofa_tx_buf, data, len);
  return (HAL_UART_Transmit_DMA(&huart1, g_vofa_tx_buf, len) == HAL_OK) ? 1 : 0;
}

bool_t vofa_printf(const char *fmt, ...)
{
  va_list args; int32_t write_len;
  if (fmt == 0) return 0;

  /* 先等上一帧发完再改写 g_vofa_tx_buf，避免破坏正在 DMA 的数据 */
  if (!uart1_wait_ready(UART1_TX_WAIT_MS)) return 0;

  va_start(args, fmt);
  write_len = vsnprintf((char *)g_vofa_tx_buf, VOFA_TX_BUF_SIZE, fmt, args);
  va_end(args);
  if ((write_len <= 0) || (write_len >= (int32_t)VOFA_TX_BUF_SIZE)) return 0;
  if (g_vofa_tx_buf[write_len - 1] != '\n') {
    if (write_len >= (int32_t)(VOFA_TX_BUF_SIZE - 1U)) return 0;
    g_vofa_tx_buf[write_len++] = '\n';
  }
  return (HAL_UART_Transmit_DMA(&huart1, g_vofa_tx_buf, (uint16_t)write_len) == HAL_OK) ? 1 : 0;
}

void app_telemetry(void)
{
  telemetry_frame_t frame;          /* ① 定义结构体（局部变量即可） */

  frame.data[0]  = 66.6f;           /* ② 只填 data[]，填几个都行 */
  frame.data[1]  = 444.77f;
  frame.data[2]  = 654.9f;
  frame.data[3]  = 2.3456f;
  frame.data[4]  = 66.6f;

//  frame.data[15] = 55.77f;

  telemetry_send(&frame);           /* ③ 发走，自动转文本 + 加换行 */
}

int fputc (int ch, FILE *f) {
HAL_UART_Transmit(&huart1, (uint8_t *)&ch, 1, 0xFFFF);
return ch;
}
