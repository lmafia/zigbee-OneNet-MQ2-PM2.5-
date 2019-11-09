/* main.c -- MQTT client example
*
* Copyright (c) 2014-2015, Tuan PM <tuanpm at live dot com>
* All rights reserved.
*
* Redistribution and use in source and binary forms, with or without
* modification, are permitted provided that the following conditions are met:
*
* * Redistributions of source code must retain the above copyright notice,
* this list of conditions and the following disclaimer.
* * Redistributions in binary form must reproduce the above copyright
* notice, this list of conditions and the following disclaimer in the
* documentation and/or other materials provided with the distribution.
* * Neither the name of Redis nor the names of its contributors may be used
* to endorse or promote products derived from this software without
* specific prior written permission.
*
* THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
* AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
* IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
* ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT OWNER OR CONTRIBUTORS BE
* LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
* CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
* SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
* INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
* CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
* ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
* POSSIBILITY OF SUCH DAMAGE.
*/




// 头文件
//==============================

#include "func.h"

//=================================
//预处理
//=================================

//==============================
// 类型定义
//=================================
typedef unsigned long 		u32_t;

//=================================
// 全局变量
//============================================================================
MQTT_Client mqttClient;			// MQTT客户端_结构体【此变量非常重要】
MQTT_Info mqtt_Info;
uint32 priv_param_start_sec;
ip_addr_t IP_Server;
struct espconn AP_NetCon;
struct espconn STA_NetCon;
os_timer_t OS_Timer_1;		// 软件定时器查询ip连接
os_timer_t OS_Timer_SNTP;
struct softap_config AP_Config;
struct station_config STA_Config;	// STA参数结构体

char waether_buf[1000]="";
uint8 times_rec =0;
//============================================================================
//自定义delay_ms
//============================================================================
void ICACHE_FLASH_ATTR delay_ms(uint32 ms)
{
	uint32 i = 0;
	for(i = 0; i < ms ; i++)
	{
		os_delay_us(1000);
		system_soft_wdt_feed();
	}
}
//============================================================================
// DNS_域名解析结束_回调函数【参数1：域名字符串指针 / 参数2：IP地址结构体指针 / 参数3：网络连接结构体指针】
//============================================================================
void ICACHE_FLASH_ATTR DNS_Over_Cb(const char * name, ip_addr_t *ipaddr, void *arg)
{

}
//============================================================================
// STA TCP天气连接断开成功的回调函数
//============================================================================
void ICACHE_FLASH_ATTR ESP8266_TCP_Disconnect_Cb(void *arg)
{

}
//============================================================================
// STA TCP & AP UDP成功发送网络数据的回调函数
//============================================================================
void ICACHE_FLASH_ATTR ESP8266_WIFI_Send_Cb(void *arg)
{
	/*os_printf("\nESP8266_WIFI_Send_OK\n");*/	// 向对方发送应答   发送数据
}
//============================================================================
// STA TCP接收回调函数  获取天气信息
//============================================================================
void ICACHE_FLASH_ATTR ESP8266_Weather_Recv_Cb(void * arg, char * pdata, unsigned short len)
{

}
//============================================================================
// STA TCP 天气连接建立成功的回调函数
//============================================================================
void ICACHE_FLASH_ATTR ESP8266_TCP_Connect_Cb(void *arg)
{
	espconn_regist_sentcb((struct espconn *)arg, ESP8266_WIFI_Send_Cb);			// 注册网络数据发送成功的回调函数
	espconn_regist_recvcb((struct espconn *)arg, ESP8266_Weather_Recv_Cb);			// 注册网络数据接收成功的回调函数
	espconn_regist_disconcb((struct espconn *)arg,ESP8266_TCP_Disconnect_Cb);	// 注册成功断开TCP连接的回调函数

	os_printf("\n--------------- ESP8266_TCP_Connect_OK ---------------\n");
}
//============================================================================
// STA TCP连接异常断开时的回调函数
//============================================================================
void ICACHE_FLASH_ATTR ESP8266_TCP_Break_Cb(void *arg,sint8 err)
{
	os_printf("\nESP8266_TCP_Break\n");
	espconn_connect(&STA_NetCon);	// 连接TCP-server
}
//============================================================================
// ESP8266_STA_NetCon_Init 网络连接初始化 连接天气的
//============================================================================
void ICACHE_FLASH_ATTR ESP8266_STA_NetCon_Init()
{

}
//============================================================================
//UDP AP 接收的回调函数;arg：espconn指针;pdata：数据指针;len：数据长度
//============================================================================
void ICACHE_FLASH_ATTR ESP8266_WIFI_Recv_Cb(void * arg, char * pdata, unsigned short len)
{

}

