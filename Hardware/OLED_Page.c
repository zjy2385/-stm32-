#include "stm32f10x.h"
#include "OLED.h"
#include "OLED_Page.h"
#include "Key.h"
#include "Delay.h"
#include <stdio.h>
#include <string.h>

#define PAGE_TOTAL  4

static uint8_t currentPage = 1;

// ========== 外部数据变量声明 ==========
extern uint8_t g_hour, g_minute;
extern float g_humidity, g_temperature;
extern float g_body_temperature;
extern float g_heart_rate;
extern float g_spo2;
extern uint8_t g_heart_rate_valid;
extern uint8_t g_spo2_valid;
extern uint16_t g_max30102_samples;

//药品参数定义 
MedParam_t g_med1 = {
    .name = "Nifedipine",
    .hours = {7, 12, 18},
    .minutes = {0, 0, 0},
    .time_count = 3,
    .next_index = 0,
    .stock = 60,
    .dose = 2
};

MedParam_t g_med2 = {
    .name = "Metformin",
    .hours = {8, 20},
    .minutes = {0, 0},
    .time_count = 2,
    .next_index = 0,
    .stock = 30,
    .dose = 1
};

// 初始化
void Med_InitNextTime(MedParam_t *med)
{
    uint8_t i;
    uint16_t current_total_minutes;
    uint16_t med_total_minutes;
    uint8_t found = 0;

    if (med->time_count == 0) return;

    current_total_minutes = g_hour * 60 + g_minute;

    for (i = 0; i < med->time_count; i++) {
        med_total_minutes = med->hours[i] * 60 + med->minutes[i];
        if (med_total_minutes > current_total_minutes) {
            med->next_index = i;
            found = 1;
            break;
        }
    }

    if (!found) {
        med->next_index = 0;
    }
}

// 获取下次吃药时间
uint8_t Med_GetNextTime(MedParam_t *med, uint8_t *hour, uint8_t *minute)
{
    if (med->time_count == 0 || med->next_index >= med->time_count) {
        return 0;
    }
    *hour = med->hours[med->next_index];
    *minute = med->minutes[med->next_index];
    return 1;
}

//确认已吃药
void Med_ConfirmTaken(MedParam_t *med)
{
    if (med->time_count > 0) {
        med->next_index++;
        if (med->next_index >= med->time_count) {
            med->next_index = 0;
        }
        if (med->stock >= med->dose) {
            med->stock -= med->dose;
        } else {
            med->stock = 0;
        }
    }
}

void Med_AutoAdvance(MedParam_t *med)
{
    uint8_t hour, minute;
    if (Med_GetNextTime(med, &hour, &minute)) {
        if (g_hour > hour || (g_hour == hour && g_minute > minute)) {
            med->next_index++;
            if (med->next_index >= med->time_count) {
                med->next_index = 0;
            }
        }
    }
}

// 各页面显示函数
static void Page1_Display(void)
{
    char buf[20];
    sprintf(buf, "Time:%02d:%02d", g_hour, g_minute);
    OLED_ShowString(1, 1, buf);
    sprintf(buf, "Humi:%.1f%%", g_humidity);
    OLED_ShowString(2, 1, buf);
    sprintf(buf, "Temp:%.1fC", g_temperature);
    OLED_ShowString(3, 1, buf);
    sprintf(buf, "Body:%.1fC", g_body_temperature);
    OLED_ShowString(4, 1, buf);
}

static void Page2_Display(void)
{
    char buf[20];
    sprintf(buf, "Time:%02d:%02d", g_hour, g_minute);
    OLED_ShowString(1, 1, buf);
    OLED_ShowString(2, 1, "HR:");
    OLED_ShowString(3, 1, "SPO2:");

    if (g_heart_rate_valid) {
        sprintf(buf, "%3.0f", g_heart_rate);
        OLED_ShowString(2, 4, buf);
    } else {
        OLED_ShowString(2, 4, "-- ");
    }

    if (g_spo2_valid) {
        sprintf(buf, "%3.0f%%", g_spo2);
        OLED_ShowString(3, 6, buf);
    } else {
        OLED_ShowString(3, 6, "-- ");
    }
    sprintf(buf, "S:%u", g_max30102_samples);
    OLED_ShowString(4, 1, buf);
}

static void Page3_Display(void)
{
    char buf[20];
    uint8_t next_hour, next_minute;

    OLED_ShowString(1, 1, "Med 1:");
    OLED_ShowString(1, 7, (char*)g_med1.name);

    if (Med_GetNextTime(&g_med1, &next_hour, &next_minute)) {
        sprintf(buf, "Next:%02d:%02d", next_hour, next_minute);
        OLED_ShowString(2, 1, buf);
    } else {
        OLED_ShowString(2, 1, "No time set");
    }

    sprintf(buf, "Dose:%u", g_med1.dose);
    OLED_ShowString(3, 1, buf);

    sprintf(buf, "Stock:%u", g_med1.stock);
    OLED_ShowString(4, 1, buf);
}

static void Page4_Display(void)
{
    char buf[20];
    uint8_t next_hour, next_minute;

    OLED_ShowString(1, 1, "Med 2:");
    OLED_ShowString(1, 7, (char*)g_med2.name);

    if (Med_GetNextTime(&g_med2, &next_hour, &next_minute)) {
        sprintf(buf, "Next:%02d:%02d", next_hour, next_minute);
        OLED_ShowString(2, 1, buf);
    } else {
        OLED_ShowString(2, 1, "No time set");
    }

    sprintf(buf, "Dose:%u", g_med2.dose);
    OLED_ShowString(3, 1, buf);

    sprintf(buf, "Stock:%u", g_med2.stock);
    OLED_ShowString(4, 1, buf);
}

// 显示指定页面
static void OLED_DisplayPage(uint8_t page)
{
    OLED_Clear();
    switch (page) {
        case 1: Page1_Display(); break;
        case 2: Page2_Display(); break;
        case 3: Page3_Display(); break;
        case 4: Page4_Display(); break;
        default: break;
    }
}

void OLED_Page_Init(void)
{
    OLED_Clear();
    OLED_ShowString(1, 1, "Hello!");
    OLED_ShowString(2, 1, "Welcome to");
    OLED_ShowString(3, 1, "STM32 System");
    Delay_ms(1000);
    
    currentPage = 1;
    OLED_DisplayPage(currentPage);
}

void OLED_Page_Next(void)
{
    currentPage++;
    if (currentPage > PAGE_TOTAL) {
        currentPage = 1;
    }
    OLED_DisplayPage(currentPage);
}

uint8_t OLED_Page_GetCurrent(void)
{
    return currentPage;
}

void OLED_Page_Update(void)
{
    switch (currentPage) {
        case 1: Page1_Display(); break;
        case 2: Page2_Display(); break;
        case 3: Page3_Display(); break;
        case 4: Page4_Display(); break;
        default: break;
    }
}
