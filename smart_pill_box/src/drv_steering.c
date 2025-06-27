#include "drv_steering.h"
#include "iot_pwm.h"

/* 舵机对应PWM */
#define ELECTRICAL_MACHINERY_PORT EPWMDEV_PWM0_M1

static bool steering_state = false;


/***************************************************************
* 函数名称: steering_dev_init
* 说    明: 舵机设备初始化
* 参    数: 无
* 返 回 值: 无
***************************************************************/
void steering_dev_init(void)
{
    IoTPwmInit(ELECTRICAL_MACHINERY_PORT);
}

/***************************************************************
* 函数名称: steering_set_state
* 说    明: 控制舵机状态
* 参    数: bool state true：打开 false：关闭
* 返 回 值: 无
***************************************************************/
void steering_set_state(bool state)
{

    

    if (state)
    {
        IoTPwmStart(ELECTRICAL_MACHINERY_PORT,8, 50);
       
        
    }
    else
    {
        IoTPwmStart(ELECTRICAL_MACHINERY_PORT,3, 50);
       
    }
   

}



int get_steering_state(void)
{
    return steering_state;
}
