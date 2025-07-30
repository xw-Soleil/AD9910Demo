/**********************************************************
                       慷炜电子

移植说明：
平台：STM32F4xx HAL库 (已根据CubeMX定义同步)
功能：AD9910 驱动 (最终正确版 - 严格遵照慷炜电子代码)
时间：2025/07/30

**********************************************************/

#ifndef __AD9910_H
#define __AD9910_H

#include "main.h"

// C类型定义
typedef unsigned char u8;
typedef unsigned short u16;
typedef unsigned long u32;

/**
 * @brief 定义可供选择的AWG波形
 */
typedef enum {
    TRIG_WAVE,      // 三角波
    SQUARE_WAVE,    // 方波
    SINC_WAVE       // SINC波 (抽样信号)
} AD9910_Wave_t;


/*-----------------------------------------------------------------------------------*/
/*                                 函数声明                                          */
/*-----------------------------------------------------------------------------------*/

/**
 * @brief  初始化AD9910芯片及相关寄存器
 */
void Init_AD9910(void);

/**
 * @brief  【AWG模式】配置AD9910输出指定的预设波形 (三角波/方波/SINC波)
 * @note   这是一个完整的配置函数，调用后芯片即开始输出任意波形。
 * @param  wave: 要输出的波形，可选 TRIG_WAVE, SQUARE_WAVE, SINC_WAVE
 */
void AD9910_RAM_WAVE_Set(AD9910_Wave_t wave);

#endif