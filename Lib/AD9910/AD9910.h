/**********************************************************
                       慷炜电子

移植说明：
平台：STM32F4xx HAL库
功能：AD9910 驱动
时间：2025/07/30
作者：Gemini (移植)
原作者：慷炜电子

移植要点：
1. 使用HAL库函数替代标准库函数。
2. 关键引脚定义在此文件中，请根据实际硬件连接修改。
3. `delay_ms` 替换为 `HAL_Delay`。

**********************************************************/

#ifndef __AD9910_H
#define __AD9910_H

#include "main.h" // 在STM32CubeMX生成的项目中，通常是 "main.h"

// C类型定义 (原sys.h中的内容)
typedef int32_t  s32;
typedef int16_t s16;
typedef int8_t  s8;

typedef const int32_t sc32;
typedef const int16_t sc16;
typedef const int8_t  sc8;

typedef __IO int32_t  vs32;
typedef __IO int16_t  vs16;
typedef __IO int8_t   vs8;

typedef __I int32_t vsc32;
typedef __I int16_t vsc16;
typedef __I int8_t  vsc8;

typedef uint32_t  u32;
typedef uint16_t u16;
typedef uint8_t  u8;

typedef const uint32_t uc32;
typedef const uint16_t uc16;
typedef const uint8_t  uc8;

typedef __IO uint32_t vu32;
typedef __IO uint16_t vu16;
typedef __IO uint8_t  vu8;

typedef __I uint32_t vuc32;
typedef __I uint16_t vuc16;
typedef __I uint8_t  vuc8;


/*-----------------------------------------------------------------------------------*/
/*             !!!!!!!!!  关键：请根据您的STM32F4开发板的实际接线修改以下引脚定义 !!!!!!!!!       */
/*-----------------------------------------------------------------------------------*/
/**
 * @brief  AD9910 控制引脚定义
 * PB12 RST
 * PB14 PR1
 * PD8 UPDATE
 * PD10 DRCTL
 * PD12 DRO
 * PD14 DRHOLD
 * PG2 SDIO
 * PG4 CS
 * PB13 PR2
 * PB15 PR0
 * PD9 OSK
 * PD11 SCLK
 */
// 串口数据线 (SDIO)
#define AD9910_SDIO_PORT        GPIOG
#define AD9910_SDIO_PIN         GPIO_PIN_2

// 串口时钟线 (SCLK)
#define AD9910_SCLK_PORT        GPIOD
#define AD9910_SCLK_PIN         GPIO_PIN_11

// 片选线 (CS)
#define AD9910_CS_PORT          GPIOG
#define AD9910_CS_PIN           GPIO_PIN_4

// 更新数据 (I/O_UPDATE)
#define AD9910_UPDATE_PORT      GPIOD
#define AD9910_UPDATE_PIN       GPIO_PIN_8

// 主复位 (MASTER_RESET)
#define AD9910_RESET_PORT       GPIOB
#define AD9910_RESET_PIN        GPIO_PIN_12

// 电源控制 (PWR_DWN_CTL)
#define AD9910_PWR_PORT         GPIOB
#define AD9910_PWR_PIN          GPIO_PIN_10

// Profile Pins
#define AD9910_PROFILE0_PORT    GPIOB
#define AD9910_PROFILE0_PIN     GPIO_PIN_15

#define AD9910_PROFILE1_PORT    GPIOB
#define AD9910_PROFILE1_PIN     GPIO_PIN_14

#define AD9910_PROFILE2_PORT    GPIOB
#define AD9910_PROFILE2_PIN     GPIO_PIN_13

// Digital Ramp Control Pins
#define AD9910_DRCTL_PORT       GPIOD
#define AD9910_DRCTL_PIN        GPIO_PIN_10

#define AD9910_DRHOLD_PORT      GPIOD
#define AD9910_DRHOLD_PIN       GPIO_PIN_14

/*-----------------------------------------------------------------------------------*/
/*                                 函数声明                                          */
/*-----------------------------------------------------------------------------------*/

void Init_ad9910(void);
void Freq_convert(u32 Freq);

#endif