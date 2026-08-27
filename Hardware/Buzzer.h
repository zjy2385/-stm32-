#ifndef __BUZZER_H
#define __BUZZER_H
void Alarm_Init(void);
void Alarm_Trigger(uint8_t enable);
void Alarm_Task(void);
#endif
