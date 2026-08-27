#include "stm32f10x.h"                  // Device header
#include "Delay.h"

#define DHT11_PORT GPIOA
#define DHT11_PIN  GPIO_Pin_0

#define DHT11_DATA_H()   GPIO_SetBits(DHT11_PORT, DHT11_PIN)
#define DHT11_DATA_L()   GPIO_ResetBits(DHT11_PORT, DHT11_PIN)
#define DHT11_DATA_READ() GPIO_ReadInputDataBit(DHT11_PORT, DHT11_PIN)

static void DHT11_Mode_IN(void);
static void DHT11_Mode_OUT(void);
static uint8_t DHT11_ReadByte(void);
//dht11DHT11 一次完整的通信会发送 40 位（5 个字节）
//湿度整数，湿度小数，温度整数，温度小数，校验和
//dht11为半双工通信
void DHT11_Init(void)
{
    RCC_APB2PeriphClockCmd(RCC_APB2Periph_GPIOA, ENABLE);
    DHT11_Mode_OUT();
    DHT11_DATA_H();
}
//输入模式
static void DHT11_Mode_IN(void)
{
    GPIO_InitTypeDef gpio;
    gpio.GPIO_Mode = GPIO_Mode_IPU;
    gpio.GPIO_Pin = DHT11_PIN;
    gpio.GPIO_Speed = GPIO_Speed_50MHz;
    GPIO_Init(DHT11_PORT, &gpio);
}
//输出模式
static void DHT11_Mode_OUT(void)
{
    GPIO_InitTypeDef gpio;
    gpio.GPIO_Mode = GPIO_Mode_Out_PP;
    gpio.GPIO_Pin = DHT11_PIN;
    gpio.GPIO_Speed = GPIO_Speed_50MHz;
    GPIO_Init(DHT11_PORT, &gpio);
}
//先读字节后才能读数据
//dht11是通过判断高电平的持续时间来判断0，1
static uint8_t DHT11_ReadByte(void)
{
	uint8_t i, data = 0;
	for (i = 0; i < 8;i ++)
	{
		//等待dht11发送数据
		while (DHT11_DATA_READ() == 0);
		Delay_us(40);
		//接收数据，接收一次数据，dht11要跳变一次高低电平
		if (DHT11_DATA_READ() == 1)
		{
			data |=(0x80 >> i);
			
			while (DHT11_DATA_READ() == 1);
		}
	}
	return data;
}
//接收dht11的数据
uint8_t DHT11_ReadData(float *shidu, float *wendu)
{
	uint8_t buf[5];
	//发起通信，等待dht11的响应
	//拉低电平至少18ms,然后拉高电平20-40us
	DHT11_Mode_OUT();
	DHT11_DATA_L();
	Delay_ms(20);
	DHT11_DATA_H();
	Delay_us(30);
	//切换会输入模式，开始接收dht11的信号
	DHT11_Mode_IN();
	//dht11发送一个010来响应从机的信号
	if(DHT11_DATA_READ() == 0)
	{
		while (DHT11_DATA_READ() == 0);
		while (DHT11_DATA_READ() == 1);
		//此时dht11响应接收，主机开始接收数据
		for(uint8_t i = 0; i < 5;i++)
		{
			buf[i] = DHT11_ReadByte();
		}
		
		if(buf[0] + buf[1] + buf[2] + buf[3] == buf[4] )
		{
			*shidu = (float)buf[0] + (float)buf[1] / 10.0f;
			*wendu = (float)buf[2] + (float)buf[3] / 10.0f;
			return 1;
		}
	}
	return 0;
}


