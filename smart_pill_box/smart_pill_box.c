/*
 * Copyright (c) 2024 iSoftStone Education Co., Ltd.
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#include <stdio.h>
#include <stdbool.h>

#include "drv_steering.h"
#include "los_task.h"
#include "ohos_init.h"
#include "cmsis_os.h"
#include "config_network.h"
#include "smart_box.h"
#include "smart_box_event.h"
#include "su_03t.h"
#include "iot.h"
#include "lcd.h"
#include "picture.h"
#include "adc_key.h"
#include "drv_sensors.h"
#include "drv_light.h"

#include <sys/time.h>
#include <time.h>
#include <math.h>
#include "iot_errno.h"
#include "ntp.h"
/*
- lcd屏幕使用SPI0               (GPIO0_C0|GPIO0_C1|GPIO0_C2|GPIO0_C3|GPIO0_A4)
- mq2烟感传感器使用ADC通道4      (GPIO0_PC4)
- 蜂鸣器使用PWM5控制             (GPIO0_C5)
- 舵机                          (GPIO0_B4)
- 按键                          (GPIO0_C7) 
- 语音模块使用串口2               (GPIO0_B2\GPIO0_B3)
- rgb灯使用GPIO控制              (GPIO0_B5/GPIO1_D0)
- sht30与bh1750与MPU6050共用I2C0 (GPIO0_A1\GPIO0_A0|GPIO0_A2)
- 人体感应使用GPIO控制            (GPIO0_PA3)
*/

#define ROUTE_SSID      "熊孩子网站"          // WiFi账号
#define ROUTE_PASSWORD "xwj200547"       // WiFi密码


#define LOG_SC_TAG                 "sc"
#define LOG_SC_TAG_INFO            "["LOG_SC_TAG"           %4d] "
#define LOG_SC_TAG_IOT             "["LOG_SC_TAG" iot       %4d] "
#define LOG_SC_TAG_NTP             "["LOG_SC_TAG" ntp       %4d] "
#define LOG_SC_TAG_KEY             "["LOG_SC_TAG" key       %4d] "
#define LOG_SC_TAG_ALARM           "["LOG_SC_TAG" alarm     %4d] "
#define LOG_SC_TAG_DISPLAY         "["LOG_SC_TAG" display   %4d] "

/***************************************************************
* 名   称: sc_ntp_time_t
* 说    明: 时间结构体
***************************************************************/
typedef struct
{
    int                        sync_status;        // ntp同步状态，0为未同步，1为同步
    int                        year;               // 年
    int                        month;              // 月
    int                        day;                // 日
    int                        hour;               // 时
    int                        min;                // 分
    int                        sec;                // 秒
} sc_ntp_time_t;


#define THREAD_START_INFO                                      \
{                                                              \
    int len = strlen(__FUNCTION__);                            \
    int count = (44 - len)/2;                                  \
    char temp[51] = {0};                                       \
    memset(temp, '=', count);                                  \
    memcpy(&temp[count], __FUNCTION__, len);                   \
    temp[strlen(temp)] = ' ';                                  \
    memcpy(&temp[strlen(temp)], "start", 5);                   \
    if(len%2)temp[strlen(temp)] = ' ';                         \
    memset(&temp[strlen(temp)], '=', count);                   \
    printf(LOG_SC_TAG_INFO"%s\r\n", __LINE__, temp);           \
}

struct tm *now_tm;

WifiLinkedInfo wifiinfo;
/***************************************************************
* 函数名称: get_sta_link
* 说    明: 获取wifi连接信息
* 参    数: 
* 返 回 值: int 0连接，-1未连接
***************************************************************/
static int get_sta_link()
{
    
    WifiLinkedInfo *info = &wifiinfo;
    
    memset(info, 0, sizeof(WifiLinkedInfo));
    if (GetLinkedInfo(info) == WIFI_SUCCESS && info->connState == WIFI_CONNECTED && info->ipAddress != 0)
    {
        return 0;
    }
    
    return -1;
}


