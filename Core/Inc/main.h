/* USER CODE BEGIN Header */
/**
  ******************************************************************************
  * @file           : main.h
  * @brief          : Header for main.c file.
  *                   This file contains the common defines of the application.
  ******************************************************************************
  * @attention
  *
  * Copyright (c) 2025 STMicroelectronics.
  * All rights reserved.
  *
  * This software is licensed under terms that can be found in the LICENSE file
  * in the root directory of this software component.
  * If no LICENSE file comes with this software, it is provided AS-IS.
  *
  ******************************************************************************
  */
/* USER CODE END Header */

/* Define to prevent recursive inclusion -------------------------------------*/
#ifndef __MAIN_H
#define __MAIN_H

#ifdef __cplusplus
extern "C" {
#endif

/* Includes ------------------------------------------------------------------*/
#include "stm32f4xx_hal.h"

/* Private includes ----------------------------------------------------------*/
/* USER CODE BEGIN Includes */
typedef enum{
  SYS_WAITING,  // 等待状态
  SYS_BASIC_OUTPUT, // 基本要求输出
  SYS_BASIC_HS_OUTPUT, // 基于已知模型的输出
  SYS_PERFORMANCE_LEARN,  // 发挥部分学习
  SYS_PERFORMANCE_LEARN_DONE, // 发挥部分学习完成
  SYS_PERFORMANCE_OUTPUT  // 发挥部分输出
} SysMode_t;
extern volatile SysMode_t sys_mode;

/* USER CODE END Includes */

/* Exported types ------------------------------------------------------------*/
/* USER CODE BEGIN ET */

/* USER CODE END ET */

/* Exported constants --------------------------------------------------------*/
/* USER CODE BEGIN EC */

/* USER CODE END EC */

/* Exported macro ------------------------------------------------------------*/
/* USER CODE BEGIN EM */

/* USER CODE END EM */

/* Exported functions prototypes ---------------------------------------------*/
void Error_Handler(void);

/* USER CODE BEGIN EFP */

/* USER CODE END EFP */

/* Private defines -----------------------------------------------------------*/
#define AD9910_RESET_Pin GPIO_PIN_12
#define AD9910_RESET_GPIO_Port GPIOB
#define AD9910_PROFILE2_Pin GPIO_PIN_13
#define AD9910_PROFILE2_GPIO_Port GPIOB
#define AD9910_PROFILE1_Pin GPIO_PIN_14
#define AD9910_PROFILE1_GPIO_Port GPIOB
#define AD9910_PROFILE0_Pin GPIO_PIN_15
#define AD9910_PROFILE0_GPIO_Port GPIOB
#define AD9910_UPDATE_Pin GPIO_PIN_8
#define AD9910_UPDATE_GPIO_Port GPIOD
#define AD9910_DRCTL_Pin GPIO_PIN_10
#define AD9910_DRCTL_GPIO_Port GPIOD
#define AD9910_SCLK_Pin GPIO_PIN_11
#define AD9910_SCLK_GPIO_Port GPIOD
#define AD9910_DRHOLD_Pin GPIO_PIN_14
#define AD9910_DRHOLD_GPIO_Port GPIOD
#define AD9910_SDIO_Pin GPIO_PIN_2
#define AD9910_SDIO_GPIO_Port GPIOG
#define AD9910_CS_Pin GPIO_PIN_4
#define AD9910_CS_GPIO_Port GPIOG

/* USER CODE BEGIN Private defines */

//======================================================================================
// 宏定义和常量
//======================================================================================
#define FFT_SIZE            2048
#define SAMPLING_RATE       1000000.0f
#define SIGNAL_AMPLITUDE    2.0f
#define SIGNAL_FREQUENCY    (100.5f * (SAMPLING_RATE / FFT_SIZE))
#define MAX_DDS_VPP        4.5f
#define MAX_OnlyDDS_VPP    1.05f
/* USER CODE END Private defines */

#ifdef __cplusplus
}
#endif

#endif /* __MAIN_H */
