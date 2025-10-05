/****************************************************************
*名    称:基于FreeRtos的智联lifeband
*作    者:刘俊坚
*创建日期:2025.2.21
*说  明:	
	1.当前代码规范、结构清晰，尽可能实现了模块化编程，一个任务管理一个硬件。
	 
	2.菜单项对应硬件模块功能
	
		菜单
			1、rtc设置 		
			2、dht11		
			3、mpu6050	
			4、max30102  	
			5、alarm设置 		
			
*操作说明:
	1.按键功能
		.按键1与按键2：主页与菜单栏的切换
		.按键3：进入菜单项
		.按键4：退出菜单项
	2.可调电阻
		.旋钮式选择对应的菜单，类似iPod
	3.蜂鸣器
		.执行对应的操作会嘀一声示意
	4.菜单项实现
		.多级菜单的访问
	5.蓝牙
		.手机连接蓝牙，主页显示蓝牙图标
		.手机连接蓝牙可设置时间、日期、闹钟
	6.mpu6050六轴螺旋仪
		.开启运动模式后，主页显示运动图标，实时显示当前计步数，和久坐提醒功能
		
		
*关键函数：
	1.app_task_menu，文件路径：main.c
		.负责对菜单项选择/进入/退出的管理
		.显示光标指示当前菜单项
		
	2.menu_show,文件路径：menu.c
		.显示菜单项图标
		.显示菜单项文字
		
*关键变量类型：	
	1.menu_t，文件路径：menu.h
		.链式存储水平左/右菜单、父/子菜单
		
*关键宏：	
	1.LCD_SAFE，文件路径：includes.h
		.使用互斥锁保护LCD访问
*****************************************************************/	


#include "includes.h"


/* 任务句柄 */
TaskHandle_t app_task_init_handle  = NULL;
TaskHandle_t app_task_key_handle   = NULL;
TaskHandle_t app_task_usart_handle = NULL;
TaskHandle_t app_task_dht11_handle = NULL;
TaskHandle_t app_task_beep_handle  = NULL;
TaskHandle_t app_task_rtc_handle  = NULL;
TaskHandle_t app_task_menu_handle  = NULL;
TaskHandle_t app_task_menu_show_handle  = NULL;
TaskHandle_t app_task_adc_handle  = NULL;
TaskHandle_t app_task_max30102_handle  = NULL;
TaskHandle_t app_task_mpu6050_handle  = NULL;
TaskHandle_t app_task_mpu6050_step_handle  = NULL;

/*软件定时器句柄*/
TimerHandle_t soft_timer_handle=NULL;


/* 任务 */ 
static void app_task_init(void* pvParameters);  
static void app_task_key(void* pvParameters); 
static void app_task_usart(void* pvParameters); 
static void app_task_dht11(void* pvParameters); 
static void app_task_beep(void* pvParameters); 
static void app_task_rtc(void* pvParameters); 
static void app_task_menu(void* pvParameters); 
static void app_task_menu_show(void* pvParameters); 
static void app_task_adc(void* pvParameters); 
static void app_task_max30102(void* pvParameters); 
static void app_task_mpu6050(void* pvParameters);
static void app_task_mpu6050_step(void* pvParameters);

/* 软件定时器 */
static void vTimer_callback(TimerHandle_t pxTimer);

/*变量*/
GPIO_InitTypeDef  	GPIO_InitStructure;
USART_InitTypeDef 	USART_InitStructure;
NVIC_InitTypeDef 	NVIC_InitStructure;
SPI_InitTypeDef  	SPI_InitStructure;
RTC_DateTypeDef 	RTC_DateStructure;
RTC_TimeTypeDef  	RTC_TimeStructure;
RTC_AlarmTypeDef  	RTC_AlarmStructure;

volatile uint32_t g_dht_get_what=FLAG_DHT_GET_NONE;
volatile uint32_t g_rtc_get_what=FLAG_RTC_GET_NONE;
volatile uint32_t g_alarm_set   =FLAG_ALARM_SET_NONE;
volatile uint32_t g_step_status =FLAG_STEP_SET_NONE;
volatile uint32_t g_ble_status  =FLAG_BLE_STATUS_NONE;
volatile uint32_t g_alarm_pic=0;
volatile uint32_t ulCount    =0;
volatile uint32_t step_count =0;


//max30102任务的变量
volatile uint32_t aun_ir_buffer[500]; //IR LED sensor data
volatile int32_t n_ir_buffer_length;    //data length
volatile uint32_t aun_red_buffer[500];    //Red LED sensor data
volatile int32_t n_sp02; //SPO2 value
volatile int8_t ch_spo2_valid;   //indicator to show if the SP02 calculation is valid
volatile int32_t n_heart_rate;   //heart rate value
volatile int8_t  ch_hr_valid;    //indicator to show if the heart rate calculation is valid
volatile uint8_t uch_dummy;


	

/* 互斥型信号量句柄 */
SemaphoreHandle_t g_mutex_printf;
SemaphoreHandle_t g_mutex_lcd;

/* 消息队列句柄 */
QueueHandle_t g_queue_usart;
QueueHandle_t g_queue_beep;

/* 事件标志组句柄 */
EventGroupHandle_t g_event_group;


