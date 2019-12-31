#include "main.h"
#include <cJSON.h>

#define WEATHER_SERVERIP "api.seniverse.com"
#define WEATHER_PORTNUM "80"

#define TIME_PORTNUM "88"
#define TIME_SERVERIP "api.k780.com"

char oneday_weather[] = "GET https://api.seniverse.com/v3/weather/now.json?key=cmuia15arkjrkjlv&location=shanghai&language=zh-Hans&unit=c\n\n";
char threeday_weather[] = "GET https://api.seniverse.com/v3/weather/daily.json?key=cmuia15arkjrkjlv&location=shanghai&language=zh-Hans&unit=c&start=0&days=3\n\n";
char bj_time[] = "GET http://api.k780.com:88/?app=life.time&appkey=10003&sign=b59bc3ef6191eb9f747dd4e83c99f2a4&format=json\n\n";

//char life[] = "https://api.seniverse.com/v3/life/suggestion.json?key=b59bc3ef6191eb9f747dd4e83c99f2a4&location=shanghai&language=zh-Hans;
//生活指数,会占用较大内存

cJSON *root = NULL;
cJSON *pSub = NULL;
cJSON *arrayItem = NULL;
cJSON *pItem = NULL;
cJSON *pSubItem = NULL;
cJSON *pChildItem = NULL;
cJSON *pLastItem = NULL;
char *pr = NULL, *utf8str = NULL;

static int conn_times = 0; //TCP连接次数

static void set_rtc_time(char *time);
static uint16_t get_http_buf(char *buf, uint16_t size);
rt_err_t esp_send_cmd(char const *cmd, char const *ack, uint16_t timeout);
int SwitchToGbk(const unsigned char *pszBufIn, int nBufInLen, unsigned char *pszBufOut, int *pnBufOutLen);
static void get_http_data(void);
static void get_threeday_weather(char *date);
static void get_oneday_weather(char *date);
static void get_beijing_time(char *date);
static void connect_oneday_serve(void);
static void connect_threeday_serve(void);
static void connect_bjtime_serve(void);
static int dev_esp8266_init(void);

#define MAX_SERVE 3
typedef void (*func)(char *);
static const func get_func[MAX_SERVE] = {get_beijing_time, get_oneday_weather, get_threeday_weather};

typedef void (*func1)();
static const func1 conn_func[MAX_SERVE] = {connect_bjtime_serve, connect_oneday_serve, connect_threeday_serve};

static uint8_t link_err_times = 0;

/*esp获取TCP数据*/
void esp_tcp_connect(void)
{
	rt_size_t res = RT_EOK;

	if (link_err_times > 5)
	{
		if (dev_esp8266_init() == RT_EOK)
			link_err_times = 0;
	}

	/*配置目标tcp服务器*/
	res = esp_send_cmd("AT", "OK", 500);				//检查是否退出透传模式
	res = esp_send_cmd("AT+CIPMUX?", "+CIPMUX:0", 500); //是否已经开启多连接
	if (res != RT_EOK)
	{
		res = esp_send_cmd("AT+CIPMUX=0", "OK", 500); //开启多连接
	}
	res = esp_send_cmd("AT+CIPSTATUS", "OK", 500); //检测wifi连接状态

	if (res == RT_EOK)
	{
		if (conn_times == MAX_SERVE)
		{
			conn_times = 0;
		}

		conn_func[conn_times]();
	}
}

static void connect_bjtime_serve(void)
{
	char *rp = NULL;
	rp = rt_malloc(512);

	rt_size_t res = RT_EOK;
	sprintf(rp, "AT+CIPSTART=\"TCP\",\"%s\",%s", TIME_SERVERIP, TIME_PORTNUM); //连接服务端
	res = esp_send_cmd((char const *)rp, "OK", 500);
	if (res == RT_EOK)
	{
		rt_kprintf("Connet URL:%s Success!\n", TIME_SERVERIP);
		res = esp_send_cmd("AT+CIPMODE=1", "OK", 500); //开启透传
		if (res == RT_EOK)
		{
			res = esp_send_cmd("AT+CIPSEND", "OK", 500); //开始透传
			if (res == RT_EOK)
			{
				at_client_send(bj_time, sizeof(bj_time)); //Get
				rt_thread_mdelay(300);					  //此处延时是为了让AT线程获取UART的buffer
				get_http_data();
			}
		}
		rt_thread_mdelay(200);
		esp_send_cmd("AT+CIPCLOSE", "OK", 200); //close tcp link
	}
	else
	{
		link_err_times++;
        
        rt_kprintf("\nesp link err times = %d\n",link_err_times);
        
		rt_thread_mdelay(1000);
	}
	rt_free(rp);
}

