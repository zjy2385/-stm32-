#ifndef __MAX30102_APP_H
#define __MAX30102_APP_H

#include "stm32f10x.h"
#include <stdint.h>

// ========== 外部全局变量 ==========
extern uint8_t g_heart_rate_valid;
extern uint8_t g_spo2_valid;
extern float g_heart_rate;
extern float g_spo2;
extern uint16_t g_max30102_samples;

// ========== 公开函数 ==========
void MAX30102_App_Init(void);
void MAX30102_App_Process(void);
uint8_t MAX30102_App_IsReady(void);
uint8_t MAX30102_App_GetPartID(void);

#endif
