#include "stm32f10x.h"                  // Device header
#include "Delay.h"

#define I2C_INT_PIN     GPIO_Pin_6
#define I2C_SCL_PIN     GPIO_Pin_7
#define I2C_SDA_PIN     GPIO_Pin_8
#define I2C_PORT        GPIOB

#define SCL_High()      GPIO_SetBits(I2C_PORT, I2C_SCL_PIN)
#define SCL_Low()       GPIO_ResetBits(I2C_PORT, I2C_SCL_PIN)
#define SDA_High()      GPIO_SetBits(I2C_PORT, I2C_SDA_PIN)
#define SDA_Low()       GPIO_ResetBits(I2C_PORT, I2C_SDA_PIN)
#define SDA_Read()      GPIO_ReadInputDataBit(I2C_PORT, I2C_SDA_PIN)
//从机只能控制sda

//初始化
void MyI2C_Init(void)
{
	//i2c要求开漏输出
	RCC_APB2PeriphClockCmd(RCC_APB2Periph_GPIOB, ENABLE);
	GPIO_InitTypeDef GPIO_InitStructure;
	GPIO_InitStructure.GPIO_Mode = GPIO_Mode_Out_OD;
	GPIO_InitStructure.GPIO_Pin = I2C_SCL_PIN | I2C_SDA_PIN;
	GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;
	GPIO_Init(I2C_PORT, &GPIO_InitStructure);
	//i2c高电平释放总线（高电平空闲）
	SCL_High();
	SDA_High();
}
//开始
void MyI2C_Start(void)
{
	//开始条件，二者都在高电平的情况下，sda0到1为起始信号
	SDA_High();
	SCL_High();
	Delay_us(4);
	SDA_Low();
	Delay_us(4);
	//scl置0不接收数据，置1接收数据
	SCL_Low();
	Delay_us(4);
}
//结束
void MyI2C_Stop(void)
{
	//结束条件，在sda是低电平的情况下由0到1发生一次跳变
	SDA_Low();
	SCL_High();
	Delay_us(4);
	SDA_High();
	Delay_us(4);
}
//发送字节(取高位)
void MyI2C_SendByte(uint8_t data)
{
	uint8_t i;
	for(i = 0; i < 8;i ++)
	{
		SCL_Low();
		Delay_us(2);
		if(data & (0x80 >> i))
		{
			SDA_High();
		}
		else
		{
			SDA_Low();
		}
		Delay_us(2);
		SCL_High();
		Delay_us(4);
		SCL_Low();
		Delay_us(2);
	}
	SDA_High();
	Delay_us(2);
}
//接收数据
uint8_t MyI2C_ReceiveByte(void)
{
	uint8_t i, data = 0;
	//释放总线，让从机控制(从机只占数据线)
	SDA_High();
	
	for (i = 0;i < 8;i ++)
	{
		SCL_High();
		Delay_us(4);
		if(SDA_Read())
		{
			data |= (0x80 >> i);
		}
		SCL_Low();
		Delay_us(2);
	}
	return data;
}
//接收应答
uint8_t MyI2C_ReceiveAck(void)
{
	uint8_t ack;
	//释放sda，等待从机回复
	SDA_High();
	//接收应答，ack：0有，1没有
	SCL_High();
	Delay_us(2);
	ack =SDA_Read();
	//接收完成后scl置低电平，等待接收新的数据
	SCL_Low();
	Delay_us(4);
	
	return ack;
}
//发送应答
void MyI2C_SendAck(uint8_t ack)
{	
	//scl为低电平，准备发送应答
	SCL_Low();
	Delay_us(2);
	// ACK 为低电平，NACK 为高电平
	if(ack == 0)
	{
		SDA_Low();
	}
	else
	{
		SDA_High();
	}
	//从机读取应答
	SCL_High();
	Delay_us(4);
	//结束应答并释放sda发送下组数据
	SCL_Low();
	SDA_High();
	Delay_us(2);
}
//指定位置写
uint8_t MyI2C_WriteReg(uint8_t dev_addr, uint8_t addr, uint8_t data)
{
	uint8_t ack;
	MyI2C_Start();
	//7位设备地址+1位读写位，0写1读
	MyI2C_SendByte((dev_addr << 1) | 0x00);
	//从机返回应答
	ack = MyI2C_ReceiveAck();
	if (ack) 
	{ 
		MyI2C_Stop(); 
		return 1; 
	}
	//发送寄存器地址
	MyI2C_SendByte(addr);
	ack = MyI2C_ReceiveAck();
	if (ack) 
	{ 
		MyI2C_Stop(); 
		return 1; 
	}
	//发送数据
	MyI2C_SendByte(data);
	ack = MyI2C_ReceiveAck();
	if (ack) 
	{ 
		MyI2C_Stop(); 
		return 1; 
	}
	MyI2C_Stop(); 
	return 0;
}
//连续读取多个字节
uint8_t MyI2C_ReadBytes(uint8_t dev_addr,
                        uint8_t addr,
                        uint8_t *data,
                        uint8_t length)
{
	uint8_t i, ack;
	if (data == 0 || length == 0)
	{
		return 1;
	}
	//第一步:发送设备地址和寄存器地址
	MyI2C_Start();

	MyI2C_SendByte((dev_addr << 1) | 0x00);
	ack = MyI2C_ReceiveAck();

	if (ack != 0)
	{
		MyI2C_Stop();
		return 1;
	}

	MyI2C_SendByte(addr);
	ack = MyI2C_ReceiveAck();

	if (ack != 0)
	{
		MyI2C_Stop();
		return 1;
	}

	//第二步：重复起始，切换到读模式
	MyI2C_Start();

	MyI2C_SendByte((dev_addr << 1) | 0x01);
	ack = MyI2C_ReceiveAck();

	if (ack != 0)
	{
		MyI2C_Stop();
		return 1;
	}
	// 第三步：连续接收数据
	for (i = 0; i < length; i++)
	{
		data[i] = MyI2C_ReceiveByte();

		//最后一个字节发送NACK，前面的字节发送ACK
		if (i == length - 1)
		{
			MyI2C_SendAck(1);
		}
		else
		{
			MyI2C_SendAck(0);
		}
	}
	MyI2C_Stop();
	return 0;
}
//指定读数据(先指定位置写找到位置)
uint8_t  MyI2C_ReadReg(uint8_t dev_addr,uint8_t addr)
{
	uint8_t data;

	if (MyI2C_ReadBytes(dev_addr, addr, &data, 1) != 0)
	{
		return 0xFF;
	}

	return data;
}
