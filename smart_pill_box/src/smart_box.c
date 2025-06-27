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

#include "smart_box.h"

#include <stdio.h>
#include <stdbool.h>

#include "iot_errno.h"

#include "iot_pwm.h"
#include "iot_gpio.h"
#include "su_03t.h"
#include "iot.h"
#include "lcd.h"
#include "picture.h"
#include "adc_key.h"
#include "lcd.h"
#include "string.h"


/***************************************************************
* 函数名称: lcd_dev_init
* 说    明: lcd初始化
* 参    数: 无
* 返 回 值: 无
***************************************************************/
void lcd_dev_init(void)
{
    lcd_init();
    lcd_fill(0, 0, LCD_W, LCD_H, LCD_WHITE);
}


/***************************************************************
* 函数名称: lcd_load_ui
* 说    明: 加载lcd ui
* 参    数: 无
* 返 回 值: 无
***************************************************************/
void lcd_show_ui(void)
{
        
}


