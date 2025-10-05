/****************************************************************
*��    ��:����FreeRtos������lifeband
*��    ��:������
*��������:2025.2.21
*˵  ��:	
	1.��ǰ����淶���ṹ������������ʵ����ģ�黯��̣�һ���������һ��Ӳ����
	 
	2.�˵����ӦӲ��ģ�鹦��
	
		�˵�
			1��rtc���� 		
			2��dht11		
			3��mpu6050	
			4��max30102  	
			5��alarm���� 		
			
*����˵��:
	1.��������
		.����1�밴��2����ҳ��˵������л�
		.����3������˵���
		.����4���˳��˵���
	2.�ɵ�����
		.��ťʽѡ���Ӧ�Ĳ˵�������iPod
	3.������
		.ִ�ж�Ӧ�Ĳ�������һ��ʾ��
	4.�˵���ʵ��
		.�༶�˵��ķ���
	5.����
		.�ֻ�������������ҳ��ʾ����ͼ��
		.�ֻ���������������ʱ�䡢���ڡ�����
	6.mpu6050����������
		.�����˶�ģʽ����ҳ��ʾ�˶�ͼ�꣬ʵʱ��ʾ��ǰ�Ʋ������;������ѹ���
		
		
*�ؼ�������
	1.app_task_menu���ļ�·����main.c
		.����Բ˵���ѡ��/����/�˳��Ĺ���
		.��ʾ���ָʾ��ǰ�˵���
		
	2.menu_show,�ļ�·����menu.c
		.��ʾ�˵���ͼ��
		.��ʾ�˵�������
		
*�ؼ��������ͣ�	
	1.menu_t���ļ�·����menu.h
		.��ʽ�洢ˮƽ��/�Ҳ˵�����/�Ӳ˵�
		
*�ؼ��꣺	
	1.LCD_SAFE���ļ�·����includes.h
		.ʹ�û���������LCD����
*****************************************************************/	


#include "includes.h"


/* ������ */
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

/*�����ʱ�����*/
TimerHandle_t soft_timer_handle=NULL;


/* ���� */ 
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

/* �����ʱ�� */
static void vTimer_callback(TimerHandle_t pxTimer);

/*����*/
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


//max30102����ı���
volatile uint32_t aun_ir_buffer[500]; //IR LED sensor data
volatile int32_t n_ir_buffer_length;    //data length
volatile uint32_t aun_red_buffer[500];    //Red LED sensor data
volatile int32_t n_sp02; //SPO2 value
volatile int8_t ch_spo2_valid;   //indicator to show if the SP02 calculation is valid
volatile int32_t n_heart_rate;   //heart rate value
volatile int8_t  ch_hr_valid;    //indicator to show if the heart rate calculation is valid
volatile uint8_t uch_dummy;


	

/* �������ź������ */
SemaphoreHandle_t g_mutex_printf;
SemaphoreHandle_t g_mutex_lcd;

/* ��Ϣ���о�� */
QueueHandle_t g_queue_usart;
QueueHandle_t g_queue_beep;

/* �¼���־���� */
EventGroupHandle_t g_event_group;


/* �����б� */
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
 * @brief ������
 * @param void:���봫�����
 * @retval ��
 */
int main(void)
{
	/* ����ϵͳ�ж����ȼ�����4 */
	NVIC_PriorityGroupConfig(NVIC_PriorityGroup_4);
	
	/* ϵͳ��ʱ���ж�Ƶ��ΪconfigTICK_RATE_HZ */
	SysTick_Config(SystemCoreClock/configTICK_RATE_HZ);								
	
	/* ��ʼ������1 */
	usart_init(115200);  

	
	
	/* ���� app_task_init���� */
	xTaskCreate((TaskFunction_t )app_task_init,  	/* ������ں��� */
			  (const char*    )"app_task_init",		/* �������� */
			  (uint16_t       )512,  				/* ����ջ��С */
			  (void*          )NULL,				/* ������ں������� */
			  (UBaseType_t    )5, 					/* ��������ȼ� */
			  (TaskHandle_t*  )&app_task_init_handle);/* ������ƿ�ָ�� */ 
				  

			  
			   
	/* ����������� */
	vTaskStartScheduler(); 
			  
	while(1);

}