static void connect_oneday_serve(void)
{
	char *rp = NULL;
	rp = rt_malloc(512);

	rt_size_t res = RT_EOK;
	sprintf(rp, "AT+CIPSTART=\"TCP\",\"%s\",%s", WEATHER_SERVERIP, WEATHER_PORTNUM); //连接服务端
	res = esp_send_cmd((char const *)rp, "OK", 500);

	if (res == RT_EOK)
	{
		rt_kprintf("Connet URL:%s Success!\n", WEATHER_SERVERIP);
		res = esp_send_cmd("AT+CIPMODE=1", "OK", 500); //开启透传
		if (res == RT_EOK)
		{
			res = esp_send_cmd("AT+CIPSEND", "OK", 500); //开始透传
			if (res == RT_EOK)
			{
				at_client_send(oneday_weather, sizeof(oneday_weather)); //Get
				rt_thread_mdelay(300);
				get_http_data();
			}
		}

		rt_thread_mdelay(200);
		esp_send_cmd("AT+CIPCLOSE", "OK", 200); //close tcp link
	}
	else
	{
		link_err_times++;
		rt_thread_mdelay(5000);
	}
	rt_free(rp);
}
static void connect_threeday_serve(void)
{
	char *rp = NULL;
	rp = rt_malloc(512);

	rt_size_t res = RT_EOK;
	sprintf(rp, "AT+CIPSTART=\"TCP\",\"%s\",%s", WEATHER_SERVERIP, WEATHER_PORTNUM); //连接服务端
	res = esp_send_cmd((char const *)rp, "OK", 500);
	if (res == RT_EOK)
	{
		rt_kprintf("Connet URL:%s Success!\n", WEATHER_SERVERIP);
		res = esp_send_cmd("AT+CIPMODE=1", "OK", 500); //开启透传
		if (res == RT_EOK)
		{
			res = esp_send_cmd("AT+CIPSEND", "OK", 500); //开始透传
			if (res == RT_EOK)
			{
				at_client_send(threeday_weather, sizeof(threeday_weather)); //Get
				rt_thread_mdelay(300);										//此处延时是为了让AT线程获取UART的buffer
				get_http_data();
			}
		}
		rt_thread_mdelay(200);
		esp_send_cmd("AT+CIPCLOSE", "OK", 200); //close tcp link
	}
	else
	{
		link_err_times++;
		rt_thread_mdelay(5000);
	}

	rt_free(rp);
}

rt_err_t esp_send_cmd(char const *cmd, char const *ack, uint16_t timeout)
{
	int result = RT_EOK;
	char buf[40];
	at_response_t esp_resp = RT_NULL;

	esp_resp = at_create_resp(1024, 0, 200);

	if (at_exec_cmd(esp_resp, cmd) != RT_EOK)
	{
		rt_kprintf("cmd err : %s", cmd);
		result = -RT_ERROR;
		goto _exit;
	}
	if (at_resp_parse_line_args_by_kw(esp_resp, ack, buf) < 0)
	{
		rt_kprintf("wait at esp_resp err!\n");
		result = -RT_ERROR;
		goto _exit;
	}

_exit:
	if (esp_resp)
	{
		at_delete_resp(esp_resp);
	}
	return result;
}

static uint16_t get_http_buf(char *buf, uint16_t size)
{
	at_client_t at_client;
	at_client = at_client_get_first();

	if (size > at_client->cur_recv_len)
		size = at_client->cur_recv_len;

	memcpy(buf, &at_client->recv_buffer[1], size - 1);

	at_client->cur_recv_len = 0;

	return size - 1;
}

static void get_http_data(void)
{
	rt_size_t res = RT_EOK;
	char r_buf[1024];
	char quit_tran[] = "+++";

	memset(r_buf, 0, sizeof(r_buf));

	res = get_http_buf(r_buf, 1024);
	if (res != 0)
	{
		get_func[conn_times++](r_buf);
		link_err_times = 0;
	}

	while (1) //要保证已经成功退出透传模式
	{
		rt_thread_mdelay(100);
		at_client_send(quit_tran, strlen(quit_tran)); //退出透传
		rt_thread_mdelay(300);
		res = esp_send_cmd("AT", "OK", 100); //检查是否退出透传模式,这里会报错，但是返回是正常的，未知原因
		if (res == RT_EOK)					 //退出透传成功
			break;
	}
}

