/**
 * @file vofa.h
 * @brief 遥测数据组帧 + 调试输出接口
 * @date 2026-06-14
 *
 * 串口输出（USART1，115200 8N1）：
 *   遥测帧 : FireWater 文本协议，形如 "66.60,66.60,...,55.77\n"
 *            VOFA+ 里数据格式选 FireWater 即可直接绘图；
 *            串口助手直接打开也是可读的十进制文本，不会出现乱码。
 *   调试   : printf() -> fputc() -> vofa_putc()，与遥测共用同一路串口。
 */
#ifndef __VOFA_H
#define __VOFA_H

#include "main.h"

typedef unsigned char bool_t;

/* 遥测通道数：发送的是文本，改这个宏即可增减通道 */
#define TELEMETRY_CHANNEL_COUNT   9

/**
 * @brief 遥测数据结构
 *
 * 只需填 data[]，telemetry_send() 会把它转成
 * "v0,v1,...,v15\n" 的 ASCII 文本帧（保留 2 位小数）发出，
 * 不需要再自己处理帧头、帧尾或字节序。
 */
typedef struct {
  float data[TELEMETRY_CHANNEL_COUNT];  /* 各通道数值 */
} telemetry_frame_t;

/* ---- 遥测接口 ---- */
void telemetry_init(void);
void telemetry_send(telemetry_frame_t *frame);

/* ---- 调试输出接口 ---- */
bool_t vofa_send_raw(const uint8_t *data, uint16_t len);
bool_t vofa_printf(const char *fmt, ...);
void vofa_init(void);

/**
 * @brief printf 重定向用的单字节发送（USART1）
 * @note  本工程使用 MicroLIB，printf 不支持 %f；
 *        要打印浮点请用 telemetry_send()。
 */
void vofa_putc(char ch);


void app_telemetry(void);

#endif /* __VOFA_H */
