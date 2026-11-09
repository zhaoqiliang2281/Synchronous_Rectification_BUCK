#ifndef INC_KEY_H_
#define INC_KEY_H_

#include "main.h"
#include "config.h"
#include "bsp_pwm.h"

// 按键防抖时间（毫秒）
#define KEY_DEBOUNCE_TIME 5
#define KEY_LONG_PRESS_TIME 1000  // 长按时间（1秒）

// 按键状态结构
typedef struct {
    uint8_t state;          // 按键状态：0=释放，1=按下
    uint8_t last_state;     // 上一次状态
    uint32_t press_time;    // 按下时间戳
    uint8_t processed;      // 是否已处理（防止重复触发）
    uint8_t long_press_processed; // 长按是否已处理
} KeyState_t;

// 函数声明
void KEY_Scan(void); // 在 main 循环中调用，用于扫描按键


#endif /* INC_KEY_H_ */
