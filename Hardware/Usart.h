#ifndef __USART_H
#define __USART_H

#include "stm32f10x.h"

extern uint8_t Serial_RxFlag;
extern char Serial_RxPacket[100];

void Usart3_Init(uint32_t baud);
void Usart3_SendByte(uint8_t data);
void Usart3_SendString(char *str);
void ParseBluetoothCommand(char *cmd);

#endif