//数字字符串转换为整型
static uint8_t str2int(uint8_t *str)
{
	uint8_t len, res;
	len = strlen((const char *)str);
	switch (len)
	{
	case 1:
		res = str[0] - 0x30;
		break;
	case 2:
		res = (str[0] - 0x30) * 10 + (str[1] - 0x30);
		break;
	default:
		break;
	}
	return res;
}

void get_beijing_time(char *date)
{
	char *pr = NULL;
	char *utf8str = NULL;

	root = cJSON_Parse(date);

	if (root != NULL)
	{
		arrayItem = cJSON_GetArrayItem(root, 1);
		pr = cJSON_Print(arrayItem);
		pItem = cJSON_Parse(pr);
		if (pItem != NULL)
		{
			pSubItem = cJSON_GetObjectItem(pItem, "datetime_1");
			if (pSubItem != NULL)
			{
				utf8str = pSubItem->valuestring; //可以将此值保存在RTC
				set_rtc_time(utf8str);
				rt_kprintf("Time: %s\n", utf8str);
			}
			pSubItem = cJSON_GetObjectItem(pItem, "week_4"); //这里使用英文表示
			if (pSubItem != NULL)
			{
				utf8str = pSubItem->valuestring;
				memcpy(rtc_time.week, utf8str, sizeof(utf8str));
				rt_kprintf("Week: %s\n", utf8str);
			}
			cJSON_Delete(pItem); //将root从链表中删除
			rt_free(pr);
		}
		cJSON_Delete(root); //将root从链表中删除
	}
}

/*获取一天天气情况*/
void get_oneday_weather(char *date)
{
	uint8_t temperature = 0;
	char *pr = NULL;
	char *utf8str = NULL;

	root = cJSON_Parse(date);

	if (root != NULL)
	{
		pSub = cJSON_GetObjectItem(root, "results");
		if (pSub != NULL)
		{
			arrayItem = cJSON_GetArrayItem(pSub, 0);
			pr = cJSON_Print(arrayItem);
			pItem = cJSON_Parse(pr);
			if (pItem != NULL)
			{
				pSubItem = cJSON_GetObjectItem(pItem, "location");
				if (pSubItem != NULL)
				{
					pChildItem = cJSON_GetObjectItem(pSubItem, "name");
					if (pChildItem != NULL)
					{
						utf8str = pChildItem->valuestring;
						rt_memcpy(one_weather.city,utf8str,strlen(utf8str));
						rt_kprintf("City: %s\n", utf8str);
					}
				}
				pSubItem = cJSON_GetObjectItem(pItem, "now");
				if (pSubItem != NULL)
				{
					pChildItem = cJSON_GetObjectItem(pSubItem, "text");
					if (pChildItem != NULL)
					{
						utf8str = pChildItem->valuestring;
						rt_memcpy(one_weather.weather,utf8str,strlen(utf8str));			
						rt_kprintf("Weather: %s\n", utf8str);
					}
					pChildItem = cJSON_GetObjectItem(pSubItem, "code");
					if (pChildItem != NULL)
					{
						utf8str = pChildItem->valuestring;
						one_weather.pic = str2int((uint8_t *)utf8str);
						rt_kprintf("Picture: %s\n", utf8str);
					}
					pChildItem = cJSON_GetObjectItem(pSubItem, "temperature");
					if (pChildItem != NULL)
					{
						utf8str = pChildItem->valuestring;
						temperature = str2int((uint8_t *)utf8str);
						one_weather.temperature = temperature; 
					}
				}
				cJSON_Delete(pItem);
				rt_free(pr);
			}
		}
		cJSON_Delete(root);
	}
}

