#ifndef __SAMPLE_H__
#define __SAMPLE_H__

#include "main.h"
#include "adc.h"
#include "tim.h"
#include "arm_math.h"
#include "VppFFTMeu.h"

#define hadcSple hadc1
#define hadcSpleSec hadc2

//中断回调结束标志位 使用枚举变量
typedef enum {
    ADC_FINISHED,    // ADC转换完成
    ADC_NOT_FINISHED // ADC转换未完成
} ADC_Status_t;

#define ADC_DMA_BUFFER_SIZE FFT_SIZE // DMA缓冲区大小

extern volatile uint16_t ADC1_Buffer[ADC_DMA_BUFFER_SIZE]; // ADC1数据缓冲区
extern volatile uint16_t ADC2_Buffer[ADC_DMA_BUFFER_SIZE * 2]; // ADC2数据缓冲区
extern volatile ADC_Status_t adc_status;
/**
 * @brief   设置TIM的周期值
 * @param   period 定时器周期值 (0-65535)
 * @param   htim 定时器句柄指针
 * @note    预分频器固定为0，实际频率 = 时钟频率 / (period + 1)
 */
void SetTIMPeriod(TIM_HandleTypeDef *htim, uint32_t period);


/**
 * @brief   设置TIM的预分频器和周期值
 * @param   prescaler 预分频器值 (0-65535)  
 * @param   period 定时器周期值 (0-65535)
 * @param   htim 定时器句柄指针
 * @note    实际频率 = 时钟频率 / ((prescaler + 1) * (period + 1))
 */
void SetTIMConfig(TIM_HandleTypeDef *htim, uint32_t prescaler, uint32_t period);

/**
 * @brief   采样ADC1数据并启动DMA
 * @note    该函数会启动ADC1的DMA采样，并使用定时器触发同步采样。
 *          注意：必须先配置好ADC和DMA，然后再调用此函数。
 */
void SampleADC_DMA(void);
/**
 * @brief   采样ADC1 2数据并启动DMA
 * @note    注意：必须先配置好ADC和DMA，然后再调用此函数。
 */
void SampleBothADC(void);

float32_t MeasureAdcInputVpp(void);

extern volatile ADC_Status_t adc_status;

#endif