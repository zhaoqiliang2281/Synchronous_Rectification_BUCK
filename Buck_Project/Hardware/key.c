#include "key.h"

// 三个按键的状态
static KeyState_t key1_state = {0, 0, 0, 0, 0};
static KeyState_t key2_state = {0, 0, 0, 0, 0};
static KeyState_t key3_state = {0, 0, 0, 0, 0};

/**
 * @brief  扫描单个按键（非阻塞防抖）
 * @param  pin_state: 当前按键引脚状态（GPIO_PIN_RESET=按下）
 * @param  key: 按键状态结构体指针
 * @param  enable_repeat: 是否启用长按后的连续触发（1=启用，0=禁用）
 * @retval 0=无操作，1=短按，2=长按
 */
static uint8_t KEY_Scan_Single(GPIO_PinState pin_state, KeyState_t *key, uint8_t enable_repeat)
{
    uint32_t current_time = HAL_GetTick();
    
    // 读取当前按键状态
    key->state = (pin_state == GPIO_PIN_RESET) ? 1 : 0;
    
    // 检测按键按下（状态从0变为1）
    if (key->state == 1 && key->last_state == 0)
    {
        // 记录按下时间戳
        key->press_time = current_time;
        key->processed = 0;
        key->long_press_processed = 0;
        key->last_state = 1;
        return 0;  // 刚按下，等待防抖
    }
    
    // 检测按键释放（状态从1变为0）
    if (key->state == 0 && key->last_state == 1)
    {
        uint32_t hold_time = current_time - key->press_time;
        
        // 只有在"未触发过长按"的情况下，才将短按识别为有效
        // 如果已经触发过长按（long_press_processed=1），则释放时忽略
        if (key->long_press_processed == 0 && hold_time >= KEY_DEBOUNCE_TIME && hold_time < KEY_LONG_PRESS_TIME)
        {
            // 只有这种情况才是有效的短按
            key->last_state = 0;
            key->processed = 0;
            key->long_press_processed = 0;
            return 1;  // 短按
        }
        
        // 其他所有情况（长按已触发或按时间太短）都重置状态后返回0
        key->last_state = 0;
        key->processed = 0;
        key->long_press_processed = 0;
        return 0;
    }
    
    // 按键持续按下，检查是否长按
    if (key->state == 1 && key->last_state == 1)
    {
        uint32_t hold_time = current_time - key->press_time;
        
        // 长按检测（超过1秒）
        if (hold_time >= KEY_LONG_PRESS_TIME && key->long_press_processed == 0)
        {
            key->long_press_processed = 1;
            return 2;  // 长按
        }
        
        // 长按后的连续触发（仅当enable_repeat=1时启用，用于KEY1/KEY3连续调整）
        // KEY2不需要连续触发，所以传入0
        if (enable_repeat && key->long_press_processed == 1 && hold_time > (KEY_LONG_PRESS_TIME + 500))
        {
            if ((hold_time % 100) == 0)
            {
                return 1;  // 长按后连续触发（按短按处理）
            }
        }
    }
    return 0;  // 无有效操作
}

/**
 * @brief  扫描按键函数，应该在 main 循环中被调用
 * @note   使用非阻塞防抖，不会阻塞系统
 */
void KEY_Scan(void)
{
    uint8_t key1_result, key2_result, key3_result;
    
    // --- KEY1 (PC8): 减少目标值（电压或电流） ---
    key1_result = KEY_Scan_Single(HAL_GPIO_ReadPin(KEY1_GPIO_Port, KEY1_Pin), &key1_state, 1); // KEY1 启用连续触发
    if (key1_result == 1)  // 短按
    {
        if (work_mode == MODE_CV)
        {
            // CV模式：调整电压
            if (target_voltage > MIN_VOLTAGE)
            {
                target_voltage -= VOLTAGE_STEP;
                if (target_voltage < MIN_VOLTAGE)
                    target_voltage = MIN_VOLTAGE;
                // 四舍五入到小数点后1位，避免浮点精度误差
                target_voltage = roundf(target_voltage * 10.0f) / 10.0f;
            }
        }
        else  // MODE_CC
        {
            // CC模式：调整电流
            if (target_current > MIN_CURRENT)
            {
                target_current -= CURRENT_STEP;
                if (target_current < MIN_CURRENT)
                    target_current = MIN_CURRENT;
                // 四舍五入到小数点后2位，避免浮点精度误差
                target_current = roundf(target_current * 100.0f) / 100.0f;
            }
        }
    }
    
    // --- KEY3 (PC7): 增加目标值（电压或电流） ---
    key3_result = KEY_Scan_Single(HAL_GPIO_ReadPin(KEY3_GPIO_Port, KEY3_Pin), &key3_state, 1); // KEY3 启用连续触发
    if (key3_result == 1)  // 短按
    {
        if (work_mode == MODE_CV)
        {
            // CV模式：调整电压
            if (target_voltage < MAX_VOLTAGE)
            {
                target_voltage += VOLTAGE_STEP;
                if (target_voltage > MAX_VOLTAGE)
                    target_voltage = MAX_VOLTAGE;
                // 四舍五入到小数点后1位，避免浮点精度误差
                target_voltage = roundf(target_voltage * 10.0f) / 10.0f;
            }
        }
        else  // MODE_CC
        {
            // CC模式：调整电流
            if (target_current < MAX_CURRENT)
            {
                target_current += CURRENT_STEP;
                if (target_current > MAX_CURRENT)
                    target_current = MAX_CURRENT;
                // 四舍五入到小数点后2位，避免浮点精度误差
                target_current = roundf(target_current * 100.0f) / 100.0f;
            }
        }
    }

    // --- KEY2 (PC9): 短按切换输出ON/OFF，长按切换CV/CC模式 ---
    key2_result = KEY_Scan_Single(HAL_GPIO_ReadPin(KEY2_GPIO_Port, KEY2_Pin), &key2_state, 0); // KEY2 禁用连续触发
    if (key2_result == 1)  // 短按：切换输出
    {
        // 切换输出使能状态
        output_enabled = !output_enabled;
        
        if (output_enabled)
        {
            // 开启输出：先使能DC_EN，再使能VOUT_EN
            HAL_GPIO_WritePin(DC_EN_GPIO_Port, DC_EN_Pin, GPIO_PIN_SET);
        }
        else
        {
            // 关闭输出：先关闭VOUT_EN，再关闭DC_EN
            HAL_GPIO_WritePin(DC_EN_GPIO_Port, DC_EN_Pin, GPIO_PIN_RESET);
        }
    }
    else if (key2_result == 2)  // 长按：切换工作模式
    {
        // 切换CV/CC模式
        if (work_mode == MODE_CV)
            work_mode = MODE_CC;
        else
            work_mode = MODE_CV;
        
        // 切换模式时，如果输出打开则关闭
        if (output_enabled)
        {
            output_enabled = 0;
            HAL_GPIO_WritePin(DC_EN_GPIO_Port, DC_EN_Pin, GPIO_PIN_RESET);
        }
    }
}
