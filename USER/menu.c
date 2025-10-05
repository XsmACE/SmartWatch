#include "includes.h"

//菜单名
static void item_time_date(void);
static void item_temp_humi(void);
static void item_step(void);
static void item_pulse(void);
static void item_alarm(void);
static void item_temp(void);
static void item_humi(void);
static void item_time(void);
static void item_date(void);
static void item_no(void);
static void item_off(void);

//菜单
static menu_t menu_temp_humi[];
static menu_t menu_dht_temp[];
static menu_t menu_dht_humi[];

static menu_t menu_time_date[];
static menu_t menu_rtc_time[];
static menu_t menu_rtc_date[];

static menu_t menu_alarm[];
static menu_t menu_pulse[];

static menu_t menu_step[];
static menu_t menu_step_sta[];

//菜单任务函数
static void menu_dht_fun(void* pvParameters);
static void menu_rtc_fun(void* pvParameters);
static void menu_alarm_fun(void* pvParameters);
static void menu_pulse_fun(void* pvParameters);
static void menu_step_fun(void* pvParameters);

/*一级菜单*/
menu_t menu_main_1[]=
{
	/*  名字             图标,              函数  左   右    父  子      */
	{item_time_date	,gImage_date_48x48,     NULL,NULL,NULL,NULL,menu_time_date},	
	{item_temp_humi	,gImage_temp_humi_48x48,NULL,NULL,NULL,NULL,menu_temp_humi},
	{item_step		,gImage_step_48x48,     NULL,NULL,NULL,NULL,menu_step},
	{item_pulse		,gImage_pulse_48x48,    NULL,NULL,NULL,NULL,menu_pulse},	
	{item_alarm		,gImage_alarm_48x48,    NULL,NULL,NULL,NULL,menu_alarm},	
	{NULL,NULL,NULL,NULL,NULL,NULL,NULL},
};


menu_t *menu_main_tbl[]={
	menu_main_1,
	NULL,
};

/*二级菜单*/
//温湿度
static menu_t menu_temp_humi[]=
{
	{item_temp,gImage_temperature_48x48,NULL,NULL,NULL,menu_main_1,menu_dht_temp},
	{item_humi,gImage_humidity_48x48,   NULL,NULL,NULL,menu_main_1,menu_dht_humi},
	{NULL,NULL,NULL,NULL,NULL,NULL,NULL},	
};

//时间日期
static menu_t menu_time_date[]=
{
	{item_time,gImage_time_48x48,   NULL,NULL,NULL,menu_main_1,menu_rtc_time},
	{item_date,gImage_date_48x48,   NULL,NULL,NULL,menu_main_1,menu_rtc_date},
	{NULL,NULL,NULL,NULL,NULL,NULL,NULL},	
};

//闹钟
static menu_t menu_alarm[]=
{
	{NULL,NULL, menu_alarm_fun ,NULL,NULL,menu_main_1,NULL},
	{NULL,NULL,NULL,NULL,NULL,NULL,NULL},	
};

//心率血氧
static menu_t menu_pulse[]=
{
	{NULL,NULL, menu_pulse_fun ,NULL,NULL,menu_main_1,NULL},
	{NULL,NULL,NULL,NULL,NULL,NULL,NULL},	
};

//运动计步
static menu_t menu_step[]=
{
	{item_no, gImage_step_no_48x48, NULL ,NULL,NULL,menu_main_1,menu_step_sta},
	{item_off,gImage_step_off_48x48, NULL ,NULL,NULL,menu_main_1,menu_step_sta},
	{NULL,NULL,NULL,NULL,NULL,NULL,NULL},	
};

/*三级菜单*/
//温度
static menu_t menu_dht_temp[]=
{
	{NULL,NULL,menu_dht_fun,NULL,NULL,menu_temp_humi,NULL},
	{NULL,NULL,NULL,NULL,NULL,NULL,NULL},	
};

//湿度
static menu_t menu_dht_humi[]=
{
	{NULL,NULL,menu_dht_fun,NULL,NULL,menu_temp_humi,NULL},
	{NULL,NULL,NULL,NULL,NULL,NULL,NULL},	
};