//============================================================================
//OS_Timer_1软件定时器初始化(ms毫秒) 它的非常重要 里面有微信配网和默认连接wifi
//============================================================================
void ICACHE_FLASH_ATTR OS_Timer_1_Init(u32 time_ms, u8 time_repetitive)
{
	os_timer_disarm(&OS_Timer_1);	// 关闭定时器
	os_timer_setfn(&OS_Timer_1,(os_timer_func_t *)OS_Timer_1_Cb, NULL);	// 设置定时器
	os_timer_arm(&OS_Timer_1, time_ms, time_repetitive);  // 使能定时器
}


//============================================================================
// ESP8266 模式配置的初始化
//============================================================================
void ICACHE_FLASH_ATTR ESP8266_AP_STA_Init()
{
	os_memset(&STA_Config, 0, sizeof(struct station_config));	// STA参数结构体 = 0
	spi_flash_read(Sector_STA_INFO*4096,(uint32 *)&STA_Config, 96);	// 读出【STA参数】(SSID/PASS)
	//写是在smartconnect那块
	STA_Config.ssid[31] = 0;		// SSID最后添'\0'
	STA_Config.password[63] = 0;	// APSS最后添'\0'
	wifi_set_opmode(0x01);
	if(wifi_station_set_config(&STA_Config) == true)	// 设置STA参数，并保存到Flash
	{
		OS_Timer_1_Init(1000, 1);
	}
}
//============================================================================
// MQTT已成功连接：ESP8266发送【CONNECT】，并接收到【CONNACK】
//============================================================================
void mqttConnectedCb(uint32_t *args)
{
    MQTT_Client* client = (MQTT_Client*)args;	// 获取mqttClient指针
    INFO("MQTT: Connected\r\n");

}
//============================================================================
// MQTT成功断开连接
//============================================================================
void mqttDisconnectedCb(uint32_t *args)
{
    MQTT_Client* client = (MQTT_Client*)args;
    INFO("MQTT: Disconnected\r\n");
}
//============================================================================
// MQTT成功发布消息
//============================================================================
void mqttPublishedCb(uint32_t *args)
{
    MQTT_Client* client = (MQTT_Client*)args;
    INFO("MQTT: Published\r\n");
}
//============================================================================
// 【接收MQTT的[PUBLISH]数据】函数		【参数1：主题 / 参数2：主题长度 / 参数3：有效载荷 / 参数4：有效载荷长度】
//===============================================================================================================
void mqttDataCb(uint32_t *args, const char* topic, uint32_t topic_len, const char *data, uint32_t data_len)
{
    char *topicBuf = (char*)os_zalloc(topic_len+1);		// 申请【主题】空间
    char *dataBuf  = (char*)os_zalloc(data_len+1);		// 申请【有效载荷】空间
    char *str;
    MQTT_Client* client = (MQTT_Client*)args;	// 获取MQTT_Client指针
    os_memcpy(topicBuf, topic, topic_len);	// 缓存主题
    topicBuf[topic_len] = 0;				// 最后添'\0'
    os_memcpy(dataBuf, data, data_len);		// 缓存有效载荷
    dataBuf[data_len] = 0;					// 最后添'\0'
    os_printf("Receive topic: %s, data: %s \r\n", topicBuf, dataBuf);	// 串口打印【主题】【有效载荷】
    os_printf("Topic_len = %d, Data_len = %d\r\n", topic_len, data_len);	// 串口打印【主题长度】【有效载荷长度】

//########################################################################################
    // 根据接收到的主题名/有效载荷，控制LED的亮/灭
    //-----------------------------------------------------------------------------------
    if( os_strncmp(topicBuf,"$creq",5) == 0 )			// 主题 == "SW_LED"
    {
    	os_printf("MQTT_GET:%s\n",dataBuf);

    }
//########################################################################################
    os_free(str);
    os_free(topicBuf);	// 释放【主题】空间
    os_free(dataBuf);	// 释放【有效载荷】空间
}
//===============================================================================================================


