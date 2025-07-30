/**********************************************************
                       慷炜电子

移植说明：
平台：STM32F4xx HAL库 (已根据CubeMX定义同步)
功能：AD9910 驱动 (包含幅度控制和任意波形功能)
时间：2025/07/30 (最终正确版)
作者：Gemini (移植) / 参考网络文章修正
原作者：慷炜电子

**********************************************************/

#include "AD9910.h"

/*-----------------------------------------------------------------------------------*/
/*                              寄存器和变量定义                                       */
/*-----------------------------------------------------------------------------------*/

// CFR1: 配置为基本模式，禁用内部profile控制，由SPI串行接口完全控制。
const u8 cfr1[] = {0x00, 0x00, 0x00, 0x00}; 

// CFR2: 【关键修正】使能幅度缩放功能
// 0x01400000 -> Bit 22 (Enable amplitude scale from profile pins) = 1. 这是幅度控制的“主开关”。
const u8 cfr2[] = {0x01, 0x40, 0x00, 0x00}; 

// CFR3: 保持不变 (配置PLL, 40M输入, 25倍频 -> 1GHz系统时钟)
const u8 cfr3[] = {0x05, 0x0F, 0x41, 0x32};

// Profile寄存器数组, 作为缓冲区存放频率、相位、幅度等参数
u8 profile11[] = {0x3f, 0xff, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00};

// 定义一个用于存放任意波形数据的全局数组
u16 Waveform_RAM[WAVE_RAM_LEN];


/*-----------------------------------------------------------------------------------*/
/*                              内部函数声明                                         */
/*-----------------------------------------------------------------------------------*/

static void txd_8bit(u8 txdat);
static void Txfrc(void);


/*-----------------------------------------------------------------------------------*/
/*                              函数实现                                             */
/*-----------------------------------------------------------------------------------*/

/**
  * @brief  通过软件模拟SPI发送8位数据
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
  */
void Init_ad9910(void)
{
    u8 m;

    // 硬件复位AD9910
    HAL_GPIO_WritePin(AD9910_RESET_GPIO_Port, AD9910_RESET_Pin, GPIO_PIN_SET);
    HAL_Delay(1);
    HAL_GPIO_WritePin(AD9910_RESET_GPIO_Port, AD9910_RESET_Pin, GPIO_PIN_RESET);
    HAL_Delay(5);
    
    // 初始化控制引脚电平, 确保它们处于已知的默认状态
    HAL_GPIO_WritePin(AD9910_PROFILE0_GPIO_Port, AD9910_PROFILE0_Pin, GPIO_PIN_RESET);
    HAL_GPIO_WritePin(AD9910_PROFILE1_GPIO_Port, AD9910_PROFILE1_Pin, GPIO_PIN_RESET);
    HAL_GPIO_WritePin(AD9910_PROFILE2_GPIO_Port, AD9910_PROFILE2_Pin, GPIO_PIN_RESET);
    HAL_GPIO_WritePin(AD9910_DRCTL_GPIO_Port, AD9910_DRCTL_Pin, GPIO_PIN_RESET);
    HAL_GPIO_WritePin(AD9910_DRHOLD_GPIO_Port, AD9910_DRHOLD_Pin, GPIO_PIN_RESET);

    // 写 CFR1 寄存器
    HAL_GPIO_WritePin(AD9910_CS_GPIO_Port, AD9910_CS_Pin, GPIO_PIN_RESET);
    txd_8bit(0x00); // CFR1 寄存器地址
    for (m = 0; m < 4; m++) txd_8bit(cfr1[m]);
    HAL_GPIO_WritePin(AD9910_CS_GPIO_Port, AD9910_CS_Pin, GPIO_PIN_SET);
    HAL_Delay(1);

    // 【关键修正】恢复对CFR2寄存器的写入操作
    HAL_GPIO_WritePin(AD9910_CS_GPIO_Port, AD9910_CS_Pin, GPIO_PIN_RESET);
    txd_8bit(0x01); // CFR2 寄存器地址
    for (m = 0; m < 4; m++) txd_8bit(cfr2[m]);
    HAL_GPIO_WritePin(AD9910_CS_GPIO_Port, AD9910_CS_Pin, GPIO_PIN_SET);
    HAL_Delay(1);

    // 写 CFR3 寄存器
    HAL_GPIO_WritePin(AD9910_CS_GPIO_Port, AD9910_CS_Pin, GPIO_PIN_RESET);
    txd_8bit(0x02); // CFR3 寄存器地址
    for (m = 0; m < 4; m++) txd_8bit(cfr3[m]);
    HAL_GPIO_WritePin(AD9910_CS_GPIO_Port, AD9910_CS_Pin, GPIO_PIN_SET);
    HAL_Delay(1);

    // 产生一次I/O Update脉冲, 让所有寄存器配置生效
    HAL_GPIO_WritePin(AD9910_UPDATE_GPIO_Port, AD9910_UPDATE_Pin, GPIO_PIN_SET);
    HAL_Delay(1);
    HAL_GPIO_WritePin(AD9910_UPDATE_GPIO_Port, AD9910_UPDATE_Pin, GPIO_PIN_RESET);
    HAL_Delay(1);
}

