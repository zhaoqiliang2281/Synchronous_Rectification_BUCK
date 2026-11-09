#ifndef TJC_HMI_H
#define TJC_HMI_H

#include "main.h"
#include "config.h"
#include "usart.h"


// TJC HMI 通信接收缓冲区大小
#define TJC_RX_BUFFER_SIZE 256

// 协议帧头与结束字节定义
#define TJC_FRAME_HEAD     0x5A
#define TJC_FRAME_END_1    0xFF
#define TJC_FRAME_END_2    0xFF
#define TJC_FRAME_END_3    0xFF

// HMI 命令字定义（MCU <- HMI），用于模拟物理按键操作
#define CMD_VOL_UP         0x01  // 电压+（模拟 KEY1）
#define CMD_VOL_DN         0x02  // 电压-（模拟 KEY3）
#define CMD_CUR_UP         0x03  // 电流+（模拟 KEY1）
#define CMD_CUR_DN         0x04  // 电流-（模拟 KEY3）
#define CMD_SET_MODE       0x05  // 工作模式切换
#define CMD_OUTPUT_CTRL    0x06  // 输出开关控制

// 故障代码定义
#define FAULT_NONE         0x00  // 无故障
#define FAULT_OVP          0x01  // 过压故障
#define FAULT_OCP          0x02  // 过流故障
#define FAULT_UVP          0x04  // 欠压故障

// TJC HMI 模块对外函数声明
void TJC_HMI_Init(void);
void TJC_HMI_SendData(float vin, float vout, float iout, float power, uint16_t pwm_duty, WorkMode_t mode, uint8_t enabled, uint8_t fault);
void TJC_HMI_Process(void);
void TJC_HMI_RxCallback(uint8_t data);

#endif // TJC_HMI_H

