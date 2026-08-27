#ifndef __OLED_PAGE_H
#define __OLED_PAGE_H

#include "stm32f10x.h"

#define MAX_MED_TIMES  3

typedef struct {
    const char* name;                    // 药品名称
    uint8_t hours[MAX_MED_TIMES];        // 小时数组
    uint8_t minutes[MAX_MED_TIMES];      // 分钟数组
    uint8_t time_count;                  // 实际时间个数
    uint8_t next_index;                  // 下次吃药时间索引
    uint16_t stock;                      // 库存（总片数）
    uint8_t dose;                        // 每次剂量（片数）
} MedParam_t;

extern MedParam_t g_med1;
extern MedParam_t g_med2;

void OLED_Page_Init(void);
void OLED_Page_Next(void);
void OLED_Page_Update(void);
uint8_t OLED_Page_GetCurrent(void);

// 药品管理函数
uint8_t Med_GetNextTime(MedParam_t *med, uint8_t *hour, uint8_t *minute);
void Med_ConfirmTaken(MedParam_t *med);
void Med_AutoAdvance(MedParam_t *med);
void Med_InitNextTime(MedParam_t *med);

#endif