/*��������*/
void lcd_startup_info(void)
{
		uint16_t x=0;
	
		/* ������ʾ��ʼλ�� */
		lcd_vs_set(0);	
		
		/* ���� */
		lcd_clear(WHITE);
		lcd_fill(0,LCD_HEIGHT,LCD_WIDTH,80,WHITE);

		lcd_draw_picture(70,100,100,66,gImage_hellokitty_100x66);
	
	
		    x=60;  lcd_show_chn(x,50,1,BLACK,WHITE,32);
			x+=40; lcd_show_chn(x,50,0,BLACK,WHITE,32);
			x+=40; lcd_show_chn(x,50,2,BLACK,WHITE,32);
	        
	
	
		x=0;
		while(x<LCD_WIDTH)
		{
			/* ������ */
			lcd_fill(x,230,10,50,BLUE);

			x+=10;
			
			vTaskDelay(50);
		}

	
		LCD_SAFE(

			x=40; 	lcd_show_chn(x,180,6,BLACK,WHITE,24);//��
			x+=40;	lcd_show_chn(x,180,7,BLACK,WHITE,24);//��
			x+=40;	lcd_show_chn(x,180,8,BLACK,WHITE,24);//��
			x+=40;	lcd_show_chn(x,180,9,BLACK,WHITE,24);//��
		);
		
		delay_ms(1000);
		
	
	
}


/*�����ʼ��*/
static void app_task_init(void* pvParameters)
{
	uint32_t i;
	
	printf("app_task_init  ok!\r\n");
	
	/* �����������ź��� */	  
	g_mutex_printf=xSemaphoreCreateMutex();
	g_mutex_lcd   = xSemaphoreCreateMutex();	
	
	/* ������Ϣ���� */
	g_queue_usart=xQueueCreate(5,128);
	g_queue_beep=xQueueCreate(5, sizeof(beep_t));
	
	/* �����¼���־�� */
	g_event_group = xEventGroupCreate();

	/*���������ʱ��*/
	soft_timer_handle = xTimerCreate
                 ( (char * ) "timer",
                   (TickType_t )1000,
                   (UBaseType_t) pdTRUE,
                   (void * )1,
                   (TimerCallbackFunction_t) vTimer_callback );
				   /* �������������ʱ�� */
	xTimerStart(soft_timer_handle, 0);	
	
	/* lcd��ʼ�� */ 
	lcd_init();
	
	//��ʾ��������
	lcd_startup_info();
	
	//led�Ƴ�ʼ��
	LED_Init();
	
	//��������ʼ��
	beep_init();
	
	//��ʪ�ȳ�ʼ��
	dht11_init();
	
	//������ʼ��
	key_init();
	
	//ʵʱʱ�ӳ�ʼ�����������ݲ���ʧ
	rtc_backup();
	
	//���ӳ�ʼ��
	alarm_a_init();
	
	//adc��ʼ��
	adc_init();	
	
	//����Ѫ����������ʼ��
	max30102_init();
	i2c_stop1();
	delay_ms(10);  // �ȴ�һ��ʱ�䣬ȷ�������ȶ�

	//��ʼ��MPU6050	
	MPU_Init();
	while(mpu_dmp_init())
	{
		printf("[ERROR] MPU6050 ERROR \r\n");
		delay_ms(500);
	}
	
	//������ʼ��
	ble_init(9600);
	
	

	/* �����õ������� */
	taskENTER_CRITICAL();
	i = 0;
	while (task_tbl[i].pxTaskCode)
	{
		xTaskCreate(task_tbl[i].pxTaskCode,		/* ������ں��� */
					task_tbl[i].pcName,			/* �������� */
					task_tbl[i].usStackDepth,	/* ����ջ��С */
					task_tbl[i].pvParameters,	/* ������ں������� */
					task_tbl[i].uxPriority,		/* ��������ȼ� */
					task_tbl[i].pxCreatedTask); /* ������ƿ�ָ�� */
		i++;
	}
	taskEXIT_CRITICAL();
	
	//����
	vTaskSuspend(app_task_menu_handle);
	vTaskSuspend(app_task_max30102_handle);

	printf("app_task_init  end!\r\n");
	
	/* ɾ���������� */
	vTaskDelete(NULL);
	
	
		
}   


