#ifndef BSP_ADC_H_
#define BSP_ADC_H_

#include "main.h"
#include "adc.h"
#include "config.h"

#define ADC_VREF          3.36f  // 实际量一下 LDO 输出，可能要微调
#define ADC_FILTER_SIZE   16     // 16 点滑动平均
#define ADC_FILTER_SHIFT  4      // 2^4 = 16，右移4位相当于除以16

void  BSP_ADC_Init(void);
void  BSP_ADC_Process(void);
float BSP_ADC_Get_Vin(void);
float BSP_ADC_Get_Vout(void);
float BSP_ADC_Get_Iout(void);

#endif
