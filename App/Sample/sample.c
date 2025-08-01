#include "sample.h"
#include "utils.h"

volatile ADC_Status_t adc_status = ADC_NOT_FINISHED;


volatile uint16_t ADC1_Buffer[ADC_DMA_BUFFER_SIZE]; // ADC1数据缓冲区
volatile uint16_t ADC2_Buffer[ADC_DMA_BUFFER_SIZE * 2]; // ADC2数据缓冲区

float32_t adc_fft_buffer[ADC_DMA_BUFFER_SIZE]; // 用于FFT处理的缓冲区

/**
 * @brief   设置TIM的周期值
 * @param   period 定时器周期值 (0-65535)
 * @param   htim 定时器句柄指针
 * @note    预分频器固定为0，实际频率 = 时钟频率 / (period + 1)
 */
void SetTIMPeriod(TIM_HandleTypeDef *htim, uint32_t period)
{
    if (period > 65535) return;
    
    HAL_TIM_Base_Stop(htim);
    __HAL_TIM_SET_AUTORELOAD(htim, period);
    HAL_TIM_GenerateEvent(htim, TIM_EVENTSOURCE_UPDATE);
}

/**
 * @brief   设置TIM的预分频器和周期值
 * @param   prescaler 预分频器值 (0-65535)  
 * @param   period 定时器周期值 (0-65535)
 * @param   htim 定时器句柄指针
 * @note    实际频率 = 时钟频率 / ((prescaler + 1) * (period + 1))
 */
void SetTIMConfig(TIM_HandleTypeDef *htim, uint32_t prescaler, uint32_t period)
{
    if (prescaler > 65535 || period > 65535) return;
    
    HAL_TIM_Base_Stop(htim);
    __HAL_TIM_SET_PRESCALER(htim, prescaler);
    __HAL_TIM_SET_AUTORELOAD(htim, period);
    HAL_TIM_GenerateEvent(htim, TIM_EVENTSOURCE_UPDATE);
}

/**
 * @brief   采样ADC1数据并启动DMA
 * @note    该函数会启动ADC1的DMA采样，并使用定时器触发同步采样。
 *          注意：必须先配置好ADC和DMA，然后再调用此函数。  
 */
void SampleADC_DMA(void)
{
  /* 重要：先开启ADC的DMA采样，再开启定时器 */

  /* 先开启ADC1的DMA采样 */
  if (HAL_ADC_Start_DMA(&hadc1, (uint32_t *)ADC1_Buffer, ADC_DMA_BUFFER_SIZE) != HAL_OK)
  {
    printf("Error starting ADC1 DMA!\n");
    Error_Handler();
  }

  // 最后开启定时器触发（这是同步采样的关键）
  if (HAL_TIM_Base_Start(&htim3) != HAL_OK)
  {
    printf("Error starting Timer!\n");
    Error_Handler();
  }
  

  /* 4. 添加超时机制避免死循环 */
  uint32_t timeout_counter = 0;
  const uint32_t TIMEOUT_LIMIT = 1000000; 

  while(adc_status == ADC_NOT_FINISHED && timeout_counter < TIMEOUT_LIMIT)
  {
    timeout_counter++;
    HAL_Delay(1); // 添加短暂延时
  }
  if(timeout_counter >= TIMEOUT_LIMIT)
  {
    printf("ADC conversion timeout!\n");
    return;
  }
  

  /* 5. 停止DMA和定时器 */
  if(HAL_ADC_Stop_DMA(&hadc1) != HAL_OK){
    printf("Error Stopping ADC1 DMA!\n");
  }
  if (HAL_TIM_Base_Stop(&htim3) != HAL_OK)
  {
    printf("Error stopping Timer!\n");
  }
  adc_status = ADC_NOT_FINISHED; // 重置状态标志以便下次使用
}

/**
 * @brief   采样ADC1 2数据并启动DMA
 * @note    注意：必须先配置好ADC和DMA，然后再调用此函数。
 */
void SampleBothADC(void)
{
  /* 开启DMA采样 */
  if (HAL_ADC_Start_DMA(&hadc1, (uint32_t *)ADC1_Buffer, ADC_DMA_BUFFER_SIZE) != HAL_OK ||
      HAL_ADC_Start_DMA(&hadc2, (uint32_t *)ADC2_Buffer, ADC_DMA_BUFFER_SIZE) != HAL_OK) {
    Error_Handler();
  }

  if (HAL_TIM_Base_Start(&htim3) != HAL_OK) {
    Error_Handler();
  }

  /* 超时机制 */
  uint32_t timeout;
  for(timeout = 1000000; adc_status == ADC_NOT_FINISHED && --timeout; HAL_Delay(1));
  if(timeout <= 0){
    printf("ADC conversion timeout!\n");
  }
  HAL_ADC_Stop_DMA(&hadc1);
  HAL_ADC_Stop_DMA(&hadc2);
  HAL_TIM_Base_Stop(&htim3);
  adc_status = ADC_NOT_FINISHED;
}

// /**
//   * @brief  Conversion complete callback.
//   * @param  hadc ADC handle
//   * @retval None
//   */
// void HAL_ADC_ConvCpltCallback(ADC_HandleTypeDef* hadc)
// {
//   if (hadc->Instance == ADC1) 
//   {
//     adc_status = ADC_FINISHED; // 设置ADC转换完成标志
//   }
// }

// /**
//   * @brief  ADC error callback.
//   * @param  hadc ADC handle
//   * @retval None
//   */
// void HAL_ADC_ErrorCallback(ADC_HandleTypeDef *hadc)
// {
//     if(hadc->Instance == ADC1)
//     {
//         printf("ADC1 Error: %lu. Overrun: %s\n",
//                hadc->ErrorCode,
//                (hadc->ErrorCode & HAL_ADC_ERROR_OVR) ? "YES" : "NO");
//         // Error_Handler(); // Consider how to handle ADC errors robustly
//     }
// }
void MeasureVppProcessData(){
  for(int i = 0; i < ADC_DMA_BUFFER_SIZE; i++) {
      // 将ADC1和ADC2的采样数据转换为浮点数
      adc_fft_buffer[i] = 3.3f * (float32_t)(ADC1_Buffer[i]) / 4095.0f; // 假设ADC分辨率为12位，参考电压为3.3V
  }

  // 使用Arm数学库进行去除直流偏置
  float32_t mean_value;
  arm_mean_f32(adc_fft_buffer, ADC_DMA_BUFFER_SIZE, &mean_value);
  for(int i = 0; i < ADC_DMA_BUFFER_SIZE; i++) {
      adc_fft_buffer[i] -= mean_value; // 去除直流偏置
  }
}
float32_t MeasureAdcInputVpp(void){
  AccurateFFT_Handle fft_handle_tmp;      // FFT模块的句柄
  SetTIMPeriod(&htim3, 1680-1); // 设置定时器周期为1000
  arm_status status = AccurateFFT_Init(&fft_handle_tmp, FFT_SIZE, 50000, WINDOW_TYPE_FLATTOP);
  if(status != ARM_MATH_SUCCESS){
    printf("FFT Init Error: %d\n", status);
    return 0.0f; // 初始化失败，返回0
  }
  adc_status = ADC_NOT_FINISHED; // 重置状态标志
  SampleADC_DMA(); // 采样ADC1数据
  MeasureVppProcessData(); // 处理采样数据
  AccurateFFT_Measure(&fft_handle_tmp, adc_fft_buffer);

  return fft_handle_tmp.result.corrected_amplitude * 2; // 返回Vpp值
}