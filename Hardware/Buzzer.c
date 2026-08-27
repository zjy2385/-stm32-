#include "stm32f10x.h"
#include "Delay.h"

#define ALARM_INTERVAL_MS   300
#define ALARM_PIN           GPIO_Pin_13
#define ALARM_LED_PIN       GPIO_Pin_14
#define ALARM_PORT          GPIOC

static uint8_t alarm_enabled = 0;
static uint8_t alarm_state = 0;
static uint16_t tick_counter = 0;

void Alarm_Init(void)
{
    GPIO_InitTypeDef gpio;

    RCC_APB2PeriphClockCmd(RCC_APB2Periph_GPIOC, ENABLE);

    // PC13（蜂鸣器 + 灯）
    gpio.GPIO_Mode = GPIO_Mode_Out_PP;
    gpio.GPIO_Pin = ALARM_PIN;
    gpio.GPIO_Speed = GPIO_Speed_50MHz;
    GPIO_Init(ALARM_PORT, &gpio);

    // PC14（额外报警灯）
    gpio.GPIO_Mode = GPIO_Mode_Out_PP;
    gpio.GPIO_Pin = ALARM_LED_PIN;
    gpio.GPIO_Speed = GPIO_Speed_50MHz;
    GPIO_Init(ALARM_PORT, &gpio);

    // 默认状态：PC13 灭/停，PC14 亮
    GPIO_ResetBits(ALARM_PORT, ALARM_PIN);
    GPIO_SetBits(ALARM_PORT, ALARM_LED_PIN);
    alarm_enabled = 0;
    alarm_state = 0;
    tick_counter = 0;
}

void Alarm_Trigger(uint8_t enable)
{
    alarm_enabled = enable;
    if (!enable) 
	{
        GPIO_ResetBits(ALARM_PORT, ALARM_PIN);
        GPIO_SetBits(ALARM_PORT, ALARM_LED_PIN);
        alarm_state = 0;
        tick_counter = 0;
    }
}

void Alarm_Task(void)
{
    if (!alarm_enabled)
	{
		return;
	}
	
    tick_counter++;
    if (tick_counter >= (ALARM_INTERVAL_MS / 10)) 
	{
        tick_counter = 0;
        
        alarm_state = !alarm_state;
        if (alarm_state) 
		{
            GPIO_SetBits(ALARM_PORT, ALARM_PIN);
            GPIO_SetBits(ALARM_PORT, ALARM_LED_PIN);
        } 
		else 
		{
            GPIO_ResetBits(ALARM_PORT, ALARM_PIN);
            GPIO_ResetBits(ALARM_PORT, ALARM_LED_PIN);
        }
    }
}

uint8_t Alarm_GetState(void)
{
    return alarm_enabled;
}