//时间
static menu_t menu_rtc_time[]=
{
	{NULL,NULL,menu_rtc_fun,NULL,NULL,menu_time_date,NULL},
	{NULL,NULL,NULL,NULL,NULL,NULL,NULL},	
};

//日期
static menu_t menu_rtc_date[]=
{
	{NULL,NULL,menu_rtc_fun,NULL,NULL,menu_time_date,NULL},
	{NULL,NULL,NULL,NULL,NULL,NULL,NULL},	
};

//运动计步
static menu_t menu_step_sta[]=
{
	{NULL,NULL, menu_step_fun ,NULL,NULL,menu_step,NULL},
	{NULL,NULL,NULL,NULL,NULL,NULL,NULL},	
};



/*汉字标签*/

//设置时间
static void item_time_date(void)
{
	uint16_t x=0;
	uint16_t y=0;
	
	x=60; lcd_show_chn(x,y+6,3,BLACK,WHITE,32);//设
	x+=40;lcd_show_chn(x,y+6,4,BLACK,WHITE,32);//置
	x+=40;lcd_show_chn(x,y+6,5,BLACK,WHITE,32);//时
	x+=40;lcd_show_chn(x,y+6,6,BLACK,WHITE,32);//间
}

//温湿度
static void item_temp_humi(void)
{
	uint16_t x=0;
	uint16_t y=0;
	
	y+=60;
	x=60; lcd_show_chn(x,y+6,7,BLACK,WHITE,32);
	x+=40;lcd_show_chn(x,y+6,8,BLACK,WHITE,32);
	x+=40;lcd_show_chn(x,y+6,9,BLACK,WHITE,32);	
}

//运动模式
static void item_step(void)
{
	uint16_t x=0;
	uint16_t y=0;
	
	y+=120;
	x=60; lcd_show_chn(x,y+6,10,BLACK,WHITE,32);
	x+=40;lcd_show_chn(x,y+6,11,BLACK,WHITE,32);
	x+=40;lcd_show_chn(x,y+6,20,BLACK,WHITE,32);
	x+=40;lcd_show_chn(x,y+6,21,BLACK,WHITE,32);
}

//心率血氧
static void item_pulse(void)
{
	uint16_t x=0;
	uint16_t y=0;
	
	y+=180;
	x=60; lcd_show_chn(x,y+6,12,BLACK,WHITE,32);
	x+=40;lcd_show_chn(x,y+6,13,BLACK,WHITE,32);
	x+=40;lcd_show_chn(x,y+6,14,BLACK,WHITE,32);
	x+=40;lcd_show_chn(x,y+6,15,BLACK,WHITE,32);	
}

//闹钟
static void item_alarm(void)
{
	uint16_t x=0;
	uint16_t y=0;
	
	y+=240;
	x=60; lcd_show_chn(x,y+6,16,BLACK,WHITE,32);
	x+=60;lcd_show_chn(x,y+6,17,BLACK,WHITE,32);	
}


//温度
static void item_temp(void)
{
	uint16_t x=0;
	uint16_t y=0;
	
	
	x=60; lcd_show_chn(x,y+6,7,BLACK,WHITE,32);
	x+=60;lcd_show_chn(x,y+6,9,BLACK,WHITE,32);	
}

//湿度
static void item_humi(void)
{
	uint16_t x=0;
	uint16_t y=0;
	
	y+=60;
	x=60; lcd_show_chn(x,y+6,8,BLACK,WHITE,32);
	x+=60;lcd_show_chn(x,y+6,9,BLACK,WHITE,32);	
}

//时间
static void item_time(void)
{
	uint16_t x=0;
	uint16_t y=0;
	
	x=60; lcd_show_chn(x,y+6,5,BLACK,WHITE,32);
	x+=60;lcd_show_chn(x,y+6,6,BLACK,WHITE,32);	
}

//日期
static void item_date(void)
{
	uint16_t x=0;
	uint16_t y=0;
	
	y+=60;
	x=60; lcd_show_chn(x,y+6,18,BLACK,WHITE,32);
	x+=60;lcd_show_chn(x,y+6,19,BLACK,WHITE,32);	
}

//开启
static void item_no(void)
{
	uint16_t x=0;
	uint16_t y=0;
	
	x=60; lcd_show_chn(x,y+6,22,BLACK,WHITE,32);//开
	x+=40;lcd_show_chn(x,y+6,23,BLACK,WHITE,32);//启
	
}

