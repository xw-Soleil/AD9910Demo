#ifndef __PROCESS_H__
#define __PROCESS_H__
/* ============================================================================ */
/*                                包含头文件                                    */  
/* ============================================================================ */
#include "main.h"
#include "arm_math.h"
#include "AD9910.h"
/* ============================================================================ */
/*                                  变量声明                                     */
/* ============================================================================ */
extern float32_t __COEFF_S2;
extern float32_t __COEFF_S1;
extern float32_t __NUMERATOR;

// --- 查找表维度定义 ---
#define FREQ_START      100.0f
#define FREQ_STOP       3000.0f
#define FREQ_STEP       100.0f
#define FREQ_POINTS     30// 30

#define VOUT_START      1.0f
#define VOUT_STOP       2.0f
#define VOUT_STEP       0.1f
#define VOUT_POINTS     11 // 11




/* ============================================================================ */
/*                                  函数声明                                     */
/* ============================================================================ */

/**
 * @brief  [启动时调用] 通过迭代搜索，生成并填充2D查找表。
 */
void Generate_Vin_LUT_2D(void);


/**
 * @brief 根据期望的输出电压峰峰值，计算“已知模型电路”所需的输入电压峰峰值。
 * @param VoutVpp: 期望的电路输出电压峰峰值 (单位: V)。
 * @param freqHz: 输入信号频率 (单位: Hz)，默认为1kHz。
 * @param coeff_s2: s^2 项的系数，默认为1.0e-8。
 * @param coeff_s1: s^1 项的系数，默认为3.0e-4。
 * @param numerator: 传递函数分子，默认为5.0。
 * @retval 计算得出的所需输入电压峰峰值 (单位: V)。
 */
float32_t VinVpp_KnownModel(float32_t VoutVpp, float32_t freqHz, float32_t coeff_s2, float32_t coeff_s1, float32_t numerator);

void SetDacOutputVpp(float32_t vpp, float32_t freqHz);

#endif /* __PROCESS_H__ */
