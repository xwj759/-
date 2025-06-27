#ifndef _DRV_STEERING_H__
#define _DRV_STEERING_H__

#include  "stdbool.h"

int get_steering_state(void);
void steering_set_state(bool state);
void steering_dev_init(void);

#endif