/***************************************************************
* 函数名称: sc_ntp_thread
* 说    明: ntp同步时间，用于与NTP服务器进行时间同步
* 参    数:
*       @*args          智慧时钟设备的结构体指针
* 返 回 值: 无
***************************************************************/
static void *sc_ntp_thread(uint32_t args)
{
    
    uint32_t now_time   = 0;
    uint32_t ntp_time   = 0;
    time_t   now_time_t = 0;
    sc_ntp_time_t ntp_time_data;
    sc_ntp_time_t *ntp  = &ntp_time_data;
    int            time = 0;
    uint8_t  network_connect_last_status = 0;            // 网络最近连通状态标志，0为上一次为关闭，1为上一次为连通
    
    // ntp初始化
    THREAD_START_INFO
    if (get_sta_link() == 0)
    {
        ntp_init();
    }
    
    printf(LOG_SC_TAG_NTP"init finish\r\n", __LINE__);
    
    while (1)
    {
        // 如果网络已连通，则进行ntp同步
        if (get_sta_link() == 0)
        {
            if (network_connect_last_status == 0)
            {
                // 如果上一次网络未连通，则进行ntp重初始化
                network_connect_last_status = 1;
                ntp_reinit();
                LOS_Msleep(2000);
            }
            
            // 每隔60秒，进行ntp同步
            if ((++time) % 300 == 0 || ntp->sync_status == 0 || ntp->year > 2030)
            {
                printf(LOG_SC_TAG_NTP"ntp_time: %10lu start======\n", __LINE__, ntp_time);
                ntp_time = ntp_get_time(NULL);
                printf(LOG_SC_TAG_NTP"ntp_time: %10lu end  ======\n", __LINE__, ntp_time);
                if (ntp_time == 0)
                {
                    ntp->sync_status = 0;
                    now_time++;
                }
                else
                {
                    ntp->sync_status = 1;
                    now_time = ntp_time;
                }
            }
            else
            {
                now_time++;
            }
        }
        else
        {
            // 如果网络未连通，则不进行ntp同步，且ntp同步状态置为0
            ntp->sync_status = 0;
            network_connect_last_status = 0;
            // 如果未连通，则时间加1（即该任务是每隔1秒执行一次）
            now_time++;
        }
        
        // 获取本地时间
        now_time_t = (time_t)now_time;
        //struct tm *now_tm = localtime(&now_time_t);
        now_tm = localtime(&now_time_t);
        // 赋值给公共结构体
        ntp->year  = now_tm->tm_year + 1900;
        ntp->month = now_tm->tm_mon + 1;
        ntp->day   = now_tm->tm_mday;
        ntp->hour  = now_tm->tm_hour;
        ntp->min   = now_tm->tm_min;
        ntp->sec   = now_tm->tm_sec;
        // 打印信息
        if (now_tm->tm_sec % 30 == 0)
        {
            printf(LOG_SC_TAG_NTP"sync_status: %s now_tm: %04d-%02d-%02d %02d:%02d:%02d\n",
                   __LINE__, (ntp->sync_status == 0) ? ("nosync") : ("sync"),
                   now_tm->tm_year + 1900,
                   now_tm->tm_mon + 1,
                   now_tm->tm_mday,
                   now_tm->tm_hour,
                   now_tm->tm_min,
                   now_tm->tm_sec);
        }
        // 等待950ms，上述任务需要消耗一定时间，预计为50msec
        LOS_Msleep(950);
    }
    return NULL;
}



/***************************************************************
 * 函数名称: create_task
 * 说    明: 创建任务
 * 参    数: unsigned int *threadID,  uint32_t size, uint8_t prio,
 *           TSK_ENTRY_FUNC func, void *args, const char *owner
 * 返 回 值: int
 ***************************************************************/
int create_task(unsigned int *threadID, uint32_t size, uint8_t prio, TSK_ENTRY_FUNC func, void *args, const char *owner)
{
    int ret = 0;
    TSK_INIT_PARAM_S task = {0};
    
    task.pfnTaskEntry = (TSK_ENTRY_FUNC)func;
    task.uwStackSize  = size;
    task.pcName       = (char *)owner;
    task.uwArg        = (uint32_t)args;
    task.usTaskPrio   = prio > 30 ? 30 : prio;
    ret = LOS_TaskCreate(threadID, &task);
    if (ret != LOS_OK)
    {
        printf("create_task", "%s ret = 0x%x\n", ret == LOS_OK ? "OK" : "FAILURE", ret);
    }
    
    return (ret == LOS_OK) ? IOT_SUCCESS : IOT_FAILURE;
}




