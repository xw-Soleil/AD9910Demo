#include "Correct.h"

float32_t CorrectVppParam = 1.0f; // 校正参数，初始值为1.0

/**
 * @brief   校正基本要求3、4问的振幅数据
 * @param   data 输入数据
 * @return  校正后的数据
 */
float32_t CorrectVppData(float32_t data)
{
    return data * CorrectVppParam; 
}