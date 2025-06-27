#ifndef _IOT_H_
#define _IOT_H_

#include <stdbool.h>

typedef struct
{
    double illumination;
    double temperature;
    double humidity;
    double gas;
    bool box_state;
    
} e_iot_data;

#define IOT_CMD_BOX_ON 0x01
#define IOT_CMD_BOX_OFF 0x02

int wait_message();
void mqtt_init();
unsigned int mqtt_is_connected();
void send_msg_to_mqtt(e_iot_data *iot_data);
void sync_network_time();
#endif // _IOT_H_