/*�����ʱ������*/
static void vTimer_callback(TimerHandle_t pxTimer)
{		
	//20���Ϩ��
	ulCount++;
	if(ulCount>=20)
	{
		/* Ϩ�� */
		LCD_SAFE
		(
			lcd_display_on(0);
		);
		ulCount=0;
		
		
	}
	
	
	
}

/*mpu6050֮�Ʋ���������������*/
static void app_task_mpu6050_step(void* pvParameters)
{
	uint32_t i=0;
	beep_t beep;
	
	unsigned long  step_count_last=0;
	uint32_t sedentary_event=0;
	/* ���ò�����ֵΪ0*/
	
	while(dmp_set_pedometer_step_count(0))
	{
		delay_ms(500);
	}
	vTaskSuspend(NULL);
	for(;;)
	{
		
		
		/* ��ȡ���� */
		dmp_get_pedometer_step_count((unsigned long *)&step_count);
		
		printf("step_count=%d\r\n",step_count);	
				
		
		i++;
		if(i>=100)
		{
			dmp_get_pedometer_step_count((unsigned long *)&step_count);
			if((step_count-step_count_last)<5)
			{
				/* �����仯���������þ����¼���־λΪ1 */
				sedentary_event=1;
			}
			step_count_last=step_count;
				
			printf("[INFO] ��ǰ����:%ld ��ǰ����:%ld\r\n",step_count,step_count_last);
			
			
			i=0;
		}
		
		//��������
		if(sedentary_event)
		{
			sedentary_event=0;
			/* ��һ��ʾ�� */
			beep.sta = 1;
			beep.duration = 10;

			xQueueSend( g_queue_beep,  /* ��Ϣ���еľ�� */
						&beep,		   /* ���͵���Ϣ���� */
						100);		   /* �ȴ�ʱ�� 100 Tick */	
			
		}
		
		
		
		delay_ms(100);
	}
}

