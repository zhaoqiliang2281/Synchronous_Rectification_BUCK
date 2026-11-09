#include "bsp_adc.h"

static uint16_t adc_dma_buf[3];   // [0]=IOUT, [1]=VIN, [2]=VOUT

static uint16_t buf_vin[ADC_FILTER_SIZE];
static uint16_t buf_vout[ADC_FILTER_SIZE];
static uint16_t buf_iout[ADC_FILTER_SIZE];

static uint32_t sum_vin, sum_vout, sum_iout;
static uint8_t  idx = 0;

static float vin, vout, iout;


void BSP_ADC_Init(void) //初始化
{
    sum_vin = sum_vout = sum_iout = 0;
    idx = 0;
    HAL_ADC_Start_DMA(&hadc1, (uint32_t*)adc_dma_buf, 3);   //开启ADC DMA采集与转运
}

void BSP_ADC_Process(void)
{
    uint16_t cur_iout = adc_dma_buf[0];
    uint16_t cur_vin  = adc_dma_buf[1];
    uint16_t cur_vout = adc_dma_buf[2];

    // 滑动平均：减去最旧的，加进新的
    sum_iout += cur_iout - buf_iout[idx];
    sum_vin  += cur_vin  - buf_vin[idx];
    sum_vout += cur_vout - buf_vout[idx];

    buf_iout[idx] = cur_iout;
    buf_vin[idx]  = cur_vin;
    buf_vout[idx] = cur_vout;

    idx = (idx + 1) & (ADC_FILTER_SIZE - 1);

    uint16_t raw_vin  = sum_vin  >> ADC_FILTER_SHIFT;
    uint16_t raw_vout = sum_vout >> ADC_FILTER_SHIFT;
    uint16_t raw_iout = sum_iout >> ADC_FILTER_SHIFT;

    // 物理量换算（分压比/运放增益从原理图算的）
    vin   = raw_vin  * (ADC_VREF / 4095.0f) * 16.0f;    // 分压 1:16
    vout = raw_vout * (ADC_VREF / 4095.0f) * 5.99f;     // 分压 1:5.99  10/VOUT=1.67   VOUT= VADC X （10）
    iout = raw_iout * (ADC_VREF / (4095.0f * 2.5f));    // INA180A2: 50V/V × 50mΩ = 2.5 V/A

    // 零点死区
    if (iout < 0.003f){iout = 0.0f;}  
    if (vout < 0.4f){vout = 0.0f;} 
}

float BSP_ADC_Get_Vin(void)   { return vin; }   // 获取输入电压（单位：V）
float BSP_ADC_Get_Vout(void)  { return vout; }  // 获取输出电压（单位：V）
float BSP_ADC_Get_Iout(void)  { return iout; }  // 获取输出电流（单位：A）

