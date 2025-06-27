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


#include "ntp.h"
#include <stdint.h>
#include "lwip/netifapi.h"

#include "lwip/sockets.h"
#include <netdb.h>
#include <stddef.h>
#include <time.h>


#define NTP_TIMESTAMP_DELTA             2208988800ull       // 15 January 1900

#define NTP_SERVER_1                    "time.windows.com"  // NTP服务端，国内推荐使用
#define NTP_SERVER_2                    "time.ustc.edu.cn"  // NTP服务端，国内推荐使用
#define NTP_SERVER_3                    "cn.ntp.org.cn"     // NTP服务端
#define NTP_SERVER_4                    "ntp.aliyun.com"    // NTP服务端

#define NTP_SERVER_NUM                  4                   // NTP服务端数量
// ntp服务器列表
char ntp_server_list[][32] =
{
    NTP_SERVER_1,
    NTP_SERVER_2,
    NTP_SERVER_3,
    NTP_SERVER_4
};

#define LI(packet)   (uint8_t) ((packet.li_vn_mode & 0xC0) >> 6) /* (li   & 11 000 000) >> 6 */
#define VN(packet)   (uint8_t) ((packet.li_vn_mode & 0x38) >> 3) /* (vn   & 00 111 000) >> 3 */
#define MODE(packet) (uint8_t) ((packet.li_vn_mode & 0x07) >> 0) /* (mode & 00 000 111) >> 0 */

/* 定义ntp的数据格式 */
typedef struct
{
    uint8_t li_vn_mode;      /* Eight bits. li, vn, and mode */
    /* li.   Two bits.   Leap indicator */
    /* vn.   Three bits. Version number of the protocol */
    /* mode. Three bits. Client will pick mode 3 for client */
    
    uint8_t stratum;         /* Eight bits. Stratum level of the local clock */
    uint8_t poll;            /* Eight bits. Maximum interval between successive messages */
    uint8_t precision;       /* Eight bits. Precision of the local clock */
    
    uint32_t rootDelay;      /* 32 bits. Total round trip delay time */
    uint32_t rootDispersion; /* 32 bits. Max error aloud from primary clock source */
    uint32_t refId;          /* 32 bits. Reference clock identifier */
    
    uint32_t refTm_s;        /* 32 bits. Reference time-stamp seconds */
    uint32_t refTm_f;        /* 32 bits. Reference time-stamp fraction of a second */
    
    uint32_t origTm_s;       /* 32 bits. Originate time-stamp seconds */
    uint32_t origTm_f;       /* 32 bits. Originate time-stamp fraction of a second */
    
    uint32_t rxTm_s;         /* 32 bits. Received time-stamp seconds */
    uint32_t rxTm_f;         /* 32 bits. Received time-stamp fraction of a second */
    
    uint32_t txTm_s;         /* 32 bits and the most important field the client cares about. Transmit time-stamp seconds */
    uint32_t txTm_f;         /* 32 bits. Transmit time-stamp fraction of a second */
    
} ntp_packet;                   /* Total: 384 bits or 48 bytes */

/* 定义数据大小端模式*/
typedef enum
{
    data_little_endian,     /* 小端模式 */
    data_big_endian         /* 大端模式 */
} data_endian;

// 定义socket套接字
static int m_ntp_sockfd = -1;

// 定义ntp数据包
static ntp_packet packet = { 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 };


/***************************************************************
* 函数名称: fill_uint32_data
* 说    明: 将uint8_t数组填充uint32_t类型的数据
* 参    数:
*         @bytes    uint8_t数组
*         @endian   数据大小端模式
* 返 回 值: 返回uint32_t类型的数据
***************************************************************/
uint32_t fill_uint32_data(uint8_t *bytes, data_endian endian)
{
    uint32_t result = 0;
    if (endian == data_big_endian)
    {
        result = (bytes[0] << 24) | (bytes[1] << 16) | (bytes[2] << 8) | (bytes[3] << 0);
    }
    else
    {
        result = (bytes[3] << 24) | (bytes[2] << 16) | (bytes[1] << 8) | (bytes[0] << 0);
    }
    
    return result;
}


/***************************************************************
* 函数名称: fill_uint16_data
* 说    明: 将uint8_t数组填充uint16_t类型的数据
* 参    数:
*         @bytes    uint8_t数组
*         @endian   数据大小端模式
* 返 回 值: 返回uint16_t类型的数据
***************************************************************/
uint16_t fill_uint16_data(uint8_t *bytes, data_endian endian)
{
    uint16_t result = 0;
    if (endian == data_big_endian)
    {
        result = (bytes[0] << 8) | (bytes[1] << 0);
    }
    else
    {
        result = (bytes[1] << 8) | (bytes[0] << 0);
    }
    
    return result;
}


/***************************************************************
* 函数名称: sendto_ntp_server
* 说    明: 发送数据包到ntp服务器,传入socket和服务器域名
* 参    数:
*         @sockfd       socket套接字句柄
*         @host_name    服务器域名
*         @serv_addr    服务器地址
* 返 回 值: 0为成功，-1为失败
***************************************************************/
static int sendto_ntp_server(int sockfd, const char *host_name, struct sockaddr_in *serv_addr)
{
    struct hostent *server;
    socklen_t addr_len = sizeof(struct sockaddr_in);
    /* NTP服务的 UDP 端口号 */
    int ntp_port = 123;
    int rc = 0;
    sa_family_t family = AF_INET;
    struct addrinfo *result = NULL;
    struct addrinfo hints = {0, AF_UNSPEC, SOCK_DGRAM, 0, 0, NULL, NULL, NULL};
    
    /* 获取服务器域名对应的IP地址*/
    if ((rc = getaddrinfo(host_name, NULL, &hints, &result)) == 0)
    {
        struct addrinfo *res = result;
        
        /* prefer ip4 addresses */
        while (res)
        {
            if (res->ai_family == AF_INET)
            {
                result = res;
                break;
            }
            res = res->ai_next;
        }
        
        if (result->ai_family == AF_INET)
        {
            serv_addr->sin_port = htons(ntp_port);
            serv_addr->sin_family =  AF_INET;
            serv_addr->sin_addr = ((struct sockaddr_in *)(result->ai_addr))->sin_addr;
        }
        else
        {
            rc = -1;
        }
        
        freeaddrinfo(result);
    }
    if (rc != -1)
    {
        rc = sendto(sockfd, (char *) &packet, sizeof(ntp_packet), 0, (const struct sockaddr *)serv_addr, addr_len);
    }
    
    return rc;
}