/* 任务列表 */
static const task_t task_tbl[] = {
	{app_task_key,  "app_task_key",512, NULL,5,&app_task_key_handle},
	{app_task_usart,"app_task_usart",512, NULL, 5, &app_task_usart_handle},
	{app_task_dht11,"app_task_dht11", 512, NULL, 5, &app_task_dht11_handle},
	{app_task_beep, "app_task_beep", 512, NULL, 5, &app_task_beep_handle},
	{app_task_rtc,  "app_task_rtc", 512, NULL, 5, &app_task_rtc_handle},
	{app_task_menu, "app_task_menu", 512, NULL, 5, &app_task_menu_handle},
	{app_task_menu_show, "app_task_menu_show", 512, NULL, 5, &app_task_menu_show_handle},
	{app_task_adc,  "app_task_adc", 512, NULL, 5, &app_task_adc_handle},
	{app_task_max30102, "app_task_max30102", 512, NULL, 5, &app_task_max30102_handle},
	{app_task_mpu6050, "app_task_mpu6050", 512, NULL, 5, &app_task_mpu6050_handle},
	{app_task_mpu6050_step, "app_task_mpu6050_step", 512, NULL, 5, &app_task_mpu6050_step_handle},
	
	{0, 0, 0, 0, 0, 0}
};


/**
 * @brief 主函数
 * @param void:无须传入参数
 * @retval 无
 */
int main(void)
{
	/* 设置系统中断优先级分组4 */
	NVIC_PriorityGroupConfig(NVIC_PriorityGroup_4);
	
	/* 系统定时器中断频率为configTICK_RATE_HZ */
	SysTick_Config(SystemCoreClock/configTICK_RATE_HZ);								
	
	/* 初始化串口1 */
	usart_init(115200);  

	
	
	/* 创建 app_task_init任务 */
	xTaskCreate((TaskFunction_t )app_task_init,  	/* 任务入口函数 */
			  (const char*    )"app_task_init",		/* 任务名字 */
			  (uint16_t       )512,  				/* 任务栈大小 */
			  (void*          )NULL,				/* 任务入口函数参数 */
			  (UBaseType_t    )5, 					/* 任务的优先级 */
			  (TaskHandle_t*  )&app_task_init_handle);/* 任务控制块指针 */ 
				  

			  
			   
	/* 开启任务调度 */
	vTaskStartScheduler(); 
			  
	while(1);

}

/*开机动画*/
void lcd_startup_info(void)
{
		uint16_t x=0;
	
		/* 设置显示起始位置 */
		lcd_vs_set(0);	
		
		/* 清屏 */
		lcd_clear(WHITE);
		lcd_fill(0,LCD_HEIGHT,LCD_WIDTH,80,WHITE);

		lcd_draw_picture(70,100,100,66,gImage_hellokitty_100x66);
	
	
		    x=60;  lcd_show_chn(x,50,1,BLACK,WHITE,32);
			x+=40; lcd_show_chn(x,50,0,BLACK,WHITE,32);
			x+=40; lcd_show_chn(x,50,2,BLACK,WHITE,32);
	        
	
	
		x=0;
		while(x<LCD_WIDTH)
		{
			/* 进度条 */
			lcd_fill(x,230,10,50,BLUE);

			x+=10;
			
			vTaskDelay(50);
		}

	
		LCD_SAFE(

			x=40; 	lcd_show_chn(x,180,6,BLACK,WHITE,24);//智
			x+=40;	lcd_show_chn(x,180,7,BLACK,WHITE,24);//能
			x+=40;	lcd_show_chn(x,180,8,BLACK,WHITE,24);//手
			x+=40;	lcd_show_chn(x,180,9,BLACK,WHITE,24);//环
		);
		
		delay_ms(1000);
		
	
	
}


/*任务初始化*/
static void app_task_init(void* pvParameters)
{
	uint32_t i;
	
	printf("app_task_init  ok!\r\n");
	
	/* 创建互斥型信号量 */	  
	g_mutex_printf=xSemaphoreCreateMutex();
	g_mutex_lcd   = xSemaphoreCreateMutex();	
	
	/* 创建消息队列 */
	g_queue_usart=xQueueCreate(5,128);
	g_queue_beep=xQueueCreate(5, sizeof(beep_t));
	
	/* 创建事件标志组 */
	g_event_group = xEventGroupCreate();

	/*创建软件定时器*/
	soft_timer_handle = xTimerCreate
                 ( (char * ) "timer",
                   (TickType_t )1000,
                   (UBaseType_t) pdTRUE,
                   (void * )1,
                   (TimerCallbackFunction_t) vTimer_callback );
				   /* 开启周期软件定时器 */
	xTimerStart(soft_timer_handle, 0);	
	
	/* lcd初始化 */ 
	lcd_init();
	
	//显示开机界面
	lcd_startup_info();
	
	//led灯初始化
	LED_Init();
	
	//蜂鸣器初始化
	beep_init();
	
	//温湿度初始化
	dht11_init();
	
	//按键初始化
	key_init();
	
	//实时时钟初始化，掉电数据不丢失
	rtc_backup();
	
	//闹钟初始化
	alarm_a_init();
	
	//adc初始化
	adc_init();	
	
	//心率血氧传感器初始化
	max30102_init();
	i2c_stop1();
	delay_ms(10);  // 等待一段时间，确保总线稳定

	//初始化MPU6050	
	MPU_Init();
	while(mpu_dmp_init())
	{
		printf("[ERROR] MPU6050 ERROR \r\n");
		delay_ms(500);
	}
	
	//蓝牙初始化
	ble_init(9600);
	
	

	/* 创建用到的任务 */
	taskENTER_CRITICAL();
	i = 0;
	while (task_tbl[i].pxTaskCode)
	{
		xTaskCreate(task_tbl[i].pxTaskCode,		/* 任务入口函数 */
					task_tbl[i].pcName,			/* 任务名字 */
					task_tbl[i].usStackDepth,	/* 任务栈大小 */
					task_tbl[i].pvParameters,	/* 任务入口函数参数 */
					task_tbl[i].uxPriority,		/* 任务的优先级 */
					task_tbl[i].pxCreatedTask); /* 任务控制块指针 */
		i++;
	}
	taskEXIT_CRITICAL();
	
	//挂起
	vTaskSuspend(app_task_menu_handle);
	vTaskSuspend(app_task_max30102_handle);

	printf("app_task_init  end!\r\n");
	
	/* 删除任务自身 */
	vTaskDelete(NULL);
	
	
		
}   


