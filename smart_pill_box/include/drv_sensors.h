#ifndef __DRV_SENSORS_H__
#define __DRV_SENSORS_H__

#include <stdbool.h>

#define MPU6050_GYRO_OUT 0x43        // MPU6050陀螺仪数据寄存器地址
#define MPU6050_ACC_OUT 0x3B         // MPU6050加速度数据寄存器地址
#define MPU6050_SLAVE_ADDRESS 0x68   // MPU6050器件读地址
#define MPU6050_ADDRESS_AD0_LOW 0x68 // address pin low (GND), default for InvenSense evaluation board
#define MPU6050_RA_CONFIG 0x1A
#define MPU6050_RA_ACCEL_CONFIG 0x1C
#define MPU6050_RA_FF_THR 0x1D
#define MPU6050_RA_FF_DUR 0x1E
#define MPU6050_RA_MOT_THR 0x1F // 运动检测阀值设置寄存器
#define MPU6050_RA_MOT_DUR 0x20 // 运动检测时间阀值
#define MPU6050_RA_ZRMOT_THR 0x21
#define MPU6050_RA_ZRMOT_DUR 0x22
#define MPU6050_RA_FIFO_EN 0x23
#define MPU6050_RA_INT_PIN_CFG 0x37 // 中断/旁路设置寄存器
#define MPU6050_RA_INT_ENABLE 0x38  // 中断使能寄存器
#define MPU6050_RA_TEMP_OUT_H 0x41
#define MPU6050_RA_USER_CTRL 0x6A
#define MPU6050_RA_PWR_MGMT_1 0x6B
#define MPU6050_RA_WHO_AM_I 0x75

void mpu6050_read_data(short *dat);

void i2c_dev_init(void);
void bh1750_read_data(double *dat);
void sht30_read_data(double *temp, double *humi);

void mq2_init(void);
void mq2_read_data(float *dat);
void beep_dev_init(void);
void beep_set_state(bool state);

void body_induction_get_state(bool *dat);
void body_induction_dev_init(void);


#endif