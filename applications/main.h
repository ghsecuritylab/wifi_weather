#ifndef __MAIN_H
#define __MAIN_H


#include <rtthread.h>
#include <rtdevice.h>
#include <board.h>
#include <msh.h>
#include <easyflash.h>
#include <fal.h>
#include <fal_def.h>
#include <dfs_fs.h>
#include <dfs_file.h>
#include <at.h>
#include <at_socket.h>
#include <fontupd.h>
#include "button.h"
#include "aht10.h"
#include "ap3216c.h"
#include "drv_lcd.h"

#define MAX_DAY   3

#define FS_PARTITION_NAME "filesystem" //SPIFLASH

#define MAX_PIC_NUM    40  //weather PICTURE
#define PIC_NAME_LEN   11

#define MAX_LCD_PIC    5 //LCD display picture
extern volatile uint8_t  change_picture;

/*led*/
#define RT_LED_THREAD_STACK_SIZE 4096
#define RT_LED_THREAD_PRIORITY 1
void led_thread_entry(void *parameter);

/*key*/
#define RT_KEY_THREAD_STACK_SIZE 1024
#define RT_KEY_THREAD_PRIORITY 2
void button_thread_entry(void *parameter);

/*sensor*/
#define RT_SENSOR_THREAD_STACK_SIZE 1024
#define RT_SENSOR_THREAD_PRIORITY 3
void sensor_thread_entry(void *parameter);

/*lcd*/
#define RT_LCD_THREAD_STACK_SIZE 4096
#define RT_LCD_THREAD_PRIORITY 4
void lcd_thread_entry(void *parameter);

/*wifi*/

#define RT_ESP_THREAD_STACK_SIZE 4096
#define RT_ESP_THREAD_PRIORITY 5
void esp_thread_entry(void *parameter);




#define AT_SEND_CMD(resp, cmd)                                                                          \
    do                                                                                                  \
    {                                                                                                   \
        if (at_exec_cmd(at_resp_set_info(resp, 256, 0, rt_tick_from_millisecond(5000)), cmd) < 0)       \
        {                                                                                               \
            rt_kprintf("RT AT send commands(%s) error!", cmd);                                               \
            result = -RT_ERROR;                                                                         \
            goto __exit;                                                                                \
        }                                                                                               \
    } while(0);                                                                                         \
      
#pragma  pack(1)
typedef struct 
{
    uint8_t start_x;
    uint8_t start_y;
    uint8_t end_x;
    uint8_t end_y;
}LCD_PIC;


typedef struct 
{
    uint8_t month;
    uint8_t day; 
    uint8_t hour; 
    uint8_t min; 
    uint8_t sec;
    uint16_t year;
    uint8_t  week[10];
}RTC_TIME;


typedef struct 
{
    uint8_t pic;
    uint8_t temperature;
    char city[16];
    char weather[8];
}ONE_WEATHER;


typedef struct 
{
    char  date[MAX_DAY][12];  //date
    char  text_day[MAX_DAY][12]; //白天天气
    char  code_day[MAX_DAY];     //天气代码
    char  text_night[MAX_DAY][12]; //夜晚天气
    char  code_night[MAX_DAY];     //天气代码
    char  high_tem[MAX_DAY];       //最高温
    char  low_tem[MAX_DAY];        //最低温
    char  wind_speed[MAX_DAY][6];    //风速
    char  wind_scale[MAX_DAY];    //风力等级
    char  wind_degree[MAX_DAY];     //风向角
    char  wind_direction[MAX_DAY][8];
    char humidity[MAX_DAY];//相对湿度，0~100，单位为百分比
}THREE_WEAHTER;
#pragma  pack()

extern LCD_PIC pic_coord;
extern RTC_TIME rtc_time;
extern ONE_WEATHER one_weather;
extern THREE_WEAHTER three_weather;

#endif