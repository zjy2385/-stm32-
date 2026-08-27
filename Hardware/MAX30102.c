#include "stm32f10x.h"                  // Device header
#include "Delay.h"
#include "I2C.h"

/* MAX30102 的 7 位 I2C 地址 */
#define MAX30102_I2C_ADDRESS       0x57

/* 返回值 */
#define MAX30102_OK                0
#define MAX30102_ERROR             1
#define MAX30102_NO_DATA           2

/* MAX30102 的寄存器地址 */
#define MAX30102_REG_INTR_STATUS_1    0x00
#define MAX30102_REG_INTR_STATUS_2    0x01
#define MAX30102_REG_INTR_ENABLE_1    0x02
#define MAX30102_REG_INTR_ENABLE_2    0x03

#define MAX30102_REG_FIFO_WR_PTR      0x04
#define MAX30102_REG_OVF_COUNTER      0x05
#define MAX30102_REG_FIFO_RD_PTR      0x06
#define MAX30102_REG_FIFO_DATA        0x07
#define MAX30102_REG_FIFO_CONFIG       0x08

#define MAX30102_REG_MODE_CONFIG       0x09
#define MAX30102_REG_SPO2_CONFIG       0x0A

#define MAX30102_REG_LED1_PA           0x0C
#define MAX30102_REG_LED2_PA           0x0D

#define MAX30102_REG_TEMP_INTEGER      0x1F
#define MAX30102_REG_TEMP_FRACTION     0x20
#define MAX30102_REG_TEMP_CONFIG       0x21

#define MAX30102_REG_REV_ID            0xFE
#define MAX30102_REG_PART_ID           0xFF

/* MAX30102 的 PART_ID 固定值 */
#define MAX30102_PART_ID_VALUE         0x15

/* FIFO 深度为 32 个样本 */
#define MAX30102_FIFO_DEPTH            32

typedef struct
{
	uint32_t red;
	uint32_t ir;

} MAX30102_SAMPLE_T;
/*
 * 函数功能：向 MAX30102 写一个寄存器
 */
uint8_t MAX30102_WriteReg(uint8_t reg, uint8_t data)
{
	return MyI2C_WriteReg(MAX30102_I2C_ADDRESS, reg, data);
}

/*
 * 函数功能：从 MAX30102 读取一个寄存器
 */
uint8_t MAX30102_ReadReg(uint8_t reg, uint8_t *data)
{
	if (data == 0)
	{
		return MAX30102_ERROR;
	}

	return MyI2C_ReadBytes(MAX30102_I2C_ADDRESS,
	                       reg,
	                       data,
	                       1);
}

/*
 * 函数功能：检测 MAX30102 是否在线
 *
 * 正常情况下 PART_ID 寄存器应该读到 0x15。
 */
uint8_t MAX30102_Check(void)
{
	uint8_t part_id;

	if (MAX30102_ReadReg(MAX30102_REG_PART_ID, &part_id) != MAX30102_OK)
	{
		return MAX30102_ERROR;
	}

	if (part_id != MAX30102_PART_ID_VALUE)
	{
		return MAX30102_ERROR;
	}

	return MAX30102_OK;
}

/*
 * 函数功能：软件复位 MAX30102
 */
uint8_t MAX30102_Reset(void)
{
	if (MAX30102_WriteReg(MAX30102_REG_MODE_CONFIG, 0x40) != 0)
	{
		return MAX30102_ERROR;
	}

	/* 等待 RESET 位自动清零 */
	Delay_ms(10);

	return MAX30102_OK;
}

/*
 * 函数功能：清空 FIFO
 *
 * FIFO 不是直接擦除，而是把读指针、写指针和溢出计数器清零。
 */
uint8_t MAX30102_ResetFIFO(void)
{
	if (MAX30102_WriteReg(MAX30102_REG_FIFO_WR_PTR, 0x00) != 0)
	{
		return MAX30102_ERROR;
	}

	if (MAX30102_WriteReg(MAX30102_REG_OVF_COUNTER, 0x00) != 0)
	{
		return MAX30102_ERROR;
	}

	if (MAX30102_WriteReg(MAX30102_REG_FIFO_RD_PTR, 0x00) != 0)
	{
		return MAX30102_ERROR;
	}

	return MAX30102_OK;
}

/*
 * 函数功能：初始化 MAX30102
 *
 * 当前版本采用轮询方式读取 FIFO，
 * 所以暂时关闭 PPG_RDY 和 A_FULL 中断。
 */
