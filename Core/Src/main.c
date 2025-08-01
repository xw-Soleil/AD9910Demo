/* USER CODE BEGIN Header */
/**
  ******************************************************************************
  * @file           : main.c
  * @brief          : Main program body
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
/* Includes ------------------------------------------------------------------*/
#include "main.h"
#include "adc.h"
#include "dac.h"
#include "dma.h"
#include "tim.h"
#include "usart.h"
#include "gpio.h"

/* Private includes ----------------------------------------------------------*/
/* USER CODE BEGIN Includes */
#include "AD9910.h"
#include <stdio.h> // For printf, if using semihosting or similar for debugging
#include "utils.h"
#include "VppFFTMeu.h"
#include "sample.h"
#include "hmi.h"
#include "Correct.h"
#include "screen.h"


#include "app_config.h"
#include "system_identification.h"
#include "realtime_filter.h"
#include "iir_filter_design.h"
/* USER CODE END Includes */

/* Private typedef -----------------------------------------------------------*/
/* USER CODE BEGIN PTD */
// Application mode to switch between identification and filtering
typedef enum {
    APP_MODE_IDENTIFICATION,
    APP_MODE_FILTERING
} AppMode_t;

/* USER CODE END PTD */

/* Private define ------------------------------------------------------------*/
/* USER CODE BEGIN PD */

/* USER CODE END PD */

/* Private macro -------------------------------------------------------------*/
/* USER CODE BEGIN PM */

/* USER CODE END PM */

/* Private variables ---------------------------------------------------------*/

/* USER CODE BEGIN PV */

/* USER CODE END PV */

/* Private function prototypes -----------------------------------------------*/
void SystemClock_Config(void);
/* USER CODE BEGIN PFP */

/* USER CODE END PFP */

/* Private user code ---------------------------------------------------------*/
/* USER CODE BEGIN 0 */


//======================================================================================
// 全局变量
//======================================================================================
AccurateFFT_Handle fft_handle;      // FFT模块的句柄
float32_t test_input_signal[FFT_SIZE]; // 输入信号缓冲区
float32_t test_input_signal2[FFT_SIZE * 2]; // 输入信号缓冲区


volatile AppMode_t g_app_mode = APP_MODE_IDENTIFICATION;
volatile SysMode_t sys_mode = SYS_BASIC_OUTPUT; // 系统模式

//======================================================================================
// 函数声明
//======================================================================================
void generate_test_signal(float32_t* p_buffer, uint16_t size);
void ProcessADCData();
/* USER CODE END 0 */

/**
  * @brief  The application entry point.
  * @retval int
  */
