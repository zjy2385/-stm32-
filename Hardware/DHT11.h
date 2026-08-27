#ifndef __DHT11_H
#define __DHT11_h
void DHT11_Init(void);
static void DHT11_Mode_IN(void);
static void DHT11_Mode_OUT(void);
static uint8_t DHT11_ReadByte(void);
uint8_t DHT11_ReadData(float *shidu, float *wendu);
#endif
