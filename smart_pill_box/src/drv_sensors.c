#include "drv_sensors.h"
#include "iot_i2c.h"
#include "stdint.h"
#include "iot_errno.h"
#include "iot_pwm.h"
#include "iot_gpio.h"
#include <math.h>
#define I2C_HANDLE EI2C0_M2
#define SHT30_I2C_ADDRESS 0x44
#define BH1750_I2C_ADDRESS 0x23
#define MPU6050_I2C_ADDRESS 0x68
#define BEEP_PORT EPWMDEV_PWM5_M0
#define GPIO_BODY_INDUCTION GPIO0_PA3
/***************************************************************
 * 函数名称: sht30_init
 * 说    明: sht30初始化
 * 参    数: 无
 * 返 回 值: uint32_t IOT_SUCCESS表示成功 IOT_FAILURE表示失败
 ***************************************************************/
static uint32_t sht30_init(void)
{
    uint32_t ret = 0;
    uint8_t send_data[2] = {0x22, 0x36};
    uint32_t send_len = 2;

    ret = IoTI2cWrite(I2C_HANDLE, SHT30_I2C_ADDRESS, send_data, send_len); 
    if (ret != IOT_SUCCESS)
    {
        printf("I2c write failure.\r\n");
        return IOT_FAILURE;
    }

    return IOT_SUCCESS;
}

/***************************************************************
 * 函数名称: bh1750_init
 * 说    明: bh1750初始化
 * 参    数: 无
 * 返 回 值: uint32_t IOT_SUCCESS表示成功 IOT_FAILURE表示失败
 ***************************************************************/
static uint32_t bh1750_init(void)
{
    uint32_t ret = 0;
    uint8_t send_data[1] = {0x10};
    uint32_t send_len = 1;

    ret = IoTI2cWrite(I2C_HANDLE, SHT30_I2C_ADDRESS, send_data, send_len); 
    if (ret != IOT_SUCCESS)
    {
        printf("I2c write failure.\r\n");
        return IOT_FAILURE;
    }

    return IOT_SUCCESS;
}

/***************************************************************
* 函数名称: sht30_calc_RH
* 说    明: 湿度计算
* 参    数: u16sRH：读取到的湿度原始数据
* 返 回 值: 计算后的湿度数据
***************************************************************/
static float sht30_calc_RH(uint16_t u16sRH)
{
    float humidityRH = 0;

    /*clear bits [1..0] (status bits)*/
    u16sRH &= ~0x0003;
    /*calculate relative humidity [%RH]*/
    /*RH = rawValue / (2^16-1) * 10*/
    humidityRH = (100 * (float)u16sRH / 65535);

    return humidityRH;
}

/***************************************************************
* 函数名称: sht30_calc_temperature
* 说    明: 温度计算
* 参    数: u16sT：读取到的温度原始数据
* 返 回 值: 计算后的温度数据
***************************************************************/
static float sht30_calc_temperature(uint16_t u16sT)
{
    float temperature = 0;

    /*clear bits [1..0] (status bits)*/
    u16sT &= ~0x0003;
    /*calculate temperature [℃]*/
    /*T = -45 + 175 * rawValue / (2^16-1)*/
    temperature = (175 * (float)u16sT / 65535 - 45);

    return temperature;
}

/***************************************************************
* 函数名称: sht30_check_crc
* 说    明: 检查数据正确性
* 参    数: data：读取到的数据
            nbrOfBytes：需要校验的数量
            checksum：读取到的校对比验值
* 返 回 值: 校验结果，0-成功 1-失败
***************************************************************/
static uint8_t sht30_check_crc(uint8_t *data, uint8_t nbrOfBytes, uint8_t checksum)
{
    uint8_t crc = 0xFF;
    uint8_t bit = 0;
    uint8_t byteCtr ;
    const int16_t POLYNOMIAL = 0x131;

    /*calculates 8-Bit checksum with given polynomial*/
    for(byteCtr = 0; byteCtr < nbrOfBytes; ++byteCtr)
    {
        crc ^= (data[byteCtr]);
        for ( bit = 8; bit > 0; --bit)
        {
            if (crc & 0x80) crc = (crc << 1) ^ POLYNOMIAL;
            else crc = (crc << 1);
        }
    }

    if(crc != checksum)
        return 1;
    else
        return 0;
}

