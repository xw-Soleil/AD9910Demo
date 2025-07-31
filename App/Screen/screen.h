/**
 * @file    screen.h
 * @brief   屏幕显示模块头文件
 * @author  Soleil
 * @date    2025
 */

#ifndef __SCREEN_H__
#define __SCREEN_H__

/* ============================================================================ */
/*                                包含头文件                                    */
/* ============================================================================ */

#include "usart.h"
#include "hmi.h"
#include "arm_math.h"    /* 包含ARM数学库以使用正弦函数 */
#include <stdlib.h>      /* for atof() */
#include <string.h>
#include <stdio.h>
#include <math.h>        /* 需要用到 log10 */
#include "main.h"        /* 包含主头文件 */

/* ============================================================================ */
/*                                  变量声明                                     */
/* ============================================================================ */


extern float32_t waveDDSFreq; // 正弦波频率 kHz
extern float32_t waveDDSVpp; // 正弦波峰峰值 V


/* ============================================================================ */
/*                                函数声明                                     */
/* ============================================================================ */

/**
 * @brief   生成文本控件指令
 * @param   page_id 页面ID
 * @param   control_id 控件ID
 * @param   text 要传入的文本
 * @param   command 输出的指令缓冲区
 * @return  指令长度
 */
uint16_t generate_text_command(uint16_t page_id, uint8_t control_id, const char *text, uint8_t *command);

/**
 * @brief   切换页面
 * @param   page_id 页面ID
 */
void SwitchPage(uint8_t page_id);

/**
 * @brief 清屏screen1 id 6 7 8 /screen2 id 9的文本控件
 * @param screen_id 指定页面
 */
void ClearScreen(uint8_t screen_id);

/**
 * @brief   发送文本到指定页面控件
 * @param   page_id 页面ID
 * @param   control_id 控件ID
 * @param   text 要显示的文本
 */
void SendText(uint16_t page_id, uint8_t control_id, const char *text);

/**
 * @brief   发送浮点数+单位到指定页面控件
 * @param   page_id 页面ID
 * @param   control_id 控件ID
 * @param   value 数值
 * @param   unit 单位字符串 (如"kHz", "V", "°C")
 * @param   decimals 小数位数
 */
void SendFloat(uint16_t page_id, uint8_t control_id, float value, const char *unit, uint8_t decimals);

/**
 * @brief   发送整数+单位到指定页面控件
 * @param   page_id 页面ID
 * @param   control_id 控件ID
 * @param   value 整数值
 * @param   unit 单位字符串 (如"Hz", "V", "mA")
 */
void SendInt(uint16_t page_id, uint8_t control_id, int value, const char *unit);



#endif /* __SCREEN_H__ */