#include "drv_light.h"
#include "iot_pwm.h"

#define LED_R_PORT EPWMDEV_PWM1_M1
#define LED_G_PORT EPWMDEV_PWM7_M1
//#define LED_B_PORT EPWMDEV_PWM0_M1
static bool g_light_state = false;


/***************************************************************
* 函数名称: light_dev_init
* 说    明: rgb灯设备初始化
* 参    数: 无
* 返 回 值: 无
***************************************************************/
void light_dev_init(void)
{
    IoTPwmInit(LED_R_PORT);
    IoTPwmInit(LED_G_PORT);
   // IoTPwmInit(LED_B_PORT);
    
}

/***************************************************************
* 函数名称: light_set_state
* 说    明: 控制灯状态
* 参    数: bool state true：红打开 false：绿关闭
* 返 回 值: 无
***************************************************************/
void light_set_state(bool state)
{

    // if (state == g_light_state)
    // {
    //     return;
    // }

    if (state)
    {
        IoTPwmStart(LED_R_PORT,99, 1000);
        IoTPwmStart(LED_G_PORT,1, 1000);
       // IoTPwmStart(LED_B_PORT,1, 1000);
        
    }
    else
    {
        IoTPwmStart(LED_R_PORT,1, 1000);
        IoTPwmStart(LED_G_PORT,99, 1000);
        //IoTPwmStart(LED_B_PORT,1, 1000);
    }
    // g_light_state = state;

}



int get_light_state(void)
{
    return g_light_state;
}