/***************************************************************
* 函数名称: sht30_read_data
* 说    明: 读取温度、湿度
* 参    数: temp,humi：读取到的数据,通过指针返回 
* 返 回 值: 无
***************************************************************/
void sht30_read_data(double *temp, double *humi)
{
    /*checksum verification*/
    uint8_t data[3];
    uint16_t tmp;
    uint8_t rc;
    /*byte 0,1 is temperature byte 4,5 is humidity*/
    uint8_t SHT30_Data_Buffer[6];
    memset(SHT30_Data_Buffer, 0, 6);
    uint8_t send_data[2] = {0xE0, 0x00};

    uint32_t send_len = 2;
    IoTI2cWrite(I2C_HANDLE, SHT30_I2C_ADDRESS, send_data, send_len);

    uint32_t receive_len = 6;
    IoTI2cRead(I2C_HANDLE, SHT30_I2C_ADDRESS, SHT30_Data_Buffer, receive_len);

    /*check temperature*/
    data[0] = SHT30_Data_Buffer[0];
    data[1] = SHT30_Data_Buffer[1];
    data[2] = SHT30_Data_Buffer[2];
    rc = sht30_check_crc(data, 2, data[2]);
    if(!rc)
    {
        tmp = ((uint16_t)data[0] << 8) | data[1];
        *temp = sht30_calc_temperature(tmp);
    }
    
    /*check humidity*/
    data[0] = SHT30_Data_Buffer[3];
    data[1] = SHT30_Data_Buffer[4];
    data[2] = SHT30_Data_Buffer[5];
    rc = sht30_check_crc(data, 2, data[2]);
    if(!rc)
    {
        tmp = ((uint16_t)data[0] << 8) | data[1];
        *humi = sht30_calc_RH(tmp);
    }
}

/***************************************************************
* 函数名称: bh1750_read_data
* 说    明: 读取光照强度
* 参    数: dat：读取到的数据
* 返 回 值: 无
***************************************************************/
void bh1750_read_data(double *dat)
{
    uint8_t send_data[1] = {0x10};
    uint32_t send_len = 1;

    IoTI2cWrite(I2C_HANDLE, BH1750_I2C_ADDRESS, send_data, send_len); 

    uint8_t recv_data[2] = {0};
    uint32_t receive_len = 2;   

    IoTI2cRead(I2C_HANDLE, BH1750_I2C_ADDRESS, recv_data, receive_len);
    *dat = (float)(((recv_data[0] << 8) + recv_data[1]) / 1.2);
}





/***************************************************************
 * 函数名称: MPU6050_Read_Buffer
 * 说    明: I2C读取一段寄存器内容存放到指定的缓冲区
 * 参    数:  reg：目标寄存器
 *          p_buffer：缓冲区
 *          value：值
 * 返 回 值: 操作结果
 ***************************************************************/
static uint8_t MPU6050_Read_Buffer(uint8_t reg, uint8_t *p_buffer, uint16_t length)
{

    uint32_t status = 0;
    uint8_t buffer[1] = {reg};

    status = IoTI2cWrite(I2C_HANDLE, MPU6050_SLAVE_ADDRESS, buffer, 1);
    if (status != IOT_SUCCESS)
    {
        printf("Error: I2C write status:%d\n", status);
        return status;
    }

    IoTI2cRead(I2C_HANDLE, MPU6050_SLAVE_ADDRESS, p_buffer, length);
    return IOT_SUCCESS;
}

/***************************************************************
 * 函数名称: mpu6050_write_reg
 * 说    明: 写数据到MPU6050寄存器
 * 参    数:  reg：目标寄存器
 *          data
 * 返 回 值: 无
 ***************************************************************/
static void mpu6050_write_reg(uint8_t reg, uint8_t data)
{
    uint8_t send_data[2] = {reg, data};

    IoTI2cWrite(I2C_HANDLE, MPU6050_SLAVE_ADDRESS, send_data, 2);
}

/***************************************************************
 * 函数名称: mpu6050_read_register
 * 说    明: 从MPU6050寄存器读取数据
 * 参    数:  reg：目标寄存器
 *          buf：缓冲区
 *          length：长度
 * 返 回 值: 无
 ***************************************************************/
static void mpu6050_read_register(uint8_t reg, unsigned char *buf, uint8_t length)
{
    MPU6050_Read_Buffer(reg, buf, length);
}

/***************************************************************
 * 函数名称: mpu6050_read_acc
 * 说    明: 读取MPU6050的加速度数据
 * 参    数:  acc_data：加速度数据
 * 返 回 值: 无
 ***************************************************************/
