#include "main.h"


#define LEDR_PIN        GET_PIN(E, 7)
#define LEDB_PIN        GET_PIN(E, 8)    
#define LEDG_PIN        GET_PIN(E, 9)   

#define  BSP_LED_GPIO_PORT             GPIOE
#define  BSP_LEDR_PIN                  1UL<<7
#define  BSP_LEDB_PIN                  1UL<<8
#define  BSP_LEDG_PIN                  1UL<<9
#define  BSP_LED_ALL_PIN               (BSP_LEDR_PIN|BSP_LEDB_PIN|BSP_LEDG_PIN)
typedef  enum  bsp_led {
    USER_LDR = 1,
    USER_LDG = 2,
    USER_LDB = 3,
    USER_LED_ALL = 4,
} BSP_LED;


static void bsp_led_toggle(BSP_LED led)
{
    
    switch(led)
    {
    case USER_LDR:
      HAL_GPIO_TogglePin(BSP_LED_GPIO_PORT,BSP_LEDR_PIN);
      break;
    case USER_LDB:
      HAL_GPIO_TogglePin(BSP_LED_GPIO_PORT,BSP_LEDB_PIN);
      break;
    case USER_LDG:
      HAL_GPIO_TogglePin(BSP_LED_GPIO_PORT,BSP_LEDG_PIN);
      break;
    case USER_LED_ALL:
     HAL_GPIO_TogglePin(BSP_LED_GPIO_PORT, BSP_LED_ALL_PIN);
        break;
    default :
      break;
    }
}

void led_thread_entry(void *parameter)
{
    uint8_t tick ;
	/* set LED0 pin mode to output */
	rt_pin_mode(LEDR_PIN, PIN_MODE_OUTPUT);
    rt_pin_mode(LEDG_PIN, PIN_MODE_OUTPUT);
    rt_pin_mode(LEDB_PIN, PIN_MODE_OUTPUT);
    rt_pin_write(LEDR_PIN,1);
    rt_pin_write(LEDG_PIN,1);
    rt_pin_write(LEDB_PIN,1);
	for (;;)
	{
        tick= rt_tick_get() % 4;
        bsp_led_toggle((BSP_LED)tick);
		rt_thread_mdelay(500);
	}
}

static int led_app_init(void)
{
    rt_thread_t tid;

	tid = rt_thread_create("led_thread", led_thread_entry, RT_NULL,
						   RT_LED_THREAD_STACK_SIZE, RT_LED_THREAD_PRIORITY, 20);
    if (tid)
    {
        rt_thread_startup(tid);
        rt_kprintf("led_thread create success!\n");
        return -RT_ENOMEM;
    }
    else
    {
        rt_kprintf("Create led_thread thread fail!");
    }
    return  RT_EOK;
}
INIT_APP_EXPORT(led_app_init);
