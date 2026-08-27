#ifndef __DS18B20_H
#define __DS18B20_H
void DS18B20_Init(void);
uint8_t DS18B20_Reset(void);
void DS18B20_WriteByte(uint8_t data);
uint8_t DS18B20_ReadByte(void);
uint8_t DS18B20_ReadTemperature(float *temperature);
#endif
