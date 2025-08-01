#include "screen.h"
#include "Correct.h"
#include "process.h"
#include "AD9910.h"
#include "utils.h"
float32_t waveDDSFreq = 1.0f; // 正弦波频率 kHz
float32_t waveDDSVpp = 1.0f; // 正弦波峰峰值 V

float32_t waveKonwnModelFreq = 1.0f; // 已知模型电路频率 kHz
float32_t waveKonwnModelVpp = 1.0f; // 已知模型电路峰峰值 V


void NotifyButton(uint16 screen_id, uint16 control_id, uint8 state){
    if(screen_id == 2 && control_id == 1){
        sys_mode = SYS_BASIC_OUTPUT; // 切换到基本输出模式
        AD9910_Set_Sine_Wave(waveDDSFreq*1000.0f, 16383 * CorVppSamIBord(waveDDSFreq, waveDDSVpp) / MAX_DDS_VPP); // 设置正弦波频率为100kHz，幅度为16383（对应3.3V）
    }
    if(screen_id == 3 && control_id == 1){
        sys_mode = SYS_BASIC_HS_OUTPUT; // 切换到已知模型输出模式
        KnownModelOutPutCorr(waveKonwnModelFreq * 1000.0f, waveKonwnModelVpp); // 设置已知模型输出电压
    }
    if((screen_id == 2 || screen_id == 3) && control_id == 4){
        Init_AD9910(); // 初始化AD9910
        sys_mode = SYS_WAITING; // 切换到等待状态
    }
    if(screen_id == 4 && control_id == 1 && state == 0){
        sys_mode = SYS_PERFORMANCE_LEARN; // 切换到性能学习模式
    }
    if(screen_id == 4 && control_id == 2 && state == 0){
        sys_mode = SYS_PERFORMANCE_OUTPUT; // 切换到性能输出模式
    }
    if(screen_id == 4 && control_id == 3 && state == 0){
        sys_mode = SYS_WAITING; // 切换到性能学习完成状态
    }
}
void NotifyText(uint16 screen_id, uint16 control_id, uint8 *str){
    if(screen_id == 2 && control_id == 5){
        float value = atof((char*)str);
        waveDDSFreq = (float32_t)value;
    }
    if(screen_id == 2 && control_id == 6){
        float value = atof((char*)str);
        waveDDSVpp = (float32_t)value;
    }

    if(screen_id == 3 && control_id == 5){
        float value = atof((char*)str);
        waveKonwnModelFreq = (float32_t)value;
    }
    if(screen_id == 3 && control_id == 6){
        float value = atof((char*)str);
        waveKonwnModelVpp = (float32_t)value;
    }

}

/**
 * 生成文本控件指令
 * @param page_id 页面ID
 * @param control_id 控件ID
 * @param text 要传入的文本
 * @param command 输出的指令缓冲区
 * @return 指令长度
 */
uint16_t generate_text_command(uint16_t page_id, uint8_t control_id, const char *text, uint8_t *command)
{
    if (text == NULL || command == NULL)
    {
        return 0;
    }

    int text_len = strlen(text);
    int cmd_index = 0;
    int src_index = 0; // 用于遍历源文本字符串

    // 固定头部
    command[cmd_index++] = 0xEE;
    command[cmd_index++] = 0xB1;
    command[cmd_index++] = 0x10;
    command[cmd_index++] = 0x00;

    // 页面ID (小端格式)
    command[cmd_index++] = page_id & 0xFF;        // 低字节
    command[cmd_index++] = (page_id >> 8) & 0xFF; // 高字节

    // 控件ID
    command[cmd_index++] = control_id;

    // 文本内容 (ASCII码)
    // --- 文本内容处理 (核心修正) ---
    while (src_index < text_len)
    {
        // 检查是否是 GBK/GB2312 编码的 'Ω' (CE A9)
        if ((text_len - src_index >= 2) &&      // 确保至少还有2个字节
            (uint8_t)text[src_index]     == 0xCE &&
            (uint8_t)text[src_index + 1] == 0xA9)
        {
            // 找到了 'Ω', 将其替换为 HMI 的自定义编码 A6 A8
            command[cmd_index++] = 0xA6;
            command[cmd_index++] = 0xB8;
            src_index += 2; // 跳过源字符串中的2个字节
        }
        else
        {
            // 其他所有字符，直接复制
            command[cmd_index++] = (uint8_t)text[src_index];
            src_index++;
        }
    }

    // 固定结尾
    command[cmd_index++] = 0xFF;
    command[cmd_index++] = 0xFC;
    command[cmd_index++] = 0xFF;
    command[cmd_index++] = 0xFF;

    return cmd_index;
}

/**
 * @brief 切换页面
 */
