#include "stm32f10x.h"
#include "Delay.h"

static uint16_t key1_press_time = 0;
static uint8_t key1_long_pressed = 0;

void Key_Init(void)
{
    RCC_APB2PeriphClockCmd(RCC_APB2Periph_GPIOB, ENABLE);
    
    GPIO_InitTypeDef GPIO_InitStructure;
    GPIO_InitStructure.GPIO_Mode = GPIO_Mode_IPU;
    GPIO_InitStructure.GPIO_Pin = GPIO_Pin_12 | GPIO_Pin_13 | GPIO_Pin_15;
    GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;
    GPIO_Init(GPIOB, &GPIO_InitStructure);
}

uint8_t Key_GetNum(void)
{
    uint8_t KeyNum = 0;
    
    // ===== Key1 (PB12) =====
    if (GPIO_ReadInputDataBit(GPIOB, GPIO_Pin_12) == 0) {
        Delay_ms(20);
        if (GPIO_ReadInputDataBit(GPIOB, GPIO_Pin_12) == 1) return 0;
        
        // 开始计时
        key1_press_time = 0;
        while (GPIO_ReadInputDataBit(GPIOB, GPIO_Pin_12) == 0) {
            Delay_ms(10);
            key1_press_time++;
            if (key1_press_time >= 50) {  // 500ms 长按
                key1_long_pressed = 1;
                while (GPIO_ReadInputDataBit(GPIOB, GPIO_Pin_12) == 0);
                Delay_ms(20);
                return 4;  // 返回 4 表示长按
            }
        }
        // 短按
        Delay_ms(20);
        if (key1_long_pressed) {
            key1_long_pressed = 0;
            return 0;
        }
        KeyNum = 1;
    }
    
    // ===== Key2 (PB13) =====
    if (GPIO_ReadInputDataBit(GPIOB, GPIO_Pin_13) == 0) {
        Delay_ms(20);
        while (GPIO_ReadInputDataBit(GPIOB, GPIO_Pin_13) == 0);
        Delay_ms(20);
        KeyNum = 2;
    }
    
    // ===== Key3 (PB15) =====
    if (GPIO_ReadInputDataBit(GPIOB, GPIO_Pin_15) == 0) {
        Delay_ms(20);
        while (GPIO_ReadInputDataBit(GPIOB, GPIO_Pin_15) == 0);
        Delay_ms(20);
        KeyNum = 3;
    }
    
    return KeyNum;
}
