/* USER CODE BEGIN Header */
/**
  ******************************************************************************
  * @file           : main.c
  * @brief          : Main program body
  ******************************************************************************
  * @attention
  *
  * Copyright (c) 2026 STMicroelectronics.
  * All rights reserved.
  *
  * This software is licensed under terms that can be found in the LICENSE file
  * in the root directory of this software component.
  * If no LICENSE file comes with this software, it is provided AS-IS.
  *
  ******************************************************************************
  */
/* USER CODE END Header */
/* Includes ------------------------------------------------------------------*/
#include "main.h"
#include "adc.h"
#include "dma.h"
#include "i2c.h"
#include "tim.h"
#include "usart.h"
#include "gpio.h"

/* Private includes ----------------------------------------------------------*/
/* USER CODE BEGIN Includes */
#include <stdio.h>
#include <string.h>
#include "bsp_adc.h"
#include "bsp_pwm.h"
#include "pid.h"
#include "Vofa.h"
#include "key.h"  
#include <math.h>   // 用于fabsf函数
#include "tjc_hmi.h"
#include "oled.h" 
#include "config.h"


// PID 控制器结构体（全局变量，定时器中断和主循环都需要访问）
PID_T pid_voltage;   // 电压PID控制器（CV模式）
PID_T pid_current;   // 电流PID控制器（CC模式）


float target_voltage = 5.0f;	// 目标电压值（单位：V），范围：5.0V ~ 15.0V
float target_current = 0.5f;	// 目标电流值（单位：A），范围：0.1A ~ 1.0A
uint8_t output_enabled = 0;		// 输出使能状态标志（0=关闭，1=开启）
uint32_t pwm_out = 0;			// 当前PWM占空比值（用于HMI显示，0-839）

WorkMode_t work_mode = MODE_CV;// 工作模式（默认恒压模式)


/* USER CODE END Includes */

/* Private typedef -----------------------------------------------------------*/
/* USER CODE BEGIN PTD */

/* USER CODE END PTD */

/* Private define ------------------------------------------------------------*/
/* USER CODE BEGIN PD */

/* USER CODE END PD */

/* Private macro -------------------------------------------------------------*/
/* USER CODE BEGIN PM */

/* USER CODE END PM */

/* Private variables ---------------------------------------------------------*/

/* USER CODE BEGIN PV */


/* USER CODE END PV */

/* Private function prototypes -----------------------------------------------*/
void SystemClock_Config(void);
/* USER CODE BEGIN PFP */

/* USER CODE END PFP */

/* Private user code ---------------------------------------------------------*/
/* USER CODE BEGIN 0 */


/* USER CODE END 0 */

/**
  * @brief  The application entry point.
  * @retval int
  */
