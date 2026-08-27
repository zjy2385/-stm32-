#ifndef __MAX30102_H
#define __MAX30102_H
typedef struct
{
	uint32_t red;
	uint32_t ir;

} MAX30102_SAMPLE_T;
#define MAX30102_OK                0
#define MAX30102_ERROR             1
#define MAX30102_NO_DATA           2

uint8_t MAX30102_WriteReg(uint8_t reg, uint8_t data);
uint8_t MAX30102_ReadReg(uint8_t reg, uint8_t *data);
uint8_t MAX30102_Check(void);
uint8_t MAX30102_Reset(void);
uint8_t MAX30102_Init(void);
uint8_t MAX30102_ResetFIFO(void);
uint8_t MAX30102_GetFIFOCount(uint8_t *count);
uint8_t MAX30102_ReadFIFO(MAX30102_SAMPLE_T *sample);
#endif
