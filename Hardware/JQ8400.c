#include "stm32f10x.h"
#include "Delay.h"
#include "JQ8400.h"
#include <stddef.h> 

//发送一个字节
static void USART1_SendByte(uint8_t data)
{
    while (USART_GetFlagStatus(USART1, USART_FLAG_TXE) == RESET);
    USART_SendData(USART1, data);
}

//发送指令帧 
static void JQ8400_SendCmd(uint8_t cmd, uint8_t *data, uint8_t len)
{
    uint8_t i;
    uint8_t sum = 0;
    uint8_t buf[10];

    buf[0] = 0xAA;
    buf[1] = cmd;
    buf[2] = len;

    for (i = 0; i < len; i++) {
        buf[3 + i] = data[i];
    }

    for (i = 0; i < 3 + len; i++) {
        sum += buf[i];
    }
    buf[3 + len] = sum;

    for (i = 0; i < 4 + len; i++) {
        USART1_SendByte(buf[i]);
    }
}

// 初始化
void JQ8400_Init(void)
{
    GPIO_InitTypeDef GPIO_InitStructure;
    USART_InitTypeDef USART_InitStructure;

    RCC_APB2PeriphClockCmd(RCC_APB2Periph_USART1, ENABLE);
    RCC_APB2PeriphClockCmd(RCC_APB2Periph_GPIOA, ENABLE);

    // 配置 TX为复用推挽输出
    GPIO_InitStructure.GPIO_Mode = GPIO_Mode_AF_PP;
    GPIO_InitStructure.GPIO_Pin = GPIO_Pin_9;
    GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;
    GPIO_Init(GPIOA, &GPIO_InitStructure);

    // 配置 RX为浮空输入
    GPIO_InitStructure.GPIO_Mode = GPIO_Mode_IN_FLOATING;
    GPIO_InitStructure.GPIO_Pin = GPIO_Pin_10;
    GPIO_Init(GPIOA, &GPIO_InitStructure);

    // 配置 USART1
    USART_InitStructure.USART_BaudRate = 9600;
    USART_InitStructure.USART_HardwareFlowControl = USART_HardwareFlowControl_None;
    USART_InitStructure.USART_Mode = USART_Mode_Tx | USART_Mode_Rx;
    USART_InitStructure.USART_Parity = USART_Parity_No;
    USART_InitStructure.USART_StopBits = USART_StopBits_1;
    USART_InitStructure.USART_WordLength = USART_WordLength_8b;
    USART_Init(USART1, &USART_InitStructure);

    // 使能 USART1
    USART_Cmd(USART1, ENABLE);

    // 等待模块上电稳定，设置默认音量
    Delay_ms(500);
    JQ8400_SetVolume(20);
}

// 播放指定曲目
void JQ8400_Play(uint16_t file_index)
{
    uint8_t data[2];
    data[0] = (file_index >> 8) & 0xFF;
    data[1] = file_index & 0xFF;
    JQ8400_SendCmd(0x07, data, 2);
}

//停止播放
void JQ8400_Stop(void)
{
    JQ8400_SendCmd(0x04, 0, 0);
}

//设置音量
void JQ8400_SetVolume(uint8_t volume)
{
    uint8_t data[1];
    if (volume > 30) volume = 30;
    data[0] = volume;
    JQ8400_SendCmd(0x13, data, 1);
}
