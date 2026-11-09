#include "tjc_hmi.h"

// 内部函数声明，处理接收到的 HMI 命令
static void TJC_HMI_ProcessCommand(uint8_t cmd, uint8_t *data, uint8_t len);


// UART 接收环形缓冲区，用于缓存串口接收的数据
static uint8_t rx_buffer[TJC_RX_BUFFER_SIZE];
static volatile uint16_t rx_write_pos = 0;  // 写指针
static volatile uint16_t rx_read_pos = 0;   // 读指针

// UART 单字节接收缓存（用于中断接收）
static uint8_t uart_rx_byte = 0;

/**
 * @brief  初始化 TJC HMI 模块
 */
void TJC_HMI_Init(void)
{
    rx_write_pos = 0;
    rx_read_pos = 0;
    
    // 启动 UART 接收中断
    HAL_UART_Receive_IT(&huart1, &uart_rx_byte, 1);
}

/**
 * @brief  UART 接收回调函数（由中断调用）
 * @param  data: 接收到的字节
 */
void TJC_HMI_RxCallback(uint8_t data)
{
    // 将数据存入环形缓冲区
    rx_buffer[rx_write_pos] = data;// 存储接收到的字节
    rx_write_pos = (rx_write_pos + 1) % TJC_RX_BUFFER_SIZE;// 环形缓冲区写指针循环
    
    // 继续接收下一字节
    HAL_UART_Receive_IT(&huart1, &uart_rx_byte, 1);
}

/**
 * @brief  向 HMI 屏幕发送数据
 * @param  vin: 输入电压 (V)
 * @param  vout: 输出电压 (V)
 * @param  iout: 输出电流 (A)
 * @param  power: 输出功率 (W)
 * @param  mode: 工作模式 (MODE_CV 或 MODE_CC)
 * @param  enabled: 输出使能状态
 * @param  fault: 故障代码
 */
void TJC_HMI_SendData(float vin, float vout, float iout, float power, uint16_t pwm_duty, WorkMode_t mode, uint8_t enabled, uint8_t fault)
{
    static char tx_buf[512];  // 增大缓冲区，确保安全传输
    int len = 0;
    
    // 将 PWM 占空比转换为 0-100% 的百分比，并进行四舍五入
    uint8_t pwm_percent = (uint8_t)(((float)pwm_duty * 100.0f / 840.0f) + 0.5f);  // 四舍五入
    
    // 将所有数据组合为一条消息（无延时，一次性发送）
    // 说明：iout 和 tgtcur 使用两位小数（x100），而不是三位小数（x1000）
    len = sprintf(tx_buf, "vin.val=%d\xff\xff\xff"
                         "vout.val=%d\xff\xff\xff"
                         "iout.val=%d\xff\xff\xff"
                         "power.val=%d\xff\xff\xff"
                         "pwmduty.val=%d\xff\xff\xff"
                         "mode.val=%d\xff\xff\xff"
                         "outsta.val=%d\xff\xff\xff"
                         "tgtvol.val=%d\xff\xff\xff"
                         "tgtcur.val=%d\xff\xff\xff"
                         "fault.val=%d\xff\xff\xff",
                  (int)(vin * 100),
                  (int)(vout * 100),
                  (int)(iout * 100),
                  (int)(power * 100),
                  (int)pwm_percent,
                  (int)mode,
                  (int)enabled,
                  (int)(target_voltage * 100),
                  (int)(target_current * 100),
                  (int)fault);
    
    // 一次性发送全部数据（阻塞模式，但在主循环中调用，不在中断中使用）
//    HAL_UART_Transmit(&huart1, (uint8_t*)tx_buf, len, 100);
				  
	// 注意：pData 参数类型为 const uint8_t*，且 Size 类型为 uint16_t
	HAL_UART_Transmit_DMA(&huart1, (uint8_t*)tx_buf, (uint16_t)len);	// 使用 DMA 发送数据，避免阻塞主循环
			  
}

/**
 * @brief  处理来自 HMI 的接收数据
 *         协议格式：0x5A + CMD + DATA + 0xFF 0xFF 0xFF
 */
