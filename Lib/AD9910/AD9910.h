/**********************************************************
                       慷炜电子

移植说明：
平台：STM32F4xx HAL库 (已根据CubeMX定义同步)
功能：AD9910 驱动 (最终集成版 - DDS与AWG模式)
时间：2025/07/30
作者：Gemini (移植)
原作者：慷炜电子

**********************************************************/

#ifndef __AD9910_H
#define __AD9910_H

#include "main.h"

// C类型定义
typedef unsigned char u8;
typedef unsigned short u16;
typedef unsigned long u32;

// 定义可供选择的AWG波形
typedef enum {
    TRIG_WAVE, SQUARE_WAVE, SINC_WAVE
} AD9910_WAVE_ENUM;

#define WAVE_RAM_LEN 1024 // 波形数据长度

/*-----------------------------------------------------------------------------------*/
/*                                 函数声明                                          */
/*-----------------------------------------------------------------------------------*/

/**
 * @brief  初始化AD9910芯片，默认进入DDS正弦波模式
 */
void Init_AD9910(void);


/* --- DDS 正弦波模式 API --- */

/**
 * @brief  输出指定频率和幅度的正弦波
 * @note   调用此函数会自动将芯片置于DDS正弦波模式
 * @param  Freq: 目标频率, 单位 Hz
 * @param  Amp:  目标幅度, 范围 0 - 16383 (0x3FFF)
 */
void AD9910_Set_Sine_Wave(u32 Freq, u16 Amp);


/* --- AWG 任意波形模式 API --- */
void AD9910_AWG_Start(AD9910_WAVE_ENUM wave);
void AD9910_AWG_Update_Freq_Phase(u32 freq_hz, u16 phase_degree);

#endif