int main(void)
{

  /* USER CODE BEGIN 1 */

  /* USER CODE END 1 */

  /* MCU Configuration--------------------------------------------------------*/

  /* Reset of all peripherals, Initializes the Flash interface and the Systick. */
  HAL_Init();

  /* USER CODE BEGIN Init */

  /* USER CODE END Init */

  /* Configure the system clock */
  SystemClock_Config();

  /* USER CODE BEGIN SysInit */

  /* USER CODE END SysInit */

  /* Initialize all configured peripherals */
  MX_GPIO_Init();
  MX_DMA_Init();
  MX_ADC1_Init();
  MX_I2C1_Init();
  MX_TIM1_Init();
  MX_TIM2_Init();
  MX_USART1_UART_Init();
  /* USER CODE BEGIN 2 */

  output_enabled = 0;
//  HAL_GPIO_WritePin(DC_EN_GPIO_Port, DC_EN_Pin, GPIO_PIN_SET);  // 高 = 使能
  HAL_GPIO_WritePin(DC_EN_GPIO_Port, DC_EN_Pin, GPIO_PIN_RESET);  // 低 = 关断（安全）

  oled_Init(); 
  BSP_ADC_Init();
  BSP_PWM_Init();
  TJC_HMI_Init();
  
  
  // 初始化电压PID（CV模式）
  pid_init(&pid_voltage, 18.0f, 2.0f, 4.5f, 5.0f, 740);//kp,ki,kd

  // 初始化电流PID（CC模式）- 参数需根据实际调试
  pid_init(&pid_current, 20.0f, 0.6f, 1.2f, 0.5f, 740);//kp,ki,kd


  HAL_TIM_Base_Start_IT(&htim2);   // 启动 1kHz 控制中断


  /* 等 1 秒让 ADC 采样稳定，再开输出 */
  HAL_Delay(1000);


  /* USER CODE END 2 */

  /* Infinite loop */
  /* USER CODE BEGIN WHILE */
  while (1)
  {
		float v_in   = BSP_ADC_Get_Vin();	  	//获取输入电压
		float v_out  = BSP_ADC_Get_Vout();		//获取输出电压
		float i_out  = BSP_ADC_Get_Iout();		//获取输出电流
		uint32_t d_raw = BSP_PWM_GetDuty();		//获取设置的PWM占空比


		// 定时发送数据到串口屏（每100ms发送一次）
		static uint32_t hmi_last_time = 0;
		uint32_t current_time = HAL_GetTick();
		if (current_time - hmi_last_time >= 100)
		{
			hmi_last_time = current_time; // 更新上次发送时间
			float power = v_out * i_out;  // 计算输出功率（输出电压 X 输出电流）
			uint8_t fault_code = 0;       // 初始化故障码为0（无故障）
			
		// 检测故障（优先级：输入过压 > 输出过压 > 输出过流 > 输入欠压，只报告最严重的一个）
		if (v_in > INPUT_OVER_VOLTAGE_THRESHOLD)
		{
			fault_code = 3;  // 输入过压
		}
		else if (v_out > OVER_VOLTAGE_THRESHOLD)
		{
			fault_code = 1;  // 输出过压
		}
		else if (i_out > OVER_CURRENT_THRESHOLD)
		{
			fault_code = 2;  // 输出过流
		}
		else if (v_in < UNDER_VOLTAGE_THRESHOLD)
		{
			fault_code = 4;  // 输入欠压
		}
			
			// 使用和OLED相同的PWM数据源（d_raw），确保显示一致
			TJC_HMI_SendData(v_in, v_out, i_out, power, d_raw, work_mode, output_enabled, fault_code);
			// 显示模式、输出状态、电压电流等信息
			oled_DisplayData_WithMode(v_in, v_out, i_out, power, d_raw, work_mode, output_enabled);
		}
		
		// 处理来自串口屏的命令
		TJC_HMI_Process();
	
		if(output_enabled){HAL_GPIO_TogglePin(USER_LED_GPIO_Port, USER_LED_Pin);}//代码运行指示灯
	  
	  // HAL_Delay(50);

    /* USER CODE END WHILE */

    /* USER CODE BEGIN 3 */
  }
  /* USER CODE END 3 */
}

/**
  * @brief System Clock Configuration
  * @retval None
  */
void SystemClock_Config(void)
{
  RCC_OscInitTypeDef RCC_OscInitStruct = {0};
  RCC_ClkInitTypeDef RCC_ClkInitStruct = {0};

  /** Configure the main internal regulator output voltage
  */
  __HAL_RCC_PWR_CLK_ENABLE();
  __HAL_PWR_VOLTAGESCALING_CONFIG(PWR_REGULATOR_VOLTAGE_SCALE2);

  /** Initializes the RCC Oscillators according to the specified parameters
  * in the RCC_OscInitTypeDef structure.
  */
  RCC_OscInitStruct.OscillatorType = RCC_OSCILLATORTYPE_HSE;
  RCC_OscInitStruct.HSEState = RCC_HSE_ON;
  RCC_OscInitStruct.PLL.PLLState = RCC_PLL_ON;
  RCC_OscInitStruct.PLL.PLLSource = RCC_PLLSOURCE_HSE;
  RCC_OscInitStruct.PLL.PLLM = 4;
  RCC_OscInitStruct.PLL.PLLN = 84;
  RCC_OscInitStruct.PLL.PLLP = RCC_PLLP_DIV2;
  RCC_OscInitStruct.PLL.PLLQ = 4;
  if (HAL_RCC_OscConfig(&RCC_OscInitStruct) != HAL_OK)
  {
    Error_Handler();
  }

  /** Initializes the CPU, AHB and APB buses clocks
  */
  RCC_ClkInitStruct.ClockType = RCC_CLOCKTYPE_HCLK|RCC_CLOCKTYPE_SYSCLK
                              |RCC_CLOCKTYPE_PCLK1|RCC_CLOCKTYPE_PCLK2;
  RCC_ClkInitStruct.SYSCLKSource = RCC_SYSCLKSOURCE_PLLCLK;
  RCC_ClkInitStruct.AHBCLKDivider = RCC_SYSCLK_DIV1;
  RCC_ClkInitStruct.APB1CLKDivider = RCC_HCLK_DIV2;
  RCC_ClkInitStruct.APB2CLKDivider = RCC_HCLK_DIV1;

  if (HAL_RCC_ClockConfig(&RCC_ClkInitStruct, FLASH_LATENCY_2) != HAL_OK)
  {
    Error_Handler();
  }
}