void TJC_HMI_Process(void)
{
    // 检查环形缓冲区中是否有可处理数据
    while (rx_read_pos != rx_write_pos)// 如果读指针不等于写指针，说明有数据可处理
    {
        uint8_t byte = rx_buffer[rx_read_pos];// 读取当前字节
        rx_read_pos = (rx_read_pos + 1) % TJC_RX_BUFFER_SIZE;// 环形缓冲区读指针循环
        
        // 简单帧解析器
        static uint8_t frame_state = 0; // 状态机状态：0=等待帧头，1=读取命令，2=读取数据，3=检查结束字节1，4=检查结束字节2
        static uint8_t cmd = 0;         // 当前命令字
        static uint8_t data_buf[16];    //  数据缓冲区，最多16字节
        // static uint8_t data_len = 0;    // 数据长度
        static uint8_t data_idx = 0;    // 数据索引
        
        switch (frame_state)
        {
            case 0:  // 等待帧头
                if (byte == TJC_FRAME_HEAD)// 如果接收到帧头字节 0x5A
                {
                    frame_state = 1;// 进入读取命令状态
                    data_idx = 0;   // 重置数据索引
                }
                break;
                
            case 1:  // 读取命令字
                cmd = byte;     // 保存命令字
                frame_state = 2;// 进入读取数据状态
                break;
                
            case 2:  // 读取数据
                if (byte == TJC_FRAME_END_1)// 如果接收到第一个结束字节 0xFF
                {
                    frame_state = 3;// 检查第一个结束字节
                }
                else
                {
                    if (data_idx < 16)// 如果数据索引小于缓冲区大小，保存数据字节
                    {
                        data_buf[data_idx++] = byte;// 保存数据字节
                    }
                }
                break;
                
            case 3:  // 检查第二个结束字节
                if (byte == TJC_FRAME_END_2)
                {
                    frame_state = 4;// 检查第三个结束字节
                }
                else
                {
                    frame_state = 0;  // 非法帧
                }
                break;
                
            case 4:  // 检查第三个结束字节
                if (byte == TJC_FRAME_END_3)
                {
                    // 接收到有效帧，处理命令
                    TJC_HMI_ProcessCommand(cmd, data_buf, data_idx);
                }
                frame_state = 0;  // 重置，准备接收下一帧
                break;
                
            default:
                frame_state = 0;
                break;
        }
    }
}

/**
 * @brief  处理 HMI 命令
 * @param  cmd: 命令 ID
 * @param  data: 数据缓冲区
 * @param  len: 数据长度
 */
static void TJC_HMI_ProcessCommand(uint8_t cmd, uint8_t *data, uint8_t len)
{
    (void)data;  // 未使用的参数
    (void)len;   // 未使用的参数
    
    switch (cmd)
    {
        case CMD_VOL_UP:  // 电压+（模拟 KEY1 在 CV 模式）
            if (work_mode == MODE_CV)
            {
                if (target_voltage < MAX_VOLTAGE)
                {
                    target_voltage += VOLTAGE_STEP;
                    if (target_voltage > MAX_VOLTAGE)
                        target_voltage = MAX_VOLTAGE;
                    // 四舍五入到小数点后一位，避免浮点精度误差
                    target_voltage = roundf(target_voltage * 10.0f) / 10.0f;
                }
            }
            break;
            
        case CMD_VOL_DN:  // 电压-（模拟 KEY3 在 CV 模式）
            if (work_mode == MODE_CV)
            {
                if (target_voltage > MIN_VOLTAGE)
                {
                    target_voltage -= VOLTAGE_STEP;
                    if (target_voltage < MIN_VOLTAGE)
                        target_voltage = MIN_VOLTAGE;
                    // 四舍五入到小数点后一位，避免浮点精度误差
                    target_voltage = roundf(target_voltage * 10.0f) / 10.0f;
                }
            }
            break;
            
        case CMD_CUR_UP:  // 电流+（模拟 KEY1 在 CC 模式）
            if (work_mode == MODE_CC)
            {
                if (target_current < MAX_CURRENT)
                {
                    target_current += CURRENT_STEP;
                    if (target_current > MAX_CURRENT)
                        target_current = MAX_CURRENT;
                    // 四舍五入到小数点后两位，避免浮点精度误差
                    target_current = roundf(target_current * 100.0f) / 100.0f;
                }
            }
            break;
            
        case CMD_CUR_DN:  // 电流-（模拟 KEY3 在 CC 模式）
            if (work_mode == MODE_CC)
            {
                if (target_current > MIN_CURRENT)
                {
                    target_current -= CURRENT_STEP;
                    if (target_current < MIN_CURRENT)
                        target_current = MIN_CURRENT;
                    // 四舍五入到小数点后两位，避免浮点精度误差
                    target_current = roundf(target_current * 100.0f) / 100.0f;
                }
            }
            break;
            
        case CMD_SET_MODE:  // 设置工作模式
            if (len >= 1)
            {
                if (data[0] == 0)
                    work_mode = MODE_CV;
                else if (data[0] == 1)
                    work_mode = MODE_CC;
                    
                // 切换模式时自动关闭输出
                if (output_enabled)
                {
                    output_enabled = 0;
                    HAL_GPIO_WritePin(DC_EN_GPIO_Port, DC_EN_Pin, GPIO_PIN_RESET);
                }
            }
            break;
            
        case CMD_OUTPUT_CTRL:  // 输出开/关控制
            if (len >= 1)
            {
                if (data[0] == 1 && !output_enabled)
                {
                    // 打开输出
                    output_enabled = 1;
                    HAL_GPIO_WritePin(DC_EN_GPIO_Port, DC_EN_Pin, GPIO_PIN_SET);
                }
                else if (data[0] == 0 && output_enabled)
                {
                    // 关闭输出
                    output_enabled = 0;
                    HAL_GPIO_WritePin(DC_EN_GPIO_Port, DC_EN_Pin, GPIO_PIN_RESET);
                }
            }
            break;
            
        default:
            break;
    }
}

/**
 * @brief  HAL UART 接收完成回调
 * @param  huart: UART 句柄
 */
void HAL_UART_RxCpltCallback(UART_HandleTypeDef *huart)
{
    if (huart->Instance == USART1)
    {
        TJC_HMI_RxCallback(uart_rx_byte);
    }
}