//关闭
static void item_off(void)
{
	uint16_t x=0;
	uint16_t y=0;
	
	y+=60;
	x=60; lcd_show_chn(x,y+6,24,BLACK,WHITE,32);//开
	x+=40;lcd_show_chn(x,y+6,25,BLACK,WHITE,32);//启
	
}

/*功能函数*/

//运动计步
static void menu_step_fun(void* pvParameters)
{
	menu_ext_t *menu_ext = (menu_ext_t *)pvParameters;
	

	if(menu_ext->key_fn==KEY_ENTER)
	{
		if(menu_ext->item_cursor==0)
		{
			g_step_status = FLAG_STEP_SET_START;
			
			lcd_draw_picture(70,90,100,100,gImage_succee_100x100);	
			
			BEEP(1);
			delay_ms(200);
			BEEP(0);
			delay_ms(200);
			BEEP(1);
			delay_ms(200);
			BEEP(0);
			
			
			/* 恢复mpu6050_step任务运行 */ 
			vTaskResume(app_task_mpu6050_step_handle);
		}
			
		
		
		if(menu_ext->item_cursor == 1)
		{
			g_step_status = FLAG_STEP_SET_NONE;
			
			lcd_draw_picture(70,90,100,100,gImage_fail_100x100);	
			
			BEEP(1);
			delay_ms(1000);
			BEEP(0);
			/* 挂起mpu6050_step任务运行 */
			vTaskSuspend(app_task_mpu6050_step_handle);
		}
			
		
		//挂起menu任务
		vTaskSuspend(app_task_menu_show_handle);
	}

	/* 识别到BACK按键编码,则停止DHT任务运行 */	
	if(menu_ext->key_fn == KEY_BACK)
	{
		
		//恢复menu任务
		vTaskResume(app_task_menu_show_handle);
	}	
	
}


//温湿度任务
static void menu_dht_fun(void* pvParameters)
{
	menu_ext_t *menu_ext = (menu_ext_t *)pvParameters;
	
	//清屏
	LCD_SAFE(
		/* 清屏 */
		lcd_clear(WHITE);
		lcd_fill(0,LCD_HEIGHT,LCD_WIDTH,80,WHITE);
		
	);
	
	if(menu_ext->key_fn==KEY_ENTER)
	{
		if(menu_ext->item_cursor == 0)
		{
			g_dht_get_what = FLAG_DHT_GET_TEMP;
			
			lcd_draw_picture(70,30,100,80,gImage_temp_100x80);
			
		}
		
		if(menu_ext->item_cursor == 1)
		{
			g_dht_get_what = FLAG_DHT_GET_HUMI;
			lcd_draw_picture(70,30,100,93,gImage_humi_100x93);
		}
		
		
		if(g_dht_get_what!=FLAG_DHT_GET_TEMP && g_dht_get_what!=FLAG_DHT_GET_HUMI)
			return;			

		/* 恢复DHT任务运行 */ 
		vTaskResume(app_task_dht11_handle);
		//挂起menu任务
		vTaskSuspend(app_task_menu_show_handle);
	}

	/* 识别到BACK按键编码,则停止DHT任务运行 */	
	if(menu_ext->key_fn == KEY_BACK)
	{
		
		g_dht_get_what=FLAG_DHT_GET_NONE;
		/* 挂起DHT任务 */
		vTaskSuspend(app_task_dht11_handle);
		
		//恢复menu任务
		vTaskResume(app_task_menu_show_handle);
	}	
	
	
}