/* mpu6050̧֮���������� */
static void app_task_mpu6050(void* pvParameters)
{
	
	float pitch,roll,yaw; //ŷ����
	
	
	
	for(;;)
	{
		
			if(mpu_dmp_get_data(&pitch,&roll,&yaw)==0)
			{
				//̧������
				if(roll>=15)
				{
					PFout(9)=PFout(10)=0;
					PEout(13)=PEout(14)=0;
					
					//��ʱ������ֵ����
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


/* max30102�������Ѫ������ */
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
		
		   ��ǰ100������ת�����洢���У��������400�������Ƶ�����
		*/
		
        for(i=100;i<500;i++)
        {
            aun_red_buffer[i-100]=aun_red_buffer[i];
            aun_ir_buffer[i-100]=aun_ir_buffer[i];
            
            /* update the signal min and max 
			   �����ź���Сֵ�����ֵ
			*/
			
            if(un_min>aun_red_buffer[i])
				un_min=aun_red_buffer[i];
			
            if(un_max<aun_red_buffer[i])
				un_max=aun_red_buffer[i];
        }
		
		/* take 100 sets of samples before calculating the heart rate 
		
		   �ڼ�������֮ǰ�ɼ�100������
		*/
		
        for(i=400;i<500;i++)
        {
            un_prev_data=aun_red_buffer[i-1];
			
            while(MAX30102_INT==1);
			
            max30102_FIFO_ReadBytes(REG_FIFO_DATA,temp);
			
			/* ���ֵ�Ի��ʵ������ */
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
		
		/* �������ʺ�Ѫ�����Ͷ� */
		maxim_heart_rate_and_oxygen_saturation( (uint32_t  *)aun_ir_buffer, 	\
												(int32_t  )n_ir_buffer_length, 	\
												(uint32_t  *)aun_red_buffer, \
												(int32_t *)&n_sp02, 		\
												(int8_t *)&ch_spo2_valid, \
												(int32_t *)&n_heart_rate, 	\
												(int8_t *)&ch_hr_valid); 
			
     
													
		/* ͨ��UART�������ͼ��������͵��ն˳��� */
		if((ch_hr_valid == 1)&& (n_heart_rate>=60) && (n_heart_rate<=100))
		{
			
				memset(heart_buf, 0, sizeof heart_buf);
				printf("����=%d\r\n", n_heart_rate);
				lcd_show_chn(60,130,19,BLACK,WHITE,24);
				lcd_show_chn(92,130,20,BLACK,WHITE,24);
				sprintf(heart_buf,":%3d",n_heart_rate);
				lcd_show_string(130,130,(const char *)heart_buf,BLACK,WHITE,24,0);
				
			
		}

		
		if((ch_spo2_valid ==1)&& (n_sp02>=95) && (n_sp02<=100))
		{
			
				memset(sp_buf, 0, sizeof sp_buf);	
				printf("Ѫ��=%d\r\n", n_sp02); 
				lcd_show_chn(60,160,21,BLACK,WHITE,24);
				lcd_show_chn(92,160,22,BLACK,WHITE,24);
				sprintf(sp_buf,":%3d",n_sp02);
				lcd_show_string(130,160,(const char *)sp_buf,BLACK,WHITE,24,0);
			
			
			
		}			
	
		delay_ms(1000);
	}
	
	
}


/* ͨ���ɵ��������adc������ת�̲˵��Ŀ��� */
static void app_task_adc(void* pvParameters)
{
	int32_t adc_vol=0;
	int32_t adc_vol_last=0;
	int32_t result;
	
	/* ��ȡ��ǰ��ѹ��ֵ */
	adc_vol_last=get_adc_vol();
	
	for(;;)
	{
		/* ��ȡ��ѹֵ */
		adc_vol = get_adc_vol();
		result=adc_vol-adc_vol_last;
		
		if(result>200 || result<-200)
		{
			/* ����KEY_DOWN/KEY_UP�����¼� */
			xEventGroupSetBits(g_event_group, result > 0 ? EVENT_GROUP_FN_KEY_DOWN : EVENT_GROUP_FN_KEY_UP);
			
			adc_vol_last=adc_vol;
		}
		vTaskDelay(100);
	}
}


/* ��ҳʵʱʱ�ӵ���ʾ */
static void app_task_menu_show(void* pvParameters)
{
	
	EventBits_t EventValue;
	
	beep_t beep;
	
	menu_ext_t  m_ext;
	
	m_ext.menu=menu_main_1;
	
	
	
	for(;;)
	{
		
		/* �ȴ��¼����е���Ӧ�¼�λ����ͬ�� */
		EventValue = xEventGroupWaitBits((EventGroupHandle_t)g_event_group,
										 (EventBits_t)EVENT_GROUP_FN_KEY_MENU_ENTER|EVENT_GROUP_FN_KEY_MENU_BACK,
										 (BaseType_t)pdTRUE,
										 (BaseType_t)pdFALSE,
										 (TickType_t)portMAX_DELAY);
		
		/* ��һ��ʾ�� */
		beep.sta = 1;
		beep.duration = 10;

		xQueueSend( g_queue_beep,  /* ��Ϣ���еľ�� */
					&beep,		   /* ���͵���Ϣ���� */
					100);		   /* �ȴ�ʱ�� 100 Tick */	
		
		
		
		//����1����˵�������
		if(EventValue & EVENT_GROUP_FN_KEY_MENU_ENTER )
		{
			
			//��ʱ������ֵ����
			ulCount=0;
			
			//����
			lcd_display_on(1);
	
			
			
			
			//����
			LCD_SAFE(
				/* ���� */
				lcd_vs_set(0);
				lcd_clear(WHITE);
				lcd_fill(0,LCD_HEIGHT,LCD_WIDTH,80,WHITE);
			);
			
			
			//����ʱ��
			vTaskSuspend(app_task_rtc_handle);
			
			menu_show(&m_ext);
			
			/* �ָ�menu�������� */
			vTaskResume(app_task_menu_handle);
			
			
		}
		
		//����2����ʱ��������
		if(EventValue & EVENT_GROUP_FN_KEY_MENU_BACK )
		{
			
			//��ʱ������ֵ����
			ulCount=0;
			
			
			//����
			LCD_SAFE(
				/* ���� */
				lcd_vs_set(0);
				lcd_clear(WHITE);
				lcd_fill(0,LCD_HEIGHT,LCD_WIDTH,80,WHITE);
				lcd_draw_picture(70,100,100,66,gImage_hellokitty_100x66);	
			);
			
			
			
			//����menu����
			vTaskSuspend(app_task_menu_handle);
				
			//�ָ�ʱ����ʾ
			vTaskResume(app_task_rtc_handle);
			
			
			//����
			lcd_display_on(1);
			
		}
	}
}


/* ���Ʋ˵�������*/
static void app_task_menu(void* pvParameters)
{
	EventBits_t EventValue;
	
	beep_t beep;

	
	//���
	uint32_t item_total;		//��¼��ǰ�˵��ж��ٸ���Ŀ
	uint32_t item_cursor = 0;	//��¼��ǰ�˵�ָ���ĸ���Ŀ
	uint32_t fun_run = 0;		//��¼�Ƿ�����Ŀ���ܺ�����ִ��
	//�ɼ���
	uint16_t vs=0;
	menu_ext_t m_ext;
	
	menu_t *m;
	
	/* ͨ���������ݻ�ȡ���˵�ҳָ�� */
	//menu_t **m_main = (menu_t **)pvParameters;	
	
	//����
	//vTaskSuspend(NULL);
	/* ����m_extָ��ĵ�ǰ�˵�����ز��� */
	m_ext.menu = *menu_main_tbl;						
	m_ext.key_fn = KEY_NONE;
	m_ext.item_cursor = item_cursor;
	m_ext.item_total = menu_item_total(m_ext.menu);	
	
	
	/* ���ü�¼�˵�ָ��m */
	m = m_ext.menu;	
	
	//menu_show(&m_ext);
	
	for(;;)
	{
		/* �ȴ��¼����е���Ӧ�¼�λ����ͬ�� */
		EventValue = xEventGroupWaitBits((EventGroupHandle_t)g_event_group,
										(EventBits_t)     EVENT_GROUP_FN_KEY_UP \
										 				| EVENT_GROUP_FN_KEY_DOWN \
										 				| EVENT_GROUP_FN_KEY_ENTER \
										 				| EVENT_GROUP_FN_KEY_BACK,
										 (BaseType_t)pdTRUE,
										 (BaseType_t)pdFALSE,
										 (TickType_t)portMAX_DELAY);
		
		/* ��һ��ʾ�� */
		beep.sta = 1;
		beep.duration = 10;

		xQueueSend( g_queue_beep,  /* ��Ϣ���еľ�� */
					&beep,		   /* ���͵���Ϣ���� */
					100);		   /* �ȴ�ʱ�� 100 Tick */	
		
		//��ʱ������ֵ����
		ulCount=0;
			

		//���ڵ��谴����������
		if(EventValue & EVENT_GROUP_FN_KEY_UP )
		{
			
			/* ������Ŀ���ܺ��������У���ʾ��Ҫ���ز��ܽ�����Ŀѡ�� */
			if (fun_run)
			{
				continue;
			}
			
			if(item_cursor )
			{
				item_cursor--;
					
				/* ����RAM�Ĵ�ֱ������ʼ��ַ */
				if(item_cursor==0)
				{
					while(vs)
					{
						vs--;
						LCD_SAFE(lcd_vs_set(vs));
						vTaskDelay(5);
					}
					
				}
				m--;//��ǰ���ܲ˵�ָ��λ��
				
				/* ��ʾ��� */
				lcd_fill(230,(item_cursor+1)*60,10,60,WHITE);		
				lcd_fill(230,item_cursor*60,10,60,GREY);
		
			}
			
			m_ext.item_cursor=item_cursor;
			
		}
			
		////���ڵ��谴����������
		if(EventValue & EVENT_GROUP_FN_KEY_DOWN)
		{
			/* ������Ŀ���ܺ��������У���ʾ��Ҫ���ز��ܽ�����Ŀѡ�� */
			if (fun_run)
			{
				continue;
			}
			
			/* ��ȡ��ǰ�˵��ж��ٸ���Ŀ */
			item_total = menu_item_total(m_ext.menu);

			/* ���浱ǰ�˵��ж��ٸ���Ŀ */
			m_ext.item_total = item_total;

			
			if(item_cursor<(item_total-1))
			{
				item_cursor++;
					
				/* ����RAM�Ĵ�ֱ������ʼ��ַ */
				if(item_cursor==4)
				{
					while(vs<60)
					{
						vs++;
						LCD_SAFE(lcd_vs_set(vs));
						vTaskDelay(5);
					}
					
				}
				m++;//��ǰ���ܲ˵�ָ��λ��
				
				/* ��ʾ��� */
				lcd_fill(230,(item_cursor-1)*60,10,60,WHITE);		
				lcd_fill(230,item_cursor*60,10,60,GREY);
			}
			
			m_ext.item_cursor=item_cursor;
		}
		
		//����3�����Ӳ˵�
		if(EventValue & EVENT_GROUP_FN_KEY_ENTER)
		{
			m_ext.key_fn=KEY_ENTER;
			/* ������Ŀ���ܺ��������У���ʾ��Ҫ���ز��ܽ�����Ŀѡ�� */
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
		
		//����4�˳��Ӳ˵�
		if(EventValue & EVENT_GROUP_FN_KEY_BACK)
		{
			m_ext.key_fn = KEY_BACK;
			/* ���Ӳ˵����ܺ�����Ч����ִ�У���Ҫ�ǹ����Ӧ���� */
			if (m->fun)
			{
				/* �������Ŀ���ܺ��������� */
				fun_run = 1;
			
				m->fun(&m_ext);
			}
			
			
			/* ���˵���Ч */
			if (m->parent)
			{
				/* ָ�򸸲˵� */
				m = m->parent;

				/* ���浱ǰ�˵� */
				m_ext.menu = m;

				/* ��λ���ֵ */
				item_cursor = 0;
				m_ext.item_cursor = 0;

				
				fun_run = 0;

				/* ��ʾ��ǰ�˵� */
				menu_show(&m_ext);
			}
			
			
		}
			
		
	}
	
}


/*ʵʱʱ����ʾ����*/
static void app_task_rtc(void* pvParameters)
{
	
	beep_t beep;
	
	uint32_t i=0;
	
	
	EventBits_t EventValue;
	uint8_t time_buf[16]={0};
	uint8_t date_buf[16]={0}; 
	uint8_t step_buf[16]={0};
	

	LCD_SAFE(
		/* ���� */
		lcd_clear(WHITE);
		lcd_fill(0,LCD_HEIGHT,LCD_WIDTH,80,WHITE);
		
		lcd_draw_picture(70,100,100,66,gImage_hellokitty_100x66);	
	);
	
	for(;;)
	{
		/* �ȴ��¼����е���Ӧ�¼�λ����ͬ�� */
		EventValue = xEventGroupWaitBits((EventGroupHandle_t)g_event_group,
										 (EventBits_t)EVENT_GROUP_RTC_WAKEUP|EVENT_GROUP_RTC_ALARM,
										 (BaseType_t)pdTRUE,
										 (BaseType_t)pdFALSE,
										 (TickType_t)portMAX_DELAY);
		
		if(EventValue & EVENT_GROUP_RTC_WAKEUP)
		{
			LCD_SAFE(
	
				//��ȡСʱ��ʾ
				RTC_GetTime(RTC_Format_BCD,&RTC_TimeStructure);
				sprintf((char *)time_buf,"%02x:",RTC_TimeStructure.RTC_Hours);
				lcd_show_string(0,0,(const char *)time_buf,BLACK,WHITE,96,0);
				//��ȡ������ʾ
				sprintf((char *)time_buf,"%02x",RTC_TimeStructure.RTC_Minutes);
				lcd_show_string(140,0,(const char *)time_buf,BLACK,WHITE,96,0);
				//��ȡ����ʾ
				lcd_draw_rectangle(170,90,220,140,RED);
				sprintf((char *)time_buf,"%02x",RTC_TimeStructure.RTC_Seconds);
				lcd_show_string(180,100,(const char *)time_buf,BLACK,WHITE,32,0);
				

				//��ȡ������ʾ
				RTC_GetDate(RTC_Format_BCD,&RTC_DateStructure);
				sprintf((char *)date_buf,"20%02x/%02x/%02x",
					RTC_DateStructure.RTC_Year,
					RTC_DateStructure.RTC_Month,
					RTC_DateStructure.RTC_Date);	
				lcd_show_string(60,170,(const char *)date_buf,BLACK,WHITE,24,0);
				
				//��ȡ������ʾ
				sprintf((char *)date_buf,"WEEK %d",RTC_DateStructure.RTC_WeekDay);
				lcd_show_string(90,200,(const char *)date_buf,BLACK,WHITE,24,0);
				
				
			);
				
			//����ͼ��
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
			
			//����ͼ��
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
			
			//�˶�ͼ��
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
			printf("�������ˣ�\n");
				
			for(i=0;i<5;i++)
			{
				/* ��һ��ʾ�� */
				beep.sta = 1;
				beep.duration = 10;

				xQueueSend( g_queue_beep,  /* ��Ϣ���еľ�� */
							&beep,		   /* ���͵���Ϣ���� */
							100);		   /* �ȴ�ʱ�� 100 Tick */	
				delay_ms(500);
			}
			
			
		}
		
		
		
		

	}
		
}   


/*��������*/
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
	
		if(EventValue & EVENT_GROUP_KEY1_DOWN)//����1
		{
			/* ����KEY_MENU_ENTER�����¼� */
			xEventGroupSetBits(g_event_group, EVENT_GROUP_FN_KEY_MENU_ENTER);
			
			
		}
		
		if(EventValue & EVENT_GROUP_KEY2_DOWN)//����2
		{
			/* ����KEY_MENU_BACK�����¼� */
			xEventGroupSetBits(g_event_group, EVENT_GROUP_FN_KEY_MENU_BACK);

		}
		
		if(EventValue & EVENT_GROUP_KEY3_DOWN)//����3
		{
			/* ����KEY_ENTER�����¼� */
			xEventGroupSetBits(g_event_group, EVENT_GROUP_FN_KEY_ENTER);

		}
		if(EventValue & EVENT_GROUP_KEY4_DOWN)//����4
		{
			/* ����KEY_BACK�����¼� */
			xEventGroupSetBits(g_event_group, EVENT_GROUP_FN_KEY_BACK);

		}
		
	}
}


/*����������*/
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

			/* ������״̬��ת */
			beep.sta ^= 1;
		}


		//Ĭ�Ͼ���
		BEEP(beep.sta);
			  
	}
}