/**
  * @brief  发送Profile寄存器的值 (私有函数)
  */
static void Txfrc(void)
{
    u8 m;
    HAL_GPIO_WritePin(AD9910_CS_GPIO_Port, AD9910_CS_Pin, GPIO_PIN_RESET);
    txd_8bit(0x0E); // Profile 0 寄存器地址
    for (m = 0; m < 8; m++) txd_8bit(profile11[m]);
    HAL_GPIO_WritePin(AD9910_CS_GPIO_Port, AD9910_CS_Pin, GPIO_PIN_SET);

    // 产生一次极短的I/O Update脉冲, 让Profile寄存器的内容生效
    HAL_GPIO_WritePin(AD9910_UPDATE_GPIO_Port, AD9910_UPDATE_Pin, GPIO_PIN_SET);
    __NOP();__NOP();__NOP();__NOP();__NOP(); 
    HAL_GPIO_WritePin(AD9910_UPDATE_GPIO_Port, AD9910_UPDATE_Pin, GPIO_PIN_RESET);
}

/**
  * @brief  (模式1: DDS) 根据输入的频率(Hz)和幅度(0-16383)计算参数并发送
  */
void Freq_Amp_convert(u32 Freq, u16 Amp)
{
    u32 ftw_val;
    
    // 幅度参数计算 (14位), 写入profile11数组的前2个字节
    if (Amp > 16383) Amp = 16383;
    profile11[0] = (u8)(Amp >> 8);
    profile11[1] = (u8)Amp;

    // 频率参数计算 (32位), 写入profile11数组的后4个字节
    ftw_val = (u32)((double)Freq * 4.294967296);
    profile11[7] = (u8)ftw_val;
    profile11[6] = (u8)(ftw_val >> 8);
    profile11[5] = (u8)(ftw_val >> 16);
    profile11[4] = (u8)(ftw_val >> 24);
    
    // 发送包含新频率和幅度的Profile数据
    Txfrc();
}

/**
 * @brief  (模式2: AWG) 在STM32中生成一个示例波形(三角波)
 */
void Generate_Triangle_Wave(void)
{
    u16 i;
    for(i = 0; i < WAVE_RAM_LEN; i++)
    {
        if(i < (WAVE_RAM_LEN / 2))
        {
            // 上升沿
            Waveform_RAM[i] = (u16)( ( (float)i / (WAVE_RAM_LEN / 2.0f) ) * 65535.0f );
        }
        else
        {
            // 下降沿
            Waveform_RAM[i] = (u16)( ( 1.0f - ( (float)(i - WAVE_RAM_LEN / 2) / (WAVE_RAM_LEN / 2.0f) ) ) * 65535.0f );
        }
    }
}

/**
  * @brief  (模式2: AWG) 将STM32中的数组写入到AD9910的RAM中
  */
void AD9910_Write_RAM(u16* pData, u16 len)
{
    u16 i;
    HAL_GPIO_WritePin(AD9910_CS_GPIO_Port, AD9910_CS_Pin, GPIO_PIN_RESET);
    txd_8bit(0x16); // RAM 写指令

    for(i = 0; i < len; i++)
    {
        txd_8bit((pData[i] & 0xFF00) >> 8); // 高字节
        txd_8bit(pData[i] & 0x00FF);       // 低字节
    }
    HAL_GPIO_WritePin(AD9910_CS_GPIO_Port, AD9910_CS_Pin, GPIO_PIN_SET);
}