/***************************************************************
* 函数名称: ntp_init
* 说    明: NTP服务初始化
* 参    数: 无
* 返 回 值: 0为成功，-1为失败
***************************************************************/
int ntp_init()
{
    int ret;
    struct timeval timeout  =
    {
        .tv_sec             = 1,
        .tv_usec            = 0,
    };
    
    // 创建 UDP socket
    m_ntp_sockfd = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);
    if (m_ntp_sockfd < 0)
    {
        printf("ntp Create socket failed\n");
        return -1;
    }
    // 设置接收数据包超时时间为1秒
    ret = setsockopt(m_ntp_sockfd, SOL_SOCKET, SO_RCVTIMEO, (const char *)&timeout, sizeof(timeout));
    if (ret != 0)
    {
        printf("setsockopt fail, ret[%d]-%d!\n", ret, __LINE__);
    }
    
    return 0;
}


/***************************************************************
* 函数名称: ntp_reinit
* 说    明: NTP服务销毁
* 参    数: 无
* 返 回 值: 0为成功，-1为失败
***************************************************************/
int ntp_reinit()
{
    lwip_close(m_ntp_sockfd);
    return ntp_init();
}


/**
 * 从ntp服务器获取时间
 *
 * @param host_name ntp服务器地址, NULL: 使用默认的ntp服务地址
 *
 * @return >0: 获取到的ntp时间
 *         =0: 获取ntp时间失败
 */
uint32_t ntp_get_time(const char *host_name)
{
    int sockfd, n, i = 0, server_num = 0;
    struct sockaddr_in serv_addr[NTP_SERVER_NUM];
    
    uint32_t new_time = 0;
    socklen_t addr_len = sizeof(struct sockaddr_in);
    
    /* Create and zero out the packet. All 48 bytes worth. */
    memset(&packet, 0, sizeof(ntp_packet));
    
    /* Set the first byte's bits to 00,011,011 for li = 0, vn = 3, and mode = 3. The rest will be left set to zero.
       Represents 27 in base 10 or 00011011 in base 2. */
    *((char *) &packet + 0) = 0x1b;
    
#define NTP_INTERNET           0x02
#define NTP_INTERNET_BUFF_LEN  18
    
    uint8_t send_data[NTP_INTERNET_BUFF_LEN] = {0};
    uint16_t check = 0;
    send_data[0] = NTP_INTERNET;
    
    /* 获取校验码的值 */
    for (uint8_t index = 0; index < NTP_INTERNET_BUFF_LEN - sizeof(check); index++)
    {
        check += (uint8_t)send_data[index];
    }
    send_data[NTP_INTERNET_BUFF_LEN - 2] = check >> 8;
    send_data[NTP_INTERNET_BUFF_LEN - 1] = check & 0xFF;
    
    memcpy(((char *)&packet + 4), send_data, NTP_INTERNET_BUFF_LEN);
    
    /* 创建 UDP socket. */
    sockfd = m_ntp_sockfd;
    if (sockfd < 0)
    {
        printf("ntp Create socket failed\n");
        return 0;
    }
    
    // 如果host_name参数不为NULL
    if (host_name)
    {
        /* 直接发送到对应的ntp服务器 */
        if (sendto_ntp_server(sockfd, host_name, serv_addr) >= 0)
        {
            server_num = 1;
        }
    }
    else
    {
        /* 使用默认的ntp服务器分别发送 */
        for (i = 0; i < NTP_SERVER_NUM; i++)
        {
            if (ntp_server_list[i] == NULL || strlen(ntp_server_list[i]) == 0)
            {
                continue;
            }
            
            if (sendto_ntp_server(sockfd, ntp_server_list[i], &serv_addr[server_num]) >= 0)
            {
                server_num++;
            }
        }
    }
    LOS_Msleep(500);
    // 等待ntp服务器返回数据
    for (i = 0; i < server_num; i++)
    {
        uint8_t recv[48] = {0};
        /* non-blocking receive the packet back from the server. If n == -1, it failed. */
        n = recvfrom(sockfd, recv, sizeof(ntp_packet), 0, (struct sockaddr *)&serv_addr[i], &addr_len);
        if (n <= 0)
        {
            struct in_addr addr;
            memcpy(&addr, &(serv_addr[i]).sin_addr.s_addr, sizeof(addr));
        }
        else if (n > 0)
        {
            struct in_addr addr;
            memcpy(&addr, &(serv_addr[i]).sin_addr.s_addr, sizeof(addr));
            memcpy(&packet, recv, sizeof(ntp_packet));
            packet.txTm_s = fill_uint32_data(&recv[40], data_big_endian) + i + 1;
        }
        
        usleep(100);
    }
    
    // 将值转化为时间
    new_time = ((uint32_t)packet.txTm_s - NTP_TIMESTAMP_DELTA);
    return (uint32_t)new_time;
}
