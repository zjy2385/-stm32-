#ifndef __DS1302_H
#define __DS1302_H
void DS1302_Init(void);
static void DS1302_WriteByte(uint8_t data);
static uint8_t DS1302_ReadByte(void);
static void DS1302_WriteReg(uint8_t addr, uint8_t data);
static uint8_t DS1302_ReadReg(uint8_t addr);
static uint8_t DS1302_BCD_To_Dec(uint8_t bcd);
void DS1302_GetTime(uint8_t *hour, uint8_t *min);
static uint8_t DS1302_Dec_To_BCD(uint8_t dec);
void DS1302_SetTime(uint8_t hour, uint8_t min);
void DS1302_SetTime(uint8_t hour, uint8_t minute);
#endif
