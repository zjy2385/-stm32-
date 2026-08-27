#include "stm32f10x.h"
#include "Delay.h"
#include "OLED.h"
#include "LED.h"
#include "Key.h"
#include "Usart.h"
#include "OLED_Page.h"
#include "DS1302.h"
#include "DHT11.h"
#include "DS18B20.h"
#include "I2C.h"
#include "MAX30102_App.h"
#include "Buzzer.h"
#include "JQ8400.h"
#include <string.h>
#include <stdio.h>

uint8_t g_hour = 0;
uint8_t g_minute = 0;
float g_humidity = 0.0f;
float g_temperature = 0.0f;
float g_body_temperature = 0.0f;

extern uint8_t g_heart_rate_valid;
extern uint8_t g_spo2_valid;
extern float g_heart_rate;
extern float g_spo2;

extern MedParam_t g_med1;
extern MedParam_t g_med2;

#define MED_COUNT  2
static uint8_t med_reminded[MED_COUNT] = {0, 0};
static uint8_t med_waiting_confirm[MED_COUNT] = {0, 0};

static uint8_t time_set_mode = 0;        
static uint8_t time_set_hour = 0;
static uint8_t time_set_minute = 0;
static uint8_t time_set_edit = 1;        

//用药提醒检测
static void CheckMedReminder(void)
{
    uint8_t next_hour, next_minute;
    uint8_t i;

    for (i = 0; i < MED_COUNT; i++) 
	{
        MedParam_t *med = (i == 0) ? &g_med1 : &g_med2;
        
        if (!Med_GetNextTime(med, &next_hour, &next_minute)) 
		{
            continue;
        }

        if (g_hour == next_hour && g_minute == next_minute && med_reminded[i] == 0) 
		{
            JQ8400_Play(1);
            med_reminded[i] = 1;
            med_waiting_confirm[i] = 1;
            
            Alarm_Trigger(1);
            Delay_ms(200);
            Alarm_Trigger(0);
        }
        
        if (g_minute != next_minute) 
		{
            med_reminded[i] = 0;
        }
    }
}

//吃药提醒蜂鸣器
static void MedRemindBuzzerTask(void)
{
    uint8_t i;
    static uint8_t buzzer_state[MED_COUNT] = {0, 0};
    static uint8_t buzzer_tick[MED_COUNT] = {0, 0};
    
    for (i = 0; i < MED_COUNT; i++) 
	{
        if (med_waiting_confirm[i]) 
		{
            buzzer_tick[i]++;
            if (buzzer_tick[i] >= 50) 
			{
                buzzer_tick[i] = 0;
                buzzer_state[i] = !buzzer_state[i];
                if (buzzer_state[i]) 
				{
                    Alarm_Trigger(1);
                } 
				else 
				{
                    Alarm_Trigger(0);
                }
            }
        } 
		else 
		{
            buzzer_tick[i] = 0;
            buzzer_state[i] = 0;
        }
    }
}

// 按键确认吃药
static void HandleMedConfirm(void)
{
    uint8_t i;
    
    for (i = 0; i < MED_COUNT; i++) 
	{
        MedParam_t *med = (i == 0) ? &g_med1 : &g_med2;
        
        if (med_waiting_confirm[i]) 
		{
            Alarm_Trigger(0);
            Med_ConfirmTaken(med);
            med_waiting_confirm[i] = 0;
            JQ8400_Stop();
            OLED_Page_Update();
        }
    }
}

// 时间设置 
static void TimeSetDisplay(void)
{
    char buf[20];
    
    OLED_ShowString(1, 1, "Time Setting    ");
    OLED_ShowString(2, 1, "                ");
    OLED_ShowString(3, 1, "                ");
    
    if (time_set_edit == 1) 
	{
        sprintf(buf, "Hour: %02d  <--", time_set_hour);
    } 
	else 
	{
        sprintf(buf, "Hour: %02d", time_set_hour);
    }
    OLED_ShowString(2, 1, buf);
    
    if (time_set_edit == 2) 
	{
        sprintf(buf, "Min : %02d  <--", time_set_minute);
    } 
	else 
	{
        sprintf(buf, "Min : %02d", time_set_minute);
    }
    OLED_ShowString(3, 1, buf);
}

