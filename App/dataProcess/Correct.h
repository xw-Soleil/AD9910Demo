#ifndef __CORRECT_H__
#define __CORRECT_H__

/* ============================================================================ */
/*                                包含头文件                                    */
/* ============================================================================ */

#include "main.h"
#include "arm_math.h"

/* ============================================================================ */
/*                                  变量声明                                     */
/* ============================================================================ */
#define FREQ_NUMES_ASB 30 // 频率采样点数
#define VOUT_NUMES_ASB 11 // 输出电压采样点数
extern float32_t CorrectVppParam; // 校正参数，初始值为1.0

extern float32_t SweepKnownBoardHsArr[FREQ_NUMES_ASB][VOUT_NUMES_ASB];
extern volatile uint32_t SweepKnownBoardHsFlag; // 用于标记是否完成采样板的扫频测量

/* ============================================================================ */
/*                                函数声明                                     */
/* ============================================================================ */

/**
 * @brief   校正基本要求3、4问的振幅数据
 * @param   data 输入数据
 * @return  校正后的数据
 */
float32_t CorVppSamIBord(float32_t freqkHz, float32_t vpp);
/**
 * @brief 测量采样板增益衰减幅度——峰峰值函数
 */
void SweepSampleBoardVppToA();
void SweepSampleBoardVppToASecond();

/**
 * @brief 测量采样板
 */
void SweepSampleBoardVppToA_Vout();
void SweepSampleBoardVppToA_VoutSecond();


void DDSOutputCorrSamInBord(float32_t freqHz, float32_t vpp);

/**
 * @brief ADC采样输入校正
 */
float32_t ADCSampleInputCorr(float32_t freqHz, float32_t vpp);

/**
 * @brief 扫频采样板Hs
 */
void SweepKnownBoardHs();

/**
 * @brief 对于已知模型输出幅值校正，集成DDS输出
 */
void KnownModelOutPutCorr(float32_t freqHz, float32_t vpp);

#endif /* __CORRECT_H__ */