/*软件定时器任务*/
static void vTimer_callback(TimerHandle_t pxTimer)
{		
	//20秒后熄屏
	ulCount++;
	if(ulCount>=20)
	{
		/* 熄屏 */
		LCD_SAFE
		(
			lcd_display_on(0);
		);
		ulCount=0;
		
		
	}
	
	
	
}

/*mpu6050之计步、久坐提醒任务*/
static void app_task_mpu6050_step(void* pvParameters)
{
	uint32_t i=0;
	beep_t beep;
	
	unsigned long  step_count_last=0;
	uint32_t sedentary_event=0;
	/* 设置步数初值为0*/
	
	while(dmp_set_pedometer_step_count(0))
	{
		delay_ms(500);
	}
	vTaskSuspend(NULL);
	for(;;)
	{
		
		
		/* 获取步数 */
		dmp_get_pedometer_step_count((unsigned long *)&step_count);
		
		printf("step_count=%d\r\n",step_count);	
				
		
		i++;
		if(i>=100)
		{
			dmp_get_pedometer_step_count((unsigned long *)&step_count);
			if((step_count-step_count_last)<5)
			{
				/* 步数变化不大，则设置久坐事件标志位为1 */
				sedentary_event=1;
			}
			step_count_last=step_count;
				
			printf("[INFO] 当前步数:%ld 以前步数:%ld\r\n",step_count,step_count_last);
			
			
			i=0;
		}
		
		//久坐提醒
		if(sedentary_event)
		{
			sedentary_event=0;
			/* 嘀一声示意 */
			beep.sta = 1;
			beep.duration = 10;

			xQueueSend( g_queue_beep,  /* 消息队列的句柄 */
						&beep,		   /* 发送的消息内容 */
						100);		   /* 等待时间 100 Tick */	
			
		}
		
		
		
		delay_ms(100);
	}
}

/* mpu6050之抬手亮屏任务 */
static void app_task_mpu6050(void* pvParameters)
{
	
	float pitch,roll,yaw; //欧拉角
	
	
	
	for(;;)
	{
		
			if(mpu_dmp_get_data(&pitch,&roll,&yaw)==0)
			{
				//抬手亮屏
				if(roll>=15)
				{
					PFout(9)=PFout(10)=0;
					PEout(13)=PEout(14)=0;
					
					//定时器计数值清零
					ulCount=0;
					
					
					lcd_display_on(1);
				
				}
				if(roll<5)
				{
					PFout(9)=PFout(10)=1;
					PEout(13)=PEout(14)=1;
				}
				delay_ms(200);
			}
		
		
		
	}
}


