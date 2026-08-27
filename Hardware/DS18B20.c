#include "stm32f10x.h"                  // Device header
#include "Delay.h"
#include "OLED.h"
#include <string.h>
#include <stdio.h>

uint8_t DS18B20_Reset(void);
void DS18B20_WriteByte(uint8_t data);
uint8_t DS18B20_ReadByte(void);

#define DS18B20_PORT     GPIOB
#define DS18B20_PIN      GPIO_Pin_2

#define DS18B20_DATA_H()   GPIO_SetBits(DS18B20_PORT, DS18B20_PIN)
#define DS18B20_DATA_L()   GPIO_ResetBits(DS18B20_PORT, DS18B20_PIN)
#define DS18B20_DATA_READ() GPIO_ReadInputDataBit(DS18B20_PORT, DS18B20_PIN)

static void DS18B20_ConfigResolution(void)
{
    // 写入配置寄存器
    DS18B20_Reset();
    DS18B20_WriteByte(0xCC);   // 跳过 ROM
    DS18B20_WriteByte(0x4E);   // 写暂存器命令
    DS18B20_WriteByte(0x00);   // TH（高温阈值，不用管）
    DS18B20_WriteByte(0x00);   // TL（低温阈值，不用管）
    DS18B20_WriteByte(0x7F);   // 配置寄存器：0x1F = 9位，0x3F = 10位，0x5F = 11位，0x7F = 12位
    Delay_ms(10);
}

//输出模式
static void DS18B20_Mode_Out(void)
{
	GPIO_InitTypeDef GPIO_InitStructure;
	GPIO_InitStructure.GPIO_Mode = GPIO_Mode_Out_PP;
	GPIO_InitStructure.GPIO_Pin = DS18B20_PIN;
	GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;
	GPIO_Init(DS18B20_PORT, &GPIO_InitStructure);
}
//输入模式
static void DS18B20_Mode_In(void)
{
	GPIO_InitTypeDef GPIO_InitStructure;
	GPIO_InitStructure.GPIO_Mode = GPIO_Mode_IPU;
	GPIO_InitStructure.GPIO_Pin = DS18B20_PIN;
	GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;
	GPIO_Init(DS18B20_PORT, &GPIO_InitStructure);
}


//引脚初始化，高电平空闲
void DS18B20_Init(void)
{
	RCC_APB2PeriphClockCmd(RCC_APB2Periph_GPIOA, ENABLE);
	DS18B20_Mode_Out();
	DS18B20_DATA_H();

    DS18B20_ConfigResolution();
}

//初始化配置，建立连接
uint8_t DS18B20_Reset(void)
{
	uint8_t p;
	//建立连接，主机拉低480-960us想ds18b20发送连接信号
	DS18B20_Mode_Out();
	DS18B20_DATA_L();
	Delay_ms(1);
	//释放总线，等待ds18b20的回答，等待15-60
	DS18B20_DATA_H();
	Delay_us(60);
	//切换为输入模式，接收ds18b20的应答信号
	DS18B20_Mode_In();
	p = DS18B20_DATA_READ();
	Delay_ms(1);		//最少480
	
	return p;  //0存在，1不存在
}
//写(先发低位)
void DS18B20_WriteByte(uint8_t data)
{
	uint8_t i;
	DS18B20_Mode_Out();
	//写1先拉低总线1-15，然后拉高至少60
	//写0拉低总线，并持续最少60，在拉高
	for(i = 0; i < 8; i ++)
	{
		DS18B20_DATA_L();
		Delay_us(2);
		
		if(data & (0x01 << i))//写1
		{
			DS18B20_DATA_H();
		}
		else				  //写0
		{
			DS18B20_DATA_L();
		}
		Delay_us(60);
		
		DS18B20_DATA_H();
		Delay_us(2);
	}
}
//读
uint8_t DS18B20_ReadByte(void)
{
	uint8_t i, data =0;
	DS18B20_Mode_Out();
	
	for(i = 0; i < 8; i ++)
	{
		//主机发送一个至少1μs的低电平信号通知从机我要开始读数据了
		//读时隙开始
		//所有读时隙必须至少需要60us，且在两次独立的时隙之间至少需要1us的恢复时间。
        DS18B20_DATA_L();
        Delay_us(2);
        
        // 释放总线，切换为输入
        DS18B20_DATA_H();
        DS18B20_Mode_In();
        Delay_us(2);
        
        // 读取数据位
        if (DS18B20_DATA_READ())
        {
            data |= (0x01 << i);
        }
        
        // 等待 60μs
        Delay_us(60);
		DS18B20_Mode_Out();
		DS18B20_DATA_H();
		Delay_us(2);
	}

	return data;
}
//读取温度
uint8_t DS18B20_ReadTemperature(float *temperature)
{
    uint8_t temp_low, temp_high;
    int16_t temp_raw;
    
    //第一次复位，通知ds18b20要接收数据
    if (DS18B20_Reset() != 0)
    {
        return 0;  // 没有检测到 DS18B20
    }
    // 2. 跳过 ROM（0xCC）
    DS18B20_WriteByte(0xCC);
    
    // 3. 开始温度转换（0x44）
    DS18B20_WriteByte(0x44);
    
    // 4. 等待转换完成
    Delay_ms(750);
    
    // 5. 再次复位，告诉ds18b20,可以吧数据发送给我了
    if (DS18B20_Reset() != 0)
    {
        return 0;
    }
    
    // 6. 跳过 ROM
    DS18B20_WriteByte(0xCC);
    
    // 7. 读取暂存器（0xBE）
    DS18B20_WriteByte(0xBE);
    
    // 8. 读取温度值（低字节 + 高字节）
    temp_low = DS18B20_ReadByte();
    temp_high = DS18B20_ReadByte();
    // 9.读取并丢弃剩余的 7 个字节，ds18b20一次发送9个字节，只要前两个字节，后面的不需要
    for (int i = 0; i < 7; i++)
    {
        DS18B20_ReadByte();
    }
    
    // 10. 合成 16 位原始值
    temp_raw = (int16_t)((temp_high << 8) | temp_low);
    
    // 11. 转换为摄氏温度
    *temperature = temp_raw * 0.0625f;
		return 1;
}


