
#include "main.h"
#include <rttlogo.h>

extern int Jpeg_Dec(char *path);

static void lcd_disp_date(void);
void lcd_disp_sensor(void);
static void lcd_disp_oneday(void);
/*天气图标素材*/
static struct dfs_fd pic_fd;
static struct dirent pic_dirent;

RTC_TIME rtc_time = {0};
ONE_WEATHER one_weather = {0};
THREE_WEAHTER three_weather = {0};
LCD_PIC pic_coord = {0};

const char pic1_str[MAX_PIC_NUM][PIC_NAME_LEN] =
    {
        {"0@1x.jpg"},
        {"1@1x.jpg"},
        {"2@1x.jpg"},
        {"3@1x.jpg"},
        {"4@1x.jpg"},
        {"5@1x.jpg"},
        {"6@1x.jpg"},
        {"7@1x.jpg"},
        {"8@1x.jpg"},
        {"9@1x.jpg"},
        {"10@1x.jpg"},
        {"11@1x.jpg"},
        {"12@1x.jpg"},
        {"13@1x.jpg"},
        {"14@1x.jpg"},
        {"15@1x.jpg"},
        {"17@1x.jpg"},
        {"18@1x.jpg"},
        {"19@1x.jpg"},
        {"20@1x.jpg"},
        {"21@1x.jpg"},
        {"22@1x.jpg"},
        {"23@1x.jpg"},
        {"24@1x.jpg"},
        {"25@1x.jpg"},
        {"26@1x.jpg"},
        {"27@1x.jpg"},
        {"28@1x.jpg"},
        {"29@1x.jpg"},
        {"30@1x.jpg"},
        {"31@1x.jpg"},
        {"32@1x.jpg"},
        {"33@1x.jpg"},
        {"34@1x.jpg"},
        {"35@1x.jpg"},
        {"36@1x.jpg"},
        {"37@1x.jpg"},
        {"38@1x.jpg"},
        {"99@1x.jpg"}};

const char pic2_str[MAX_PIC_NUM][PIC_NAME_LEN] =
    {
        {"0@2x.jpg"},
        {"1@2x.jpg"},
        {"2@2x.jpg"},
        {"3@2x.jpg"},
        {"4@2x.jpg"},
        {"5@2x.jpg"},
        {"6@2x.jpg"},
        {"7@2x.jpg"},
        {"8@2x.jpg"},
        {"9@2x.jpg"},
        {"10@2.jpg"},
        {"11@2x.jpg"},
        {"12@2x.jpg"},
        {"13@2x.jpg"},
        {"14@2x.jpg"},
        {"15@2x.jpg"},
        {"17@2x.jpg"},
        {"18@2x.jpg"},
        {"19@2x.jpg"},
        {"20@2x.jpg"},
        {"21@2x.jpg"},
        {"22@2x.jpg"},
        {"23@2x.jpg"},
        {"24@2x.jpg"},
        {"25@2x.jpg"},
        {"26@2x.jpg"},
        {"27@2x.jpg"},
        {"28@2x.jpg"},
        {"29@2x.jpg"},
        {"30@2x.jpg"},
        {"31@2x.jpg"},
        {"32@2x.jpg"},
        {"33@2x.jpg"},
        {"34@2x.jpg"},
        {"35@2x.jpg"},
        {"36@2x.jpg"},
        {"37@2x.jpg"},
        {"38@2x.jpg"},
        {"99@2x.jpg"}};

