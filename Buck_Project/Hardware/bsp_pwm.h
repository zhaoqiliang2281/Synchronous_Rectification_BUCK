#ifndef BSP_PWM_H_
#define BSP_PWM_H_

#include "main.h"

#define PWM_PERIOD     839    // 100kHz
#define MIN_DUTY_RAW   100    // ~12% 最小占空比
#define MAX_DUTY_RAW   740    // ~88% 最大占空比

void BSP_PWM_Init(void);
void BSP_PWM_SetDuty(uint32_t raw);   // 0 = 完全关闭
uint32_t BSP_PWM_GetDuty(void);

#endif
