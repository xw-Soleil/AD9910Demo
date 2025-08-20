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
// ??????
//======================================================================================
AccurateFFT_Handle fft_handle;      // FFT??????
float32_t test_input_signal[FFT_SIZE]; // ????????????
float32_t test_input_signal2[FFT_SIZE * 2]; // ????????????


volatile AppMode_t g_app_mode = APP_MODE_IDENTIFICATION;
volatile SysMode_t sys_mode = SYS_BASIC_OUTPUT; // ????

//======================================================================================
// ????????
//======================================================================================
void generate_test_signal(float32_t* p_buffer, uint16_t size);
void ProcessADCData();


void set2to1Mode(uint8_t sel) {
    HAL_GPIO_WritePin(GPIOC, GPIO_PIN_0, sel ? GPIO_PIN_SET : GPIO_PIN_RESET);
}

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
  // HAL_Delay(100); // ??????????????????
  VisualTFT_Init(); // ???????????
  Init_AD9910(); // ?????AD9910
//   AD9910_Set_Sine_Wave(1000, 16383 * 1.0f / MAX_DDS_VPP);
//   DDSOutputCorrSamInBord(1000, 2.0f);
//   dacSampleBoardCorr();
//   dacSampleBoardError();
  
//   SampleInBoardCorr();
//   SampleInBoardError();
//   SampleDDSBoardCorr();
//   SampleDDSBoardCorrError();
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
  // AD9910_Set_Sine_Wave(1000, 16383 * 1.0f / MAX_DDS_VPP); // ?????????????100kHz???????16383?????3.3V??
  // HAL_Delay(100); // ??????????????????
  // float32_t vpp = MeasureAdcInputVpp();

  // --- STAGE 1: SYSTEM IDENTIFICATION ---
  

  /* USER CODE END 2 */

  /* Infinite loop */
  /* USER CODE BEGIN WHILE */
  static SysMode_t previous_sys_mode = SYS_WAITING; // ?????????????????????
  static int filter_initialized = 0; // ????????????? initialized ???

  while (1)
  {
    VisualTFT_Poll();
    
    /* USER CODE END WHILE */

    /* USER CODE BEGIN 3 */
    // ==============================================================================
    // ?????????????????
    // ==============================================================================
    // ???????????????????????????????
    if (sys_mode != previous_sys_mode)
    {
        // --- a. ?????????????????? ---
        if (previous_sys_mode == SYS_PERFORMANCE_OUTPUT)
        {
            RealtimeFilter_Stop(); // ???????OUTPUT???????
            filter_initialized = 0; // ?????????????????
            printf("Stopped Real-time Filter.\n");
            SendText(4, 4, "");
        }

        // --- b. ????????????????? ---
        switch (sys_mode)
        {
            case SYS_PERFORMANCE_LEARN:
                set2to1Mode(0);
                g_app_mode = APP_MODE_IDENTIFICATION;
                SysId_Init(); // ??????LEARN???????????
                printf("Entering LEARN mode, SysId initialized.\n");
                break;

            case SYS_PERFORMANCE_LEARN_DONE:
                
                FilterType_t filter_type = SysId_GetFinalFilterType(); // ?????????????????
                if(filter_type == FILTER_TYPE_LPF) {
                    SendText(4, 4, "��ͨ�˲���");
                }
                else if(filter_type == FILTER_TYPE_HPF) {
                    SendText(4, 4, "��ͨ�˲���");
                }
                else if(filter_type == FILTER_TYPE_BPF) {
                    SendText(4, 4, "��ͨ�˲���");
                }
                else if(filter_type == FILTER_TYPE_BSF) {
                    SendText(4, 4, "�����˲���");
                }
                else {
                    SendText(4, 4, "");
                }
                break;
            case SYS_PERFORMANCE_OUTPUT:
                set2to1Mode(1);
                if (!filter_initialized) // ????????????
                {
                    printf("Entering OUTPUT mode, configuring filter...\n");
                    g_app_mode = APP_MODE_FILTERING;
                    
                    FilterParams_t_double fitted_params = SysId_GetFittedParams();
                    // FilterParams_t_double fitted_params;
                    // fitted_params.k = 0.9688;
                    // fitted_params.w0 = 2*PI * 20.83; // 20.83 Hz
                    // fitted_params.q = 0.0010;
                    // FilterType_t filter_type = FILTER_TYPE_BPF;
                    FilterType_t filter_type = SysId_GetPreliminaryFilterType(); // ????
                    BiquadCoeffs iir_coeffs;

                    design_biquad_filter(
                        (FilterType_t)filter_type,
                        fitted_params.k, fitted_params.w0 / 2 / PI, fitted_params.q,
                        REALTIME_SAMPLING_RATE, &iir_coeffs);

                    RealtimeFilter_Init(&iir_coeffs);
                    RealtimeFilter_Start();
                    filter_initialized = 1;
                    printf("Filter configured and started.\n");

                }
                break;
            
            // ????????????????????
            case SYS_BASIC_OUTPUT:

            case SYS_BASIC_HS_OUTPUT:
                // ...
                break;
            default:
                // ?????????????????
                break;
        }

        // --- c. ??????????????????? ---
        previous_sys_mode = sys_mode;
    }


    // ==============================================================================
    // ?????????????????????????????????
    // ==============================================================================
    if (sys_mode == SYS_PERFORMANCE_LEARN)
    {
        // ????????????????????????while?????????
        // ??????????????????????????????????
        if (!SysId_IsDone())
        {
            SysId_RunStateMachine();
        }
        else
        {
            // ?????????????????????????
            printf("\nSystem Identification complete!\n");
            sys_mode = SYS_PERFORMANCE_LEARN_DONE; // ????????????????
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
  RCC_OscInitStruct.PLL.PLLN = 168;
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
      // ??ADC1??ADC2???????????????????
      test_input_signal[i] = 3.3f * (float32_t)(ADC1_Buffer[i]) / 4095.0f; // ????ADC??????12????????????3.3V
  }

  // ???Arm?????????????????
  float32_t mean_value;
  arm_mean_f32(test_input_signal, ADC_DMA_BUFFER_SIZE, &mean_value);
  for(int i = 0; i < ADC_DMA_BUFFER_SIZE; i++) {
      test_input_signal[i] -= mean_value; // ?????????
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
    adc_status = ADC_FINISHED;
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
        SysId_ADCErrorCallback(); // <--- ?????????
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
