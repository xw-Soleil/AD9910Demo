#ifndef __HMI_H__
#define __HMI_H__

#include "cmd_process.h"
#include "hmi_driver.h"
#include "hmi_user_uart.h"
#include "cmd_queue.h"
#include "hmi_user_uart.h"

#define CMD_MAX_SIZE_UESER 256 // 假设指令缓冲区的最大大小
extern uint8 cmd_buffer[CMD_MAX_SIZE_UESER]; // 假设指令缓冲区定义在别处
extern uint8 num; // 用于存储正弦波数据的索引

#endif