/*获取三天天气*/
void get_threeday_weather(char *date)
{
	uint8_t days = 0;

	char *pr1 = NULL;
	char *pr2 = NULL;

	root = cJSON_Parse(date);

	if (root != NULL)
	{
		pSub = cJSON_GetObjectItem(root, "results"); //根据键获取对应的值（cjson对象）
		if (pSub != NULL)
		{
			arrayItem = cJSON_GetArrayItem(pSub, 0); //根据下标获取cjosn对象数组中的对象
			pr1 = cJSON_Print(arrayItem);
			pItem = cJSON_Parse(pr1);
			if (pItem != NULL)
			{
				pSubItem = cJSON_GetObjectItem(pItem, "daily");
				if (pSubItem != NULL)
				{
					days = cJSON_GetArraySize(pSubItem); //获取cjson对象数组成员的个数

					for (uint8_t i = 0; i < days; i++)
					{
						arrayItem = cJSON_GetArrayItem(pSubItem, i);
						pr2 = cJSON_Print(arrayItem);
						pLastItem = cJSON_Parse(pr2);

						if (pLastItem != NULL)
						{						
							pChildItem = cJSON_GetObjectItem(pLastItem, "date"); //日期
                            rt_memcpy(three_weather.date[i],pChildItem->valuestring,strlen(pChildItem->valuestring));
							rt_kprintf("date : %s\n", pChildItem->valuestring);
                            

							pChildItem = cJSON_GetObjectItem(pLastItem, "text_day"); //白天天气
                            rt_memcpy(three_weather.text_day[i],pChildItem->valuestring,strlen(pChildItem->valuestring));
							rt_kprintf("text_day : %s\n", pChildItem->valuestring);
                            
                            pChildItem = cJSON_GetObjectItem(pLastItem, "code_day"); //天气代码
                            three_weather.code_day[i] = str2int((uint8_t *)pChildItem->valuestring);
							rt_kprintf("text_day : %s\n", pChildItem->valuestring);
                            
							pChildItem = cJSON_GetObjectItem(pLastItem, "text_night"); //夜晚天气
                            rt_memcpy(three_weather.text_night[i],pChildItem->valuestring,strlen(pChildItem->valuestring));
							rt_kprintf("text_night : %s\n", pChildItem->valuestring);

                            pChildItem = cJSON_GetObjectItem(pLastItem, "code_night"); //天气代码
                            three_weather.code_night[i] = str2int((uint8_t *)pChildItem->valuestring);
							rt_kprintf("text_day : %s\n", pChildItem->valuestring);
                            
							pChildItem = cJSON_GetObjectItem(pLastItem, "high"); //最高气温
                            three_weather.high_tem[i] = str2int((uint8_t *)pChildItem->valuestring);
							rt_kprintf("high temperature : %s\n", pChildItem->valuestring);

							pChildItem = cJSON_GetObjectItem(pLastItem, "low"); //最低气温
                            three_weather.low_tem[i] = str2int((uint8_t *)pChildItem->valuestring);
							rt_kprintf("low temperature : %s\n", pChildItem->valuestring);
                         
                            pChildItem = cJSON_GetObjectItem(pLastItem, "wind_direction"); //风向
                            rt_memcpy(three_weather.wind_direction[i],pChildItem->valuestring,strlen(pChildItem->valuestring));
							rt_kprintf("wind direction degree : %s\n", pChildItem->valuestring);
                            
							pChildItem = cJSON_GetObjectItem(pLastItem, "wind_direction_degree"); //风向角度，范围0~360
                            three_weather.wind_degree[i] = str2int((uint8_t *)pChildItem->valuestring);
							rt_kprintf("wind direction degree : %s\n", pChildItem->valuestring);

							pChildItem = cJSON_GetObjectItem(pLastItem, "wind_speed"); //风速，单位km/h（当unit=c时）、mph（当unit=f时）
                            rt_memcpy(three_weather.wind_speed[i],pChildItem->valuestring,strlen(pChildItem->valuestring));
							rt_kprintf("wind speed : %s\n", pChildItem->valuestring);

							pChildItem = cJSON_GetObjectItem(pLastItem, "wind_scale"); //风力等级
                            three_weather.wind_scale[i] = str2int((uint8_t *)pChildItem->valuestring);
							rt_kprintf("wind scale : %s\n", pChildItem->valuestring);
                            
                            pChildItem = cJSON_GetObjectItem(pLastItem, "humidity"); //相对湿度
                            three_weather.humidity[i] = str2int((uint8_t *)pChildItem->valuestring);
							rt_kprintf("humidity : %s\n", pChildItem->valuestring);
						}
						rt_free(pr2);			 
						cJSON_Delete(pLastItem); 
					}
				}
				cJSON_Delete(pItem);
				rt_free(pr1);
			}
		}
		cJSON_Delete(root);
	}
}

static void cjson_mem_malloc(void)
{
	root = rt_malloc(sizeof(cJSON));
	RT_ASSERT(root != RT_NULL);

	pSub = rt_malloc(sizeof(cJSON));
	RT_ASSERT(pSub != RT_NULL);

	pItem = rt_malloc(sizeof(cJSON));
	RT_ASSERT(pItem != RT_NULL);

	pSubItem = rt_malloc(sizeof(cJSON));
	RT_ASSERT(pSubItem != RT_NULL);

	arrayItem = rt_malloc(sizeof(cJSON));
	RT_ASSERT(arrayItem != RT_NULL);

	pLastItem = rt_malloc(sizeof(cJSON));
	RT_ASSERT(pLastItem != RT_NULL);
}