/*��������*/
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
		
		//���ڽ�������
		xQueueReceive(g_queue_usart,buf,portMAX_DELAY);	  
		
	
		printf("%s\r\n",buf);

		
		//����ʱ��
		if(g_rtc_get_what==FLAG_RTC_GET_TIME || g_ble_status==FLAG_BLE_STATUS_CONNECT)
		{
			//��TIME SET-10-20-30#�� 
			if(strstr((const char*)buf,"TIME SET"))
			{
				
				p=strtok((char *)buf,"-");
				//Сʱ10
				p=strtok(NULL,"-");
				hours=atoi(p);
				//����20
				p = strtok(NULL,"-");
				minutes = atoi(p);
				//��30
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
		
		//��������
		if(g_rtc_get_what==FLAG_RTC_GET_DATE || g_ble_status==FLAG_BLE_STATUS_CONNECT)
		{
			
			//��DATE SET-2023-5-25-4#��
			if(strstr((const char*)buf,"DATE SET"))
			{
				p=strtok((char *)buf,"-");
				p=strtok(NULL,"-");//2023
				year=atoi(p);
				//��ȡ�·�
				p = strtok(NULL,"-");
				month = atoi(p);

				//��ȡ����
				p = strtok(NULL,"-");
				day = atoi(p);
				//printf("p=%s",p);

				
				//��ȡ���ڼ�
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
		
		//��������
		if(g_alarm_set==FLAG_ALARM_SET_START || g_ble_status==FLAG_BLE_STATUS_CONNECT)
		{
			//��ALARM SET-10-20-30#�� 
			if(strstr((const char*)buf,"ALARM SET"))
			{
				
				//�ر�����
				RTC_AlarmCmd(RTC_Alarm_A, DISABLE);
				
				p=strtok((char *)buf,"-");
				//Сʱ10
				p=strtok(NULL,"-");
				hours=atoi(p);
				//����20
				p = strtok(NULL,"-");
				minutes = atoi(p);

				//��30
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
				RTC_AlarmStructure.RTC_AlarmMask = RTC_AlarmMask_DateWeekDay;	//�������ں����ڣ���������ÿ�춼��Ч

		
				/* ����RTC��A���ӣ�ע��RTC���������������ֱ�Ϊ����A������B */
				RTC_SetAlarm(RTC_Format_BIN, RTC_Alarm_A, &RTC_AlarmStructure);
				
				/* ��RTC������A����*/
				RTC_AlarmCmd(RTC_Alarm_A, ENABLE);
				
				printf("alarm set ok!\r\n");
				
				if(g_alarm_set==FLAG_ALARM_SET_START )
					lcd_draw_picture(70,90,100,100,gImage_succee_100x100);	
				
				//��������ͼ��
				g_alarm_pic=1;
				
			}
			if(strstr((const char*)buf,"ALARM OFF"))
			{
				printf("���ӹر�!\r\n");
				//�ر�����
				RTC_AlarmCmd(RTC_Alarm_A, DISABLE);
				
				if(g_alarm_set==FLAG_ALARM_SET_START )
					lcd_draw_picture(70,90,100,100,gImage_fail_100x100);	
	
				//Ϩ������ͼ��
				g_alarm_pic=0;
			}
		}
		
		
		
	}
	
} 


/*��ʪ������*/
static void app_task_dht11(void* pvParameters)
{
	uint8_t dht11_data[5]={0};
	char buf[16]={0};
	
	int32_t rt=-1;	
	
	vTaskSuspend(NULL);
	
	/* ��ʼ������ʱһ�� */
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
