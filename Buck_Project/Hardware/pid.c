#include "pid.h"

void pid_init(PID_T *p, float kp, float ki, float kd, float target, float limit)
{
    p->kp = kp; p->ki = ki; p->kd = kd;
    p->target = target; p->limit = limit;
    p->integral = 0; p->last_error = 0; p->last2_error = 0; p->out = 0;
}

void pid_set_target(PID_T *p, float target) { p->target = target; }

void pid_reset(PID_T *p)
{
    p->integral = 0; p->last_error = 0; p->last2_error = 0; p->out = 0;
}

float pid_calc_incremental(PID_T *p, float current)
{
    float error = p->target - current;
    float p_out = p->kp * (error - p->last_error);
    float i_out = p->ki * error;
    float d_out = p->kd * (error - 2*p->last_error + p->last2_error);

    p->out += p_out + i_out + d_out;

    // 增量限幅
    if (p->out > p->limit) p->out = p->limit;
    if (p->out < -p->limit) p->out = -p->limit;

    p->last2_error = p->last_error;
    p->last_error = error;
    return p->out;
}

void pid_set_params(PID_T * p, float _kp, float _ki, float _kd)
{
    p->kp = _kp;
    p->ki = _ki;
    p->kd = _kd;
}
void pid_get_params(PID_T * p, float *_kp, float *_ki, float *_kd)
{
    *_kp = p->kp;   
    *_ki = p->ki;
    *_kd = p->kd;
}

float pid_get_target(PID_T *p)
{
	return p->target;
}
float pid_get_out(PID_T *p)
{
	return p->out;
}


/*
 * 命令格式(区分大小写):
 *   PI  -> KP += STEP
 *   Pi  -> KP -= STEP
 *   II  -> KI += STEP
 *   Ii  -> KI -= STEP
 *   DI  -> KD += STEP
 *   Di  -> KD -= STEP
 *
 * 逻辑:第 1 个字符是参数名(P/I/D),
 *       第 2 个字符是大小写(I=加, i=减)
 *       用 last_char 记住第一个字符,收到第二个再判断
 */

//static char last_char = 0;

//void HAL_UART_RxCpltCallback(UART_HandleTypeDef *huart)
//{
//    if (huart != &huart1) return;

//    uint8_t c = rx_byte;

//    /* 先看是否是"第二个字符"：I 或 i */
//    if (last_char != 0 && (c == 'I' || c == 'i'))
//    {
//        float delta = (c == 'I') ? PID_STEP : -PID_STEP;

//        switch (last_char)
//        {
//            case 'P': KP += delta; break;
//            case 'I': KI += delta; break;
//            case 'D': KD += delta; break;
//        }
//        pid_set_params(&pid_volt, KP, KI, KD);

//		//回传测试
////        char buf[64];
////        int n = snprintf(buf, sizeof(buf),
////                         "KP=%.3f KI=%.3f KD=%.3f\r\n", KP, KI, KD);
////        HAL_UART_Transmit(&huart1, (uint8_t*)buf, n, 10);

//        last_char = 0;
//    }
//    /* 否则看是否是新的参数名 */
//    else if (c == 'P' || c == 'I' || c == 'D')
//    {
//        last_char = c;
//    }
//    else
//    {
//        last_char = 0;
//    }
//	
//    HAL_UART_Receive_IT(&huart1, &rx_byte, 1);
//}

// 调试代码备份，测试部分
// PID_T pid_volt, pid_curr;
// float  target_voltage = 5.0f;
// float  target_current = 0.5f;
// uint8_t output_enabled = 0;

//	//自定义调节  
//	telemetry_frame_t frame;          /* ① 定义结构体（局部变量即可） */

