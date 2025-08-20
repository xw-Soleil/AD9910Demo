/************************************版权申明********************************************
**                             广州大彩光电科技有限公司
**                             http://www.gz-dc.com
**-----------------------------------文件信息--------------------------------------------
** 文件名称:   hmi_user_uart.c
** 修改时间:   2018-05-18
** 文件说明:   用户MCU串口驱动函数库
** 技术支持：  Tel: 020-82186683  Email: hmi@gz-dc.com Web:www.gz-dc.com
--------------------------------------------------------------------------------------

--------------------------------------------------------------------------------------
使用必读
hmi_user_uart.c中的串口发送接收函数共3个函数：串口初始化Uartinti()、发送1个字节SendChar()、
发送字符串SendStrings().若移植到其他平台，需要修改底层寄
存器设置,但禁止修改函数名称，否则无法与HMI驱动库(hmi_driver.c)匹配。
--------------------------------------------------------------------------------------



----------------------------------------------------------------------------------------
1. 基于STM32平台串口驱动
----------------------------------------------------------------------------------------*/

#include "hmi_user_uart.h"
#include "usart.h"

/*!
*   \brief  发送1个字节
*   \param  t 发送的字节
*/

void  SendChar(uchar t)
{
	//printf("%c", t);
	//WRITE_REG(huart2.Instance->DR,( uint16_t)t);
	HAL_UART_Transmit(&huart4, (uint8_t *)&t, 1, 1000);

	//while( HAL_UART_Transmit(&huart2, (uint8_t *)(&t), sizeof(t), 1000) != HAL_OK);
	//while(HAL_UART_GetState(&huart2) != HAL_UART_STATE_RESET);
    //HAL_UART_Transmit_IT(&huart2, &t, 1);
}

#include "hmi.h"
#include "utils.h"

uint8  cmd_buffer[CMD_MAX_SIZE_UESER];		//指令缓冲区
uint8_t uart4_rx_buffer = 0;  // UART4接收缓冲区（单字节）
/**
 * @brief  初始化串口屏相关的数据
 * @note 在调用此函数之前请先确保串口中断已经初始化
 */
void VisualTFT_Init(void)
{
  /* 广州大彩 7 寸串口屏 UART 相关初始化 */

  // 启动UART4接收中断
  if(HAL_UART_Receive_IT(&huart4, &uart4_rx_buffer, 1) != HAL_OK)
  {
      printf("Error starting UART4 receive interrupt!\n");
      Error_Handler();
  }

  
  // 重置指令队列
  queue_reset();
  
  // 等待一小段时间，确保屏幕准备就绪
  HAL_Delay(300);
}

/**
 * @brief  在主循环中轮询处理来自串口屏的指令。
 */
void VisualTFT_Poll(void)
{

	size_t size = 0;

	// 7寸屏指令处理
	size = queue_find_cmd(cmd_buffer, CMD_MAX_SIZE_UESER); // 从缓冲区中获取一条指令
	if (size > 0 && cmd_buffer[1] != 0x07)			 // 接收到指令 ，及判断是否为开机提令
	{
		ProcessMessage((PCTRL_MSG)cmd_buffer, size); // 指令处理
	}
	else if (size > 0 && cmd_buffer[1] == 0x07) // 如果为指令0x07就软重置STM32
	{
		NVIC_SystemReset();
	}
}

/**
 * @brief UART接收完成回调函数
 * @param huart UART句柄
 */
void HAL_UART_RxCpltCallback(UART_HandleTypeDef *huart)
{
    if(huart->Instance == UART4)
    {
        // 将接收到的数据放入队列（保持你的queue_push调用）
        queue_push(uart4_rx_buffer);
        // 重新启动下一次接收中断
        HAL_UART_Receive_IT(&huart4, &uart4_rx_buffer, 1);
    }
}