// user_init：entry of user application, init user function here
//===================================================================================================================
void user_init(void)
{
	uart_init(9600,0);// 串口波特率设为115200
	delay_ms(10);
	os_printf("\r\n=======================================\r\n");
	ESP8266_AP_STA_Init();
	//灯GPIO配置
	PIN_FUNC_SELECT(PERIPHS_IO_MUX_GPIO2_U, FUNC_GPIO2);
	GPIO_OUTPUT_SET(GPIO_ID_PIN(2),0);
	//读内存的wifi 账号密码
	os_memset(&STA_Config,0,sizeof(struct station_config));			// STA_INFO = 0
	spi_flash_read(Sector_STA_INFO*4096,(uint32 *)&STA_Config, 96);	// 读出【STA参数】(SSID/PASS)
	spi_flash_read(0x98*4096,(uint32 *)&mqtt_Info,sizeof(MQTT_Info));
	STA_Config.ssid[31] = 0;		// SSID最后添'\0'
	STA_Config.password[63] = 0;	// APSS最后添'\0'

    CFG_Load();	// 加载/更新系统参数【WIFI参数、MQTT参数】

    // 网络连接参数赋值：服务端域名【mqtt_test_jx.mqtt.iot.gz.baidubce.com】、网络连接端口【1883】、安全类型【0：NO_TLS】
	//-------------------------------------------------------------------------------------------------------------------
	MQTT_InitConnection(&mqttClient, sysCfg.mqtt_host, sysCfg.mqtt_port, sysCfg.security);

	// MQTT连接参数赋值：客户端标识符【..】、MQTT用户名【..】、MQTT密钥【..】、保持连接时长【120s】、清除会话【1：clean_session】
	//----------------------------------------------------------------------------------------------------------------------------
	MQTT_InitClient(&mqttClient, sysCfg.device_id, sysCfg.mqtt_user, sysCfg.mqtt_pass, sysCfg.mqtt_keepalive, 1);

	// 设置MQTT相关函数
	//--------------------------------------------------------------------------------------------------
	MQTT_OnConnected(&mqttClient, mqttConnectedCb);			// 设置【MQTT成功连接】函数的另一种调用方式
	MQTT_OnDisconnected(&mqttClient, mqttDisconnectedCb);	// 设置【MQTT成功断开】函数的另一种调用方式
	MQTT_OnPublished(&mqttClient, mqttPublishedCb);			// 设置【MQTT成功发布】函数的另一种调用方式
	MQTT_OnData(&mqttClient, mqttDataCb);					// 设置【接收MQTT数据】函数的另一种调用方式


	os_printf("\r\nSystem started ...\r\n");
	delay_ms(50);
    os_printf("PM2.5:%s\r\n",mqtt_Info.pm25);
    delay_ms(10);
    os_printf("SMOKE:%s\r\n",mqtt_Info.smoke);
    delay_ms(10);
    os_printf("TP:%s\r\n",mqtt_Info.temperature);
    delay_ms(10);
    os_printf("LEVEL:%s\r\n",mqtt_Info.level);
    delay_ms(30);
}
//===================================================================================================================
/******************************************************************************
 * FunctionName : user_rf_cal_sector_set
 * Description  : SDK just reversed 4 sectors, used for rf init data and paramters.
 *                We add this function to force users to set rf cal sector, since
 *                we don't know which sector is free in user's application.
 *                sector map for last several sectors : ABCCC
 *                A : rf cal
 *                B : rf init data
 *                C : sdk parameters
 * Parameters   : none
 * Returns      : rf cal sector
 *******************************************************************************/
uint32 ICACHE_FLASH_ATTR
user_rf_cal_sector_set(void)
{
    enum flash_size_map size_map = system_get_flash_size_map();
    uint32 rf_cal_sec = 0;

    switch (size_map) {
        case FLASH_SIZE_4M_MAP_256_256:
            rf_cal_sec = 128 - 5;
            break;

        case FLASH_SIZE_8M_MAP_512_512:
            rf_cal_sec = 256 - 5;
            break;

        case FLASH_SIZE_16M_MAP_512_512:
        case FLASH_SIZE_16M_MAP_1024_1024:
            rf_cal_sec = 512 - 5;
            break;

        case FLASH_SIZE_32M_MAP_512_512:
        case FLASH_SIZE_32M_MAP_1024_1024:
            rf_cal_sec = 1024 - 5;
            break;

        case FLASH_SIZE_64M_MAP_1024_1024:
            rf_cal_sec = 2048 - 5;
            break;
        case FLASH_SIZE_128M_MAP_1024_1024:
            rf_cal_sec = 4096 - 5;
            break;
        default:
            rf_cal_sec = 0;
            break;
    }

    return rf_cal_sec;
}
