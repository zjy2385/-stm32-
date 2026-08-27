#include "stm32f10x.h"
#include "Usart.h"
#include "OLED_Page.h"
#include "DS1302.h"
#include <string.h>
#include <stdio.h>

uint8_t Serial_RxFlag;
char Serial_RxPacket[100];

// 外部变量引用
extern float g_humidity, g_temperature, g_body_temperature;
extern float g_heart_rate, g_spo2;
extern MedParam_t g_med1, g_med2;
extern uint8_t g_hour, g_minute;

//蓝牙指令
void ParseBluetoothCommand(char *cmd)
{
    char buf[100];
    uint8_t hour, minute;
    uint8_t h1, m1, h2, m2, h3, m3, dose, stock;
    uint8_t h1_2, m1_2, h2_2, m2_2, dose2, stock2;

    //1. 查询所有数据
    if (strcmp(cmd, "DATA") == 0) 
	{
        sprintf(buf, "Time:%02d:%02d\r\n", g_hour, g_minute);
        Usart3_SendString(buf);
        sprintf(buf, "Humi:%.1f%% Temp:%.1fC Body:%.1fC\r\n", 
                g_humidity, g_temperature, g_body_temperature);
        Usart3_SendString(buf);
        sprintf(buf, "HR:%.0f SPO2:%.0f%%\r\n", g_heart_rate, g_spo2);
        Usart3_SendString(buf);
        
        uint8_t h, m;
        Med_GetNextTime(&g_med1, &h, &m);
        sprintf(buf, "Med1:%s Next:%02d:%02d Dose:%d Stock:%d\r\n", 
                g_med1.name, h, m, g_med1.dose, g_med1.stock);
        Usart3_SendString(buf);
        
        Med_GetNextTime(&g_med2, &h, &m);
        sprintf(buf, "Med2:%s Next:%02d:%02d Dose:%d Stock:%d\r\n", 
                g_med2.name, h, m, g_med2.dose, g_med2.stock);
        Usart3_SendString(buf);
        return;
    }

    //2. 设置时间：@TIME,14,30 
    if (strncmp(cmd, "TIME,", 5) == 0) 
	{
        if (sscanf(cmd + 5, "%hhu,%hhu", &hour, &minute) == 2) 
		{
            if (hour < 24 && minute < 60) 
			{
                DS1302_SetTime(hour, minute);
                sprintf(buf, "Time set to %02d:%02d\r\n", hour, minute);
                Usart3_SendString(buf);
                g_hour = hour;
                g_minute = minute;
            } 
			else 
			{
                Usart3_SendString("Invalid time\r\n");
            }
        } 
		else 
		{
            Usart3_SendString("Format: @TIME,HH,MM\r\n");
        }
        return;
    }

    //3. 设置药品1：@MED1,7,30,12,0,18,30,2,60 
    if (strncmp(cmd, "MED1,", 5) == 0) 
	{
        if (sscanf(cmd + 5, "%hhu,%hhu,%hhu,%hhu,%hhu,%hhu,%hhu,%hhu", 
                   &h1, &m1, &h2, &m2, &h3, &m3, &dose, &stock) == 8) 
		{
            if (h1 < 24 && m1 < 60 && h2 < 24 && m2 < 60 && 
                h3 < 24 && m3 < 60 && dose > 0 && stock > 0) 
			{
                g_med1.hours[0] = h1;
                g_med1.minutes[0] = m1;
                g_med1.hours[1] = h2;
                g_med1.minutes[1] = m2;
                g_med1.hours[2] = h3;
                g_med1.minutes[2] = m3;
                g_med1.time_count = 3;
                g_med1.dose = dose;
                g_med1.stock = stock;
                Med_InitNextTime(&g_med1);
                Usart3_SendString("Med1 updated\r\n");
            } 
			else 
			{
                Usart3_SendString("Invalid Med1 params\r\n");
            }
        } 
		else 
		{
            Usart3_SendString("Format: @MED1,H1,M1,H2,M2,H3,M3,Dose,Stock\r\n");
        }
        return;
    }

    //4. 设置药品2：@MED2,8,30,20,30,1,30 
    if (strncmp(cmd, "MED2,", 5) == 0) 
	{
        if (sscanf(cmd + 5, "%hhu,%hhu,%hhu,%hhu,%hhu,%hhu", 
                   &h1_2, &m1_2, &h2_2, &m2_2, &dose2, &stock2) == 6) 
		{
            if (h1_2 < 24 && m1_2 < 60 && h2_2 < 24 && m2_2 < 60 && dose2 > 0 && stock2 > 0) 
			{
                g_med2.hours[0] = h1_2;
                g_med2.minutes[0] = m1_2;
                g_med2.hours[1] = h2_2;
                g_med2.minutes[1] = m2_2;
                g_med2.time_count = 2;
                g_med2.dose = dose2;
                g_med2.stock = stock2;
                Med_InitNextTime(&g_med2);
                Usart3_SendString("Med2 updated\r\n");
            } 
			else 
			{
                Usart3_SendString("Invalid Med2 params\r\n");
            }
        } 
		else 
		{
            Usart3_SendString("Format: @MED2,H1,M1,H2,M2,Dose,Stock\r\n");
        }
        return;
    }

    //5. 帮助 
    if (strcmp(cmd, "HELP") == 0) 
	{
        Usart3_SendString("Commands:\r\n");
        Usart3_SendString("@DATA - Show all data\r\n");
        Usart3_SendString("@TIME,HH,MM - Set time\r\n");
        Usart3_SendString("@MED1,H1,M1,H2,M2,H3,M3,Dose,Stock - Set Med1\r\n");
        Usart3_SendString("@MED2,H1,M1,H2,M2,Dose,Stock - Set Med2\r\n");
        Usart3_SendString("@HELP - Show this\r\n");
        return;
    }

    // 未知指令
    Usart3_SendString("Unknown command. Send @HELP\r\n");
}