int main(void)
{

  /* USER CODE BEGIN 1 */

  /* USER CODE END 1 */

  /* MCU Configuration--------------------------------------------------------*/

  /* Reset of all peripherals, Initializes the Flash interface and the Systick. */
  HAL_Init();

  /* USER CODE BEGIN Init */

  /* USER CODE END Init */

  /* Configure the system clock */
  SystemClock_Config();

  /* USER CODE BEGIN SysInit */

  /* USER CODE END SysInit */

  /* Initialize all configured peripherals */
  MX_GPIO_Init();
  MX_DMA_Init();
  MX_USART1_UART_Init();
  MX_ADC1_Init();
  MX_TIM3_Init();
  MX_UART4_Init();
  MX_ADC2_Init();
  MX_DAC_Init();
  MX_TIM2_Init();
  MX_TIM6_Init();
  MX_ADC3_Init();
  /* USER CODE BEGIN 2 */
  // HAL_Delay(100); // 确保所有外设初始化完成
  VisualTFT_Init(); // 初始化串口屏
  Init_AD9910(); // 初始化AD9910
  // float32_t vpp = MeasureAdcInputVpp();
  // float32_t vppCorr = ADCSampleInputCorr(100, vpp);

  // vpp = MeasureAdcInputVpp();
  // vppCorr = ADCSampleInputCorr(300, vpp);

  // vpp = MeasureAdcInputVpp();
  // vppCorr = ADCSampleInputCorr(3000, vpp);

  // vpp = MeasureAdcInputVpp();
  // vppCorr = ADCSampleInputCorr(100000, vpp);

  // DDSOutputCorrSamInBord(100, 1.0f);
  // DDSOutputCorrSamInBord(100, 1.1f);
  // DDSOutputCorrSamInBord(100, 1.3f);
  // DDSOutputCorrSamInBord(100, 1.5f);
  // DDSOutputCorrSamInBord(100, 1.7f);
  // DDSOutputCorrSamInBord(100, 1.9f);
  // DDSOutputCorrSamInBord(100, 2.0f);

  // SweepKnownBoardHs();
  // Init_AD9910();
  // AD9910_Set_Sine_Wave(1000, 16383 * 1.0f / MAX_DDS_VPP); // 设置正弦波频率为100kHz，幅度为16383（对应3.3V）
  // HAL_Delay(100); // 确保所有外设初始化完成
  // float32_t vpp = MeasureAdcInputVpp();

  // --- STAGE 1: SYSTEM IDENTIFICATION ---
  

  /* USER CODE END 2 */

  /* Infinite loop */
  /* USER CODE BEGIN WHILE */
  static SysMode_t previous_sys_mode = SYS_WAITING; // 初始值设为一个无效或默认状态
  static int filter_initialized = 0; // 用这个代替之前的 initialized 标志

  while (1)
  {
    VisualTFT_Poll();
    
    /* USER CODE END WHILE */

    /* USER CODE BEGIN 3 */
    // ==============================================================================
    // 核心修改：状态切换检测
    // ==============================================================================
    // 只有当模式发生改变时，才执行下面的代码块
    if (sys_mode != previous_sys_mode)
    {
        // --- a. 处理离开上一个状态的逻辑 ---
        if (previous_sys_mode == SYS_PERFORMANCE_OUTPUT)
        {
            RealtimeFilter_Stop(); // 只有在离开OUTPUT模式时才停止
            filter_initialized = 0; // 重置滤波器初始化标志
            printf("Stopped Real-time Filter.\n");
            SendText(4, 4, "");
        }

        // --- b. 处理进入新状态的逻辑 ---
        switch (sys_mode)
        {
            case SYS_PERFORMANCE_LEARN:
                g_app_mode = APP_MODE_IDENTIFICATION;
                SysId_Init(); // 只在进入LEARN模式时初始化一次
                printf("Entering LEARN mode, SysId initialized.\n");
                break;

            case SYS_PERFORMANCE_LEARN_DONE:
                FilterType_t filter_type = SysId_GetFilterType(); // 获取辨识的滤波器类型
                if(filter_type == FILTER_TYPE_LPF) {
                    SendText(4, 4, "低通滤波器");
                }
                else if(filter_type == FILTER_TYPE_HPF) {
                    SendText(4, 4, "高通滤波器");
                }
                else if(filter_type == FILTER_TYPE_BPF) {
                    SendText(4, 4, "带通滤波器");
                }
                else if(filter_type == FILTER_TYPE_BSF) {
                    SendText(4, 4, "带阻滤波器");
                }
                else {
                    SendText(4, 4, "未知滤波器");
                }
                break;
            case SYS_PERFORMANCE_OUTPUT:
                if (!filter_initialized) // 确保只初始化一次
                {
                    printf("Entering OUTPUT mode, configuring filter...\n");
                    g_app_mode = APP_MODE_FILTERING;
                    
                    FilterParams_t fitted_params = SysId_GetFittedParams();
                    FilterType_t filter_type = SysId_GetFilterType();
                    BiquadCoeffs iir_coeffs;

                    design_biquad_filter(
                        (FilterType_t)filter_type,
                        fitted_params.k, fitted_params.f0, fitted_params.q,
                        REALTIME_SAMPLING_RATE, &iir_coeffs);

                    RealtimeFilter_Init(&iir_coeffs);
                    RealtimeFilter_Start();
                    filter_initialized = 1;
                    printf("Filter configured and started.\n");
                }
                break;
            
            // 可以为其他模式添加入场逻辑
            case SYS_BASIC_OUTPUT:
            case SYS_BASIC_HS_OUTPUT:
                // ...
                break;
            default:
                // 其他状态不需要特殊处理
                break;
        }

        // --- c. 更新状态，准备下一次检测 ---
        previous_sys_mode = sys_mode;
    }


    // ==============================================================================
    // 当前状态的持续性任务（每次循环都可能执行）
    // ==============================================================================
    if (sys_mode == SYS_PERFORMANCE_LEARN)
    {
        // ！！！【重要】移除了阻塞的while循环！！！
        // 现在每次主循环只调用一次状态机，不会卡死系统
        if (!SysId_IsDone())
        {
            SysId_RunStateMachine();
        }
        else
        {
            // 当辨识完成后，自动切换到下一个状态
            printf("\nSystem Identification complete!\n");
            sys_mode = SYS_PERFORMANCE_LEARN_DONE; // 自动切换到学习完成状态
        }
    }
    
  }
  /* USER CODE END 3 */
}

/**
  * @brief System Clock Configuration
  * @retval None
  */
