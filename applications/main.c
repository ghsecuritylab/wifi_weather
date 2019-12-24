
#include <rtthread.h>
#include <rtdevice.h>
#include <board.h>
#include <msh.h>
#include <easyflash.h>
#include <fal.h>
#include <fal_def.h>
#include <rttlogo.h>
#include <dfs_fs.h>
#include <dfs_file.h>
#include <at.h>
#include <at_socket.h>
#include <fontupd.h>
#include "button.h"
#include "aht10.h"
#include "ap3216c.h"
#include "drv_lcd.h"

int Jpeg_Dec(char *path);
void weather_open_picture(char *name);
void esp_tcp_connect(void);
int Cjson_Mem_Malloc(void);
/*天气图标素材*/
static struct dfs_fd pic_fd;
static struct dirent pic_dirent;
static int pic_offset = 0;

static char pic_str[17][11] =
	{
		{"70@1x.jpg"},
		{"1@1x.jpg"},
		{"2@1x.jpg"},
		{"3@1x.jpg"},
		{"4@1x.jpg"},
		{"5@1x.jpg"},
		{"6@1x.jpg"},
		{"7@1x.jpg"},
		{"8@1x.jpg"},
		{"9@1x.jpg"},
		{"10@1.jpg"},
		{"11@1x.jpg"},
		{"12@1x.jpg"},
		{"13@1x.jpg"},
		{"14@1x.jpg"},
		{"15@1x.jpg"},
		{"16@1x.jpg"}};
/*User Task Define*/
#define LED_TASK
#define BUTTON_TASK
#define SENSOR_TASK
#define LCD_TASK
#define WLAN_TASK

/*User Task*/
#ifdef LED_TASK
#define RT_LED_THREAD_STACK_SIZE 4096
#define RT_LED_THREAD_PRIORITY 1

#define LED0_PIN GET_PIN(E, 7)
void led_thread_entry(void *parameter);
/* defined the LED0 pin: PE7 */
#endif

#ifdef BUTTON_TASK
#define RT_KEY_THREAD_STACK_SIZE 1024
#define RT_KEY_THREAD_PRIORITY 2

#define KEY0_PIN GET_PIN(D, 10)
#define KEY1_PIN GET_PIN(D, 9)
#define KEY2_PIN GET_PIN(D, 8)
#define KEY3_PIN GET_PIN(C, 13)

Button_t KEY0;
Button_t KEY1;
Button_t KEY2;
Button_t KEY3;

void button_thread_entry(void *parameter);
#endif

#ifdef SENSOR_TASK
#define RT_SENSOR_THREAD_STACK_SIZE 1024
#define RT_SENSOR_THREAD_PRIORITY 3

void sensor_thread_entry(void *parameter);
/* defined the LED0 pin: PE7 */
uint16_t ps_data = 0;
float brightness = 0;
float humidity = 0;
float temperature = 0;
#endif

#ifdef LCD_TASK
#define RT_LCD_THREAD_STACK_SIZE 4096
#define RT_LCD_THREAD_PRIORITY 4
void lcd_thread_entry(void *parameter);
#endif

#ifdef WLAN_TASK
#define RT_ESP_THREAD_STACK_SIZE 4096
#define RT_ESP_THREAD_PRIORITY 5
void esp_thread_entry(void *parameter);
#endif

/*切换图片信号量*/
static struct rt_mailbox mb = {0};
/* 用于放邮件的内存池 */
static char mb_pool[4];

#define FS_PARTITION_NAME "filesystem"
/*Main Function Make User Function*/

/*filesystem 
  offset 0x00200000
  lenth  0x00e00000
用来存放字库与图片
*/