/* max30102检测心率血样任务 */
static void app_task_max30102(void* pvParameters)
{
	
	char heart_buf[64]={0};
	char sp_buf[64]={0};
	
	uint32_t un_min, un_max, un_prev_data;  
	int32_t i;
	int32_t n_brightness;
	float f_temp;

	uint8_t temp[6];

	un_min=0x3FFFF;
	un_max=0;
	
	//buffer length of 100 stores 5 seconds of samples running at 100sps
	n_ir_buffer_length=500;
	
	//read the first 500 samples, and determine the signal range
    for(i=0;i<n_ir_buffer_length;i++){
		
		//wait until the interrupt pin asserts
        while(MAX30102_INT==1);   
        
		max30102_FIFO_ReadBytes(REG_FIFO_DATA,temp);
		
		// Combine values to get the actual number
		aun_red_buffer[i] =  (long)((long)((long)temp[0]&0x03)<<16) | (long)temp[1]<<8 | (long)temp[2]; 
		
		// Combine values to get the actual number
		aun_ir_buffer[i] = (long)((long)((long)temp[3] & 0x03)<<16) |(long)temp[4]<<8 | (long)temp[5];   
            
        if(un_min>aun_red_buffer[i])
            un_min=aun_red_buffer[i];    //update signal min
        if(un_max<aun_red_buffer[i])
            un_max=aun_red_buffer[i];    //update signal max
    }
	un_prev_data=aun_red_buffer[i];
	
	//calculate heart rate and SpO2 after first 500 samples (first 5 seconds of samples)
    maxim_heart_rate_and_oxygen_saturation( (uint32_t  *)aun_ir_buffer, 	\
											(int32_t  )n_ir_buffer_length, 	\
											(uint32_t  *)aun_red_buffer, \
											(int32_t *)&n_sp02, 		\
											(int8_t *)&ch_spo2_valid, \
											(int32_t *)&n_heart_rate, 	\
											(int8_t *)&ch_hr_valid); 
	//printf("init max30102 end\r\n");
	
	
	for(;;)
	{
		
		i=0;
        un_min=0x3FFFF;
        un_max=0;
		n_ir_buffer_length=500;
		
		/* dumping the first 100 sets of samples in the memory and shift the last 400 sets of samples to the top
		
		   将前100组样本转储到存储器中，并将最后400组样本移到顶部
		*/
		
        for(i=100;i<500;i++)
        {
            aun_red_buffer[i-100]=aun_red_buffer[i];
            aun_ir_buffer[i-100]=aun_ir_buffer[i];
            
            /* update the signal min and max 
			   更新信号最小值和最大值
			*/
			
            if(un_min>aun_red_buffer[i])
				un_min=aun_red_buffer[i];
			
            if(un_max<aun_red_buffer[i])
				un_max=aun_red_buffer[i];
        }
		
		/* take 100 sets of samples before calculating the heart rate 
		
		   在计算心率之前采集100组样本
		*/
		
        for(i=400;i<500;i++)
        {
            un_prev_data=aun_red_buffer[i-1];
			
            while(MAX30102_INT==1);
			
            max30102_FIFO_ReadBytes(REG_FIFO_DATA,temp);
			
			/* 组合值以获得实际数字 */
			aun_red_buffer[i] =  ((temp[0]&0x03)<<16) |(temp[1]<<8) | temp[2];   
			aun_ir_buffer[i] =   ((temp[3]&0x03)<<16) |(temp[4]<<8) | temp[5];   
        
            if(aun_red_buffer[i]>un_prev_data)
            {
                f_temp=aun_red_buffer[i]-un_prev_data;
				
                f_temp/=(un_max-un_min);
				
                f_temp*=MAX_BRIGHTNESS;
				
                n_brightness-=(int32_t)f_temp;
				
                if(n_brightness<0)
                    n_brightness=0;
            }
            else
            {
                f_temp=un_prev_data-aun_red_buffer[i];
				
                f_temp/=(un_max-un_min);
				
                f_temp*=MAX_BRIGHTNESS;
				
                n_brightness+=(int32_t)f_temp;
				
                if(n_brightness>MAX_BRIGHTNESS)
                    n_brightness=MAX_BRIGHTNESS;
            }
		}
		
		/* 计算心率和血氧饱和度 */
		maxim_heart_rate_and_oxygen_saturation( (uint32_t  *)aun_ir_buffer, 	\
												(int32_t  )n_ir_buffer_length, 	\
												(uint32_t  *)aun_red_buffer, \
												(int32_t *)&n_sp02, 		\
												(int8_t *)&ch_spo2_valid, \
												(int32_t *)&n_heart_rate, 	\
												(int8_t *)&ch_hr_valid); 
			
     
													
		/* 通过UART将样本和计算结果发送到终端程序 */
		if((ch_hr_valid == 1)&& (n_heart_rate>=60) && (n_heart_rate<=100))
		{
			
				memset(heart_buf, 0, sizeof heart_buf);
				printf("心率=%d\r\n", n_heart_rate);
				lcd_show_chn(60,130,19,BLACK,WHITE,24);
				lcd_show_chn(92,130,20,BLACK,WHITE,24);
				sprintf(heart_buf,":%3d",n_heart_rate);
				lcd_show_string(130,130,(const char *)heart_buf,BLACK,WHITE,24,0);
				
			
		}

		
		if((ch_spo2_valid ==1)&& (n_sp02>=95) && (n_sp02<=100))
		{
			
				memset(sp_buf, 0, sizeof sp_buf);	
				printf("血氧=%d\r\n", n_sp02); 
				lcd_show_chn(60,160,21,BLACK,WHITE,24);
				lcd_show_chn(92,160,22,BLACK,WHITE,24);
				sprintf(sp_buf,":%3d",n_sp02);
				lcd_show_string(130,160,(const char *)sp_buf,BLACK,WHITE,24,0);
			
			
			
		}			
	
		delay_ms(1000);
	}
	
	
}


/* 通过可调电阻控制adc，用作转盘菜单的控制 */
static void app_task_adc(void* pvParameters)
{
	int32_t adc_vol=0;
	int32_t adc_vol_last=0;
	int32_t result;
	
	/* 获取当前电压初值 */
	adc_vol_last=get_adc_vol();
	
	for(;;)
	{
		/* 获取电压值 */
		adc_vol = get_adc_vol();
		result=adc_vol-adc_vol_last;
		
		if(result>200 || result<-200)
		{
			/* 发送KEY_DOWN/KEY_UP按键事件 */
			xEventGroupSetBits(g_event_group, result > 0 ? EVENT_GROUP_FN_KEY_DOWN : EVENT_GROUP_FN_KEY_UP);
			
			adc_vol_last=adc_vol;
		}
		vTaskDelay(100);
	}
}


