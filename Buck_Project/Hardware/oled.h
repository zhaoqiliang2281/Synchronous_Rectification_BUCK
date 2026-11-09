#ifndef OLED_H_ 
#define OLED_H_

#include "main.h" 
#include "key.h"  // 包含WorkMode_t定义

#define OLED_I2C_ADDR       0x78
#define OLED_WIDTH          128
#define OLED_HEIGHT         64
#define OLED_BUFFER_SIZE    (OLED_WIDTH * OLED_HEIGHT / 8)


uint8_t oled_Init(void);

void oled_DisplayData_WithMode(float v_in, float v_out, float i_out,float Power, uint32_t duty_raw, WorkMode_t mode, uint8_t enabled);

#endif /* OLED_H_ */
