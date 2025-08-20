// utils.h

#ifndef UTILS_H
#define UTILS_H

#include "stm32f4xx_hal.h"
#include <stdio.h>

// 串口重定向配置
#include "usart.h"

// 函数声明
void Utils_Init(void);

// 编译器兼容性宏定义
#ifdef __GNUC__
#define PUTCHAR_PROTOTYPE int __io_putchar(int ch)
#define GETCHAR_PROTOTYPE int __io_getchar(void)
#else
#define PUTCHAR_PROTOTYPE int fputc(int ch, FILE *f)
#define GETCHAR_PROTOTYPE int fgetc(FILE *f)
#endif /* __GNUC__ */

// 函数原型声明
PUTCHAR_PROTOTYPE;
GETCHAR_PROTOTYPE;

#endif // UTILS_H