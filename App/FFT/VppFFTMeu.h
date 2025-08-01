/***************************************************************************************
 * @file    accurate_fft.h
 * @author  Gemini
 * @brief   精确FFT幅值测量模块的头文件
 * @version 1.1
 *
 * @note
 * 定义了用于精确FFT测量的配置、结果结构体以及公共函数接口。
 ***************************************************************************************/

#ifndef ACCURATE_FFT_H
#define ACCURATE_FFT_H

#include "arm_math.h"
#include <stdint.h>
#include "main.h"

//======================================================================================
// 公共宏定义和枚举
//======================================================================================

typedef enum {
    WINDOW_TYPE_FLATTOP,
    WINDOW_TYPE_HANN
    // 可在此处添加其他窗函数类型
} AccurateFFT_WindowType;

extern float32_t g_window_coeffs[FFT_SIZE];
extern float32_t g_fft_output[FFT_SIZE];
extern float32_t g_fft_magnitude[FFT_SIZE / 2];


//======================================================================================
// 数据结构定义
//======================================================================================

/**
 * @brief FFT测量结果结构体
 */
typedef struct {
    float32_t   displayed_amplitude;    // FFT直接测量的峰值幅值 (V)
    float32_t   corrected_amplitude;    // 经过补偿后的精确幅值 (V)
    float32_t   measured_frequency;     // 测量到的信号频率 (Hz)
} AccurateFFT_Result;

/**
 * @brief FFT配置和实例句柄
 */
typedef struct {
    // --- 配置参数 ---
    uint16_t    fft_size;               // FFT点数
    float32_t   sampling_rate;          // 采样率 (Hz)
    AccurateFFT_WindowType window_type; // 使用的窗函数类型

    // --- 内部状态和缓冲区 ---
    arm_rfft_fast_instance_f32 fft_instance; // CMSIS-DSP FFT实例
    float32_t* p_window_coeffs;        // 指向窗函数系数缓冲区的指针
    float32_t* p_fft_output;           // 指向FFT输出缓冲区的指针
    float32_t* p_fft_magnitude;        // 指向幅值谱缓冲区的指针

    // --- 补偿参数 ---
    float32_t   cpg_db;                 // 相干功率增益 (dB)
    float32_t   scalloping_loss_db;     // 最大扇贝损失 (dB)

    // --- 测量结果 ---
    AccurateFFT_Result result;          // 存储最新的测量结果

} AccurateFFT_Handle;


//======================================================================================
// 公共函数声明
//======================================================================================

/**
 * @brief  初始化精确FFT测量模块
 * @param  p_handle: 指向FFT句柄的指针
 * @param  fft_size: FFT点数 (e.g., 1024)
 * @param  sampling_rate: 采样率 (Hz)
 * @param  window_type: 选择的窗函数类型
 * @retval arm_status: ARM_MATH_SUCCESS 如果成功, 否则返回错误码
 */
arm_status AccurateFFT_Init(AccurateFFT_Handle* p_handle, uint16_t fft_size, float32_t sampling_rate, AccurateFFT_WindowType window_type);

/**
 * @brief  执行一次精确的幅值测量
 * @param  p_handle: 指向已初始化的FFT句柄的指针
 * @param  p_input_signal: 指向输入信号数据缓冲区的指针 (长度必须等于fft_size)
 * @retval arm_status: ARM_MATH_SUCCESS 如果成功, 否则返回错误码
 */
arm_status AccurateFFT_Measure(AccurateFFT_Handle* p_handle, float32_t* p_input_signal);


#endif // ACCURATE_FFT_H