/***************************************************************
 * 函数名称: iot_thread
 * 说    明: iot线程
 * 参    数: 无
 * 返 回 值: 无
 ***************************************************************/
void iot_thread(void *args)
{
    uint8_t mac_address[12] = {0x00, 0xdc, 0xb6, 0x90, 0x01, 0x00,0};

  char ssid[32]=ROUTE_SSID;
  char password[32]=ROUTE_PASSWORD;
  char mac_addr[32]={0};


  FlashDeinit();
  FlashInit();

  VendorSet(VENDOR_ID_WIFI_MODE, "STA", 3); // 配置为Wifi STA模式
  VendorSet(VENDOR_ID_MAC, mac_address, 6); // 多人同时做该实验，请修改各自不同的WiFi MAC地址
  VendorSet(VENDOR_ID_WIFI_ROUTE_SSID, ssid, sizeof(ssid));
  VendorSet(VENDOR_ID_WIFI_ROUTE_PASSWD, password,sizeof(password));

reconnect:
  SetWifiModeOff();
  int ret = SetWifiModeOn();
  if(ret != 0){
    printf("wifi connect failed,please check wifi config and the AP!\n");
    return;
  }


  mqtt_init();

  while (1) {
    if (!wait_message()) { 
      goto reconnect;
    }
    LOS_Msleep(1);
  }
}

unsigned char display;
bool steering_state=false;
bool light_state=false;
bool beep_state=false;
bool come_eat=false;
unsigned char dis2;
unsigned int storage_time[]={10,1,3};
unsigned char eat_1[3]={5,4,3};
unsigned char eat_2[3]={2,3,4};
unsigned char eat_3[3]={3,4,5};
unsigned char eat_time[3][2]={8,30,12,10,18,30};
unsigned char eat_index=0;

void smart_home_key_process(uint8_t key_no)
{
    switch(key_no)
    {
        case KEY_UP: if(come_eat) return;
        if(display==2&&dis2==0) {
            steering_state=false;
            steering_set_state(steering_state);
        }
        if((display==2&&dis2==0)||display==0||display==1) {
            display=(display+1)%3;
        
             lcd_fill(0,0,320,240,LCD_WHITE);
        }
        
        if(display==2) {
             if(dis2>=1&&dis2<=3) storage_time[dis2-1]++;
             else if(dis2>=4&&dis2<=9) {
                switch (dis2)
                {
                case 4:
                    eat_time[0][0]++;
                    break;
                case 5:
                    eat_time[0][1]++;
                    break;
                case 6:
                    eat_time[1][0]++;
                    break;
                case 7:
                    eat_time[1][1]++;
                    break;
                case 8:
                    eat_time[2][0]++;
                    break;        
                case 9:
                    eat_time[2][1]++;
                    break;
                }
             }
             else if(dis2>=10&&dis2<=12) eat_1[dis2-10]++;
             else if(dis2>=13&&dis2<=15) eat_2[dis2-13]++;
             else if(dis2>=16&&dis2<=18) eat_3[dis2-16]++;
        }
       

        break;
        case KEY_DOWN:
        if(display==2&&dis2==0) {
            steering_state=!steering_state;
            steering_set_state(steering_state);
        }

        if(display==2) {
             if(dis2>=1&&dis2<=3) storage_time[dis2-1]--;
             else if(dis2>=4&&dis2<=9) {
                switch (dis2)
                {
                case 4:
                    eat_time[0][0]--;
                    break;
                case 5:
                    eat_time[0][1]--;
                    break;
                case 6:
                    eat_time[1][0]--;
                    break;
                case 7:
                    eat_time[1][1]--;
                    break;
                case 8:
                    eat_time[2][0]--;
                    break;        
                case 9:
                    eat_time[2][1]--;
                    break;
                }
             }
             else if(dis2>=10&&dis2<=12) eat_1[dis2-10]--;
             else if(dis2>=13&&dis2<=15) eat_2[dis2-13]--;
             else if(dis2>=16&&dis2<=18) eat_3[dis2-16]--;
        }


        if(come_eat){
            come_eat=false;
            steering_state=false;
            steering_set_state(steering_state);
            
            if(++eat_index==3) eat_index=0;
        }
        
        break;
        case KEY_RIGHT: if(come_eat) return;
        if(display==2) {
            if(++dis2==19) dis2=0;
        }
        break;
        case KEY_LEFT: if(come_eat) return;
        if(display==2) {
            if(--dis2==255) dis2=18;
        }
        break;

    }

}