int main(void)
{
	static struct dfs_fd p_fd;
	rt_thread_t tid;

	fal_init();
	while (easyflash_init() != EF_NO_ERR)
	{
	};

	/* Create a block device on the file system partition of spi flash */
	struct rt_device *flash_dev = fal_blk_device_create(FS_PARTITION_NAME);
	if (flash_dev == NULL)
	{
		rt_kprintf("Can't create a block device on '%s' partition.\n", FS_PARTITION_NAME);
	}
	else
	{
		rt_kprintf("Create a block device on the %s partition of flash successful.\n", FS_PARTITION_NAME);
	}
	if (dfs_mount("sd0", "/", "elm", 0, 0) == 0)
	{
		rt_kprintf("[TF]Filesystem initialized!\n");
	}
	else
	{
		rt_kprintf("[TF]Failed to initialize filesystem!\n");
	}

	if (dfs_file_open(&p_fd, "sf", O_DIRECTORY | O_CREAT) == 0)
	{
		/* mount the file system from "filesystem" partition of spi flash. */
		if (dfs_mount(FS_PARTITION_NAME, "/sf", "elm", 0, 0) == 0)
		{
			rt_kprintf("Filesystem initialized!\n");
		}
		else
		{
			rt_kprintf("Failed to initialize filesystem!\n");
			rt_kprintf("You should create a filesystem on the block device first!\n");
		}
	}
#ifdef LED_TASK
	tid = rt_thread_create("led_thread", led_thread_entry, RT_NULL,
						   RT_LED_THREAD_STACK_SIZE, RT_LED_THREAD_PRIORITY, 20);
	RT_ASSERT(tid != RT_NULL);
	rt_thread_startup(tid);
#endif
#ifdef BUTTON_TASK
	tid = rt_thread_create("button_thread", button_thread_entry, RT_NULL,
						   RT_KEY_THREAD_STACK_SIZE, RT_KEY_THREAD_PRIORITY, 20);
	RT_ASSERT(tid != RT_NULL);
	rt_thread_startup(tid);
#endif

#ifdef SENSOR_TASK
	tid = rt_thread_create("sensor_thread", sensor_thread_entry, RT_NULL,
						   RT_SENSOR_THREAD_STACK_SIZE, RT_SENSOR_THREAD_PRIORITY, 20);
	RT_ASSERT(tid != RT_NULL);
	rt_thread_startup(tid);
#endif

#ifdef LCD_TASK
	tid = rt_thread_create("lcd_thread", lcd_thread_entry, RT_NULL,
						   RT_LCD_THREAD_STACK_SIZE, RT_LCD_THREAD_PRIORITY, 20);
	RT_ASSERT(tid != RT_NULL);
	rt_thread_startup(tid);
#endif

#ifdef WLAN_TASK
	tid = rt_thread_create("esp_thread", esp_thread_entry, RT_NULL,
						   RT_ESP_THREAD_STACK_SIZE, RT_ESP_THREAD_PRIORITY, 20);
	RT_ASSERT(tid != RT_NULL);
	rt_thread_startup(tid);
#endif

	/* 初始化一个 mailbox */
	if (rt_mb_init(&mb,
				   "mbt",						/* 名称是 mbt */
				   &mb_pool[0],					/* 邮箱用到的内存池是 mb_pool */
				   sizeof(mb_pool) / 4,			/* 邮箱中的邮件数目，因为一封邮件占 4 字节 */
				   RT_IPC_FLAG_FIFO) != RT_EOK) /* 采用 FIFO 方式进行线程等待 */
	{
		rt_kprintf("init mailbox failed.\n");
		return -1;
	}

	for (;;)
	{
		rt_thread_mdelay(1000);
	}
}

#ifdef LED_TASK
void led_thread_entry(void *parameter)
{
	/* set LED0 pin mode to output */
	rt_pin_mode(LED0_PIN, PIN_MODE_OUTPUT);

	for (;;)
	{
		rt_pin_write(LED0_PIN, PIN_HIGH);
		rt_thread_mdelay(500);
		rt_pin_write(LED0_PIN, PIN_LOW);
		rt_thread_mdelay(500);
	}
}
#endif

#ifdef BUTTON_TASK
void Key0_Dowm_CallBack(void *btn)
{
	pic_offset++;
	if (pic_offset > 16)
		pic_offset = 0;

	rt_mb_send(&mb, (rt_ubase_t)&pic_str[pic_offset]); //发送邮箱

	rt_kprintf("Key0 Single click!\r\n");
}

void Key1_Dowm_CallBack(void *btn)
{
	pic_offset--;
	if (pic_offset < 0)
		pic_offset = 16;

	rt_mb_send(&mb, (rt_ubase_t)&pic_str[pic_offset]); //发送邮箱
}

void Key2_Dowm_CallBack(void *btn)
{
	rt_kprintf("Key2 Single click!\r\n");
}

void Key3_Dowm_CallBack(void *btn)
{
	rt_kprintf("Key3 Single click!\r\n");
}

uint8_t Read_Key0_Value(void)
{
	if (rt_pin_read(KEY0_PIN))
		return 1;
	else
		return 0;
}

uint8_t Read_Key1_Value(void)
{
	if (rt_pin_read(KEY1_PIN))
		return 1;
	else
		return 0;
}
uint8_t Read_Key2_Value(void)
{
	if (rt_pin_read(KEY2_PIN))
		return 1;
	else
		return 0;
}
uint8_t Read_Key3_Value(void)
{
	if (rt_pin_read(KEY3_PIN))
		return 1;
	else
		return 0;
}

void button_thread_entry(void *parameter)
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
	for (;;)
	{
		Button_Process();
		rt_thread_mdelay(20);
	}
}
#endif