//设置时间日期任务
static void menu_rtc_fun(void* pvParameters)
{
	menu_ext_t *menu_ext = (menu_ext_t *)pvParameters;
	
		
	//清屏
	LCD_SAFE(
		/* 清屏 */
		lcd_clear(WHITE);
		lcd_fill(0,LCD_HEIGHT,LCD_WIDTH,80,WHITE);
		
	);
	
	if(menu_ext->key_fn==KEY_ENTER)
	{
		//时间设置
		if(menu_ext->item_cursor == 0)
		{
			g_rtc_get_what = FLAG_RTC_GET_TIME;
			
			lcd_draw_picture(90,90,48,48,gImage_time_48x48);	
	
			
			
		}
		
		//日期设置
		if(menu_ext->item_cursor == 1)
		{
			g_rtc_get_what = FLAG_RTC_GET_DATE;
			
			lcd_draw_picture(90,90,48,48,gImage_date_48x48);	
	
		}
		
		
		if(g_rtc_get_what!=FLAG_RTC_GET_DATE && g_rtc_get_what!=FLAG_RTC_GET_TIME)
			return;	

		//挂起menu任务
		vTaskSuspend(app_task_menu_show_handle);
	}
	
	
	if(menu_ext->key_fn == KEY_BACK)
	{
		g_rtc_get_what=FLAG_RTC_GET_NONE;
		//恢复menu任务
		vTaskResume(app_task_menu_show_handle);
	}	
	
	
}

//设置闹钟任务
static void menu_alarm_fun(void* pvParameters)
{
	menu_ext_t *menu_ext = (menu_ext_t *)pvParameters;
	
		
	//清屏
	LCD_SAFE(
		/* 清屏 */
		lcd_clear(WHITE);
		lcd_fill(0,LCD_HEIGHT,LCD_WIDTH,80,WHITE);
		
	);
	
	if(menu_ext->key_fn==KEY_ENTER)
	{
		//闹钟图标
		lcd_draw_picture(90,90,48,48,gImage_alarm_48x48);	
	
		g_alarm_set=FLAG_ALARM_SET_START;
		//挂起menu任务
		vTaskSuspend(app_task_menu_show_handle);
		
		if(g_alarm_set!=FLAG_ALARM_SET_START)
			return;	
		
	}
	
	if(menu_ext->key_fn==KEY_BACK)
	{
		g_alarm_set=FLAG_ALARM_SET_NONE;
		
		//恢复menu任务
		vTaskResume(app_task_menu_show_handle);
	}
	
	
}

//心率血氧任务
static void menu_pulse_fun(void* pvParameters)
{
	menu_ext_t *menu_ext = (menu_ext_t *)pvParameters;
	
		
	//清屏
	LCD_SAFE(
		/* 清屏 */
		lcd_clear(WHITE);
		lcd_fill(0,LCD_HEIGHT,LCD_WIDTH,80,WHITE);
		
	);
	
	if(menu_ext->key_fn==KEY_ENTER)
	{
		//心率血氧图标
		lcd_draw_picture(90,80,48,48,gImage_pulse_48x48);	
	
		//恢复max30102任务
		vTaskResume(app_task_max30102_handle);
		
		//挂起menu任务
		vTaskSuspend(app_task_menu_show_handle);
		//vTaskSuspend(app_task_rtc_handle);

		
	}
	
	if(menu_ext->key_fn==KEY_BACK)
	{
		
		//挂起max30102任务
		vTaskSuspend(app_task_max30102_handle);
		
		//恢复menu任务
		vTaskResume(app_task_menu_show_handle);
		//vTaskResume(app_task_rtc_handle);
		
	}
}

/**
 * @brief 获取当前菜单的项目总数
 * @param .menu 指向当前菜单
 * @retval 当前菜单项目总数
 * @details 无
 */
uint32_t menu_item_total(menu_t *menu)
{
	menu_t *m = menu;

	uint32_t item_count=0;

	while(m->item)
	{
		/* 指向下一个菜单 */
		m++;

		/* 统计项目数量 */
		item_count++;
	}
		
	return item_count;
}



//菜单栏显示
void menu_show(menu_ext_t *menu_ext)
{
	menu_ext_t *m_ext=menu_ext;
	menu_t * m=m_ext->menu;
	
	uint8_t y=0;
	
	/* 显示当前菜单所有项目名称 */
	lcd_vs_set(0);	
	
	LCD_SAFE
	(
		lcd_clear(WHITE);
		lcd_fill(0,LCD_HEIGHT,LCD_WIDTH,80,WHITE);
	);
	
	while(1)
	{
		if(m->item==NULL)
			break;
		LCD_SAFE
		(
			lcd_draw_picture(0,y,48,48,m->pic);
			m->item();
		);
		
		y+=60;
		m++;
	}
	
	/* 拥有子菜单,显示光标 */
	if(m->child)
	{
		m_ext->item_cursor = 0;		
	}	
	
	

}
	
	

