// utils.c

#include "utils.h"

// 配置 printf 重定向使用的 UART
#define PRINTF_UART huart1

/**
 * @brief 工具模块初始化
 */
void Utils_Init(void)
{
    // 可在此处添加其他工具初始化代码
    // 目前 printf 重定向无需特殊初始化
}

/**
 * @brief printf 字符输出重定向 (GCC 编译器)
 */
#ifdef __GNUC__
PUTCHAR_PROTOTYPE
{
    HAL_UART_Transmit(&PRINTF_UART, (uint8_t *)&ch, 1, HAL_MAX_DELAY);
    return ch;
}

/**
 * @brief scanf 字符输入重定向 (GCC 编译器)
 */
GETCHAR_PROTOTYPE
{
    uint8_t ch = 0;
    HAL_UART_Receive(&PRINTF_UART, &ch, 1, HAL_MAX_DELAY);
    return ch;
}

#else
/**
 * @brief printf 字符输出重定向 (其他编译器)
 */
int fputc(int ch, FILE *f)
{
    HAL_UART_Transmit(&PRINTF_UART, (uint8_t *)&ch, 1, HAL_MAX_DELAY);
    return ch;
}

/**
 * @brief scanf 字符输入重定向 (其他编译器)
 */
int fgetc(FILE *f)
{
    uint8_t ch = 0;
    HAL_UART_Receive(&PRINTF_UART, &ch, 1, HAL_MAX_DELAY);
    return ch;
}
#endif /* __GNUC__ */