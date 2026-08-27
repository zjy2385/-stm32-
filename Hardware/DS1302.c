#include "stm32f10x.h"                  // Device header
#include "OLED.h"
#include "Delay.h"
//地址
#define DS1302_REG_SECONDS_W 0x80
#define DS1302_REG_SECONDS_R 0x81
#define DS1302_REG_MINUTES_W 0x82
#define DS1302_REG_MINUTES_R 0x83
#define DS1302_REG_HOURS_W   0x84
#define DS1302_REG_HOURS_R   0x85
#define DS1302_REG_WP_W      0x8E
//初始化引脚
#define DS1302_GPIO_PORT GPIOA
#define DS1302_GPIO_RCC  RCC_APB2Periph_GPIOA
#define DS1302_RST_PIN   GPIO_Pin_5
#define DS1302_IO_PIN    GPIO_Pin_6
#define DS1302_CLK_PIN   GPIO_Pin_7
//高低电平设置
#define DS1302_RST_H() GPIO_SetBits(DS1302_GPIO_PORT, DS1302_RST_PIN)
#define DS1302_RST_L() GPIO_ResetBits(DS1302_GPIO_PORT, DS1302_RST_PIN)
#define DS1302_CLK_H() GPIO_SetBits(DS1302_GPIO_PORT, DS1302_CLK_PIN)
#define DS1302_CLK_L() GPIO_ResetBits(DS1302_GPIO_PORT, DS1302_CLK_PIN)
#define DS1302_IO_H()  GPIO_SetBits(DS1302_GPIO_PORT, DS1302_IO_PIN)
#define DS1302_IO_L()  GPIO_ResetBits(DS1302_GPIO_PORT, DS1302_IO_PIN)
#define DS1302_IO_READ() GPIO_ReadInputDataBit(DS1302_GPIO_PORT, DS1302_IO_PIN)
//初始化
void DS1302_Init(void)
{
	RCC_APB2PeriphClockCmd(DS1302_GPIO_RCC, ENABLE);
	
	GPIO_InitTypeDef GPIO_Init_Structure;
	GPIO_Init_Structure.GPIO_Mode = GPIO_Mode_Out_PP;
	GPIO_Init_Structure.GPIO_Pin = DS1302_RST_PIN | DS1302_IO_PIN | DS1302_CLK_PIN;
	GPIO_Init_Structure.GPIO_Speed = GPIO_Speed_50MHz;
	GPIO_Init(GPIOA, &GPIO_Init_Structure);
	
	//DS1302 要求：
	DS1302_RST_L();		//CE 默认低电平（高电平才激活）
	DS1302_CLK_L();		//SCLK 默认低电平（空闲时低）
	DS1302_IO_L();		//IO 默认低电平（避免浮空）
	
}
//DS1302高位发送
//写入数据
static void DS1302_WriteByte(uint8_t data)
{
	uint8_t i;
	for (i = 0; i < 8; i ++)
	{
		if(data & (0x01 << i))
			DS1302_IO_H();
		else
			DS1302_IO_L();
		
		DS1302_CLK_L();
		DS1302_CLK_H();
	}
	DS1302_CLK_L();
}
static uint8_t DS1302_ReadByte(void)
{
	uint8_t i, data = 0;
	GPIO_InitTypeDef GPIO_Init_Structure;
	GPIO_Init_Structure.GPIO_Mode = GPIO_Mode_IN_FLOATING;
	GPIO_Init_Structure.GPIO_Pin = DS1302_IO_PIN;
	GPIO_Init_Structure.GPIO_Speed = GPIO_Speed_50MHz;
	GPIO_Init(GPIOA, &GPIO_Init_Structure);	
	
	for (i = 0; i< 8; i++)
	{
		DS1302_CLK_L();
		DS1302_CLK_H();
		if(DS1302_IO_READ())
		{
			data |=(0x01 << i);
		}
	}
	// 2. 切换回输出模式
	GPIO_Init_Structure.GPIO_Mode = GPIO_Mode_Out_PP;
	GPIO_Init_Structure.GPIO_Pin = DS1302_IO_PIN;
	GPIO_Init_Structure.GPIO_Speed = GPIO_Speed_50MHz;
	GPIO_Init(GPIOA, &GPIO_Init_Structure);		

    DS1302_CLK_L();  // 恢复空闲状态
    return data;
}
//写入到指定寄存器
static void DS1302_WriteReg(uint8_t addr, uint8_t data)
{
	DS1302_RST_H();
	DS1302_WriteByte(addr);
	DS1302_WriteByte(data);
	DS1302_RST_L();
}
//读取指定寄存器
static uint8_t DS1302_ReadReg(uint8_t addr)
{
	uint8_t data;
	DS1302_RST_H();
	DS1302_WriteByte(addr);
	data = DS1302_ReadByte();
	DS1302_RST_L();
	return data;
}
//bcd转换
static uint8_t DS1302_BCD_To_Dec(uint8_t bcd)
{
	uint8_t Dec;
	Dec = (bcd >> 4) * 10 + (bcd & 0x0F);
	return Dec;
}

static uint8_t DS1302_Dec_To_BCD(uint8_t dec)
{
    return ((dec / 10) << 4) | (dec % 10);
}

//读取时间
void DS1302_GetTime(uint8_t *hour, uint8_t *min)
{
    uint8_t bcd_min, bcd_hour;
    
//    bcd_sec  = DS1302_ReadReg(DS1302_REG_SECONDS_R);
    bcd_min  = DS1302_ReadReg(DS1302_REG_MINUTES_R);
    bcd_hour = DS1302_ReadReg(DS1302_REG_HOURS_R);
    
//    *sec  = DS1302_BCD_To_Dec(bcd_sec);
    *min  = DS1302_BCD_To_Dec(bcd_min);
    *hour = DS1302_BCD_To_Dec(bcd_hour);
}

void DS1302_SetTime(uint8_t hour, uint8_t min)
{	
	//关闭写保护
	DS1302_WriteReg(DS1302_REG_WP_W, 0x00);
	DS1302_WriteReg(DS1302_REG_HOURS_W, DS1302_Dec_To_BCD(hour));
	DS1302_WriteReg(DS1302_REG_MINUTES_W, DS1302_Dec_To_BCD(min));
	DS1302_WriteReg(DS1302_REG_SECONDS_W, 0x00);
	//打开写保护
	DS1302_WriteReg(DS1302_REG_WP_W, 0x80);
}