static void mpu6050_read_acc(short *acc_data)
{
    uint8_t buf[6];
    mpu6050_read_register(MPU6050_ACC_OUT, buf, 6);
    acc_data[0] = (buf[0] << 8) | buf[1];
    acc_data[1] = (buf[2] << 8) | buf[3];
    acc_data[2] = (buf[4] << 8) | buf[5];
}

/***************************************************************
 * 函数名称: action_interrupt
 * 说    明: 运动中断设置
 * 参    数: 无
 * 返 回 值: 无
 ***************************************************************/
static void action_interrupt()
{
    mpu6050_write_reg(MPU6050_RA_MOT_THR, 0x03); // 运动阈值
    mpu6050_write_reg(MPU6050_RA_MOT_DUR, 0x14); // 检测时间20ms 单位1ms
}

/***************************************************************
 * 函数名称: mpu6050_init
 * 说    明: mpu6050初始化
 * 参    数: 无
 * 返 回 值: 无
 ***************************************************************/
void mpu6050_init(void)
{
    mpu6050_write_reg(MPU6050_RA_PWR_MGMT_1, 0X80); // 复位MPU6050
    LOS_Msleep(200);
    mpu6050_write_reg(MPU6050_RA_PWR_MGMT_1, 0X00);   // 唤醒MPU6050
    mpu6050_write_reg(MPU6050_RA_INT_ENABLE, 0X00);   // 关闭所有中断
    mpu6050_write_reg(MPU6050_RA_USER_CTRL, 0X00);    // I2C主模式关闭
    mpu6050_write_reg(MPU6050_RA_FIFO_EN, 0X00);      // 关闭FIFO
    mpu6050_write_reg(MPU6050_RA_INT_PIN_CFG, 0X80);  // 中断的逻辑电平模式,设置为0，中断信号为高电；设置为1，中断信号为低电平时。
    action_interrupt();                               // 运动中断
    mpu6050_write_reg(MPU6050_RA_CONFIG, 0x04);       // 配置外部引脚采样和DLPF数字低通滤波器
    mpu6050_write_reg(MPU6050_RA_ACCEL_CONFIG, 0x1C); // 加速度传感器量程和高通滤波器配置
    mpu6050_write_reg(MPU6050_RA_INT_PIN_CFG, 0X1C);  // INT引脚低电平平时
    mpu6050_write_reg(MPU6050_RA_INT_ENABLE, 0x40);   // 中断使能寄存器
}

/***************************************************************
 * 函数功能: 读取MPU6050的ID
 * 输入参数: 无
 * 返 回 值: 无
 * 说    明: 无
 ***************************************************************/
static uint8_t mpu6050_read_id()
{
    unsigned char buff = 0;
    mpu6050_read_register(MPU6050_RA_WHO_AM_I, &buff, 1);
    if (buff != 0x68)
    {
        printf("MPU6050 dectected error Re:%u\n", buff);
        return 0;
    }
    else
    {
        return 1;
    }
}

/***************************************************************
 * 函数名称: mpu6050_read_data
 * 说    明: 读取数据
 * 参    数: 无
 * 返 回 值: 无
 ***************************************************************/
void mpu6050_read_data(short *dat)
{
    short accel[3];
    short temp;
    if (mpu6050_read_id() == 0)
    {
        while (1)
            ;
    }
    mpu6050_read_acc(accel);
    dat[0] = accel[0];
    dat[1] = accel[1];
    dat[2] = accel[2];
    LOS_Msleep(500);
}


/***************************************************************
* 函数名称: i2c_dev_init
* 说    明: i2c设备初始化
* 参    数: 无
* 返 回 值: 无
***************************************************************/
void i2c_dev_init(void)
{
    IoTI2cInit(I2C_HANDLE, EI2C_FRE_400K);
    sht30_init();
    bh1750_init();
    mpu6050_init();
}



#define CAL_PPM 20 // 校准环境中PPM值
#define RL 1       // RL阻值

static float m_r0; // 元件在干净空气中的阻值

#define MQ2_ADC_CHANNEL 4

/***************************************************************
* 函数名称: mq2_dev_init
* 说    明: 初始化ADC
* 参    数: 无
* 返 回 值: 0为成功，反之为失败
***************************************************************/
unsigned int mq2_dev_init(void)
{
    unsigned int ret = 0;

    ret = IoTAdcInit(MQ2_ADC_CHANNEL);

    if(ret != IOT_SUCCESS)
    {
        printf("%s, %s, %d: ADC Init fail\n", __FILE__, __func__, __LINE__);
    }

    return 0;
}

