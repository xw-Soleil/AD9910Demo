#include "Correct.h"
#include "app_config.h"
#include <math.h>
#include "arm_math.h" // 假设已包含
#include "utils.h"
#include "AD9910.h"
#include "process.h"
#include "sample.h"
#include "system_identification.h"


float32_t dacSampleBoardArr[DDS_FREQ_NUMES_ASB];



/**
 * @brief DAC输出信号经过采样板衰减后校正
 */
void dacSampleBoardCorr(){
    float32_t target_freq, target_vin;
    // float32_t measured_vout; // 不再需要单个的测量变量
    target_vin = 2.0f;
    generate_sine_table(target_vin / 2.0f); // 生成正弦波表

    // 用于存储单次设置下的多次测量结果
    float32_t VoutMeasurements[DDS_ITERATIONS]; 
    float32_t mean_vout; // 用于存储计算出的平均值

    for (uint32_t i = 0; i < DDS_FREQ_NUMES_ASB; i++)
    {
        target_freq = FREQ_START + i * FREQ_STEP;
        {
            

            // 1. 设置DAC输出电压，这个动作只需要执行一次
            start_sine_output(target_freq);

            // 2. 等待电路稳定，这个延时也只需要一次
            HAL_Delay(STABILIZE_DELAY_MS);

            // 3. 进行多次测量，并将结果存入数组
            for (uint32_t k = 0; k < DDS_ITERATIONS; k++)
            {
                VoutMeasurements[k] = MeasureAdcInputVpp();
                // 可以在这里加入一个极短的延时，如果ADC采样过快的话
                // HAL_Delay(1); 
            }

            // 4. 使用 ARM Math 库高效计算测量结果的平均值
            arm_mean_f32(VoutMeasurements, DDS_ITERATIONS, &mean_vout);

            stop_sine_output(); // 停止输出信号

            // 5. 使用稳定后的平均值进行计算和存储
            dacSampleBoardArr[i] = ADCSampleInputCorr(target_freq, mean_vout) / target_vin; // 计算增益衰减幅度
        }
    }
}

float32_t dacSampleBoardArrError[DDS_FREQ_NUMES_ASB] ;

void dacSampleBoardError(){
    float32_t target_freq, target_vin;
    // float32_t measured_vout; // 不再需要单个的测量变量
    target_vin = 2.0f;
    generate_sine_table(target_vin / 2.0f); // 生成正弦波表

    // 用于存储单次设置下的多次测量结果
    float32_t VoutMeasurements[DDS_ITERATIONS]; 
    float32_t mean_vout; // 用于存储计算出的平均值

    for (uint32_t i = 0; i < DDS_FREQ_NUMES_ASB; i++)
    {
        target_freq = FREQ_START + i * FREQ_STEP;
        {
            

            // 1. 设置DAC输出电压，这个动作只需要执行一次
            start_sine_output(target_freq);

            // 2. 等待电路稳定，这个延时也只需要一次
            HAL_Delay(STABILIZE_DELAY_MS);

            // 3. 进行多次测量，并将结果存入数组
            for (uint32_t k = 0; k < DDS_ITERATIONS; k++)
            {
                VoutMeasurements[k] = MeasureAdcInputVpp();
                // 可以在这里加入一个极短的延时，如果ADC采样过快的话
                // HAL_Delay(1); 
            }

            // 4. 使用 ARM Math 库高效计算测量结果的平均值
            arm_mean_f32(VoutMeasurements, DDS_ITERATIONS, &mean_vout);

            stop_sine_output(); // 停止输出信号

            // 5. 使用稳定后的平均值进行计算和存储
            dacSampleBoardArrError[i] = mean_vout / (target_vin * dacSampleBoardArr[i]); // 计算增益衰减幅度
        }
    }
}