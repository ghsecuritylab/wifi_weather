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
    * AT_Device  
    * AT  
    * EasyFlash
    * Finsh
    * SAL
    * Sensor
    * TJpgDec
    * Filesystem
    * aht10
    * ap3216c

## 记录进度  

### 已完成
 * 1.wifi模块已配置完成,可以get到http的json数据  

 * 2.文件系统配置完成,可以正常访问SD和SPIFLASH

 * 3.目前需要制作字库,来显示汉字部分,字库使用正点原子制作的字库(注意：该字库为GBK字库，请确认.c文件类型是否为GBK，否则无法解析)
 
 * 4.上电挂载文件系统,挂载成功后,检测SPIFLASH中是否有字库文件,如果没有,更新SD卡字库到SPIFLASH
 ### 待完成  

 * 3.将JPG图片也更新到SPIFLASH中
