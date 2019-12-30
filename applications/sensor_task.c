#include "main.h"

static uint16_t ps_data = 0;
static float brightness = 0;
static float humidity = 0;
static float temperature = 0;

void sensor_thread_entry(void *parameter)
{
	aht10_device_t dev1;				/* device object */
	const char *i2c1_bus_name = "i2c4"; /* aht10 i2c bus station */
	ap3216c_device_t dev2;				/* device object */
	const char *i2c2_bus_name = "i2c1"; /* i2c bus */
	dev1 = aht10_init(i2c1_bus_name);  /*get aht10 dev struct*/
	dev2 = ap3216c_init(i2c2_bus_name);

	for (;;)
	{
		if (dev1 != RT_NULL)
		{
			/* read humidity */
			humidity = aht10_read_humidity(dev1);
			/* read temperature */
			temperature = aht10_read_temperature(dev1);

		}
		if (dev2 != RT_NULL)
		{
			/* read ps data */
			ps_data = ap3216c_read_ps_data(dev2);

			/* read als data */
			brightness = ap3216c_read_ambient_light(dev2);
		}
		rt_thread_mdelay(1000); //1S读取一次传感器设备数据
	}
}

void lcd_disp_sensor(void)
{
    lcd_show_string(0, 176, 16, "humidity:  . %", humidity);
	lcd_show_num(72, 176, (int)humidity, 1, 16);
	lcd_show_num(96, 176, (int)(humidity * 10) % 10, 1, 16);
    
    lcd_show_string(0, 192, 16, "temperature:  .", temperature);
	lcd_show_num(96, 192, (int)temperature, 1, 16);
	lcd_show_num(120, 192, (int)(temperature * 10) % 10, 1, 16);
    
    lcd_show_string(0, 208, 16, "ps_data:    ", temperature);
	lcd_show_num(64, 208, (int)ps_data, 1, 16);
    
    lcd_show_string(0, 224, 16, "brightness:    .  ", brightness);
	lcd_show_num(88, 224, (int)brightness, 1, 16);
	lcd_show_num(128, 224, (int)(brightness * 10) % 10, 1, 16);
}

static int sensor_app_init(void)
{
    rt_thread_t tid;

	tid = rt_thread_create("sensor_thread", sensor_thread_entry, RT_NULL,
						   RT_SENSOR_THREAD_STACK_SIZE, RT_SENSOR_THREAD_PRIORITY, 20);
    if (tid)
    {
        rt_thread_startup(tid);
        rt_kprintf("sensor_thread create success!\n");
    }
    else
    {
        rt_kprintf("Create sensor_thread thread fail!");
        return -RT_ENOMEM;
    }
    return RT_EOK;
}
INIT_APP_EXPORT(sensor_app_init);