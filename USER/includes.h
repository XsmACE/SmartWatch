#ifndef __INCLUDES_H__
#define __INCLUDES_H__

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <stdint.h>

#include "sys.h"
#include "delay.h"
#include "usart.h"
#include "led.h"
#include "dht11.h"
#include "FreeRTOS.h"
#include "task.h"
#include "semphr.h"
#include "beep.h"
#include "queue.h"
#include "event_groups.h"
#include "key.h"
#include "tft.h"
#include "pic.h"
#include "rtc.h"
#include "menu.h"
#include "adc.h"
#include "max30102.h"
#include "i2c.h"
#include "algorithm.h"
#include "ble.h"
#include "timers.h"
#include "inv_mpu.h"
#include "inv_mpu_dmp_motion_driver.h"
#include "mpu6050.h"
#include "mpu6050_i2c.h"



/* 宏定义 */
#define EVENT_GROUP_KEY1_DOWN		0x01
#define EVENT_GROUP_KEY2_DOWN		0x02
#define EVENT_GROUP_KEY3_DOWN		0x04
#define EVENT_GROUP_KEY4_DOWN		0x08
#define EVENT_GROUP_KEYALL_DOWN		0x0F


#define EVENT_GROUP_FN_KEY_MENU_ENTER 0x100 //S1
#define EVENT_GROUP_FN_KEY_MENU_BACK  0x200 //S2

#define EVENT_GROUP_FN_KEY_UP       0x10000
#define EVENT_GROUP_FN_KEY_DOWN     0x20000
#define EVENT_GROUP_FN_KEY_ENTER	0x40000 //S3
#define EVENT_GROUP_FN_KEY_BACK		0x80000 //S4

#define EVENT_GROUP_RTC_WAKEUP		0x10
#define EVENT_GROUP_RTC_ALARM		0x20

#define FLAG_DHT_GET_NONE			0
#define FLAG_DHT_GET_TEMP			1
#define FLAG_DHT_GET_HUMI			2

#define FLAG_RTC_GET_NONE			0
#define FLAG_RTC_GET_DATE			1
#define FLAG_RTC_GET_TIME			2

#define FLAG_ALARM_SET_NONE         0
#define FLAG_ALARM_SET_START        1

#define FLAG_BLE_STATUS_NONE        0
#define FLAG_BLE_STATUS_CONNECT     1


#define FLAG_STEP_SET_NONE          0
#define FLAG_STEP_SET_START         1


#define MAX_BRIGHTNESS 255

typedef struct __task_t
{
	TaskFunction_t pxTaskCode;
	const char * const pcName;		
	const configSTACK_DEPTH_TYPE usStackDepth;
	void * const pvParameters;
	UBaseType_t uxPriority;
	TaskHandle_t * const pxCreatedTask;
}task_t;


typedef struct __beep_t
{
	uint32_t sta;				//1-工作 0-停止
	uint32_t duration;			//持续时间，单位毫秒
}beep_t;

extern GPIO_InitTypeDef  	GPIO_InitStructure;
extern NVIC_InitTypeDef 	NVIC_InitStructure;		
extern SPI_InitTypeDef  	SPI_InitStructure;
extern USART_InitTypeDef 	USART_InitStructure;


//变量
extern volatile uint32_t g_dht_get_what;
extern volatile uint32_t g_rtc_get_what;
extern volatile uint32_t g_alarm_set;
extern volatile uint32_t g_alarm_pic;
extern volatile uint32_t g_ble_status;
extern volatile uint32_t g_step_status;

/* 任务句柄 */
extern TaskHandle_t app_task_init_handle;
extern TaskHandle_t app_task_key_handle;
extern TaskHandle_t app_task_usart_handle;
extern TaskHandle_t app_task_dht11_handle;
extern TaskHandle_t app_task_beep_handle;
extern TaskHandle_t app_task_rtc_handle;
extern TaskHandle_t app_task_menu_handle;
extern TaskHandle_t app_task_menu_show_handle;
extern TaskHandle_t app_task_max30102_handle;
extern TaskHandle_t app_task_mpu6050_handle;
extern TaskHandle_t app_task_mpu6050_step_handle;

/* 互斥型信号量句柄 */
extern SemaphoreHandle_t g_mutex_printf;
extern SemaphoreHandle_t g_mutex_lcd;

/* 消息队列句柄 */
extern QueueHandle_t g_queue_usart;
extern QueueHandle_t g_queue_beep;


/* 事件标志组句柄 */
extern EventGroupHandle_t g_event_group;


/* lcd互斥锁高度封装 */
#define LCD_SAFE(__CODE)                                \
		do                                               \
		{                                                \
			xSemaphoreTake(g_mutex_lcd, portMAX_DELAY); \
			__CODE;                                  	\
			xSemaphoreGive(g_mutex_lcd);                \
		} while (0)                                      \


#endif