/* USER CODE BEGIN 4 */



void HAL_TIM_PeriodElapsedCallback(TIM_HandleTypeDef *htim)
{
    if (htim->Instance != TIM2) return;

		BSP_ADC_Process();  // 1kHz ADC采样处理（在定时器中断中调用，确保实时性）
		KEY_Scan(); 		//按键扫描
		// 1. 读取ADC数据（ADC_Process在main循环中调用，这里直接读取已处理的数据）
		float v_in   = BSP_ADC_Get_Vin();
		float v_out  = BSP_ADC_Get_Vout();
		float i_out  = BSP_ADC_Get_Iout();

		// 2. 安全检查（保护逻辑）
		if (v_in > INPUT_OVER_VOLTAGE_THRESHOLD || v_out > OVER_VOLTAGE_THRESHOLD || i_out > OVER_CURRENT_THRESHOLD || v_in < UNDER_VOLTAGE_THRESHOLD)
		{
			// 保护触发，关闭输出
			output_enabled = 0;	 // 关闭输出
			BSP_PWM_SetDuty(0);  // 使用统一接口停止PWM（0值表示完全关闭）
			HAL_GPIO_WritePin(DC_EN_GPIO_Port, DC_EN_Pin, GPIO_PIN_RESET);// 低 = 关断（安全）
			pwm_out = 0;         // 清零PWM显示值
			pid_reset(&pid_voltage);// 重置电压PID，清除历史误差
			pid_reset(&pid_current);// 重置电流PID，清除历史误差
		}
		
		// 3. PID控制逻辑（支持CV/CC模式切换）
		if (output_enabled)
		{
			float pwm_out_float = 0.0f;
			
			if (work_mode == MODE_CV)
			{
				// CV模式：恒压控制
				pid_set_target(&pid_voltage, target_voltage);
				pwm_out_float = pid_calc_incremental(&pid_voltage, v_out);
				pwm_out = target_voltage + (uint32_t)pwm_out_float;
				
				// 限制PWM输出在安全范围内
				if(pwm_out >= MAX_DUTY_RAW)
					pwm_out = MAX_DUTY_RAW;
				else if(pwm_out <= MIN_DUTY_RAW)
					pwm_out = MIN_DUTY_RAW;
				
				BSP_PWM_SetDuty(pwm_out);
			}
			else  // MODE_CC
			{
				// CC模式：恒流控制
				pid_set_target(&pid_current, target_current);
				pwm_out_float = pid_calc_incremental(&pid_current, i_out);
				pwm_out = target_current + (uint32_t)pwm_out_float;  // 电流单位转换
				
				// 限制PWM输出在安全范围内
				if(pwm_out >= MAX_DUTY_RAW)
					pwm_out = MAX_DUTY_RAW;
				else if(pwm_out <= MIN_DUTY_RAW)
					pwm_out = MIN_DUTY_RAW;
				
				BSP_PWM_SetDuty(pwm_out);
			}
		}
		else if(output_enabled == 0)
		{
			// 输出关闭时，停止PWM，并重置PID状态
			BSP_PWM_SetDuty(0);       // 使用统一接口停止PWM（0值表示完全关闭）
			pwm_out = 0;              // 清零PWM显示值
			pid_reset(&pid_voltage);  // 重置PID，清除历史误差
			pid_reset(&pid_current);  // 重置电流PID
		}
	
}

/* USER CODE END 4 */

/**
  * @brief  This function is executed in case of error occurrence.
  * @retval None
  */
void Error_Handler(void)
{
  /* USER CODE BEGIN Error_Handler_Debug */
  /* User can add his own implementation to report the HAL error return state */
  __disable_irq();
  while (1)
  {
  }
  /* USER CODE END Error_Handler_Debug */
}

#ifdef  USE_FULL_ASSERT
/**
  * @brief  Reports the name of the source file and the source line number
  *         where the assert_param error has occurred.
  * @param  file: pointer to the source file name
  * @param  line: assert_param error line source number
  * @retval None
  */
void assert_failed(uint8_t *file, uint32_t line)
{
  /* USER CODE BEGIN 6 */
  /* User can add his own implementation to report the file name and line number,
     ex: printf("Wrong parameters value: file %s on line %d\r\n", file, line) */
  /* USER CODE END 6 */
}
#endif /* USE_FULL_ASSERT */