/*打开图片*/
static void weather_open_picture(char *name)
{
    char *fullpath = NULL;
    struct stat pic_stat;
    int length;
    char path[20] = "/PICTURE";
    /* list directory */

    if (dfs_file_open(&pic_fd, path, O_DIRECTORY) == 0)
    {
        //		rt_kprintf("Directory %s:\n", path);
        do
        {
            rt_memset(&pic_dirent, 0, sizeof(struct dirent));
            length = dfs_file_getdents(&pic_fd, &pic_dirent, sizeof(struct dirent));
            if (length > 0)
            {
                rt_memset(&pic_stat, 0, sizeof(struct stat));

                /* build full path for each file */
                fullpath = dfs_normalize_path(path, pic_dirent.d_name);
                if (fullpath == NULL)
                    break;

                if (dfs_file_stat(fullpath, &pic_stat) == 0)
                {
                    if (strcmp(pic_dirent.d_name, name) == 0)
                    {
                        Jpeg_Dec(fullpath); //解码图片
                        rt_free(fullpath);
                        break; //已经找到要显示的图片，退出本次循环
                    }
                }
                //				else
                //				{
                //					lcd_show_string(0, 0, 24, "can't find picture : %s\n", name);
                //				}

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

static int filesystem_init(void)
{
    static struct dfs_fd p_fd = {0};
    //    rt_err_t result = RT_EOK;

    fal_init();
    while (easyflash_init() != EF_NO_ERR)
    {
    };

    /* Create a block device on the file system partition of spi flash */
    struct rt_device *flash_dev = fal_blk_device_create(FS_PARTITION_NAME); //创建块设备
    if (flash_dev == NULL)
    {
        rt_kprintf("Can't create a block device on '%s' partition.\n", FS_PARTITION_NAME);
        return -1;
    }
    else
    {
        rt_kprintf("Create a block device on the %s partition of flash successful.\n", FS_PARTITION_NAME);
    }

    //	result = dfs_mkfs("elm", FS_PARTITION_NAME); //在指定的挂载设备中创建文件系统
    //    while(result != 0)
    //    {
    //        result = dfs_mkfs("elm", FS_PARTITION_NAME);
    //    }

    if (dfs_mount(FS_PARTITION_NAME, "/", "elm", 0, 0) == 0) //挂载 SPIFLASH 块设备到 '/'根目录
    {
        rt_kprintf("Filesystem initialized!\n");
    }
    else
    {
        rt_kprintf("Failed to initialize filesystem!\n");
        rt_kprintf("You should create a filesystem on the block device first!\n");
        return -1;
    }

    if (dfs_file_open(&p_fd, "sd", O_DIRECTORY | O_CREAT) == 0) //创建 "sd" 目录
    {
        if (dfs_mount("sd0", "/sd", "elm", 0, 0) == 0) // 挂载tf卡到sd目录下
        {
            rt_kprintf("[TF]Filesystem initialized!\n");
        }
        else
        {
            rt_kprintf("[TF]Failed to initialize filesystem!\n");
            return -1;
        }
    }
    else
    {
        if (dfs_mount("sd0", "/sd", "elm", 0, 0) == 0)
        {
            rt_kprintf("[TF]Filesystem initialized!\n");
        }
        else
        {
            rt_kprintf("[TF]Failed to initialize filesystem!\n");
            return -1;
        }
    }
    return 0;
}

static int substr(char *src, char *dst, int start, int end)
{
    int i = 0;
    if (end < start)
        return 0;
    if (src == NULL && dst == NULL)
        return 0;

    for (i = start; i < end; i++)
    {
        dst[i - start] = src[i];
    }
    return 1;
}

/* output current time */
static void lcd_disp_date(void)
{
    char date[16];
    time_t now;
    char *timer = NULL;

    rt_memset(date, 0, 16);
    now = time(RT_NULL);
    timer = ctime(&now);

    rt_memset(date, 0, 16);
    substr(timer, date, 20, 24);
    lcd_show_string(152, 0, 16, "%s-", date); //year
    rt_memset(date, 0, 16);

    substr(timer, date, 4, 7);
    lcd_show_string(192, 0, 16, "%s-", date); //month
    rt_memset(date, 0, 16);

    substr(timer, date, 8, 10);
    lcd_show_string(224, 0, 16, "%s", date); //date
    rt_memset(date, 0, 16);

    substr(timer, date, 0, 3);
    lcd_show_string(180, 20, 16, "%s", date); //week
    rt_memset(date, 0, 16);

    substr(timer, date, 11, 19);
    lcd_show_string(160, 40, 16, "%s", date); //time hour:min:sec
}

/*主界面*/
static void lcd_disp_oneday(void)
{
    lcd_show_hz(0, 0, 200, 16, "城市", 16);
    lcd_show_string(32, 0, 16, ":");
    lcd_show_hz(0, 20, 200, 16, "天气", 16);
    lcd_show_string(32, 20, 16, ":");
    lcd_show_hz(0, 40, 200, 16, "温度", 16);
    lcd_show_string(32, 40, 16, ":");
    lcd_show_hz(40, 0, 200, 16, (rt_uint8_t *)one_weather.city, 16);
    lcd_show_hz(40, 20, 200, 16, (rt_uint8_t *)one_weather.weather, 16);
    lcd_show_num(40, 40, one_weather.temperature, sizeof(one_weather.temperature), 16);

    pic_coord.start_x = 70;
    pic_coord.start_y = 70;
    pic_coord.end_x = 70;
    pic_coord.end_y = 70;

    weather_open_picture((char *)pic2_str[one_weather.pic]);
}
/*天气具体情况显示*/
static void lcd_disp_threeday(void)
{
    lcd_show_hz(0, 0, 200, 16, "城市", 16);
    lcd_show_string(32, 0, 16, ":");
    lcd_show_hz(40, 0, 200, 16, (rt_uint8_t *)one_weather.city, 16);

    lcd_show_hz(0, 20, 200, 16, "日期", 16);
    lcd_show_string(32, 20, 16, ":%s", three_weather.date[change_picture - 1]); //date

    lcd_show_hz(0, 40, 200, 16, "白天天气", 16);
    lcd_show_string(64, 40, 16, ":");
    lcd_show_hz(72, 40, 200, 16, (rt_uint8_t *)three_weather.text_day[change_picture - 1], 16);

    lcd_show_hz(0, 60, 200, 16, "夜晚天气", 16);
    lcd_show_string(64, 60, 16, ":");
    lcd_show_hz(72, 60, 200, 16, (rt_uint8_t *)three_weather.text_night[change_picture - 1], 16);

    lcd_show_hz(120, 60, 200, 16, "湿度", 16);
    lcd_show_string(152, 60, 16, ":");
    lcd_show_num(160, 60, three_weather.humidity[change_picture - 1], sizeof(three_weather.humidity[change_picture - 1]), 16);

    lcd_show_hz(0, 80, 200, 16, "风向", 16);
    lcd_show_string(32, 80, 16, ":");
    lcd_show_hz(40, 80, 200, 16, (rt_uint8_t *)three_weather.wind_direction[change_picture - 1], 16);

    lcd_show_hz(120, 80, 200, 16, "风速", 16);
    lcd_show_string(152, 80, 16, ":%s", three_weather.wind_speed[change_picture - 1]);

    lcd_show_hz(0, 100, 200, 16, "风向角", 16);
    lcd_show_string(48, 100, 16, ":");
    lcd_show_num(56, 100, three_weather.wind_degree[change_picture - 1], sizeof(three_weather.wind_degree[change_picture - 1]), 16);

    lcd_show_hz(120, 100, 200, 16, "风力等级", 16);
    lcd_show_string(184, 100, 16, ":");
    lcd_show_num(192, 100, three_weather.wind_scale[change_picture - 1], sizeof(three_weather.wind_scale[change_picture - 1]), 16);

    pic_coord.start_x = 0;
    pic_coord.start_y = 120;
    pic_coord.end_x = 0;
    pic_coord.end_y = 120;
    weather_open_picture((char *)pic2_str[three_weather.code_day[change_picture - 1]]);

    pic_coord.start_x = 120;
    pic_coord.start_y = 120;
    pic_coord.end_x = 120;
    pic_coord.end_y = 120;
    weather_open_picture((char *)pic2_str[three_weather.code_night[change_picture - 1]]);
}

void lcd_thread_entry(void *parameter)
{
    static uint8_t new_weather = 0;

    while (filesystem_init() < 0)
        ;
    while (font_init()) //更新字库
    {
        update_font(30, 160, 16, "/sd");
    }
    //    copy("/sd/PICTURE", "/");

    /* show RT-thread logo on lcd */
    lcd_clear(WHITE);
    lcd_show_image(0, 69, 240, 69, image_rttlogo);
    rt_thread_mdelay(5000);

    lcd_clear(WHITE);

    for (;;)
    {
        if (change_picture != new_weather)
        {
            new_weather = change_picture;
            lcd_clear(WHITE);
            rt_thread_mdelay(100);
            lcd_clear(WHITE);
        }
        if (change_picture > 0 && change_picture < 4) //三天天气
        {
            lcd_disp_threeday();
        }
        else if (change_picture == 4) //传感器数据
        {
            lcd_disp_sensor();
//              pic_coord.start_x = 0;
//              pic_coord.start_y = 0;
//              pic_coord.end_x = 0;
//              pic_coord.end_y = 0;
//              lcd_show_image(0,0,240,240,gImage_rtt);
        }
        else //一天天气
        {
            lcd_disp_oneday();
        }

        lcd_disp_date();

        rt_thread_mdelay(1000);
    }
}

static int lcd_app_init(void)
{
    rt_thread_t tid;

    tid = rt_thread_create("lcd_thread", lcd_thread_entry, RT_NULL,
                           RT_LCD_THREAD_STACK_SIZE, RT_LCD_THREAD_PRIORITY, 20);
    if (tid)
    {
        rt_thread_startup(tid);
        rt_kprintf("lcd_thread create success!\n");
    }
    else
    {
        rt_kprintf("Create lcd_thread thread fail!");
        return -RT_ENOMEM;
    }
    return RT_EOK;
}
INIT_APP_EXPORT(lcd_app_init);