void smart_home_iot_cmd_process(int iot_cmd)
{
    switch (iot_cmd)
    {
        case IOT_CMD_BOX_ON:
            steering_state=true;
            steering_set_state(steering_state);
            
            break;
        case IOT_CMD_BOX_OFF:
            steering_state=false;
            steering_set_state(steering_state);
           
            break;
    }
}

void smart_home_su03t_cmd_process(int su03t_cmd)
{
    unsigned char eat[3]={eat_1[eat_index],eat_2[eat_index],eat_3[eat_index]};
    unsigned char time1[2]={eat_time[(eat_index+1)%3][0],eat_time[(eat_index+1)%3][1]};
    switch (su03t_cmd)
    {
        case 0x0101:
            if(come_eat){
            come_eat=false;
            steering_state=false;
            steering_set_state(steering_state);
            
            if(++eat_index==3) eat_index=0;
            su03t_send_uchar_msg(2, time1); 
        }
            
            break;
        case 0x0102:          
            su03t_send_uchar_msg(1, eat);            
            break;
    }
}


/***************************************************************
 * 函数名称: smart_box_thread
 * 说    明: 智慧药盒主线程
 * 参    数: 无
 * 返 回 值: 无
 ***************************************************************/
void smart_box_thread(void *arg)
{
    
    double illumination_range = 50.0;
    double temperature_range = 35.0;
    double humidity_range = 80.0;

    e_iot_data iot_data = {0};

    mq2_init();
    i2c_dev_init();
    lcd_dev_init();
    light_dev_init();
    su03t_init();
    
    beep_dev_init();
    body_induction_dev_init();
    steering_dev_init();
    //lcd_show_ui();
     
   //key:
    while(1)
    {
   
        event_info_t event_info = {0};
        //等待事件触发,如有触发,则立即处理对应事件,如未等到,则执行默认的代码逻辑,更新屏幕
        int ret = smart_home_event_wait(&event_info,3000);
        if(ret == LOS_OK){
            //收到指令
            printf("event recv %d ,%d\n",event_info.event,event_info.data.iot_data);
            switch (event_info.event)
            {
                case event_key_press:
                    smart_home_key_process(event_info.data.key_no);
                    
                     //goto key;
                    break;
                case event_iot_cmd:
                    smart_home_iot_cmd_process(event_info.data.iot_data);
                    break;
                case event_su03t:
                    smart_home_su03t_cmd_process(event_info.data.su03t_data);
                    break;
               default:break;
            }
            
        }

        double temp,humi,lum;
        float gas;
        short accelerated[3];
        bool body;
        sht30_read_data(&temp,&humi);
        bh1750_read_data(&lum);
        mpu6050_read_data(accelerated);
         
        mq2_read_data(&gas);
        
        body_induction_get_state(&body);
        printf("温度:%.2lf\n湿度:%.2lf\n光照:%.2lf\n加速度:%hd,,%hd,,%hd\nmq2:%.2lf\n人体:%d\n",temp,humi,lum,accelerated[0],accelerated[1],accelerated[2],gas,body);
        
        if(temp>50|humi>80|lum>150|accelerated[2]<1800|gas>50){
            light_state=true;
        }
        else{
            light_state=false;
        }       
        light_set_state(light_state);
        
        if(come_eat==false&&now_tm->tm_hour==eat_time[eat_index][0]&&now_tm->tm_min==eat_time[eat_index][1]){
            beep_state=true;
            beep_set_state(beep_state);
        }

        if(beep_state&&body){
            beep_state=false;
            beep_set_state(beep_state);
            come_eat=true;
            steering_state=true;
            steering_set_state(steering_state);
        }


        if(now_tm->tm_hour==0) {
            eat_index=0;
            storage_time[0]++;
            storage_time[1]++;
            storage_time[2]++;
        }



        if (mqtt_is_connected()) 
        {
            //发送iot数据
            iot_data.illumination = lum;
            iot_data.temperature = temp;
            iot_data.humidity = humi;
            iot_data.gas = gas;
            iot_data.box_state = steering_state;
           
            send_msg_to_mqtt(&iot_data);

           
        }        
        
        switch(display)
        {
            case 0:
            lcd_draw_line(107,0,107,200,LCD_BLACK);
            lcd_draw_line(214,0,214,200,LCD_BLACK);
            lcd_draw_line(0,100,320,100,LCD_BLACK);
            lcd_draw_line(0,200,320,200,LCD_BLACK);
            
            lcd_show_chinese(1,1,"一号药盒",LCD_DARKBLUE,LCD_WHITE,24,0);
            lcd_show_chinese(108,1,"二号药盒",LCD_DARKBLUE,LCD_WHITE,24,0);
            lcd_show_chinese(215,1,"三号药盒",LCD_DARKBLUE,LCD_WHITE,24,0);

            lcd_show_int_num(5,105,eat_1[eat_index],3,LCD_DARKBLUE,LCD_WHITE,32);
            lcd_show_int_num(112,105,eat_2[eat_index],3,LCD_DARKBLUE,LCD_WHITE,32);
            lcd_show_int_num(219,105,eat_3[eat_index],3,LCD_DARKBLUE,LCD_WHITE,32);

            lcd_show_string(0,210,"Next Time:",LCD_BROWN,LCD_WHITE,32,0);
            lcd_show_int_num(170,210,eat_time[eat_index][0],3,LCD_DARKBLUE,LCD_WHITE,32);
            lcd_show_string(230,210,":",LCD_DARKBLUE,LCD_WHITE,32,0);
            lcd_show_int_num(250,210,eat_time[eat_index][1],3,LCD_DARKBLUE,LCD_WHITE,32);


            break;
            case 1:
            
         lcd_show_picture(0,0,64,64,Light_picture);
        lcd_show_float_num1(0,84,lum,5,LCD_DARKBLUE,LCD_WHITE,24);
        lcd_show_picture(84,0,64,64,humidity_picture);
        lcd_show_float_num1(84,84,humi,4,LCD_DARKBLUE,LCD_WHITE,24);
        lcd_show_picture(168,0,51,64,temperature_picture);
        lcd_show_float_num1(168,84,temp,4,LCD_DARKBLUE,LCD_WHITE,24);
        lcd_show_picture(252,0,64,64,gas_picture);
        lcd_show_float_num1(252,84,gas,4,LCD_DARKBLUE,LCD_WHITE,24);

        lcd_show_string(0,140,"Time:",LCD_BROWN,LCD_WHITE,32,0);
        lcd_show_int_num(150,140,now_tm->tm_hour,2,LCD_DARKBLUE,LCD_WHITE,32);
        lcd_show_string(215,140,":",LCD_DARKBLUE,LCD_WHITE,32,0);
        lcd_show_int_num(248,140,now_tm->tm_min,2,LCD_DARKBLUE,LCD_WHITE,32);
            
            break;
            case 2:
            lcd_draw_line(50,0,50,240,LCD_BLACK);
            lcd_draw_line(140,0,140,240,LCD_BLACK);
            lcd_draw_line(230,0,230,240,LCD_BLACK);
            
            lcd_show_string(0,0,"button",LCD_DARKBLUE,dis2==0? LCD_GRAY:LCD_WHITE,16,0);

            lcd_show_int_num(51,69,storage_time[0],4,LCD_DARKBLUE,dis2==1? LCD_GRAY:LCD_WHITE,32);
            lcd_show_int_num(51,139,storage_time[1],4,LCD_DARKBLUE,dis2==2? LCD_GRAY:LCD_WHITE,32);
            lcd_show_int_num(51,209,storage_time[2],4,LCD_DARKBLUE,dis2==3? LCD_GRAY:LCD_WHITE,32);

            lcd_show_int_num(141,31,eat_time[0][0],3,LCD_DARKBLUE,dis2==4? LCD_GRAY:LCD_WHITE,16);
            lcd_show_string(173,31,":",LCD_DARKBLUE,LCD_WHITE,16,0);
            lcd_show_int_num(190,31,eat_time[0][1],3,LCD_DARKBLUE,dis2==5? LCD_GRAY:LCD_WHITE,16);
            lcd_show_int_num(141,101,eat_time[0][0],3,LCD_DARKBLUE,dis2==4? LCD_GRAY:LCD_WHITE,16);
            lcd_show_string(173,101,":",LCD_DARKBLUE,LCD_WHITE,16,0);
            lcd_show_int_num(190,101,eat_time[0][1],3,LCD_DARKBLUE,dis2==5? LCD_GRAY:LCD_WHITE,16);
            lcd_show_int_num(141,171,eat_time[0][0],3,LCD_DARKBLUE,dis2==4? LCD_GRAY:LCD_WHITE,16);
            lcd_show_string(173,171,":",LCD_DARKBLUE,LCD_WHITE,16,0);
            lcd_show_int_num(190,171,eat_time[0][1],3,LCD_DARKBLUE,dis2==5? LCD_GRAY:LCD_WHITE,16);

            lcd_show_int_num(141,54,eat_time[1][0],3,LCD_DARKBLUE,dis2==6? LCD_GRAY:LCD_WHITE,16);
            lcd_show_string(173,54,":",LCD_DARKBLUE,LCD_WHITE,16,0);
            lcd_show_int_num(190,54,eat_time[1][1],3,LCD_DARKBLUE,dis2==7? LCD_GRAY:LCD_WHITE,16);
             lcd_show_int_num(141,124,eat_time[1][0],3,LCD_DARKBLUE,dis2==6? LCD_GRAY:LCD_WHITE,16);
            lcd_show_string(173,124,":",LCD_DARKBLUE,LCD_WHITE,16,0);
            lcd_show_int_num(190,124,eat_time[1][1],3,LCD_DARKBLUE,dis2==7? LCD_GRAY:LCD_WHITE,16);
             lcd_show_int_num(141,194,eat_time[1][0],3,LCD_DARKBLUE,dis2==6? LCD_GRAY:LCD_WHITE,16);
            lcd_show_string(173,194,":",LCD_DARKBLUE,LCD_WHITE,16,0);
            lcd_show_int_num(190,194,eat_time[1][1],3,LCD_DARKBLUE,dis2==7? LCD_GRAY:LCD_WHITE,16);

            lcd_show_int_num(141,78,eat_time[2][0],3,LCD_DARKBLUE,dis2==8? LCD_GRAY:LCD_WHITE,16);
            lcd_show_string(173,78,":",LCD_DARKBLUE,LCD_WHITE,16,0);
            lcd_show_int_num(190,78,eat_time[2][1],3,LCD_DARKBLUE,dis2==9? LCD_GRAY:LCD_WHITE,16);
            lcd_show_int_num(141,148,eat_time[2][0],3,LCD_DARKBLUE,dis2==8? LCD_GRAY:LCD_WHITE,16);
            lcd_show_string(173,148,":",LCD_DARKBLUE,LCD_WHITE,16,0);
            lcd_show_int_num(190,148,eat_time[2][1],3,LCD_DARKBLUE,dis2==9? LCD_GRAY:LCD_WHITE,16);
            lcd_show_int_num(141,218,eat_time[2][0],3,LCD_DARKBLUE,dis2==8? LCD_GRAY:LCD_WHITE,16);
            lcd_show_string(173,218,":",LCD_DARKBLUE,LCD_WHITE,16,0);
            lcd_show_int_num(190,218,eat_time[2][1],3,LCD_DARKBLUE,dis2==9? LCD_GRAY:LCD_WHITE,16);

            lcd_show_int_num(240,31,eat_1[0],3,LCD_DARKBLUE,dis2==10? LCD_GRAY:LCD_WHITE,16);
            lcd_show_int_num(240,54,eat_1[1],3,LCD_DARKBLUE,dis2==11? LCD_GRAY:LCD_WHITE,16);
            lcd_show_int_num(240,78,eat_1[2],3,LCD_DARKBLUE,dis2==12? LCD_GRAY:LCD_WHITE,16);
            lcd_show_int_num(240,101,eat_2[0],3,LCD_DARKBLUE,dis2==13? LCD_GRAY:LCD_WHITE,16);
            lcd_show_int_num(240,124,eat_2[1],3,LCD_DARKBLUE,dis2==14? LCD_GRAY:LCD_WHITE,16);
            lcd_show_int_num(240,148,eat_2[2],3,LCD_DARKBLUE,dis2==15? LCD_GRAY:LCD_WHITE,16);
            lcd_show_int_num(240,171,eat_3[0],3,LCD_DARKBLUE,dis2==16? LCD_GRAY:LCD_WHITE,16);
            lcd_show_int_num(240,194,eat_3[1],3,LCD_DARKBLUE,dis2==17? LCD_GRAY:LCD_WHITE,16);
            lcd_show_int_num(240,218,eat_3[2],3,LCD_DARKBLUE,dis2==18? LCD_GRAY:LCD_WHITE,16);

            lcd_draw_line(0,30,320,30,LCD_BLACK);
            lcd_draw_line(0,100,320,100,LCD_BLACK);
            lcd_draw_line(0,170,320,170,LCD_BLACK);
            lcd_draw_line(140,53,320,53,LCD_BLACK);
            lcd_draw_line(140,77,320,77,LCD_BLACK);
            lcd_draw_line(140,123,320,123,LCD_BLACK);
            lcd_draw_line(140,147,320,147,LCD_BLACK);
            lcd_draw_line(140,193,320,193,LCD_BLACK);
            lcd_draw_line(140,217,320,217,LCD_BLACK);


            lcd_show_chinese(60,0,"存储时间",LCD_DARKBLUE,LCD_WHITE,16,0);
            lcd_show_chinese(150,0,"吃药时间",LCD_DARKBLUE,LCD_WHITE,16,0);
            lcd_show_chinese(240,0,"吃药数量",LCD_DARKBLUE,LCD_WHITE,16,0);

            lcd_show_chinese(0,70,"一号",LCD_DARKBLUE,LCD_WHITE,24,0);         
            lcd_show_chinese(0,140,"二号",LCD_DARKBLUE,LCD_WHITE,24,0);        
            lcd_show_chinese(0,210,"三号",LCD_DARKBLUE,LCD_WHITE,24,0);
            
            break;

        }
        
        
        //lcd_show_ui();
    }
}