/* 主页实时时钟的显示 */
static void app_task_menu_show(void* pvParameters)
{
	
	EventBits_t EventValue;
	
	beep_t beep;
	
	menu_ext_t  m_ext;
	
	m_ext.menu=menu_main_1;
	
	
	
	for(;;)
	{
		
		/* 等待事件组中的相应事件位，或同步 */
		EventValue = xEventGroupWaitBits((EventGroupHandle_t)g_event_group,
										 (EventBits_t)EVENT_GROUP_FN_KEY_MENU_ENTER|EVENT_GROUP_FN_KEY_MENU_BACK,
										 (BaseType_t)pdTRUE,
										 (BaseType_t)pdFALSE,
										 (TickType_t)portMAX_DELAY);
		
		/* 嘀一声示意 */
		beep.sta = 1;
		beep.duration = 10;

		xQueueSend( g_queue_beep,  /* 消息队列的句柄 */
					&beep,		   /* 发送的消息内容 */
					100);		   /* 等待时间 100 Tick */	
		
		
		
		//按键1进入菜单任务栏
		if(EventValue & EVENT_GROUP_FN_KEY_MENU_ENTER )
		{
			
			//定时器计数值清零
			ulCount=0;
			
			//亮屏
			lcd_display_on(1);
	
			
			
			
			//清屏
			LCD_SAFE(
				/* 清屏 */
				lcd_vs_set(0);
				lcd_clear(WHITE);
				lcd_fill(0,LCD_HEIGHT,LCD_WIDTH,80,WHITE);
			);
			
			
			//挂起时钟
			vTaskSuspend(app_task_rtc_handle);
			
			menu_show(&m_ext);
			
			/* 恢复menu任务运行 */
			vTaskResume(app_task_menu_handle);
			
			
		}
		
		//按键2返回时钟主界面
		if(EventValue & EVENT_GROUP_FN_KEY_MENU_BACK )
		{
			
			//定时器计数值清零
			ulCount=0;
			
			
			//清屏
			LCD_SAFE(
				/* 清屏 */
				lcd_vs_set(0);
				lcd_clear(WHITE);
				lcd_fill(0,LCD_HEIGHT,LCD_WIDTH,80,WHITE);
				lcd_draw_picture(70,100,100,66,gImage_hellokitty_100x66);	
			);
			
			
			
			//挂起menu任务
			vTaskSuspend(app_task_menu_handle);
				
			//恢复时钟显示
			vTaskResume(app_task_rtc_handle);
			
			
			//亮屏
			lcd_display_on(1);
			
		}
	}
}


/* 控制菜单栏任务*/
static void app_task_menu(void* pvParameters)
{
	EventBits_t EventValue;
	
	beep_t beep;

	
	//光标
	uint32_t item_total;		//记录当前菜单有多少个项目
	uint32_t item_cursor = 0;	//记录当前菜单指向哪个项目
	uint32_t fun_run = 0;		//记录是否有项目功能函数在执行
	//可见区
	uint16_t vs=0;
	menu_ext_t m_ext;
	
	menu_t *m;
	
	/* 通过参数传递获取主菜单页指针 */
	//menu_t **m_main = (menu_t **)pvParameters;	
	
	//挂起
	//vTaskSuspend(NULL);
	/* 配置m_ext指向的当前菜单及相关参数 */
	m_ext.menu = *menu_main_tbl;						
	m_ext.key_fn = KEY_NONE;
	m_ext.item_cursor = item_cursor;
	m_ext.item_total = menu_item_total(m_ext.menu);	
	
	
	/* 配置记录菜单指针m */
	m = m_ext.menu;	
	
	//menu_show(&m_ext);
	
	for(;;)
	{
		/* 等待事件组中的相应事件位，或同步 */
		EventValue = xEventGroupWaitBits((EventGroupHandle_t)g_event_group,
										(EventBits_t)     EVENT_GROUP_FN_KEY_UP \
										 				| EVENT_GROUP_FN_KEY_DOWN \
										 				| EVENT_GROUP_FN_KEY_ENTER \
										 				| EVENT_GROUP_FN_KEY_BACK,
										 (BaseType_t)pdTRUE,
										 (BaseType_t)pdFALSE,
										 (TickType_t)portMAX_DELAY);
		
		/* 嘀一声示意 */
		beep.sta = 1;
		beep.duration = 10;

		xQueueSend( g_queue_beep,  /* 消息队列的句柄 */
					&beep,		   /* 发送的消息内容 */
					100);		   /* 等待时间 100 Tick */	
		
		//定时器计数值清零
		ulCount=0;
			

		//调节电阻按键往左向上
		if(EventValue & EVENT_GROUP_FN_KEY_UP )
		{
			
			/* 若有项目功能函数在运行，提示需要返回才能进行项目选择 */
			if (fun_run)
			{
				continue;
			}
			
			if(item_cursor )
			{
				item_cursor--;
					
				/* 设置RAM的垂直滚动起始地址 */
				if(item_cursor==0)
				{
					while(vs)
					{
						vs--;
						LCD_SAFE(lcd_vs_set(vs));
						vTaskDelay(5);
					}
					
				}
				m--;//当前功能菜单指向位置
				
				/* 显示光标 */
				lcd_fill(230,(item_cursor+1)*60,10,60,WHITE);		
				lcd_fill(230,item_cursor*60,10,60,GREY);
		
			}
			
			m_ext.item_cursor=item_cursor;
			
		}
			
		////调节电阻按键往右向下
		if(EventValue & EVENT_GROUP_FN_KEY_DOWN)
		{
			/* 若有项目功能函数在运行，提示需要返回才能进行项目选择 */
			if (fun_run)
			{
				continue;
			}
			
			/* 获取当前菜单有多少个项目 */
			item_total = menu_item_total(m_ext.menu);

			/* 保存当前菜单有多少个项目 */
			m_ext.item_total = item_total;

			
			if(item_cursor<(item_total-1))
			{
				item_cursor++;
					
				/* 设置RAM的垂直滚动起始地址 */
				if(item_cursor==4)
				{
					while(vs<60)
					{
						vs++;
						LCD_SAFE(lcd_vs_set(vs));
						vTaskDelay(5);
					}
					
				}
				m++;//当前功能菜单指向位置
				
				/* 显示光标 */
				lcd_fill(230,(item_cursor-1)*60,10,60,WHITE);		
				lcd_fill(230,item_cursor*60,10,60,GREY);
			}
			
			m_ext.item_cursor=item_cursor;
		}
		
		//按键3进入子菜单
		if(EventValue & EVENT_GROUP_FN_KEY_ENTER)
		{
			m_ext.key_fn=KEY_ENTER;
			/* 若有项目功能函数在运行，提示需要返回才能进行项目选择 */
			if (fun_run)
			{
				continue;
			}

			
			
			m_ext.item_cursor=item_cursor;
			
			if(m->child && !m->fun)
			{
				m=m->child;
				
				m_ext.menu=m;
				
				menu_show(&m_ext);
				
				item_cursor = 0;	
			}
			
			if(!m->child && m->fun)
			{
				fun_run=1;
				m->fun(&m_ext);
			}
			
			
		}
		
		//按键4退出子菜单
		if(EventValue & EVENT_GROUP_FN_KEY_BACK)
		{
			m_ext.key_fn = KEY_BACK;
			/* 若子菜单功能函数有效，先执行，主要是挂起对应任务 */
			if (m->fun)
			{
				/* 标记有项目功能函数在运行 */
				fun_run = 1;
			
				m->fun(&m_ext);
			}
			
			
			/* 父菜单有效 */
			if (m->parent)
			{
				/* 指向父菜单 */
				m = m->parent;

				/* 保存当前菜单 */
				m_ext.menu = m;

				/* 复位光标值 */
				item_cursor = 0;
				m_ext.item_cursor = 0;

				
				fun_run = 0;

				/* 显示当前菜单 */
				menu_show(&m_ext);
			}
			
			
		}
			
		
	}
	
}