//	frame.data[0]  = (float)output_enabled;           /* ② 只填 data[]，填几个都行 */
//	frame.data[1]  = (float)pid_get_target(&pid_volt);
//	frame.data[2]  = (float)pid_get_out(&pid_volt);
//	frame.data[3]  = (float)BSP_ADC_Get_Vin();
//	frame.data[4]  = (float)BSP_ADC_Get_Vout();
//	frame.data[5]  = (float)BSP_ADC_Get_Iout();
//	  
//	float KP_Temp = 0.0f;
//	float KI_Temp = 0.0f;
//	float KD_Temp = 0.0f;
//	pid_get_params(&pid_volt,&KP_Temp,&KI_Temp,&KD_Temp);
//	
//	frame.data[6]  = (float)KP_Temp;
//	frame.data[7]  = (float)KI_Temp;
//	frame.data[8]  = (float)KD_Temp;
//	
//	telemetry_send(&frame);           /* ③ 发走，自动转文本 + 加换行 */
	  
	  
//	  app_telemetry();
	  
//	printf("OUT=%d  VIN=%.1f  V=%.2f/%.2fV  I=%.3fA  duty=%d\r\n",
//	output_enabled, BSP_ADC_Get_Vin(), target_voltage,
//	BSP_ADC_Get_Vout(), BSP_ADC_Get_Iout(), BSP_PWM_GetDuty());
//	HAL_Delay(200);
	  
//	printf("OUT=%d  V=%.2f/%.2fV  I=%.3fA  duty=%d\r\n",
//	output_enabled, target_voltage, BSP_ADC_Get_Vout(),
//	BSP_ADC_Get_Iout(), BSP_PWM_GetDuty());

//	HAL_GPIO_TogglePin(USER_LED_GPIO_Port, USER_LED_Pin);
//	HAL_Delay(500);

	  
	/* 测试：逐步提高占空比，观察 VOUT 变化 */
//	BSP_ADC_Process();   // 每 50ms 跑一次滤波   
//	static uint32_t test_duty = 0;
//	test_duty = 400;
////	if (test_duty > 400) test_duty = 0;   // 循环 0~400
//	BSP_PWM_SetDuty(test_duty); 
//	
//	printf("duty=%3d  VIN=%.2f  VOUT=%.2f  IOUT=%.3f\r\n",
//		   test_duty, BSP_ADC_Get_Vin(), BSP_ADC_Get_Vout(), BSP_ADC_Get_Iout());
//		   
//	HAL_GPIO_TogglePin(USER_LED_GPIO_Port, USER_LED_Pin);
//	HAL_Delay(500);
	  
	  
//	printf("VIN=%.2fV  VOUT=%.2fV  IOUT=%.3fA\r\n",
//		   BSP_ADC_Get_Vin(), BSP_ADC_Get_Vout(), BSP_ADC_Get_Iout());

//	HAL_GPIO_TogglePin(USER_LED_GPIO_Port, USER_LED_Pin);
//	HAL_Delay(500);


//	HAL_GPIO_TogglePin(USER_LED_GPIO_Port, USER_LED_Pin);
//	printf("tick: %d ms\r\n", HAL_GetTick());
//	HAL_Delay(500);



//    BSP_ADC_Process();

//    float vin  = BSP_ADC_Get_Vin();
//    float vout = BSP_ADC_Get_Vout();
//    float iout = BSP_ADC_Get_Iout();

//    /* 保护：任何故障都关断 */
//	// 保护判断改成：
//	if (HAL_GetTick() - protect_enable_tick > 50)  // 开机50ms内不进保护
//	{
//		if (vin > 48.0f || vout > 16.0f || iout > 1.2f || vin < 18.0f)
//		{
//			output_enabled = 0;
//			BSP_PWM_SetDuty(0);
//			HAL_GPIO_WritePin(DC_EN_GPIO_Port, DC_EN_Pin, GPIO_PIN_RESET);  // 低 = 关断
//			pid_reset(&pid_volt);
//			pid_reset(&pid_curr);
//			return;
//		}
//	}
//	
//    if (!output_enabled) {
//        BSP_PWM_SetDuty(0);
//        return;
//    }

//    /* CV 模式：恒压控制 */
//    pid_set_target(&pid_volt, target_voltage);
//    float delta = pid_calc_incremental(&pid_volt, vout);

//    static uint32_t duty = 100;// 从最小占空比开始
//    duty += (int32_t)delta;
//    if (duty > 740) duty = 740;
//    if (duty < 100) duty = 100;

//    BSP_PWM_SetDuty(duty);