uint8_t MAX30102_Init(void)
{
	uint8_t dummy;

	/*
	 * 注意：
	 * MyI2C_Init() 建议在 main 中调用，
	 * 这样可以清楚看到底层总线和传感器的初始化顺序。
	 */

	/* 第一步：确认芯片在线 */
	if (MAX30102_Check() != MAX30102_OK)
	{
		return MAX30102_ERROR;
	}

	/* 第二步：软件复位 */
	if (MAX30102_Reset() != MAX30102_OK)
	{
		return MAX30102_ERROR;
	}

	/* 读取状态寄存器，清除上电状态 */
	MAX30102_ReadReg(MAX30102_REG_INTR_STATUS_1, &dummy);
	MAX30102_ReadReg(MAX30102_REG_INTR_STATUS_2, &dummy);

	/* 第三步：关闭中断，当前使用轮询 */
	if (MAX30102_WriteReg(MAX30102_REG_INTR_ENABLE_1, 0x00) != 0)
	{
		return MAX30102_ERROR;
	}

	if (MAX30102_WriteReg(MAX30102_REG_INTR_ENABLE_2, 0x00) != 0)
	{
		return MAX30102_ERROR;
	}

	/* 第四步：清空 FIFO */
	if (MAX30102_ResetFIFO() != MAX30102_OK)
	{
		return MAX30102_ERROR;
	}

	/*
	 * 第五步：配置 FIFO
	 *
	 * 0x0F：
	 * FIFO_A_FULL = 0
	 * FIFO_ROLLOVER_EN = 0
	 * SMP_AVE = 1
	 */
	if (MAX30102_WriteReg(MAX30102_REG_FIFO_CONFIG, 0x0F) != 0)
	{
		return MAX30102_ERROR;
	}

	/*
	 * 第六步：配置 SpO2 模式
	 *
	 * 0x27：
	 * ADC 范围：4096
	 * 采样率：100Hz
	 * LED 脉宽：411us
	 */
	if (MAX30102_WriteReg(MAX30102_REG_SPO2_CONFIG, 0x27) != 0)
	{
		return MAX30102_ERROR;
	}

	/*
	 * 第七步：配置红光和红外光 LED 电流
	 */
	if (MAX30102_WriteReg(MAX30102_REG_LED1_PA, 0x24) != 0)
	{
		return MAX30102_ERROR;
	}

	if (MAX30102_WriteReg(MAX30102_REG_LED2_PA, 0x24) != 0)
	{
		return MAX30102_ERROR;
	}

	/*
	 * 第八步：进入 SpO2 工作模式
	 *
	 * 0x03 表示 SpO2 模式。
	 * 这一步放在最后，配置完成后才开始采样。
	 */
	if (MAX30102_WriteReg(MAX30102_REG_MODE_CONFIG, 0x03) != 0)
	{
		return MAX30102_ERROR;
	}

	return MAX30102_OK;
}

/*
 * 函数功能：获取 FIFO 中可读取的样本数量
 */
uint8_t MAX30102_GetFIFOCount(uint8_t *count)
{
    uint8_t write_pointer;
    uint8_t read_pointer;
    uint8_t overflow_count;

    if (count == 0)
    {
        return MAX30102_ERROR;
    }

    if (MAX30102_ReadReg(MAX30102_REG_FIFO_WR_PTR, &write_pointer) != MAX30102_OK)
    {
        return MAX30102_ERROR;
    }

    if (MAX30102_ReadReg(MAX30102_REG_FIFO_RD_PTR, &read_pointer) != MAX30102_OK)
    {
        return MAX30102_ERROR;
    }

    write_pointer &= 0x1F;
    read_pointer &= 0x1F;

    if (write_pointer == read_pointer)
    {
        if (MAX30102_ReadReg(MAX30102_REG_OVF_COUNTER, &overflow_count) != MAX30102_OK)
        {
            return MAX30102_ERROR;
        }

        if ((overflow_count & 0x1F) != 0)
        {
            *count = MAX30102_FIFO_DEPTH;
            return MAX30102_OK;
        }

        *count = 0;
        return MAX30102_OK;
    }

    if (write_pointer > read_pointer)
    {
        *count = write_pointer - read_pointer;
    }
    else
    {
        *count = (uint8_t)(MAX30102_FIFO_DEPTH + write_pointer - read_pointer);
    }

    return MAX30102_OK;
}
/*
 * 函数功能：读取 FIFO 中的一个样本
 */
uint8_t MAX30102_ReadFIFO(MAX30102_SAMPLE_T *sample)
{
	uint8_t fifo_count;
	uint8_t fifo_data[6];

	if (sample == 0)
	{
		return MAX30102_ERROR;
	}

	/* 先判断 FIFO 中有没有数据 */
	if (MAX30102_GetFIFOCount(&fifo_count) != MAX30102_OK)
	{
		return MAX30102_ERROR;
	}

	if (fifo_count == 0)
	{
		return MAX30102_NO_DATA;
	}

	/*
	 * SpO2 模式下：
	 * 3 字节红光数据
	 * 3 字节红外数据
	 */
	if (MyI2C_ReadBytes(MAX30102_I2C_ADDRESS,
	                    MAX30102_REG_FIFO_DATA,
	                    fifo_data,
	                    6) != 0)
	{
		return MAX30102_ERROR;
	}

	/*
	 * FIFO 数据是 18 位有效数据，
	 * 高字节只有低 2 位有效。
	 */
	sample->red = ((uint32_t)(fifo_data[0] & 0x03) << 16)
	            | ((uint32_t)fifo_data[1] << 8)
	            | fifo_data[2];

	sample->ir = ((uint32_t)(fifo_data[3] & 0x03) << 16)
	           | ((uint32_t)fifo_data[4] << 8)
	           | fifo_data[5];

	return MAX30102_OK;
}
