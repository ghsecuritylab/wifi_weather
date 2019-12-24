#include <stm32l475xx.h>
#include <rtthread.h>
#include <string.h>
#include <stdio.h>
#include <at.h>
#include <sal_socket.h>
#include <stdio.h>
#include <at_socket.h>
#include <cJSON.h>
#include <drv_lcd.h>

#define WEATHER_SERVERIP "api.seniverse.com"
#define WEATHER_PORTNUM "80"

#define TIME_PORTNUM "88"
#define TIME_SERVERIP "api.k780.com"

char oneday_weather[] = "GET https://api.seniverse.com/v3/weather/now.json?key=cmuia15arkjrkjlv&location=shanghai&language=zh-Hans&unit=c\n\n";
char threeday_weather[] = "GET https://api.seniverse.com/v3/weather/daily.json?key=cmuia15arkjrkjlv&location=shanghai&language=zh-Hans&unit=c&start=0&days=5\n\n";
char bj_time[] = "GET http://api.k780.com:88/?app=life.time&appkey=10003&sign=b59bc3ef6191eb9f747dd4e83c99f2a4&format=json\n\n";

cJSON *root = NULL;
cJSON *pSub = NULL;
cJSON *arrayItem = NULL;
cJSON *pItem = NULL;
cJSON *pSubItem = NULL;
cJSON *pChildItem = NULL;
cJSON *pLastItem = NULL;
char *pr = NULL, *utf8str = NULL;


static int conn_times = 0;

static void get_threeday_weather(char *date);
static void get_oneday_weather(char *date);
static void get_beijing_time(char *date);

static void connect_oneday_serve(void);
static void connect_threeday_serve(void);
static void connect_bjtime_serve(void);

#define MAX_SERVE   3
typedef  void(*func)(char *);
static const func get_func[MAX_SERVE] = {get_beijing_time,get_oneday_weather,get_threeday_weather};

typedef  void(*func1)();
static const func1 conn_func[MAX_SERVE] = {connect_bjtime_serve,connect_oneday_serve,connect_threeday_serve};


static uint16_t get_http_buf(char *buf, uint16_t size);
rt_err_t esp_send_cmd(char const *cmd, char const *ack, uint16_t timeout);
int SwitchToGbk(const unsigned char *pszBufIn, int nBufInLen, unsigned char *pszBufOut, int *pnBufOutLen);
static void get_http_data(void);


//void esp_netstat(void)
//{
//	char quit_tran[] = "+++";
//	rt_err_t res;
//	at_client_send("quit_tran", strlen(quit_tran)); //get
//	rt_thread_mdelay(500);
//	res = esp_send_cmd("AT", "OK", 200); //检查是否退出透传模式
//	if (res == RT_EOK)
//	{
//		res = esp_send_cmd("AT+CIPSTATUS", "+CIPSTATUS", 500); //检测wifi连接状态
//		if (res == RT_EOK)
//		{
//			//释放邮箱
//		}
//	}
//}


/*esp获取TCP数据*/
void esp_tcp_connect(void)
{
    static unsigned char times = 1;
    rt_size_t res = RT_EOK;
    
    if(times)
    {
        esp_send_cmd("AT+CIPMUX=0","OK",100);
        times = 0;
    }
    rt_thread_mdelay(100);
	/*配置目标tcp服务器*/
    res = esp_send_cmd("AT", "OK", 500); //检查是否退出透传模式
    rt_thread_mdelay(100);
    res = esp_send_cmd("AT+CIPSTATUS", "OK", 500); //检测wifi连接状态
    if(res != RT_EOK)
    {
        res = esp_send_cmd("AT+CIPSTATUS", "OK", 500); 
    }

	if (res == RT_EOK)
	{
      if(conn_times == MAX_SERVE)
      {
        conn_times = 0;
      }
      
      conn_func[conn_times]();
	}
}

