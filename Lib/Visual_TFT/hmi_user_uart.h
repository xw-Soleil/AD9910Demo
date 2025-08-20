/*! 
*  \file hmi_driver.h
*  \brief 串口初始化
*  \version 1.0
*  \date 2012-2018
*  \copyright 广州大彩光电科技有限公司
*/
#ifndef _USER_UART__
#define _USER_UART__



#define uchar    unsigned char
#define uint8    unsigned char
#define uint16   unsigned short int
#define uint32   unsigned long
#define int16    short int
#define int32    long






/*****************************************************************
* 名    称： SendChar()
* 功    能： 发送1个字节
* 入口参数： t  发送的字节
* 出口参数： 无
*****************************************************************/
void  SendChar(uchar t);

/**
 * @brief  初始化串口屏相关的硬件和数据。
 * @note   此函数应在主循环前调用。
 */
void VisualTFT_Init(void);


/**
 * @brief  在主循环中轮询处理来自串口屏的指令。
 * @note   此函数应在 while(1) 循环中被持续调用。
 */
void VisualTFT_Poll(void);


#endif