/*实时时钟显示任务*/
static void app_task_rtc(void* pvParameters)
{
	
	beep_t beep;
	
	uint32_t i=0;
	
	
	EventBits_t EventValue;
	uint8_t time_buf[16]={0};
	uint8_t date_buf[16]={0}; 
	uint8_t step_buf[16]={0};
	

	LCD_SAFE(
		/* 清屏 */
		lcd_clear(WHITE);
		lcd_fill(0,LCD_HEIGHT,LCD_WIDTH,80,WHITE);
		
		lcd_draw_picture(70,100,100,66,gImage_hellokitty_100x66);	
	);
	
	for(;;)
	{
		/* 等待事件组中的相应事件位，或同步 */
		EventValue = xEventGroupWaitBits((EventGroupHandle_t)g_event_group,
										 (EventBits_t)EVENT_GROUP_RTC_WAKEUP|EVENT_GROUP_RTC_ALARM,
										 (BaseType_t)pdTRUE,
										 (BaseType_t)pdFALSE,
										 (TickType_t)portMAX_DELAY);
		
		if(EventValue & EVENT_GROUP_RTC_WAKEUP)
		{
			LCD_SAFE(
	
				//获取小时显示
				RTC_GetTime(RTC_Format_BCD,&RTC_TimeStructure);
				sprintf((char *)time_buf,"%02x:",RTC_TimeStructure.RTC_Hours);
				lcd_show_string(0,0,(const char *)time_buf,BLACK,WHITE,96,0);
				//获取分钟显示
				sprintf((char *)time_buf,"%02x",RTC_TimeStructure.RTC_Minutes);
				lcd_show_string(140,0,(const char *)time_buf,BLACK,WHITE,96,0);
				//获取秒显示
				lcd_draw_rectangle(170,90,220,140,RED);
				sprintf((char *)time_buf,"%02x",RTC_TimeStructure.RTC_Seconds);
				lcd_show_string(180,100,(const char *)time_buf,BLACK,WHITE,32,0);
				

				//获取日期显示
				RTC_GetDate(RTC_Format_BCD,&RTC_DateStructure);
				sprintf((char *)date_buf,"20%02x/%02x/%02x",
					RTC_DateStructure.RTC_Year,
					RTC_DateStructure.RTC_Month,
					RTC_DateStructure.RTC_Date);	
				lcd_show_string(60,170,(const char *)date_buf,BLACK,WHITE,24,0);
				
				//获取星期显示
				sprintf((char *)date_buf,"WEEK %d",RTC_DateStructure.RTC_WeekDay);
				lcd_show_string(90,200,(const char *)date_buf,BLACK,WHITE,24,0);
				
				
			);
				
			//蓝牙图标
			if(g_ble_status==FLAG_BLE_STATUS_CONNECT)
			{
				LCD_SAFE(
				lcd_draw_picture(10,100,32,32,gImage_ble_32x32);
				);
				
			}	
			if(g_ble_status==FLAG_BLE_STATUS_NONE)
			{
				LCD_SAFE(
				lcd_fill(10,100,32,32,WHITE);
				);
			}
			
			//闹钟图标
			if(g_alarm_pic)
			{
				LCD_SAFE(
				lcd_draw_picture(10,140,32,32,gImage_alarm_32x32);
				);
			}
			else
			{
				LCD_SAFE(
				lcd_fill(10,140,32,32,WHITE);
				);
			}
			
			//运动图标
			if(g_step_status == FLAG_STEP_SET_START)
			{
				sprintf((char *)step_buf,"%d",step_count);
				lcd_show_string(200,190,(const char *)step_buf,RED,WHITE,32,0);
				
				LCD_SAFE(
				lcd_draw_picture(10,180,32,32,gImage_step_32x32);
				);
			}
			
			if(g_step_status == FLAG_STEP_SET_NONE)
			{
				LCD_SAFE(
				lcd_fill(10,180,32,32,WHITE);
				);
				
				dmp_set_pedometer_step_count(0);
			}
			
			
				
				
		}
		
		if(EventValue & EVENT_GROUP_RTC_ALARM)
		{
			printf("闹钟响了！\n");
				
			for(i=0;i<5;i++)
			{
				/* 嘀一声示意 */
				beep.sta = 1;
				beep.duration = 10;

				xQueueSend( g_queue_beep,  /* 消息队列的句柄 */
							&beep,		   /* 发送的消息内容 */
							100);		   /* 等待时间 100 Tick */	
				delay_ms(500);
			}
			
			
		}
		
		
		
		

	}
		
}   


