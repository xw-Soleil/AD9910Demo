/***************************************************************************************
 * @file    accurate_fft.c
 * @author  Gemini
 * @brief   精确FFT幅值测量模块的源文件
 * @version 1.1
 *
 * @note
 * 实现了精确FFT测量的初始化和执行函数。
 ***************************************************************************************/

#include "VppFFTMeu.h"
#include <stdlib.h> // for malloc


float32_t g_window_coeffs[FFT_SIZE];
float32_t g_fft_output[FFT_SIZE];
float32_t g_fft_magnitude[FFT_SIZE / 2];

AccurateFFT_Handle g_my_fft_handle;

//======================================================================================
// 私有函数声明
//======================================================================================
static void generate_window_coeffs(float32_t* p_coeffs, uint16_t size, AccurateFFT_WindowType type);


//======================================================================================
// 公共函数定义
//======================================================================================

/**
 * @brief  初始化精确FFT测量模块
 */
arm_status AccurateFFT_Init(AccurateFFT_Handle* p_handle, uint16_t fft_size, float32_t sampling_rate, AccurateFFT_WindowType window_type)
{
    if (p_handle == NULL) {
        return ARM_MATH_ARGUMENT_ERROR;
    }

    // 1. 设置配置参数
    p_handle->fft_size = fft_size;
    p_handle->sampling_rate = sampling_rate;
    p_handle->window_type = window_type;

    // 2. 为内部缓冲区分配内存
    // 注意: 在实际嵌入式项目中，您可能更倾向于使用静态分配的全局数组而非malloc
    p_handle->p_window_coeffs = g_window_coeffs;
    p_handle->p_fft_output = g_fft_output;
    p_handle->p_fft_magnitude = g_fft_magnitude;

    if (p_handle->p_window_coeffs == NULL || p_handle->p_fft_output == NULL || p_handle->p_fft_magnitude == NULL) {
        // 内存分配失败
        return ARM_MATH_TEST_FAILURE;
    }

    // 3. 初始化CMSIS-DSP FFT实例
    arm_status status = arm_rfft_fast_init_f32(&p_handle->fft_instance, fft_size);
    if (status != ARM_MATH_SUCCESS) {
        return status;
    }

    // 4. 根据窗类型设置补偿值并生成系数
    switch (window_type) {
        case WINDOW_TYPE_FLATTOP:
            p_handle->cpg_db = 13.3f;
            p_handle->scalloping_loss_db = 0.02f;
            break;
        case WINDOW_TYPE_HANN:
            p_handle->cpg_db = 6.02f;
            p_handle->scalloping_loss_db = 1.42f;
            break;
        default:
            return ARM_MATH_ARGUMENT_ERROR; // 不支持的窗类型
    }
    generate_window_coeffs(p_handle->p_window_coeffs, fft_size, window_type);

    return ARM_MATH_SUCCESS;
}


/**
 * @brief  执行一次精确的幅值测量
 */
arm_status AccurateFFT_Measure(AccurateFFT_Handle* p_handle, float32_t* p_input_signal)
{
    if (p_handle == NULL || p_input_signal == NULL) {
        return ARM_MATH_ARGUMENT_ERROR;
    }

    // 1. 对信号加窗 (输入信号被修改)
    arm_mult_f32(p_input_signal, p_handle->p_window_coeffs, p_input_signal, p_handle->fft_size);

    // 2. 执行FFT
    arm_rfft_fast_f32(&p_handle->fft_instance, p_input_signal, p_handle->p_fft_output, 0);

    // 3. 计算FFT输出的幅值
    arm_cmplx_mag_f32(p_handle->p_fft_output, p_handle->p_fft_magnitude, p_handle->fft_size / 2);

    // 4. 幅值谱归一化 (转换为单边谱)
    p_handle->p_fft_magnitude[0] = p_handle->p_fft_magnitude[0] / p_handle->fft_size;
    for (int i = 1; i < p_handle->fft_size / 2; i++) {
        p_handle->p_fft_magnitude[i] = p_handle->p_fft_magnitude[i] * 2.0f / p_handle->fft_size;
    }

    // 5. 找到峰值幅值及其索引
    float32_t max_amplitude_displayed = 0.0f;
    uint32_t peak_index = 0;
    arm_max_f32(&p_handle->p_fft_magnitude[1], (p_handle->fft_size / 2) - 1, &max_amplitude_displayed, &peak_index);
    peak_index += 1; // 补偿数组偏移

    // 6. 计算补偿系数并应用
    float32_t total_loss_db = p_handle->cpg_db + p_handle->scalloping_loss_db;
    float32_t correction_factor = powf(10.0f, total_loss_db / 20.0f);
    float32_t corrected_amplitude = max_amplitude_displayed * correction_factor;

    // 7. 存储结果到句柄中
    p_handle->result.displayed_amplitude = max_amplitude_displayed;
    p_handle->result.corrected_amplitude = corrected_amplitude;
    p_handle->result.measured_frequency = (float32_t)peak_index * p_handle->sampling_rate / p_handle->fft_size;

    return ARM_MATH_SUCCESS;
}


//======================================================================================
// 私有函数定义
//======================================================================================

/**
 * @brief  生成窗函数系数
 */
static void generate_window_coeffs(float32_t* p_coeffs, uint16_t size, AccurateFFT_WindowType type)
{
    if (type == WINDOW_TYPE_FLATTOP) {
        const float32_t a0 = 0.21557895f;
        const float32_t a1 = 0.41663158f;
        const float32_t a2 = 0.277263158f;
        const float32_t a3 = 0.083578947f;
        const float32_t a4 = 0.006947368f;
        for (int i = 0; i < size; i++) {
            p_coeffs[i] = a0 -
                          a1 * arm_cos_f32(2 * PI * i / (size - 1)) +
                          a2 * arm_cos_f32(4 * PI * i / (size - 1)) -
                          a3 * arm_cos_f32(6 * PI * i / (size - 1)) +
                          a4 * arm_cos_f32(8 * PI * i / (size - 1));
        }
    }
    else if (type == WINDOW_TYPE_HANN) {
        for (int i = 0; i < size; i++) {
            p_coeffs[i] = 0.5f * (1.0f - arm_cos_f32(2 * PI * i / (size - 1)));
        }
    }
}