//进入时间设置
static void EnterTimeSetting(void)
{
    time_set_mode = 1;
    time_set_hour = g_hour;
    time_set_minute = g_minute;
    time_set_edit = 1;
    OLED_Clear();
    TimeSetDisplay();
}

// 退出时间设置
static void ExitTimeSetting(void)
{
    DS1302_SetTime(time_set_hour, time_set_minute);
    time_set_mode = 0;
    OLED_Page_Init();
}

//主函数
int main(void)
{
    uint8_t key;
    
    OLED_Init();
    LED_Init();
    Key_Init();
    DS1302_Init();
    Usart3_Init(9600);
    DHT11_Init();
    DS18B20_Init();
    MyI2C_Init();      
    MAX30102_App_Init();
    Alarm_Init();
    JQ8400_Init();          

    DS1302_GetTime(&g_hour, &g_minute);
    Med_InitNextTime(&g_med1);
    Med_InitNextTime(&g_med2);
    OLED_Page_Init();

    while (1)
    {
        static uint16_t sensor_tick = 0;

        // 正常模式
        if (time_set_mode == 0) 
		{
            DS1302_GetTime(&g_hour, &g_minute);
            DHT11_ReadData(&g_humidity, &g_temperature);

            if (++sensor_tick >= 50) 
			{
                sensor_tick = 0;
                DS18B20_ReadTemperature(&g_body_temperature);
            }

            MAX30102_App_Process();
            CheckMedReminder();
            MedRemindBuzzerTask();
			//阈值
            if ((g_body_temperature > 30.0f && g_body_temperature < 42.0f && g_body_temperature > 37.5f) ||
                (g_heart_rate_valid && (g_heart_rate > 100.0f || g_heart_rate < 50.0f)) ||
                (g_spo2_valid && g_spo2 < 90.0f))
            {
                // 报警逻辑由 Alarm_Task 处理
            }

            Alarm_Task();
            OLED_Page_Update();
        }

        //蓝牙指令处理
        if (Serial_RxFlag == 1) 
		{
            Serial_RxFlag = 0;
            ParseBluetoothCommand(Serial_RxPacket);
        }

        //按键处理
        key = Key_GetNum();

        if (key == 4) 
		{
            if (time_set_mode == 0) 
			{
                EnterTimeSetting();
            } 
			else 
			{
                ExitTimeSetting();
            }
            continue;
        }
        else if (key == 1) 
		{
            if (time_set_mode == 0) 
			{
                OLED_Page_Next();
            } 
			else 
			{
                if (time_set_edit == 1) 
				{
                    time_set_edit = 2;
                } 
				else 
				{
                    time_set_edit = 1;
                }
                TimeSetDisplay();
            }
        }
        else if (key == 2) 
		{
            if (time_set_mode == 0) 
			{
                HandleMedConfirm();
            }
			else 
			{
                if (time_set_edit == 1) 
				{
                    time_set_hour++;
                    if (time_set_hour >= 24) time_set_hour = 0;
                }
			else 
				{
                    time_set_minute++;
                    if (time_set_minute >= 60) time_set_minute = 0;
                }
                TimeSetDisplay();
            }
        }
        else if (key == 3) 
		{
            if (time_set_mode == 0) 
			{
                HandleMedConfirm();
            } 
			else 
			{
                if (time_set_edit == 1) 
				{
                    if (time_set_hour == 0) time_set_hour = 23;
                    else time_set_hour--;
                } 
				else 
				{
                    if (time_set_minute == 0) time_set_minute = 59;
                    else time_set_minute--;
                }
                TimeSetDisplay();
            }
        }

        Delay_ms(10);
    }
}
