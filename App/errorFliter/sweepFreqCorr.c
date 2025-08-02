#include "Correct.h"
#include "app_config.h"
#include <math.h>
#include "arm_math.h" // 假设已包含
#include "utils.h"
#include "AD9910.h"
#include "process.h"
#include "sample.h"

/**
 * @brief 扫频校准---全环路
 */

#define SWLA_Start_Freq SWEEP_FREQ_START
#define SWLA_End_Freq SWEEP_FREQ_END

#define SWLA_FREQ_NUMES_ASB SWEEP_POINTS // 频率采样点数
static int findFreqIdx(float target_freq, float start_freq, float end_freq, int num_points);


// --- 全局校准数据数组 ---
// 1. 创建一个数组来存储对数间隔的频率点本身
float32_t g_FrequencyPoints[SWLA_FREQ_NUMES_ASB] = {0};
// 2. 创建一个数组来存储每个频率点对应的测量增益
//adc采样比上dds输出
float32_t g_MeasuredGains[SWLA_FREQ_NUMES_ASB] ={
    0.529423833, 0.537258923, 0.546774626, 0.557919562, 0.566783369, 0.575909078, 0.582748592, 0.591090739, 
    0.598039925, 0.606383741, 0.612548649, 0.619048536, 0.625235856, 0.630693018, 0.636016130, 0.641309321, 
    0.646132708, 0.650800645, 0.654637337, 0.659044683, 0.662763059, 0.666532934, 0.669921398, 0.672986209, 
    0.676119626, 0.678644419, 0.681524932, 0.684223950, 0.686625242, 0.688623428, 0.690696418, 0.692629337, 
    0.694491208, 0.696222484, 0.697926581, 0.699389935, 0.700728238, 0.701337993, 0.703232110, 0.704458416, 
    0.704816818, 0.706636906, 0.707403004, 0.708521664, 0.708923817, 0.709830761, 0.710850716, 0.711489737, 
    0.712054551, 0.712654293, 0.713202298, 0.713732660, 0.714359760, 0.714591563, 0.714886487, 0.715655148, 
    0.715929747, 0.715665817, 0.716594398, 0.716891706, 0.717369616, 0.717447221, 0.717691123, 0.717891216, 
    0.718241870, 0.718383610, 0.718522966, 0.718749702, 0.718889058, 0.719046891, 0.719501555, 0.719088137, 
    0.719466269, 0.719790697, 0.719745815, 0.719582021, 0.719959557, 0.719874561, 0.720113695, 0.720260382, 
    0.720360994, 0.720665634, 0.720535100, 0.721012056, 0.721405685, 0.720407784, 0.721381664, 0.721067250, 
    0.720810413, 0.720834553, 0.720673263, 0.720911264, 0.720979035, 0.720991313, 0.721321285, 0.721167386, 
    0.721279681, 0.721205771, 0.721627533, 0.721426904, 0.721153080, 0.721672058, 0.720899880, 0.722257912, 
    0.722275555, 0.721066058, 0.722144186, 0.721390724, 0.722310841, 0.722454727, 0.723615348, 0.721540987, 
    0.722767174, 0.721385419, 0.721729219, 0.722065389, 0.722657919, 0.724194229, 0.721676767, 0.723159969, 
    0.722252309, 0.724923432, 0.724913359, 0.722591221, 0.721918046, 0.722699344, 0.723096669, 0.723069727, 
    0.721706629, 0.724714458, 0.724555790, 0.723277986, 0.724205494, 0.724225581, 0.724519253, 0.723128796, 
    0.726127148, 0.724808753, 0.723064125, 0.724502742, 0.723851442, 0.726496935, 0.724758923, 0.726688445, 
    0.728866398, 0.760563552, 0.724557400, 0.725874960, 0.728517592, 0.730827391, 0.731393814, 0.729039013, 
    0.730080664, 0.730054677, 0.733886302, 0.736687183, 0.727977216, 0.732884467, 0.736844063, 0.728091061, 
    0.730684459, 0.733419240, 0.730842531, 0.730554402, 0.736086309, 0.735336483, 0.735841930, 0.733602285, 
    0.747673988, 0.733259380, 0.737029612, 0.734551430, 0.744212329, 0.741283834, 0.739867449, 0.742406845, 
    0.736030281, 0.741340816, 0.750074208, 0.736093938, 0.741770446, 0.746789038, 0.744669735, 0.737841606, 
    0.739707708, 0.756381333, 0.735705078, 0.744056880, 0.737643898, 0.751425743, 0.742254674, 0.741985142, 
    0.731102467, 0.723004878, 0.737524569, 0.734617233, 0.817452967, 0.740219593, 0.734504223, 0.755705357}; // 预先计算好的增益值

float32_t g_GainsError[SWLA_FREQ_NUMES_ASB] = {0}; // 用于存储增益误差



float getCorrectCoe(float target_freq) {
    int index = findFreqIdx(target_freq, SWEEP_FREQ_START, SWEEP_FREQ_END, SWEEP_POINTS);
    return g_MeasuredGains[index];
}


/**
 * @brief (第一步) 初始化频率数组，生成所有对数间隔的频率点。
 *        这个函数应该在系统启动时被调用一次。
 */
