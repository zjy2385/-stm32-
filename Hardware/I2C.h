#ifndef __I2C_H
#define __I2C_h
void MyI2C_Init(void);
void MyI2C_Start(void);
void MyI2C_Stop(void);
void MyI2C_SendByte(uint8_t data);
uint8_t MyI2C_ReceiveByte(void);
uint8_t MyI2C_ReceiveAck(void);
void MyI2C_SendAck(uint8_t ack);
uint8_t MyI2C_WriteReg(uint8_t dev_addr, uint8_t addr, uint8_t data);
uint8_t MyI2C_ReadBytes(uint8_t dev_addr,
                        uint8_t addr,
                        uint8_t *data,
                        uint8_t length);
uint8_t  MyI2C_ReadReg(uint8_t dev_addr,uint8_t addr);
#endif
