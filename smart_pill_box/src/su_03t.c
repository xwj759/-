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

#include "su_03t.h"

#include "los_task.h"
#include "ohos_init.h"

#include "iot_errno.h"
#include "iot_uart.h"

#include "smart_box.h"
#include "smart_box_event.h"

#include <stdio.h>
#include <stdint.h>
#include <stdbool.h>
#include <string.h>

#define UART2_HANDLE EUART2_M1

#define MSG_QUEUE_LENGTH                                16
#define BUFFER_LEN                                      50

// extern bool motor_state;
// extern bool light_state;
// extern bool auto_state;
// extern unsigned int m_su03_msg_queue;

/***************************************************************
* 函数名称: su_03t_thread
* 说    明: 语音模块处理线程
* 参    数: 无
* 返 回 值: 无
***************************************************************/
static void su_03t_thread(void *arg)
{
    IotUartAttribute attr;
    // double *data_ptr = NULL;
    unsigned int ret = 0;

    IoTUartDeinit(UART2_HANDLE);
    
    attr.baudRate = 115200;
    attr.dataBits = IOT_UART_DATA_BIT_8;
    attr.pad = IOT_FLOW_CTRL_NONE;
    attr.parity = IOT_UART_PARITY_NONE;
    attr.rxBlock = IOT_UART_BLOCK_STATE_BLOCK;
    attr.stopBits = IOT_UART_STOP_BIT_1;
    attr.txBlock = IOT_UART_BLOCK_STATE_BLOCK;
    
    ret = IoTUartInit(UART2_HANDLE, &attr);
    if (ret != IOT_SUCCESS)
    {
        printf("%s, %d: IoTUartInit(%d) failed!\n", __FILE__, __LINE__, ret);
        return;
    }

    event_info_t event = {0};
    event.event = event_su03t;

    while(1)
    {
        uint8_t data[64] = {0};
        uint8_t rec_len = IoTUartRead(UART2_HANDLE, data, sizeof(data));

      
        if (rec_len != 0)
        {
            uint16_t command = data[0] << 8 | data[1];
            event.data.su03t_data = command;
            smart_box_event_send(&event);
            
        }

        LOS_Msleep(500);
    }
}

/***************************************************************
* 函数名称: su03t_send_uchar_msg
* 说    明: 发送uchar类型数据到语音模块
* 参    数: 无
* 返 回 值: 无
***************************************************************/
void su03t_send_uchar_msg(uint8_t index, unsigned char *dat)
{
    uint8_t buf[50] = {0};
    uint8_t *buf_ptr = buf;
    uint8_t *u8_ptr = dat;

    *buf_ptr = 0xAA;
    buf_ptr++;
    *buf_ptr = 0x55;
    buf_ptr++;
    *buf_ptr = index;
    buf_ptr++;

    for (uint8_t i = 0; i < sizeof(dat); i++)
    {
        *buf_ptr = u8_ptr[i];
        buf_ptr++;
    }

    *buf_ptr = 0x55;
    buf_ptr++;
    *buf_ptr = 0xAA;

    IoTUartWrite(UART2_HANDLE, buf, sizeof(buf));
}

/***************************************************************
* 函数名称: su03t_init
* 说    明: 语音模块初始化
* 参    数: 无
* 返 回 值: 无
***************************************************************/
void su03t_init(void)
{
    unsigned int thread_id;
    TSK_INIT_PARAM_S task = {0};
    unsigned int ret = LOS_OK;

    task.pfnTaskEntry = (TSK_ENTRY_FUNC)su_03t_thread;
    task.uwStackSize = 2048;
    task.pcName = "su-03t thread";
    task.usTaskPrio = 24;
    ret = LOS_TaskCreate(&thread_id, &task);
    if (ret != LOS_OK)
    {
        printf("Falied to create task ret:0x%x\n", ret);
        return;
    }
}