/*按键任务*/
static void app_task_key(void* pvParameters)
{
	EventBits_t EventValue;
	
	for(;;)
	{
		 EventValue = xEventGroupWaitBits(
            g_event_group,   /* The event group being tested. */
            EVENT_GROUP_KEYALL_DOWN, //0x0F
            pdTRUE,        /* BIT_0 & BIT_4 should be cleared before returning. */
            pdFALSE,       /* Don't wait for both bits, either bit will do. */
            portMAX_DELAY );/* Wait a maximum of 100ms for either bit to be set. */
	
		if(EventValue & EVENT_GROUP_KEY1_DOWN)//按键1
		{
			/* 发送KEY_MENU_ENTER按键事件 */
			xEventGroupSetBits(g_event_group, EVENT_GROUP_FN_KEY_MENU_ENTER);
			
			
		}
		
		if(EventValue & EVENT_GROUP_KEY2_DOWN)//按键2
		{
			/* 发送KEY_MENU_BACK按键事件 */
			xEventGroupSetBits(g_event_group, EVENT_GROUP_FN_KEY_MENU_BACK);

		}
		
		if(EventValue & EVENT_GROUP_KEY3_DOWN)//按键3
		{
			/* 发送KEY_ENTER按键事件 */
			xEventGroupSetBits(g_event_group, EVENT_GROUP_FN_KEY_ENTER);

		}
		if(EventValue & EVENT_GROUP_KEY4_DOWN)//按键4
		{
			/* 发送KEY_BACK按键事件 */
			xEventGroupSetBits(g_event_group, EVENT_GROUP_FN_KEY_BACK);

		}
		
	}
}


/*蜂鸣器任务*/
static void app_task_beep(void* pvParameters)
{
	beep_t beep;
	
	BaseType_t rt=pdFALSE;
	
	for(;;)
	{
		rt=xQueueReceive(
					   g_queue_beep,
					   &beep,
					   portMAX_DELAY );
		if(rt!=pdPASS)
			continue;
		
		if (beep.sta)
		{
			BEEP(beep.sta);

			while (beep.duration--)
				delay_ms(10);

			/* 蜂鸣器状态翻转 */
			beep.sta ^= 1;
		}


		//默认静音
		BEEP(beep.sta);
			  
	}
}

/*串口任务*/
static void app_task_usart(void* pvParameters)
{

	
	char buf[128]={0};
	char *p=NULL;
	uint32_t year = 0;
	uint32_t month = 0;
	uint32_t day = 0;
	uint32_t week_day = 0;
	uint32_t hours = 0;
	uint32_t minutes = 0;
	uint32_t seconds = 0;
	
	
	for(;;)
	{
		
		//串口接收数据
		xQueueReceive(g_queue_usart,buf,portMAX_DELAY);	  
		
	
		printf("%s\r\n",buf);

		
		//设置时间
		if(g_rtc_get_what==FLAG_RTC_GET_TIME || g_ble_status==FLAG_BLE_STATUS_CONNECT)
		{
			//“TIME SET-10-20-30#” 
			if(strstr((const char*)buf,"TIME SET"))
			{
				
				p=strtok((char *)buf,"-");
				//小时10
				p=strtok(NULL,"-");
				hours=atoi(p);
				//分钟20
				p = strtok(NULL,"-");
				minutes = atoi(p);
				//秒30
				p = strtok(NULL,"-");
				seconds = atoi(p);
						
				RTC_TimeStructure.RTC_Hours   = hours;
				RTC_TimeStructure.RTC_Minutes = minutes;
				RTC_TimeStructure.RTC_Seconds = seconds; 
				RTC_SetTime(RTC_Format_BIN, &RTC_TimeStructure);

				printf("set time ok!\r\n");	
				
				if(g_rtc_get_what==FLAG_RTC_GET_TIME)
					lcd_draw_picture(70,90,100,100,gImage_succee_100x100);	
	
			}
		}
		
		//设置日期
		if(g_rtc_get_what==FLAG_RTC_GET_DATE || g_ble_status==FLAG_BLE_STATUS_CONNECT)
		{
			
			//“DATE SET-2023-5-25-4#”
			if(strstr((const char*)buf,"DATE SET"))
			{
				p=strtok((char *)buf,"-");
				p=strtok(NULL,"-");//2023
				year=atoi(p);
				//提取月份
				p = strtok(NULL,"-");
				month = atoi(p);

				//提取天数
				p = strtok(NULL,"-");
				day = atoi(p);
				//printf("p=%s",p);

				
				//提取星期几
				p = strtok(NULL,"-");
				week_day = atoi(p);
				//printf("p=%s",p);
				
				RTC_DateStructure.RTC_Year = year-2000;
				RTC_DateStructure.RTC_Month = month;
				RTC_DateStructure.RTC_Date = day;
				RTC_DateStructure.RTC_WeekDay = week_day;
				
				RTC_SetDate(RTC_Format_BIN, &RTC_DateStructure);
				
				printf("set date ok!\r\n");	
				
				if(g_rtc_get_what==FLAG_RTC_GET_DATE)
					lcd_draw_picture(70,90,100,100,gImage_succee_100x100);	
	
			}
		}
		
		//设置闹钟
		if(g_alarm_set==FLAG_ALARM_SET_START || g_ble_status==FLAG_BLE_STATUS_CONNECT)
		{
			//“ALARM SET-10-20-30#” 
			if(strstr((const char*)buf,"ALARM SET"))
			{
				
				//关闭闹钟
				RTC_AlarmCmd(RTC_Alarm_A, DISABLE);
				
				p=strtok((char *)buf,"-");
				//小时10
				p=strtok(NULL,"-");
				hours=atoi(p);
				//分钟20
				p = strtok(NULL,"-");
				minutes = atoi(p);

				//秒30
				p = strtok(NULL,"-");
				seconds = atoi(p);
				
				if(hours>12)
				{
					RTC_AlarmStructure.RTC_AlarmTime.RTC_H12 = RTC_H12_PM;
				}
				else{
					RTC_AlarmStructure.RTC_AlarmTime.RTC_H12 = RTC_H12_AM;
				}
				
				RTC_AlarmStructure.RTC_AlarmTime.RTC_Hours   = hours;
				RTC_AlarmStructure.RTC_AlarmTime.RTC_Minutes = minutes;
				RTC_AlarmStructure.RTC_AlarmTime.RTC_Seconds = seconds;
							
//				RTC_AlarmStructure.RTC_AlarmDateWeekDay = 0x31;
//				RTC_AlarmStructure.RTC_AlarmDateWeekDaySel = RTC_AlarmDateWeekDaySel_Date;
				RTC_AlarmStructure.RTC_AlarmMask = RTC_AlarmMask_DateWeekDay;	//屏蔽日期和星期，就是闹钟每天都生效

		
				/* 配置RTC的A闹钟，注：RTC的闹钟有两个，分别为闹钟A与闹钟B */
				RTC_SetAlarm(RTC_Format_BIN, RTC_Alarm_A, &RTC_AlarmStructure);
				
				/* 让RTC的闹钟A工作*/
				RTC_AlarmCmd(RTC_Alarm_A, ENABLE);
				
				printf("alarm set ok!\r\n");
				
				if(g_alarm_set==FLAG_ALARM_SET_START )
					lcd_draw_picture(70,90,100,100,gImage_succee_100x100);	
				
				//点亮闹钟图标
				g_alarm_pic=1;
				
			}
			if(strstr((const char*)buf,"ALARM OFF"))
			{
				printf("闹钟关闭!\r\n");
				//关闭闹钟
				RTC_AlarmCmd(RTC_Alarm_A, DISABLE);
				
				if(g_alarm_set==FLAG_ALARM_SET_START )
					lcd_draw_picture(70,90,100,100,gImage_fail_100x100);	
	
				//熄灭闹钟图标
				g_alarm_pic=0;
			}
		}
		
		
		
	}
	
} 