static void connect_oneday_serve(void)
{
    char *rp = NULL;
    rp = rt_malloc(512);
    
    rt_size_t res = RT_EOK;
    sprintf(rp, "AT+CIPSTART=\"TCP\",\"%s\",%s", WEATHER_SERVERIP, WEATHER_PORTNUM); //连接服务端
    rt_thread_mdelay(100);
	res = esp_send_cmd((char const *)rp, "OK", 500);
	if (res == RT_EOK)
	{
		rt_kprintf("Connet URL:%s Success!\n", WEATHER_SERVERIP);
        rt_thread_mdelay(100);
	    res = esp_send_cmd("AT+CIPMODE=1", "OK", 500); //开启透传
		if (res == RT_EOK)
		{   
          rt_thread_mdelay(100);
		  res = esp_send_cmd("AT+CIPSEND", "OK", 500); //开始透传
		  if (res == RT_EOK)
				{   
                    rt_thread_mdelay(100);
					at_client_send(oneday_weather, sizeof(oneday_weather)); //Get
                    rt_thread_mdelay(300);//此处延时是为了让AT线程获取UART的buffer
					get_http_data();
				}
			}
            rt_thread_mdelay(100);
			esp_send_cmd("AT+CIPCLOSE", "OK", 200); //关闭连接
	}
    
    rt_free(rp);
}
static void connect_threeday_serve(void)
{
    char *rp = NULL;
    rp = rt_malloc(512);
    
    rt_size_t res = RT_EOK;
    sprintf(rp, "AT+CIPSTART=\"TCP\",\"%s\",%s", WEATHER_SERVERIP, WEATHER_PORTNUM); //连接服务端
    rt_thread_mdelay(100);
	res = esp_send_cmd((char const *)rp, "OK", 500);
	if (res == RT_EOK)
	{
		rt_kprintf("Connet URL:%s Success!\n", WEATHER_SERVERIP);
        rt_thread_mdelay(100);
	    res = esp_send_cmd("AT+CIPMODE=1", "OK", 500); //开启透传
		if (res == RT_EOK)
		{   
          rt_thread_mdelay(100);
		  res = esp_send_cmd("AT+CIPSEND", "OK", 500); //开始透传
		  if (res == RT_EOK)
				{   
                    rt_thread_mdelay(100);
					at_client_send(threeday_weather, sizeof(threeday_weather)); //Get
                    rt_thread_mdelay(300);//此处延时是为了让AT线程获取UART的buffer
					get_http_data();
				}
			}
            rt_thread_mdelay(100);
			esp_send_cmd("AT+CIPCLOSE", "OK", 200); //关闭连接
	}
    rt_free(rp);
}

static void connect_bjtime_serve(void)
{
    char *rp = NULL;
    rp = rt_malloc(512);
    
    rt_size_t res = RT_EOK;
    sprintf(rp, "AT+CIPSTART=\"TCP\",\"%s\",%s", TIME_SERVERIP, TIME_PORTNUM); //连接服务端
    rt_thread_mdelay(100);
	res = esp_send_cmd((char const *)rp, "OK", 500);
	if (res == RT_EOK)
	{
		rt_kprintf("Connet URL:%s Success!\n", TIME_SERVERIP);
        rt_thread_mdelay(100);
	    res = esp_send_cmd("AT+CIPMODE=1", "OK", 500); //开启透传
		if (res == RT_EOK)
		{   
          rt_thread_mdelay(100);
		  res = esp_send_cmd("AT+CIPSEND", "OK", 500); //开始透传
		  if (res == RT_EOK)
		  {   
                    rt_thread_mdelay(100);
					at_client_send(bj_time, sizeof(bj_time)); //Get
                    rt_thread_mdelay(300);//此处延时是为了让AT线程获取UART的buffer
					get_http_data();
		  }
		}
            rt_thread_mdelay(100);
			esp_send_cmd("AT+CIPCLOSE", "OK", 200); //关闭连接
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
		result =  -RT_ERROR;
        goto _exit;
	}
    rt_thread_mdelay(200);
	if (at_resp_parse_line_args_by_kw(esp_resp, ack, buf) < 0)
	{
		rt_kprintf("wait at esp_resp err!\n");
		result =  -RT_ERROR;
        goto _exit;
	}
    
_exit:
    if (esp_resp)
    {
        at_delete_resp(esp_resp);
    }
    return result;
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
        
    memset(r_buf,0,sizeof(r_buf));

	res = get_http_buf(r_buf, 1024);
	if (res != 0)
	{
        get_func[conn_times++](r_buf);
	}

	while (1) //要保证已经成功退出透传模式
	{
        rt_thread_mdelay(100);
		at_client_send(quit_tran, strlen(quit_tran)); //退出透传
        rt_thread_mdelay(300);
		res = esp_send_cmd("AT", "OK", 100); //检查是否退出透传模式,这里会报错，但是返回是正常的，未知原因
		if (res == RT_EOK) //退出透传成功
			break;
	}
}

void get_beijing_time(char *date)
{
	pr = rt_malloc(100);
	utf8str = rt_malloc(50);

	memset(pr, 0, 100);
	memset(utf8str, 0, 50);

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
                    rt_kprintf("Time: %s\n",utf8str);
                    lcd_show_string(80,0,16,"%s",utf8str);
				}
				pSubItem = cJSON_GetObjectItem(pItem, "week_4");//这里使用英文表示
				if (pSubItem != NULL)
				{
					utf8str = pSubItem->valuestring;
                    rt_kprintf("Week: %s\n",utf8str);
                    lcd_show_string(100,20,16,"Week:%s",utf8str);
				}

			}
	}
    cJSON_Delete(root);//将root从链表中删除
	rt_free(pr);
	rt_free(utf8str);
}