void Initialize_Frequency_Array(void) {
    float log_start = log10f(SWLA_Start_Freq);
    float log_end = log10f(SWLA_End_Freq);
    float log_step = (log_end - log_start) / (float)(SWLA_FREQ_NUMES_ASB - 1);

    for (int i = 0; i < SWLA_FREQ_NUMES_ASB; i++) {
        float current_log_val = log_start + i * log_step;
        g_FrequencyPoints[i] = powf(10, current_log_val);
    }
}


/**
 * @brief (第二步) 执行完整的扫频校准流程。
 *        这个函数会在需要进行校准时被调用。
 */
void SweepWholeLoopCorr(void) {
    Initialize_Frequency_Array();
    float32_t target_freq;
    const float32_t target_vin = 3.0f; // 假设输入电压恒定为3.0V

    float32_t vout_measurements[MAX_ITERATIONS]; // 用于存储多次测量结果
    float32_t mean_vout;

    printf("Starting sweep calibration...\n");

    // 遍历预先计算好的频率点数组
    for (uint32_t i = 0; i < SWLA_FREQ_NUMES_ASB; i++)
    {
        // 从数组中获取当前要扫描的对数频率
        target_freq = g_FrequencyPoints[i];

        SetDacOutputVpp(target_vin, target_freq); // 设置DAC输出电压
        
        // 2. 等待电路稳定
        HAL_Delay(STABILIZE_DELAY_MS);
        
        // 3. 对当前频率进行多次测量
        for (uint32_t k = 0; k < MAX_ITERATIONS; k++)
        {
            vout_measurements[k] = MeasureAdcInputVpp();
            // 可选的短延时
            // HAL_Delay(1); 
        }

        // 4. 计算多次测量的平均值以提高稳定性
        arm_mean_f32(vout_measurements, MAX_ITERATIONS, &mean_vout);

        // 5. 计算增益并存储到结果数组中
        // 索引 i 在 g_FrequencyPoints 和 g_MeasuredGains 之间是对应的
        g_MeasuredGains[i] = mean_vout / target_vin; 
        
        // 打印调试信息
        printf("Index[%3lu]: Freq=%.2f Hz, Gain=%.4f\n", i, target_freq, g_MeasuredGains[i]);
    }
    
    printf("Sweep calibration finished!\n");
}

/**
 * @brief (第三步) 对校准过的频率点进行增益误差计算。
 */
void SweepWholeLoopError(void) {
    Initialize_Frequency_Array();
    float32_t target_freq;
    const float32_t target_vin = 2.0f; // 假设输入电压恒定为3.0V

    float32_t vout_measurements[MAX_ITERATIONS]; // 用于存储多次测量结果
    float32_t mean_vout;

    printf("Starting sweep calibration...\n");

    // 遍历预先计算好的频率点数组
    for (uint32_t i = 0; i < SWLA_FREQ_NUMES_ASB; i++)
    {
        // 从数组中获取当前要扫描的对数频率
        target_freq = g_FrequencyPoints[i];
        int freq_idx = findFreqIdx(target_freq, SWLA_Start_Freq, SWLA_End_Freq, SWLA_FREQ_NUMES_ASB);
        SetDacOutputVpp(target_vin , target_freq); // 设置DAC输出电压

        // 2. 等待电路稳定
        HAL_Delay(STABILIZE_DELAY_MS);
        

        // 3. 对当前频率进行多次测量
        for (uint32_t k = 0; k < MAX_ITERATIONS; k++)
        {
            vout_measurements[k] = MeasureAdcInputVpp();
            // 可选的短延时
            // HAL_Delay(1); 
        }

        // 4. 计算多次测量的平均值以提高稳定性
        arm_mean_f32(vout_measurements, MAX_ITERATIONS, &mean_vout);

        // 5. 计算增益并存储到结果数组中
        // 索引 i 在 g_FrequencyPoints 和 g_MeasuredGains 之间是对应的
        g_GainsError[i] = mean_vout /( target_vin * g_MeasuredGains[freq_idx]); 

        // 打印调试信息
        printf("Index[%3lu]: Freq=%.2f Hz, Gain=%.4f\n", i, target_freq, g_MeasuredGains[i]);
    }
    
    printf("Sweep calibration finished!\n");
}





/**
 * @brief 通过直接数学计算，在一个对数间隔数组中找到最接近目标频率的索引。
 *
 * @param target_freq  要查找的目标频率 (Hz)。
 * @param start_freq   生成数组时使用的起始频率。
 * @param end_freq     生成数组时使用的结束频率。
 * @param num_points   数组中的总点数。
 * @return             最接近目标频率的整数索引。
 */
static int findFreqIdx(float target_freq, float start_freq, float end_freq, int num_points) {
    if (num_points < 2) {
        return 0;
    }
    
    // 边界检查：如果目标频率超出范围，直接返回第一个或最后一个索引
    if (target_freq <= start_freq) {
        return 0;
    }
    if (target_freq >= end_freq) {
        return num_points - 1;
    }

    // 反解公式
    float log_start = log10f(start_freq);
    float log_end = log10f(end_freq);
    float log_step = (log_end - log_start) / (float)(num_points - 1);
    
    // 计算理想的浮点索引
    float ideal_index = (log10f(target_freq) - log_start) / log_step;

    // 四舍五入到最近的整数索引
    int nearest_index = (int)roundf(ideal_index);
    
    // 再次进行边界检查以防万一
    if (nearest_index < 0) return 0;
    if (nearest_index >= num_points) return num_points - 1;

    return nearest_index;
}