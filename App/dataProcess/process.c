#include "process.h"

#include "arm_math.h" // 引入ARM CMSIS-DSP库

float32_t __COEFF_S2 = 1.0e-8f; // s^2 项的系数 (10^-8)
float32_t __COEFF_S1 = 3.0e-4f; // s^1 项的系数 (3*10^-4)
float32_t __NUMERATOR = 5.0f; // 传递函数分子

/**
 * @brief 根据期望的输出电压峰峰值，计算“已知模型电路”所需的输入电压峰峰值。
 * @param VoutVpp: 期望的电路输出电压峰峰值 (单位: V)。
 * @param freqHz: 输入信号频率 (单位: Hz)，默认为1kHz。
 * @param coeff_s2: s^2 项的系数，默认为1.0e-8。
 * @param coeff_s1: s^1 项的系数，默认为3.0e-4。
 * @param numerator: 传递函数分子，默认为5.0。
 * @retval 计算得出的所需输入电压峰峰值 (单位: V)。
 */
float32_t VinVpp_KnownModel(float32_t VoutVpp, float32_t freqHz, float32_t coeff_s2, float32_t coeff_s1, float32_t numerator)
{
    /* --- 常量定义 --- */
    // 根据题目中的传递函数 H(s) 和频率 f=1kHz 定义常量
    float32_t FREQ_HZ = freqHz;           // 输入信号频率 (1kHz)
    float32_t COEFF_S2 = coeff_s2;          // s^2 项的系数 (10^-8)
    float32_t COEFF_S1 = coeff_s1;          // s^1 项的系数 (3*10^-4)
    float32_t NUMERATOR = numerator;            // 传递函数分子

    /* --- 变量定义 --- */
    float32_t omega, omega_sq;
    float32_t real_part, imag_part;
    float32_t denom_mag_sq, denom_mag;
    float32_t gain;
    float32_t VinVpp;

    /* --- 步骤1: 计算角频率 ω (omega) --- */
    // ω = 2 * π * f
    // arm_math.h 中定义了 PI_F32 作为float32类型的π
    omega = 2.0f * PI * FREQ_HZ;

    /* --- 步骤2: 计算传递函数分母的实部和虚部 --- */
    // 分母 = (1 - 10⁻⁸ω²) + j * (3×10⁻⁴ω)
    omega_sq = omega * omega;
    real_part = 1.0f - (COEFF_S2 * omega_sq);
    imag_part = COEFF_S1 * omega;

    /* --- 步骤3: 计算分母模的平方 --- */
    // |D|^2 = Real² + Imag²
    denom_mag_sq = (real_part * real_part) + (imag_part * imag_part);

    /* --- 步骤4: 使用 ARM Math 库计算分母的模 --- */
    // |D| = sqrt(|D|^2)
    // arm_sqrt_f32 是一个针对ARM Cortex-M4/M7内核优化的平方根函数
    arm_sqrt_f32(denom_mag_sq, &denom_mag);

    /* --- 步骤5: 计算电路在1kHz时的增益 --- */
    // Gain = |H(jω)| = 5 / |D|
    gain = NUMERATOR / denom_mag;

    /* --- 步骤6: 根据增益和期望输出计算所需输入 --- */
    // VinVpp = VoutVpp / Gain
    if (gain > 1e-9f) // 防止除以一个非常小或为零的数
    {
        VinVpp = VoutVpp / gain;
    }
    else
    {
        // 错误处理：增益异常，返回0
        VinVpp = 0.0f;
    }

    return VinVpp;
}