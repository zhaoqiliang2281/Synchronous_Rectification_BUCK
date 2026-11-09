#ifndef CONFIG_H_
#define CONFIG_H_

#include "main.h"
#include <stdio.h>
#include <string.h>
#include <math.h> // 用于 roundf() 函数#include <stdio.h>
#include <string.h>

// 工作模式枚举
typedef enum {
    MODE_CV = 0,  // 恒压模式
    MODE_CC = 1   // 恒流模式
} WorkMode_t;


//工程全局变量外部声明
extern WorkMode_t work_mode;      // 工作模式（CV/CC）


// 外部变量声明
extern float target_voltage;      // 目标电压值（单位：V）
extern float target_current;      // 目标电流值（单位：A）
extern uint8_t output_enabled;    // 输出使能状态（0=关闭，1=开启）

extern uint32_t pwm_out;          // 当前PWM占空比值（用于HMI显示，0-839）



// 按键增减设置
#define VOLTAGE_STEP 1.0f        // 电压步进值：1.0 V
#define CURRENT_STEP 0.10f       // 电流步进值：0.10 A

//设定范围
#define MIN_VOLTAGE  5.0f        // 电压最小值
#define MAX_VOLTAGE  19.0f       // 电压最大值
#define MIN_CURRENT  0.1f        // 电流最小值
#define MAX_CURRENT  1.0f        // 电流最大值


// 保护阈值
#define OVER_VOLTAGE_THRESHOLD    		20.0f  	// 输出过压保护阈值（V）
#define OVER_CURRENT_THRESHOLD    		1.2f   	// 过流保护阈值（A）
#define INPUT_OVER_VOLTAGE_THRESHOLD  	48.0f  	// 输入过压保护阈值（V）
#define UNDER_VOLTAGE_THRESHOLD   		18.0f  	// 输入欠压保护阈值（V）







#endif /* CONFIG_H_ */
