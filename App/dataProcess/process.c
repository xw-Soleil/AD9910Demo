#include "process.h"
#include "Correct.h"
#include "sample.h"
#include "arm_math.h"
#include "stdint.h"
#include "math.h" // For fabsf

/* ================================================================================= */
/*                             1. 宏定义与全局变量                                   */
/* ================================================================================= */

float32_t __COEFF_S2 = 1.0e-8f; // s^2 项的系数 (10^-8)
float32_t __COEFF_S1 = 3.0e-4f; // s^1 项的系数 (3*10^-4)
float32_t __NUMERATOR = 5.0f; // 传递函数分子

// --- 全局2D查找表和状态标志 ---
static float32_t g_vin_lookup_table[FREQ_POINTS][VOUT_POINTS];
static volatile uint8_t g_is_lut_populated = 0; // volatile防止编译器优化


void SetDacOutputVpp(float32_t vpp, float32_t freqHz){
    AD9910_Set_Sine_Wave(freqHz, 16383 *vpp  / MAX_DDS_VPP); // 设置正弦波频率为100kHz，幅度为16383（对应3.3V）
}

void Delay_ms(uint32_t ms){
    HAL_Delay(ms); // 使用HAL库的延时函数
}

// 内部辅助函数：根据理论公式计算初始猜测值
static float32_t calculate_initial_guess(float32_t vout_vpp, float32_t freq_hz, float32_t s2, float32_t s1, float32_t num)
{
    float32_t omega, omega_sq, real_part, imag_part, denom_mag_sq, denom_mag, gain;
    omega = 2.0f * PI * freq_hz;
    omega_sq = omega * omega;
    real_part = 1.0f - (s2 * omega_sq);
    imag_part = s1 * omega;
    denom_mag_sq = (real_part * real_part) + (imag_part * imag_part);
    arm_sqrt_f32(denom_mag_sq, &denom_mag);
    if (denom_mag < 1e-9f) return 0.0f;
    gain = num / denom_mag;
    if (gain < 1e-9f) return 0.0f;
    return vout_vpp / gain;
}

/**
 * @brief  [核心函数] 生成2D查找表，包含频率和输出电压的组合。
 */
void Generate_Vin_LUT_2D(void)
{
    float32_t target_freq, target_vout;
    float32_t vin_to_try, measured_vout, error;

    for (uint32_t i = 0; i < FREQ_POINTS; i++)
    {
        target_freq = FREQ_START + i * FREQ_STEP;
        for (uint32_t j = 0; j < VOUT_POINTS; j++)
        {
            target_vout = VOUT_START + j * VOUT_STEP;

            // 使用理论公式计算一个初始猜测值
            vin_to_try = calculate_initial_guess(target_vout, target_freq, 1.0e-8f, 3.0e-4f, 5.0f);
            if (vin_to_try <= 0.0f) { vin_to_try = target_vout; }

            for (uint32_t k = 0; k < MAX_ITERATIONS; k++)
            {
                SetDacOutputVpp(vin_to_try, target_freq);
                Delay_ms(STABILIZE_DELAY_MS);
                measured_vout = MeasureAdcInputVpp();
                error = target_vout - measured_vout;

                if (fabsf(error) < TARGET_PRECISION_V) break;
                
                if (measured_vout > 1e-6f) {
                    vin_to_try = vin_to_try * (target_vout / measured_vout);
                } else {
                    vin_to_try += 0.1f;
                }
            }
            g_vin_lookup_table[i][j] = vin_to_try;
        }
    }
    g_is_lut_populated = 1;
}

/**
 * @brief  [最终函数] 根据期望输出电压计算所需输入电压。
 * @note   函数内部自动判断是否已校准：
 *         - 若已校准，则从2D查找表获取高精度数据，忽略理论系数。
 *         - 若未校准，则使用传入的理论系数按原始公式计算。
 * @param VoutVpp:    期望的电路输出电压峰峰值 (单位: V)。
 * @param freqHz:     输入信号频率 (单位: Hz)。
 * @param coeff_s2, coeff_s1, numerator: 传递函数的理论系数 (仅在未校准时使用)。
 * @retval 计算得出的所需输入电压峰峰值 (单位: V)。
 */
float32_t VinVpp_KnownModel(float32_t VoutVpp, float32_t freqHz, float32_t coeff_s2, float32_t coeff_s1, float32_t numerator)
{
    // *** 核心智能切换逻辑 ***
    if (g_is_lut_populated)
    {
        // --- 方案A: 已校准，从2D查找表获取精确值 ---
        
        // 参数范围检查
        if (freqHz < FREQ_START || freqHz > FREQ_STOP || VoutVpp < VOUT_START || VoutVpp > VOUT_STOP) {
            return 0.0f; // 参数超出校准范围
        }
        
        // 计算索引
        uint32_t freq_index = lroundf((freqHz - FREQ_START) / FREQ_STEP);
        uint32_t vout_index = lroundf((VoutVpp - VOUT_START) / VOUT_STEP);
    
        // 索引安全检查
        if (freq_index >= FREQ_POINTS || vout_index >= VOUT_POINTS) {
            return 0.0f;
        }

        // 直接查表返回高精度结果
        return g_vin_lookup_table[freq_index][vout_index];
    }
    else
    {
        // --- 方案B: 未校准，执行您的原始理论计算代码 ---
        float32_t omega, omega_sq;
        float32_t real_part, imag_part;
        float32_t denom_mag_sq, denom_mag;
        float32_t gain;
        float32_t VinVpp_calc;

        omega = 2.0f * PI * freqHz;
        omega_sq = omega * omega;
        real_part = 1.0f - (coeff_s2 * omega_sq);
        imag_part = coeff_s1 * omega;
        denom_mag_sq = (real_part * real_part) + (imag_part * imag_part);
        arm_sqrt_f32(denom_mag_sq, &denom_mag);
        if (denom_mag < 1e-9f) { gain = 0.0f; } else { gain = numerator / denom_mag; }
        if (gain > 1e-9f) { VinVpp_calc = VoutVpp / gain; } else { VinVpp_calc = 0.0f; }

        return VinVpp_calc;
    }
}