#ifdef SENSOR_TASK
void sensor_thread_entry(void *parameter)
{
	aht10_device_t dev1;				/* device object */
	const char *i2c1_bus_name = "i2c4"; /* aht10 i2c bus station */

	ap3216c_device_t dev2;				/* device object */
	const char *i2c2_bus_name = "i2c1"; /* i2c bus */

	dev1 = aht10_init(i2c1_bus_name); /*get aht10 dev struct*/

	dev2 = ap3216c_init(i2c2_bus_name);

	for (;;)
	{
		if (dev1 != RT_NULL)
		{
			/* read humidity */
			humidity = aht10_read_humidity(dev1);
			lcd_show_string(0, 176, 16, "humidity:  . %", humidity);
			lcd_show_num(72, 176, (int)humidity, 1, 16);
			lcd_show_num(96, 176, (int)(humidity * 10) % 10, 1, 16);

			/* read temperature */
			temperature = aht10_read_temperature(dev1);
			lcd_show_string(0, 192, 16, "temperature:  . %", temperature);
			lcd_show_num(96, 192, (int)temperature, 1, 16);
			lcd_show_num(128, 192, (int)(temperature * 10) % 10, 1, 16);
		}
		else
			lcd_show_string(0, 176, 32, "no aht10!");

		if (dev2 != RT_NULL)
		{
			/* read ps data */
			ps_data = ap3216c_read_ps_data(dev2);
			lcd_show_string(0, 208, 16, "ps_data:    ", temperature);
			lcd_show_num(64, 208, (int)ps_data, 1, 16);

			/* read als data */
			brightness = ap3216c_read_ambient_light(dev2);
			lcd_show_string(0, 224, 16, "brightness:  . %", brightness);
			lcd_show_num(88, 224, (int)brightness, 1, 16);
			lcd_show_num(112, 224, (int)(brightness * 10) % 10, 1, 16);
		}
		rt_thread_mdelay(1000); //1S读取一次传感器设备数据
	}
}
#endif

#ifdef LCD_TASK
void lcd_thread_entry(void *parameter)
{
	char *sem_val = 0;

	/* clear lcd */
	lcd_clear(WHITE);

	while (font_init()) //更新字库
	{
		update_font(30, 160, 16, "/");
	}
	/* show RT-thread logo on lcd */
	lcd_show_image(0, 69, 240, 69, image_rttlogo);
	rt_thread_mdelay(3000);
	lcd_clear(WHITE);
	lcd_show_hz(0, 0, 200, 16, "城市", 16);
    lcd_show_string(32, 0, 16, ":");
	lcd_show_hz(0, 20, 200, 16, "天气", 16);
    lcd_show_string(32, 20, 16, ":");
	lcd_show_hz(0, 40, 200, 16, "温度", 16);
    lcd_show_string(32, 40, 16, ":");

	for (;;)
	{
		if (rt_mb_recv(&mb, (rt_ubase_t *)&sem_val, RT_WAITING_FOREVER) == RT_EOK)
		{
			weather_open_picture(sem_val);
		}
		rt_thread_mdelay(1000);
	}
}
#endif

/*该任务1分钟执行一次，获取时间->本日天气->最近三天天气
时间可以同步到RTC，考虑到程序处理,存在误差
*/
void esp_thread_entry(void *parameter)
{
   Cjson_Mem_Malloc(); 
	for (;;)
	{
		rt_thread_mdelay(20000);
		esp_tcp_connect();
	}
}

/*打开图片*/
void weather_open_picture(char *name)
{
	char *fullpath = NULL;
	struct stat pic_stat;
	int length;
	char path[20] = "/PICTURE";
	/* list directory */

	if (dfs_file_open(&pic_fd, path, O_DIRECTORY) == 0)
	{
		rt_kprintf("Directory %s:\n", path);
		do
		{
			memset(&pic_dirent, 0, sizeof(struct dirent));
			length = dfs_file_getdents(&pic_fd, &pic_dirent, sizeof(struct dirent));
			if (length > 0)
			{
				memset(&pic_stat, 0, sizeof(struct stat));

				/* build full path for each file */
				fullpath = dfs_normalize_path(path, pic_dirent.d_name);
				if (fullpath == NULL)
					break;

				if (dfs_file_stat(fullpath, &pic_stat) == 0)
				{
					if (strcmp(pic_dirent.d_name, name) == 0)
					{
                        lcd_fill(0,70,240,175,BLACK);        
						Jpeg_Dec(fullpath); //解码图片
						rt_free(fullpath);
						break; //已经找到要显示的图片，退出本次循环
					}
				}
				else
				{
					lcd_show_string(0, 0, 24, "can't find picture : %s\n", name);
				}

				rt_free(fullpath);
			}
		} while (length > 0);

		dfs_file_close(&pic_fd);
	}
	else
	{
		rt_kprintf("No such directory\n");
	}
	if (name == NULL)
		rt_free(path);
}