/*获取一天天气情况*/
void get_oneday_weather(char *date)
{
	uint8_t temperature = 0;

	pr = rt_malloc(200);
	utf8str = rt_malloc(50);

	memset(pr, 0, 200);
	memset(utf8str, 0, 50);

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
						lcd_show_hz(40, 0, 200, 16, (rt_uint8_t *)utf8str, 16);
                        rt_kprintf("City: %s\n",utf8str);
					}
				}
				pSubItem = cJSON_GetObjectItem(pItem, "now");
				if (pSubItem != NULL)
				{
					pChildItem = cJSON_GetObjectItem(pSubItem, "text");
					if (pChildItem != NULL)
					{
						utf8str = pChildItem->valuestring;
						lcd_show_hz(40, 20, 200, 16, (rt_uint8_t *)utf8str, 16);
                        rt_kprintf("Weather: %s\n",utf8str);
					}
					pChildItem = cJSON_GetObjectItem(pSubItem, "code");
					if (pChildItem != NULL)
					{
						utf8str = pChildItem->valuestring;
						//show_weather_icon((u8 *)gbkstr,0); //此处发送邮箱到图标显示线程
					}
					pChildItem = cJSON_GetObjectItem(pSubItem, "temperature");
					if (pChildItem != NULL)
					{
						utf8str = pChildItem->valuestring;
						temperature = str2int((uint8_t *)utf8str);
						lcd_show_num(40, 40, temperature, sizeof(temperature), 16);
                        rt_kprintf("Temperature: %s\n",utf8str);
					}
				}
			}
		}
	}
    cJSON_Delete(root);
	rt_free(pr);
	rt_free(utf8str);
}


/*获取三天天气*/
void get_threeday_weather(char *date)
{
    uint8_t days = 0;
	pr = rt_malloc(500);
	utf8str = rt_malloc(50);

	memset(pr, 0, 500);
	memset(utf8str, 0, 50);

	root = cJSON_Parse(date);

	if (root != NULL)
	{
		pSub = cJSON_GetObjectItem(root, "results");//根据键获取对应的值（cjson对象）
		if (pSub != NULL)
		{
			arrayItem = cJSON_GetArrayItem(pSub, 0);//根据下标获取cjosn对象数组中的对象
			pr = cJSON_Print(arrayItem);
			pItem = cJSON_Parse(pr);
			if (pItem != NULL)
			{
				pSubItem = cJSON_GetObjectItem(pItem, "daily");
				if (pSubItem != NULL)
				{
                    days = cJSON_GetArraySize(pSubItem); //获取cjson对象数组成员的个数
                    
                    for(uint8_t i =0 ;i<days;i++)
                    {                
                       	arrayItem = cJSON_GetArrayItem(pSubItem,i);
						pr = cJSON_Print(arrayItem);
						pLastItem = cJSON_Parse(pr);
                        
					   if (pLastItem != NULL)
					   {
                            pChildItem =  cJSON_GetObjectItem(pLastItem,"date"); //日期
                            rt_kprintf("date : %s\n",pChildItem->valuestring);
                            
                            pChildItem =  cJSON_GetObjectItem(pLastItem,"text_day");//白天天气
                            rt_kprintf("text_day : %s\n",pChildItem->valuestring);
                            
                            pChildItem =  cJSON_GetObjectItem(pLastItem,"text_night");//夜晚天气
                            rt_kprintf("text_night : %s\n",pChildItem->valuestring);
                             
                            pChildItem =  cJSON_GetObjectItem(pLastItem,"high");  //最高气温
                            rt_kprintf("high temperature : %s\n",pChildItem->valuestring);
                            
                            pChildItem =  cJSON_GetObjectItem(pLastItem,"low");//最低气温
                            rt_kprintf("low temperature : %s\n",pChildItem->valuestring);
                            
                            pChildItem =  cJSON_GetObjectItem(pLastItem,"wind_direction_degree");//风向角度，范围0~360
                            rt_kprintf("wind direction degree : %s\n",pChildItem->valuestring);
                            
                            pChildItem =  cJSON_GetObjectItem(pLastItem,"wind_speed");//风速，单位km/h（当unit=c时）、mph（当unit=f时）
                            rt_kprintf("wind speed : %s\n",pChildItem->valuestring);
                            
                            pChildItem =  cJSON_GetObjectItem(pLastItem,"wind_scale");//风力等级
                            rt_kprintf("wind scale : %s\n",pChildItem->valuestring);
					   }
                    }
				}
			}
		}
	}
    cJSON_Delete(root);
	rt_free(pr);
	rt_free(utf8str);
}




int Cjson_Mem_Malloc(void)
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
    
    return 0;
}
