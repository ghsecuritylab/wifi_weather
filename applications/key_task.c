#include "main.h"



extern char pic_str[17][11];


#define KEY0_PIN GET_PIN(D, 10)
#define KEY1_PIN GET_PIN(D, 9)
#define KEY2_PIN GET_PIN(D, 8)
#define KEY3_PIN GET_PIN(C, 13)

static Button_t KEY0;
static Button_t KEY1;
static Button_t KEY2;
static Button_t KEY3;

static volatile int pic_offset = 0; //图片偏移位置

volatile uint8_t  change_picture = 0;

static void Key0_Dowm_CallBack(void *btn)
{
    change_picture--;
    
    if(change_picture == 0 || change_picture > MAX_LCD_PIC)
      change_picture = MAX_LCD_PIC-1;
}

static void Key1_Dowm_CallBack(void *btn)
{
    change_picture++;
    if(change_picture > MAX_LCD_PIC-1)
      change_picture = 0;
}

static void Key2_Dowm_CallBack(void *btn)
{
	rt_kprintf("Key2 Single click!\r\n");
}

static void Key3_Dowm_CallBack(void *btn)
{
	rt_kprintf("Key3 Single click!\r\n");
}

static uint8_t Read_Key0_Value(void)
{
	if (rt_pin_read(KEY0_PIN))
		return 1;
	else
		return 0;
}

static uint8_t Read_Key1_Value(void)
{
	if (rt_pin_read(KEY1_PIN))
		return 1;
	else
		return 0;
}
static uint8_t Read_Key2_Value(void)
{
	if (rt_pin_read(KEY2_PIN))
		return 1;
	else
		return 0;
}
static uint8_t Read_Key3_Value(void)
{
	if (rt_pin_read(KEY3_PIN))
		return 1;
	else
		return 0;
}

static void Dev_Key_Init(void)
{
    	/*Init Button Dev*/
	rt_pin_mode(KEY0_PIN, PIN_MODE_INPUT);
	rt_pin_mode(KEY1_PIN, PIN_MODE_INPUT);
	rt_pin_mode(KEY2_PIN, PIN_MODE_INPUT);
	rt_pin_mode(KEY3_PIN, PIN_MODE_INPUT);

	Button_Create("KEY0", &KEY0, Read_Key0_Value, 0);
	Button_Attach(&KEY0, BUTTON_DOWM, Key0_Dowm_CallBack); //单击
	//  Button_Attach(&KEY0, BUTTON_DOUBLE, NULL); //双击
	//  Button_Attach(&KEY0, BUTTON_LONG, NULL);     //长按

	Button_Create("KEY1", &KEY1, Read_Key1_Value, 0);
	Button_Attach(&KEY1, BUTTON_DOWM, Key1_Dowm_CallBack); //单击
	//  Button_Attach(&KEY1, BUTTON_DOUBLE, NULL); //双击
	//  Button_Attach(&KEY1, BUTTON_LONG, NULL);     //长按

	Button_Create("KEY2", &KEY2, Read_Key2_Value, 0);
	Button_Attach(&KEY2, BUTTON_DOWM, Key2_Dowm_CallBack); //单击
	//  Button_Attach(&KEY2, BUTTON_DOUBLE, SW3_Double_CallBack); //双击
	//  Button_Attach(&KEY2, BUTTON_LONG, SW3_Long_CallBack);     //长按

	Button_Create("KEY3", &KEY3, Read_Key3_Value, 1);
	Button_Attach(&KEY3, BUTTON_DOWM, Key3_Dowm_CallBack); //单击
	//  Button_Attach(&KEY3, BUTTON_DOUBLE, SW3_Double_CallBack); //双击
	//  Button_Attach(&KEY3, BUTTON_LONG, SW3_Long_CallBack);     //长按
}

void button_thread_entry(void *parameter)
{
	Dev_Key_Init();
	for (;;)
	{
		Button_Process();
		rt_thread_mdelay(20);
	}
}

static int key_app_init(void)
{
    rt_thread_t tid;
    
	tid = rt_thread_create("button_thread", button_thread_entry, RT_NULL,
						   RT_KEY_THREAD_STACK_SIZE, RT_KEY_THREAD_PRIORITY, 20);
    if (tid)
    {
        rt_thread_startup(tid);
        rt_kprintf("button_thread create success!\n");
    }
    else
    {
        rt_kprintf("Create button_thread thread fail!");
        return -RT_ENOMEM;
    }
     return RT_EOK;
}
INIT_APP_EXPORT(key_app_init);