/***************************************************************
* 函数名称: Get_Voltage
* 说    明: 获取ADC电压值
* 参    数: 无
* 返 回 值: 电压值
***************************************************************/
static float adc_get_voltage(void)
{
    unsigned int ret = IOT_SUCCESS;
    unsigned int data = 0;

    ret = IoTAdcGetVal(MQ2_ADC_CHANNEL, &data);

    if (ret != IOT_SUCCESS)
    {
        printf("%s, %s, %d: ADC Read Fail\n", __FILE__, __func__, __LINE__);
        return 0.0;
    }

    return (float)(data * 3.3 / 1024.0);
}

/***************************************************************
 * 函数名称: mq2_ppm_calibration
 * 说    明: 传感器校准函数
 * 参    数: 无
 * 返 回 值: 无
 ***************************************************************/
void mq2_ppm_calibration(void) 
{
  float voltage = adc_get_voltage();
  float rs = (5 - voltage) / voltage * RL;
  m_r0 = rs / powf(CAL_PPM / 613.9f, 1 / -2.074f);
  
}

/***************************************************************
 * 函数名称: get_mq2_ppm
 * 说    明: 获取PPM函数
 * 参    数: 无
 * 返 回 值: ppm
 ***************************************************************/
float get_mq2_ppm(void) 
{
  float voltage, rs, ppm;

  voltage = adc_get_voltage();
  rs = (5 - voltage) / voltage * RL;      // 计算rs
  
  ppm = 613.9f * powf(rs / m_r0, -2.074f); // 计算ppm
  
  return ppm;
}





/***************************************************************
 * 函数名称: mq2_init
 * 说    明: mq2初始化
 * 参    数: 无
 * 返 回 值: 无
 ***************************************************************/
void mq2_init(void)
{
    mq2_dev_init();
    LOS_Msleep(1000);
    mq2_ppm_calibration();
}

/***************************************************************
 * 函数名称: mq2_read_data
 * 说    明: 读取mq2传感器数据
 * 参    数: 无
 * 返 回 值: 无
 ***************************************************************/
void mq2_read_data(float *dat)
{
    *dat = get_mq2_ppm();
    
}

/***************************************************************
 * 函数名称: beep_dev_init
 * 说    明: 蜂鸣器初始化
 * 参    数: 无
 * 返 回 值: 无
 ***************************************************************/
void beep_dev_init(void)
{
    IoTPwmInit(BEEP_PORT);
}

/***************************************************************
 * 函数名称: beep_set_pwm
 * 说    明: 设置蜂鸣器PWM
 * 参    数: 无
 * 返 回 值: 无
 ***************************************************************/
void beep_set_pwm(unsigned int duty)
{
    IoTPwmStart(BEEP_PORT, duty, 1000);
}

/***************************************************************
 * 函数名称: beep_set_state
 * 说    明: 设置蜂鸣器状态
 * 参    数: 无
 * 返 回 值: 无
 ***************************************************************/
void beep_set_state(bool state)
{
    static bool last_state = false;

    if (state == last_state)
    {
        return;
    }

    if (state)
    {
        beep_set_pwm(20);
    }
    else
    {
        beep_set_pwm(1);
        IoTPwmStop(BEEP_PORT);
    }

    last_state = state;
}



/***************************************************************
 * 函数名称: body_induction_get_state
 * 说    明: 获取人体感应状态
 * 参    数: 无
 * 返 回 值: 无
 ***************************************************************/
void body_induction_get_state(bool *dat)
{
    IotGpioValue value = IOT_GPIO_VALUE0;

    IoTGpioGetInputVal(GPIO_BODY_INDUCTION, &value);

    if (value) 
    {
        *dat = true;
    }
    else
    {
        *dat = false;
    }
}

/***************************************************************
 * 函数名称: body_induction_dev_init
 * 说    明: 人体感应传感器初始化
 * 参    数: 无
 * 返 回 值: 无
 ***************************************************************/
void body_induction_dev_init(void)
{
    IoTGpioInit(GPIO_BODY_INDUCTION);
    IoTGpioSetDir(GPIO_BODY_INDUCTION, IOT_GPIO_DIR_IN);
}