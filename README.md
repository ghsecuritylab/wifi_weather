

**<u>不要使用env工具修改此工程！！！！！</u>**

**<u>不要使用env工具修改此工程！！！！！</u>**

**<u>不要使用env工具修改此工程！！！！！</u>**





# 项目介绍 

## 功能描述  

  1.使用ESPWIFI模块连接到天气网站，定时获取天气信息，本地MCU通过解析JSON数据，将天气信息显示在LCD上.  
  2.本地MCU定时采集传感器状态，显示在LCD上.

## 硬件平台

  1.正点原子潘多拉开发板.  
  2.ESP8266串口转WIFI模块.  
  3.SD卡

## 软件平台

  本项目基于RT-Thread创建.  

### 软件包

* aht10
* ap3216c
* AT
* button_drive
* Cjson
* EasyFlash
* Fal
* Filesystem
* Fines
* ChineseFont
* Netdev
* Sal
* Sensor
* Tjpgdec

## 记录进度  

### 已完成
- wifi模块已配置完成,可以get到http的json数据  
- 文件系统配置完成,可以正常访问SD和SPIFLASH
- 目前需要制作字库,来显示汉字部分,字库使用正点原子制作的字库(注意：该字库为GBK字库，请确认.c文件类型是否为GBK，否则无法解析)
- 上电挂载文件系统,挂载成功后,检测SPIFLASH中是否有字库文件,如果没有,更新SD卡字库到SPIFLASH
- JPG图片也可以更新到SPIFLASH中。存在的问题：SPIFLASH的文件系统，无法保存文件内容。比如：在根目录'/'创建文件（SPIFLASH挂载在'/'下），系统复位后此文件会丢失。

### 已知问题|注意事项

- 1.wifi模块已配置完成,可以get到http的json数据  
- 2.文件系统配置完成,可以正常访问SD和SPIFLASH
- 3.目前需要制作字库,来显示汉字部分,字库使用正点原子制作的字库(注意：该字库为GBK字库，请确认.c文件类型是否为GBK，否则无法解析)
- 4.上电挂载文件系统,挂载成功后,检测SPIFLASH中是否有字库文件,如果没有,更新SD卡字库到SPIFLASH 
- 5.将JPG图片可以更新到SPIFLASH中,目前存在问题,SPIFLASH文件系统分区,无法保存文件

* jpg图片存在部分图片无法解码。
* esp任务存在tcp连接失败的情况，需要加强wifi连接的稳定性。
* 中文格式显示GBK232 ,此工程格式为UTF8，所以要将UTF8格式的汉字转换为GBK编码。
* 如果看到提示"Failed to initialize filesystem!，这是因为指定的挂载设备中还没有创建文件系统。
  在msh 中使用命令mkfs -t elm filesystem 可以在名为“filesystem” 的块设备上创建elm-fat 类
  型的文件系统。

### WIFI打印
![image](https://github.com/liukang96/wifi_weather/blob/liukang-branch/picture/URL.JPG)



#### CJSON使用注意事项：

* Cjson使用注意事项：内存的申请与释放。Cjson在使用过程中，很容易产生内存碎片。

* 调用cJSON_Parse()函数，解析JSON数据包，并按照cJSON结构体的结构序列化整个数据包。使用该函数会通过malloc()函数在内存中开辟一个空间，使用完成需要手动释放。

* 通过cJSON_Delete()，释放cJSON_Parse()分配出来的内存空间 

  `
  char *root = NULL;  
  root = malloc(64);  
  root = cJSON_Parse(date);  
     ...  
  cJSON_Delete(root);  
  free(root );  
  `

  **有几个Parse就要有几个Delete!!!**

  **有几个Parse就要有几个Delete!!!**

  **有几个Parse就要有几个Delete!!!**

  

* cJSON_Print会申请内存，使用完要释放掉。

  `
  char *pr1= NULL;     
  char *pr2= NULL;     
  pr1= cJSON_Print(item1);     
  pr2 = cJSON_Print(item2);     
       ...     
  free(pr1);     
  free(pr2);     
  `
  **有几个Printf就要有几个free!!!**

  **有几个Printf就要有几个free!!!**

  **有几个Printf就要有几个free!!!**

**通过打印我们可以看到,内存被完美释放掉了**