void SwitchPage(uint8_t page_id)
{
    uint8_t cmd[9] = {0xEE, 0xB1, 0x00, 0x00, 0x00, 0xFF, 0xFC, 0xFF, 0xFF};
    cmd[4] = page_id; // 页面ID（单字节）
    HAL_UART_Transmit(&huart4, cmd, sizeof(cmd) / sizeof(cmd[0]), 0xFF);
}


/**
 * @brief 清屏screen1(ID 6,7,8) / screen2(ID 9及其他控件) 的内容
 * @param screen_id 指定页面ID (1 或 2)
 */
void ClearScreen(uint8_t screen_id)
{
    switch (screen_id)
    {
        case 1:
        {
            // 清空页面1的文本控件 (ID 6, 7, 8) 指令: EE B1 12 00 01 00 06 00 00 00 07 00 00 00 08 00 00 FF FC FF FF
            uint8_t cmd_screen1[] = {0xEE, 0xB1, 0x12, 0x00, 0x01, 0x00, 0x06, 0x00, 0x00, 0x00, 0x07, 0x00, 0x00, 0x00, 0x08, 0x00, 0x00, 0xFF, 0xFC, 0xFF, 0xFF};
            HAL_UART_Transmit(&huart4, cmd_screen1, sizeof(cmd_screen1), 0xFF);
            break;
        }
        case 2:
        {
            // 清空页面2的文本控件 (ID 9)
            // 指令: EE B1 10 00 02 00 09 FF FC FF FF
            uint8_t cmd_clear_text[] = {0xEE, 0xB1, 0x10, 0x00, 0x02, 0x00, 0x09, 0xFF, 0xFC, 0xFF, 0xFF};
            HAL_UART_Transmit(&huart4, cmd_clear_text, sizeof(cmd_clear_text), 0xFF);

            // 重置页面2的曲线控件 (ID 1) 的通道0
            // 指令: EE B1 33 00 02 00 01 00 FF FC FF FF
            uint8_t cmd_reset_control1[] = {0xEE, 0xB1, 0x33, 0x00, 0x02, 0x00, 0x01, 0x00, 0xFF, 0xFC, 0xFF, 0xFF};
            HAL_UART_Transmit(&huart4, cmd_reset_control1, sizeof(cmd_reset_control1), 0xFF);

            // 重置页面2的曲线控件 (ID 1) 的通道1, 指令: EE B1 33 00 02 00 01 01 FF FC FF FF
            uint8_t cmd_reset_control2[] = {0xEE, 0xB1, 0x33, 0x00, 0x02, 0x00, 0x01, 0x01, 0xFF, 0xFC, 0xFF, 0xFF};
            HAL_UART_Transmit(&huart4, cmd_reset_control2, sizeof(cmd_reset_control2), 0xFF);
            break;
        }
        default:
            break;
    }
}

/**
 * @brief 发送文本到指定页面控件
 * @param page_id 页面ID
 * @param control_id 控件ID
 * @param text 要显示的文本
 */
void SendText(uint16_t page_id, uint8_t control_id, const char *text)
{
    uint8_t command_buffer[128];
    uint16_t cmd_length;

    cmd_length = generate_text_command(page_id, control_id, text, command_buffer);
    if (cmd_length > 0)
    {
        HAL_UART_Transmit(&huart4, command_buffer, cmd_length, 0xFF);
    }
}

/**
 * @brief 发送浮点数+单位到指定页面控件
 * @param page_id 页面ID
 * @param control_id 控件ID
 * @param value 数值
 * @param unit 单位字符串 (如"kHz", "V", "°C")
 * @param decimals 小数位数
 */
void SendFloat(uint16_t page_id, uint8_t control_id, float value, const char *unit, uint8_t decimals)
{
    char text_buffer[32];
    char format[16];

    // 生成格式字符串
    if (unit != NULL)
    {
        snprintf(format, sizeof(format), "%%.%df %s", decimals, unit);
    }
    else
    {
        snprintf(format, sizeof(format), "%%.%df", decimals);
    }

    // 格式化数值
    snprintf(text_buffer, sizeof(text_buffer), format, value);

    // 发送到屏幕
    SendText(page_id, control_id, text_buffer);
}

/**
 * @brief 发送整数+单位到指定页面控件
 * @param page_id 页面ID
 * @param control_id 控件ID
 * @param value 整数值
 * @param unit 单位字符串 (如"Hz", "V", "mA")
 */
void SendInt(uint16_t page_id, uint8_t control_id, int value, const char *unit)
{
    char text_buffer[32];

    if (unit != NULL)
    {
        snprintf(text_buffer, sizeof(text_buffer), "%d %s", value, unit);
    }
    else
    {
        snprintf(text_buffer, sizeof(text_buffer), "%d", value);
    }

    SendText(page_id, control_id, text_buffer);
}