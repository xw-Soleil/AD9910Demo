/**********************************************************
                       慷炜电子

移植说明：
平台：STM32F4xx HAL库 (已根据CubeMX定义同步)
功能：AD9910 驱动
时间：2025/07/30 (修正版)
作者：Gemini (移植)
原作者：慷炜电子

**********************************************************/

#include "AD9910.h"

// 寄存器配置数据 (与原代码保持一致)
const u8 cfr1[] = {0x00, 0x40, 0x00, 0x00};
const u8 cfr3[] = {0x05, 0x0F, 0x41, 0x32};
u8 profile11[] = {0x3f, 0xff, 0x00, 0x00, 0x25, 0x09, 0x7b, 0x42};

// 内部函数声明
// 注意：IO初始化函数现在不再需要，因为CubeMX会完成所有工作
static void txd_8bit(u8 txdat);
static void Txfrc(void);


/**
  * @brief  通过软件模拟SPI发送8位数据
  * @param  txdat: 要发送的字节
  * @retval None
  */
static void txd_8bit(u8 txdat)
{
    u8 i;
    for (i = 0; i < 8; i++)
    {
        HAL_GPIO_WritePin(AD9910_SCLK_GPIO_Port, AD9910_SCLK_Pin, GPIO_PIN_RESET);
        
        if (txdat & 0x80) {
            HAL_GPIO_WritePin(AD9910_SDIO_GPIO_Port, AD9910_SDIO_Pin, GPIO_PIN_SET);
        } else {
            HAL_GPIO_WritePin(AD9910_SDIO_GPIO_Port, AD9910_SDIO_Pin, GPIO_PIN_RESET);
        }
        
        HAL_GPIO_WritePin(AD9910_SCLK_GPIO_Port, AD9910_SCLK_Pin, GPIO_PIN_SET);
        txdat <<= 1;
    }
    HAL_GPIO_WritePin(AD9910_SCLK_GPIO_Port, AD9910_SCLK_Pin, GPIO_PIN_RESET);
}


/**
  * @brief  初始化AD9910芯片
  * @param  None
  * @retval None
  * @note   此函数假设CubeMX生成的 MX_GPIO_Init() 已被调用
  */
void Init_ad9910(void)
{
    u8 m;

    // 硬件复位AD9910
    HAL_GPIO_WritePin(AD9910_RESET_GPIO_Port, AD9910_RESET_Pin, GPIO_PIN_SET);
    HAL_Delay(1);
    HAL_GPIO_WritePin(AD9910_RESET_GPIO_Port, AD9910_RESET_Pin, GPIO_PIN_RESET);
    HAL_Delay(5);
    
    // 初始化控制引脚电平
    HAL_GPIO_WritePin(AD9910_PROFILE0_GPIO_Port, AD9910_PROFILE0_Pin, GPIO_PIN_RESET);
    HAL_GPIO_WritePin(AD9910_PROFILE1_GPIO_Port, AD9910_PROFILE1_Pin, GPIO_PIN_RESET);
    HAL_GPIO_WritePin(AD9910_PROFILE2_GPIO_Port, AD9910_PROFILE2_Pin, GPIO_PIN_RESET);
    HAL_GPIO_WritePin(AD9910_DRCTL_GPIO_Port, AD9910_DRCTL_Pin, GPIO_PIN_RESET);
    HAL_GPIO_WritePin(AD9910_DRHOLD_GPIO_Port, AD9910_DRHOLD_Pin, GPIO_PIN_RESET);


    // 写 CFR1 寄存器
    HAL_GPIO_WritePin(AD9910_CS_GPIO_Port, AD9910_CS_Pin, GPIO_PIN_RESET);
    txd_8bit(0x00); // CFR1 寄存器地址
    for (m = 0; m < 4; m++) {
        txd_8bit(cfr1[m]);
    }
    HAL_GPIO_WritePin(AD9910_CS_GPIO_Port, AD9910_CS_Pin, GPIO_PIN_SET);
    HAL_Delay(1);

    // 写 CFR3 寄存器
    HAL_GPIO_WritePin(AD9910_CS_GPIO_Port, AD9910_CS_Pin, GPIO_PIN_RESET);
    txd_8bit(0x02); // CFR3 寄存器地址
    for (m = 0; m < 4; m++) {
        txd_8bit(cfr3[m]);
    }
    HAL_GPIO_WritePin(AD9910_CS_GPIO_Port, AD9910_CS_Pin, GPIO_PIN_SET);
    HAL_Delay(1);

    // 更新寄存器内容
    HAL_GPIO_WritePin(AD9910_UPDATE_GPIO_Port, AD9910_UPDATE_Pin, GPIO_PIN_SET);
    HAL_Delay(1);
    HAL_GPIO_WritePin(AD9910_UPDATE_GPIO_Port, AD9910_UPDATE_Pin, GPIO_PIN_RESET);
    HAL_Delay(1);
}

/**
  * @brief  发送Profile寄存器的值 (包含频率、相位、幅度信息)
  * @param  None
  * @retval None
  */
static void Txfrc(void)
{
    u8 m;

    HAL_GPIO_WritePin(AD9910_CS_GPIO_Port, AD9910_CS_Pin, GPIO_PIN_RESET);
    txd_8bit(0x0E); // Profile 0 寄存器地址
    for (m = 0; m < 8; m++) {
        txd_8bit(profile11[m]);
    }
    HAL_GPIO_WritePin(AD9910_CS_GPIO_Port, AD9910_CS_Pin, GPIO_PIN_SET);

    // 更新寄存器，让新配置生效
    HAL_GPIO_WritePin(AD9910_UPDATE_GPIO_Port, AD9910_UPDATE_Pin, GPIO_PIN_SET);
    __NOP();__NOP();__NOP();__NOP();__NOP(); 
    HAL_GPIO_WritePin(AD9910_UPDATE_GPIO_Port, AD9910_UPDATE_Pin, GPIO_PIN_RESET);
}

/**
  * @brief  根据输入的频率(Hz)计算频率调谐字(FTW)并发送
  * @param  Freq: 目标频率，单位 Hz
  * @retval None
  */
void Freq_convert(u32 Freq)
{
    // FTW = (Desired Freq * 2^32) / SysClk
    // 因子 = 2^32 / 1,000,000,000 = 4.294967296
    u32 Temp = (u32)((double)Freq * 4.294967296);

    profile11[7] = (u8)Temp;
    profile11[6] = (u8)(Temp >> 8);
    profile11[5] = (u8)(Temp >> 16);
    profile11[4] = (u8)(Temp >> 24);
    
    Txfrc();
}