void SystemClock_Config(void)
{
  RCC_OscInitTypeDef RCC_OscInitStruct = {0};
  RCC_ClkInitTypeDef RCC_ClkInitStruct = {0};

  /** Configure the main internal regulator output voltage
  */
  __HAL_RCC_PWR_CLK_ENABLE();
  __HAL_PWR_VOLTAGESCALING_CONFIG(PWR_REGULATOR_VOLTAGE_SCALE1);

  /** Initializes the RCC Oscillators according to the specified parameters
  * in the RCC_OscInitTypeDef structure.
  */
  RCC_OscInitStruct.OscillatorType = RCC_OSCILLATORTYPE_HSI;
  RCC_OscInitStruct.HSIState = RCC_HSI_ON;
  RCC_OscInitStruct.HSICalibrationValue = RCC_HSICALIBRATION_DEFAULT;
  RCC_OscInitStruct.PLL.PLLState = RCC_PLL_ON;
  RCC_OscInitStruct.PLL.PLLSource = RCC_PLLSOURCE_HSI;
  RCC_OscInitStruct.PLL.PLLM = 8;
  RCC_OscInitStruct.PLL.PLLN = 160;
  RCC_OscInitStruct.PLL.PLLP = RCC_PLLP_DIV2;
  RCC_OscInitStruct.PLL.PLLQ = 4;
  if (HAL_RCC_OscConfig(&RCC_OscInitStruct) != HAL_OK)
  {
    Error_Handler();
  }

  /** Initializes the CPU, AHB and APB buses clocks
  */
  RCC_ClkInitStruct.ClockType = RCC_CLOCKTYPE_HCLK|RCC_CLOCKTYPE_SYSCLK
                              |RCC_CLOCKTYPE_PCLK1|RCC_CLOCKTYPE_PCLK2;
  RCC_ClkInitStruct.SYSCLKSource = RCC_SYSCLKSOURCE_PLLCLK;
  RCC_ClkInitStruct.AHBCLKDivider = RCC_SYSCLK_DIV1;
  RCC_ClkInitStruct.APB1CLKDivider = RCC_HCLK_DIV4;
  RCC_ClkInitStruct.APB2CLKDivider = RCC_HCLK_DIV2;

  if (HAL_RCC_ClockConfig(&RCC_ClkInitStruct, FLASH_LATENCY_5) != HAL_OK)
  {
    Error_Handler();
  }
}

/* USER CODE BEGIN 4 */
void ProcessADCData(){
  for(int i = 0; i < ADC_DMA_BUFFER_SIZE; i++) {
      // 将ADC1和ADC2的采样数据转换为浮点数
      test_input_signal[i] = 3.3f * (float32_t)(ADC1_Buffer[i]) / 4095.0f; // 假设ADC分辨率为12位，参考电压为3.3V
  }

  // 使用Arm数学库进行去除直流偏置
  float32_t mean_value;
  arm_mean_f32(test_input_signal, ADC_DMA_BUFFER_SIZE, &mean_value);
  for(int i = 0; i < ADC_DMA_BUFFER_SIZE; i++) {
      test_input_signal[i] -= mean_value; // 去除直流偏置
  }
}
void generate_test_signal(float32_t* p_buffer, uint16_t size)
{
    float32_t time_step = 1.0f / SAMPLING_RATE;
    for (int i = 0; i < size; i++) {
        p_buffer[i] = SIGNAL_AMPLITUDE * arm_sin_f32(2 * PI * SIGNAL_FREQUENCY * i * time_step);
    }
}


// --- Top-Level Interrupt Service Routine Callbacks ---
// These callbacks redirect to the appropriate module based on the current app mode.

void HAL_ADC_ConvCpltCallback(ADC_HandleTypeDef* hadc) {
    if (g_app_mode == APP_MODE_IDENTIFICATION) {
        SysId_ADCCallback();
    } else { // APP_MODE_FILTERING
        RealtimeFilter_ADCFullCpltCallback();
    }
}

void HAL_ADC_ConvHalfCpltCallback(ADC_HandleTypeDef* hadc) {
    if (g_app_mode == APP_MODE_FILTERING) {
        RealtimeFilter_ADCHalfCpltCallback();
    }
    // No action in identification mode for half complete
}
// In main.c

void HAL_ADC_ErrorCallback(ADC_HandleTypeDef *hadc) {
    if (g_app_mode == APP_MODE_IDENTIFICATION) {
        SysId_ADCErrorCallback(); // <--- 添加这一行
    } else if (g_app_mode == APP_MODE_FILTERING) {
        RealtimeFilter_ADCErrorCallback();
    }
}
/* USER CODE END 4 */

/**
  * @brief  This function is executed in case of error occurrence.
  * @retval None
  */
void Error_Handler(void)
{
  /* USER CODE BEGIN Error_Handler_Debug */
  /* User can add his own implementation to report the HAL error return state */
  __disable_irq();
  while (1)
  {
  }
  /* USER CODE END Error_Handler_Debug */
}
#ifdef USE_FULL_ASSERT
/**
  * @brief  Reports the name of the source file and the source line number
  *         where the assert_param error has occurred.
  * @param  file: pointer to the source file name
  * @param  line: assert_param error line source number
  * @retval None
  */
void assert_failed(uint8_t *file, uint32_t line)
{
  /* USER CODE BEGIN 6 */
  /* User can add his own implementation to report the file name and line number,
     ex: printf("Wrong parameters value: file %s on line %d\r\n", file, line) */
  /* USER CODE END 6 */
}
#endif /* USE_FULL_ASSERT */