/***************************************************************
 * 函数名称: smart_pill_box
 * 说    明: 开机自启动调用函数
 * 参    数: 无
 * 返 回 值: 无
 ***************************************************************/
void smart_pill_box()
{
   
    unsigned int thread_id_1;
    unsigned int thread_id_2;
    unsigned int thread_id_3;
    TSK_INIT_PARAM_S task_1 = {0};
    TSK_INIT_PARAM_S task_2 = {0};
    TSK_INIT_PARAM_S task_3 = {0};
    unsigned int ret = LOS_OK;

    smart_box_event_init();

    task_1.pfnTaskEntry = (TSK_ENTRY_FUNC)smart_box_thread;
    task_1.uwStackSize = 2048;
    task_1.pcName = "smart box thread";
    task_1.usTaskPrio = 24;
    
    ret = LOS_TaskCreate(&thread_id_1, &task_1);
    if (ret != LOS_OK)
    {
        printf("Falied to create task ret:0x%x\n", ret);
        return;
    }

    task_2.pfnTaskEntry = (TSK_ENTRY_FUNC)adc_key_thread;
    task_2.uwStackSize = 2048;
    task_2.pcName = "key thread";
    task_2.usTaskPrio = 24;
    ret = LOS_TaskCreate(&thread_id_2, &task_2);
    if (ret != LOS_OK)
    {
        printf("Falied to create task ret:0x%x\n", ret);
        return;
    }

    task_3.pfnTaskEntry = (TSK_ENTRY_FUNC)iot_thread;
    task_3.uwStackSize = 20480*5;
    task_3.pcName = "iot thread";
    task_3.usTaskPrio = 24;
    ret = LOS_TaskCreate(&thread_id_3, &task_3);
    if (ret != LOS_OK)
    {
        printf("Falied to create task ret:0x%x\n", ret);
        return;
    }

    unsigned int thread_id2;

    
    create_task(&thread_id2,1024 * 4,  20, sc_ntp_thread, NULL , "sc ntp");
}

APP_FEATURE_INIT(smart_pill_box);