#include "bsp_pwm.h"
#include "tim.h"

static uint32_t duty = 0;

void BSP_PWM_Init(void)
{
    duty = 0;
    __HAL_TIM_SET_COMPARE(&htim1, TIM_CHANNEL_1, duty);
    HAL_TIM_PWM_Start(&htim1, TIM_CHANNEL_1);
}

void BSP_PWM_SetDuty(uint32_t raw)
{
    if (raw == 0) 
	{
        duty = 0;
    } 
	else if (raw > MAX_DUTY_RAW) 
	{
        duty = MAX_DUTY_RAW;
    } 
	else if (raw < MIN_DUTY_RAW) 
	{
        duty = MIN_DUTY_RAW;
    } 
	else 
	{
        duty = raw;
    }
    __HAL_TIM_SET_COMPARE(&htim1, TIM_CHANNEL_1, duty);
}

uint32_t BSP_PWM_GetDuty(void) { return duty; }