//蓝牙通信协议
void Usart3_Init(uint32_t baud)
{
    RCC_APB1PeriphClockCmd(RCC_APB1Periph_USART3, ENABLE);
    RCC_APB2PeriphClockCmd(RCC_APB2Periph_GPIOB, ENABLE);
    
    GPIO_InitTypeDef GPIO_InitStructure;
    GPIO_InitStructure.GPIO_Mode = GPIO_Mode_AF_PP;
    GPIO_InitStructure.GPIO_Pin = GPIO_Pin_10;
    GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;
    GPIO_Init(GPIOB, &GPIO_InitStructure);
    
    GPIO_InitStructure.GPIO_Mode = GPIO_Mode_IN_FLOATING;
    GPIO_InitStructure.GPIO_Pin = GPIO_Pin_11;
    GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;
    GPIO_Init(GPIOB, &GPIO_InitStructure);
    
    USART_InitTypeDef Usart_InitStructure;
    Usart_InitStructure.USART_BaudRate = baud;
    Usart_InitStructure.USART_HardwareFlowControl = USART_HardwareFlowControl_None;
    Usart_InitStructure.USART_Mode = USART_Mode_Rx | USART_Mode_Tx;
    Usart_InitStructure.USART_Parity = USART_Parity_No;
    Usart_InitStructure.USART_StopBits = USART_StopBits_1;
    Usart_InitStructure.USART_WordLength = USART_WordLength_8b;
    USART_Init(USART3, &Usart_InitStructure);
    
    USART_ITConfig(USART3, USART_IT_RXNE, ENABLE);
    
    NVIC_PriorityGroupConfig(NVIC_PriorityGroup_2);
    NVIC_InitTypeDef NVIC_InitStructure;
    NVIC_InitStructure.NVIC_IRQChannel = USART3_IRQn;
    NVIC_InitStructure.NVIC_IRQChannelCmd = ENABLE;
    NVIC_InitStructure.NVIC_IRQChannelPreemptionPriority = 0;
    NVIC_InitStructure.NVIC_IRQChannelSubPriority = 1;
    NVIC_Init(&NVIC_InitStructure);
    
    USART_Cmd(USART3, ENABLE);
}

//发送字节
void Usart3_SendByte(uint8_t data)
{
    while (USART_GetFlagStatus(USART3, USART_FLAG_TXE) == RESET);
    USART_SendData(USART3, data);
}

//发送字符串
void Usart3_SendString(char *str)
{
    while (*str) 
	{
        Usart3_SendByte(*str++);
    }
}

//中断服务函数 
void USART3_IRQHandler(void)
{
    static uint8_t RxState = 0;
    static uint8_t pRxPacket = 0;
    
    if (USART_GetITStatus(USART3, USART_IT_RXNE) == SET) 
	{
        uint8_t RxData = USART_ReceiveData(USART3);
        
        if (RxState == 0) 
		{
            if (RxData == '@' && Serial_RxFlag == 0) 
			{
                RxState = 1;
                pRxPacket = 0;
            }
        } 
		else if (RxState == 1) 
		{
            if (RxData == '\r') 
			{
                RxState = 2;
            } 
			else 
			{
                Serial_RxPacket[pRxPacket] = RxData;
                pRxPacket++;
            }
        } 
		else if (RxState == 2) 
		{
            if (RxData == '\n') 
			{
                RxState = 0;
                Serial_RxPacket[pRxPacket] = '\0';
                Serial_RxFlag = 1;
            }
        }
        USART_ClearFlag(USART3, USART_IT_RXNE);
    }
}