static void set_rtc_time(char *time)
{
	int len = 0;
	len = strlen(time);
	if (len < 19) //(2019-12-26 17:25:30)
		return;
	rtc_time.year = (time[0] - 0x30) * 1000 + (time[1] - 0x30) * 100 + (time[2] - 0x30) * 10 + (time[3] - 0x30);
	rtc_time.month = (time[5] - 0x30) * 10 + (time[6] - 0x30);
	rtc_time.day = (time[8] - 0x30) * 10 + (time[9] - 0x30);
	rtc_time.hour = (time[11] - 0x30) * 10 + (time[12] - 0x30);
	rtc_time.min = (time[14] - 0x30) * 10 + (time[15] - 0x30);
	rtc_time.sec = (time[17] - 0x30) * 10 + (time[18] - 0x30);

	if (rtc_time.year > 2099 || rtc_time.year < 2000)
	{
		rt_kprintf("year is out of range [2000-2099]\n");
		return;
	}
	if (rtc_time.month == 0 || rtc_time.month > 12)
	{
		rt_kprintf("month is out of range [1-12]\n");
		return;
	}
	if (rtc_time.day == 0 || rtc_time.day > 31)
	{
		rt_kprintf("day is out of range [1-31]\n");
		return;
	}
	if (rtc_time.hour > 23)
	{
		rt_kprintf("hour is out of range [0-23]\n");
		return;
	}
	if (rtc_time.min > 59)
	{
		rt_kprintf("minute is out of range [0-59]\n");
		return;
	}
	if (rtc_time.sec > 59)
	{
		rt_kprintf("second is out of range [0-59]\n");
		return;
	}
	set_time(rtc_time.hour, rtc_time.min, rtc_time.sec);
	set_date(rtc_time.year, rtc_time.month, rtc_time.day);
}

static int dev_esp8266_init(void)
{
	at_response_t resp = RT_NULL;
	rt_err_t result = RT_EOK;
	rt_size_t i;

	resp = at_create_resp(128, 0, rt_tick_from_millisecond(5000));
	if (!resp)
	{
		rt_kprintf("No memory for response structure!");
		result = -RT_ENOMEM;
		goto __exit;
	}

	rt_thread_delay(rt_tick_from_millisecond(5000));
	/* reset module */
	AT_SEND_CMD(resp, "AT+RST");
	/* reset waiting delay */
	rt_thread_delay(rt_tick_from_millisecond(1000));
	/* disable echo */
	AT_SEND_CMD(resp, "ATE0");
	/* set current mode to Wi-Fi station */
	AT_SEND_CMD(resp, "AT+CWMODE=1");
	/* get module version */
	AT_SEND_CMD(resp, "AT+GMR");
	/* show module version */
	for (i = 0; i < resp->line_counts - 1; i++)
	{
		rt_kprintf("%s", at_resp_get_line(resp, i + 1));
	}
	//    AT_SEND_CMD(resp, "AT+CWLAP"); /* show all wifi ap */
	/* connect to WiFi AP */
	if (at_exec_cmd(at_resp_set_info(resp, 128, 0, 20 * RT_TICK_PER_SECOND), "AT+CWJAP=\"%s\",\"%s\"",
					AT_DEVICE_WIFI_SSID, AT_DEVICE_WIFI_PASSWORD) != RT_EOK)
	{
		rt_kprintf("AT network initialize failed, check ssid(%s) and password(%s).", AT_DEVICE_WIFI_SSID, AT_DEVICE_WIFI_PASSWORD);
		result = -RT_ERROR;
		goto __exit;
	}

	AT_SEND_CMD(resp, "AT+CIPMUX=0"); //开启单连接

__exit:
	if (resp)
	{
		at_delete_resp(resp);
	}
	return result;
}

void esp_thread_entry(void *parameter)
{
	cjson_mem_malloc();
	for (;;)
	{
		rt_thread_mdelay(20000);
		esp_tcp_connect();
	}
}

int esp_app_init(void)
{
	rt_thread_t tid;

	tid = rt_thread_create("esp_thread", esp_thread_entry, RT_NULL, RT_ESP_THREAD_STACK_SIZE, RT_ESP_THREAD_PRIORITY, 20);
	if (tid)
	{
		rt_thread_startup(tid);
		rt_kprintf("esp_thread create success!\n");
	}
	else
	{
		rt_kprintf("Create esp_thread thread fail!");
		return -RT_ENOMEM;
	}
	return RT_EOK;
}
INIT_APP_EXPORT(esp_app_init);