/*温湿度任务*/
static void app_task_dht11(void* pvParameters)
{
	uint8_t dht11_data[5]={0};
	char buf[16]={0};
	
	int32_t rt=-1;	
	
	vTaskSuspend(NULL);
	
	/* 初始化后延时一会 */
	vTaskDelay(1000);

	
	for(;;)
	{
		rt = dht11_read_data(dht11_data);
		
		
		if(g_dht_get_what==FLAG_DHT_GET_TEMP  && rt == 0)
		{
			sprintf((char *)buf,"T:%d.%d",dht11_data[2],dht11_data[3]);
			lcd_show_string(70,160,(const char *)buf,BLACK,WHITE,32,0);
				
		}
		if(g_dht_get_what==FLAG_DHT_GET_HUMI  && rt == 0)
		{
			sprintf((char *)buf,"H:%d.%d",dht11_data[0],dht11_data[1]);
			lcd_show_string(70,160,(const char *)buf,BLACK,WHITE,32,0);
		
		}	
		
	
		vTaskDelay(6000);
	}
}


/*-----------------------------------------------------------*/

void vApplicationMallocFailedHook( void )
{
	/* vApplicationMallocFailedHook() will only be called if
	configUSE_MALLOC_FAILED_HOOK is set to 1 in FreeRTOSConfig.h.  It is a hook
	function that will get called if a call to pvPortMalloc() fails.
	pvPortMalloc() is called internally by the kernel whenever a task, queue,
	timer or semaphore is created.  It is also called by various parts of the
	demo application.  If heap_1.c or heap_2.c are used, then the size of the
	heap available to pvPortMalloc() is defined by configTOTAL_HEAP_SIZE in
	FreeRTOSConfig.h, and the xPortGetFreeHeapSize() API function can be used
	to query the size of free heap space that remains (although it does not
	provide information on how the remaining heap might be fragmented). */
	taskDISABLE_INTERRUPTS();
	for( ;; );
}
/*-----------------------------------------------------------*/

void vApplicationIdleHook( void )
{
	/* vApplicationIdleHook() will only be called if configUSE_IDLE_HOOK is set
	to 1 in FreeRTOSConfig.h.  It will be called on each iteration of the idle
	task.  It is essential that code added to this hook function never attempts
	to block in any way (for example, call xQueueReceive() with a block time
	specified, or call vTaskDelay()).  If the application makes use of the
	vTaskDelete() API function (as this demo application does) then it is also
	important that vApplicationIdleHook() is permitted to return to its calling
	function, because it is the responsibility of the idle task to clean up
	memory allocated by the kernel to any task that has since been deleted. */
}
/*-----------------------------------------------------------*/

void vApplicationStackOverflowHook( TaskHandle_t pxTask, char *pcTaskName )
{
	( void ) pcTaskName;
	( void ) pxTask;

	/* Run time stack overflow checking is performed if
	configCHECK_FOR_STACK_OVERFLOW is defined to 1 or 2.  This hook
	function is called if a stack overflow is detected. */
	taskDISABLE_INTERRUPTS();
	for( ;; );
}


void vApplicationTickHook( void )
{

}
