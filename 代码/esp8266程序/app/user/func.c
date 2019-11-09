/*
 * func.c
 *
 *  Created on: 2019年6月2日
 *      Author: Administrator
 */

#include "func.h"
#define GET_WEATHER_TIMES 200
#define RESET_TIMES 18
os_timer_t os_Get_Weather5min_Temp3s;

void string_Packet_Publish(MQTT_Client* client,char *data)
{

	uint8 size = os_strlen(data);
	char *payload = (char *)os_zalloc(size+3);
	payload[0] = 0x05 & 0xff;
	payload[1] = (size>>8) & 0xff;
	payload[2] = size & 0xff;
	os_memcpy(payload+3, data, size);

	MQTT_Publish(client, "$dp", payload, size+3, 0, 0);
	save_Mqtt_Info(&mqtt_Info);
	os_free(payload);

}
void ICACHE_FLASH_ATTR save_Mqtt_Info(MQTT_Info *pdata)
{
	spi_flash_erase_sector(0x98);						// 擦除扇区
	spi_flash_write(0x98*4096, (uint32 *)pdata, sizeof(MQTT_Info));	// 写入扇区
}

// SmartConfig状态发生改变时，进入此回调函数
//--------------------------------------------
// 参数1：sc_status status / 参数2：无类型指针【在不同状态下，[void *pdata]的传入参数是不同的】
//=================================================================================================================
void ICACHE_FLASH_ATTR smartconfig_done(sc_status status, void *pdata)
{
	os_printf("\r\n------ smartconfig_done ------\r\n");	// ESP8266网络状态改变
    switch(status)
    {
    	// CmartConfig等待
		//……………………………………………………
		case SC_STATUS_WAIT:		// 初始值
			os_printf("\r\nSC_STATUS_WAIT\r\n");
		break;
		//……………………………………………………
		// 发现【WIFI信号】（8266在这种状态下等待配网）
		//…………………………………………………………………………………………………
		case SC_STATUS_FIND_CHANNEL:
			os_printf("\r\nSC_STATUS_FIND_CHANNEL\r\n");
			os_printf("\r\n---- Please Use WeChat to SmartConfig ------\r\n\r\n");

		break;
		//…………………………………………………………………………………………………

        // 正在获取【SSID】【PSWD】
        //…………………………………………………………………………………………………
        case SC_STATUS_GETTING_SSID_PSWD:
            os_printf("\r\nSC_STATUS_GETTING_SSID_PSWD\r\n");

            // 【SC_STATUS_GETTING_SSID_PSWD】状态下，参数2==SmartConfig类型指针
            //-------------------------------------------------------------------
			sc_type *type = pdata;		// 获取【SmartConfig类型】指针
			// 配网方式 == 【ESPTOUCH】
			//-------------------------------------------------
            if (*type == SC_TYPE_ESPTOUCH)
            { os_printf("\r\nSC_TYPE:SC_TYPE_ESPTOUCH\r\n"); }
            // 配网方式 == 【AIRKISS】||【ESPTOUCH_AIRKISS】
            //-------------------------------------------------
            else
            { os_printf("\r\nSC_TYPE:SC_TYPE_AIRKISS\r\n"); }

	    break;
	    //…………………………………………………………………………………………………

	    // 成功获取到【SSID】【PSWD】，保存STA参数，并连接WIFI
	    //…………………………………………………………………………………………………
        case SC_STATUS_LINK:
            os_printf("\r\nSC_STATUS_LINK\r\n");
            // 【SC_STATUS_LINK】状态下，参数2 == STA参数指针
            //------------------------------------------------------------------
            struct station_config *sta_conf = pdata;	// 获取【STA参数】指针

            // 将【SSID】【PASS】保存到【外部Flash】中
            //--------------------------------------------------------------------------
			spi_flash_erase_sector(Sector_STA_INFO);						// 擦除扇区
			spi_flash_write(Sector_STA_INFO*4096, (uint32 *)sta_conf, 96);	// 写入扇区
			//--------------------------------------------------------------------------
	        wifi_station_set_config(sta_conf);			// 设置STA参数【Flash】
	        wifi_station_disconnect();					// 断开STA连接
	        wifi_station_connect();						// ESP8266连接WIFI


	    break;
	    //…………………………………………………………………………………………………


        // ESP8266作为STA，成功连接到WIFI
	    //…………………………………………………………………………………………………
        case SC_STATUS_LINK_OVER:
            os_printf("\r\nSC_STATUS_LINK_OVER\r\n");

            smartconfig_stop();		// 停止SmartConfig
            ESP8266_Func_Cmd();
			os_printf("\r\n---- ESP8266 Connect to WIFI Successfully ----\r\n");


	    break;
	    //…………………………………………………………………………………………………

    }
}


void ICACHE_FLASH_ATTR ESP8266_Func_Cmd()
{
	MQTT_Connect(&mqttClient);
}

// OS_Timer_1_cb 回调函数
void ICACHE_FLASH_ATTR OS_Timer_1_Cb()
{
	//os_printf("@0X01: SW0: %s TH: %s TM: %s\r\n",mqtt_Info.switch_state, mqtt_Info.hour,mqtt_Info.min);
		u8 S_WIFI_STA_Connect;			// WIFI接入状态标志

		struct ip_info ST_ESP8266_IP;	// ESP8266的IP信息
		u8 ESP8266_IP[4];				// ESP8266的IP地址

		// 查询STA接入WIFI状态
		//-----------------------------------------------------
		S_WIFI_STA_Connect = wifi_station_get_connect_status();
		//---------------------------------------------------
		// Station连接状态表
		// 0 == STATION_IDLE -------------- STATION闲置
		// 1 == STATION_CONNECTING -------- 正在连接WIFI
		// 2 == STATION_WRONG_PASSWORD ---- WIFI密码错误
		// 3 == STATION_NO_AP_FOUND ------- 未发现指定WIFI
		// 4 == STATION_CONNECT_FAIL ------ 连接失败
		// 5 == STATION_GOT_IP ------------ 获得IP，连接成功
		//---------------------------------------------------

		switch(S_WIFI_STA_Connect)
		{
			case 0 : 	os_printf("\nSTATION_IDLE\n");				break;
			case 1 : 	os_printf("\nSTATION_CONNECTING\n");		break;
			case 2 : 	os_printf("\nSTATION_WRONG_PASSWORD\n");	break;
			case 3 : 	os_printf("\nSTATION_NO_AP_FOUND\n");		break;
			case 4 : 	os_printf("\nSTATION_CONNECT_FAIL\n");		break;
			case 5 : 	os_printf("\nSTATION_GOT_IP\n");			break;
		}

		// 成功接入WIFI【STA模式下，如果开启DHCP(默认)，则ESO8266的IP地址由WIFI路由器自动分配】
		//----------------------------------------------------------------------------------------
		if( S_WIFI_STA_Connect == STATION_GOT_IP )	// 判断是否获取IP
		{
			// 获取ESP8266_Station模式下的IP地址
			// DHCP-Client默认开启，ESP8266接入WIFI后，由路由器分配IP地址，IP地址不确定
			//--------------------------------------------------------------------------
			wifi_get_ip_info(STATION_IF,&ST_ESP8266_IP);	// 参数2：IP信息结构体指针

			// ESP8266_AP_IP.ip.addr是32位二进制代码，转换为点分十进制形式
			//----------------------------------------------------------------------------
			ESP8266_IP[0] = ST_ESP8266_IP.ip.addr;		// IP地址高八位 == addr低八位
			ESP8266_IP[1] = ST_ESP8266_IP.ip.addr>>8;	// IP地址次高八位 == addr次低八位
			ESP8266_IP[2] = ST_ESP8266_IP.ip.addr>>16;	// IP地址次低八位 == addr次高八位
			ESP8266_IP[3] = ST_ESP8266_IP.ip.addr>>24;	// IP地址低八位 == addr高八位

			// 显示ESP8266的IP地址
			//-----------------------------------------------------------------------------------------------
			os_printf("ESP8266_IP = %d.%d.%d.%d\n",
					ESP8266_IP[0],ESP8266_IP[1],ESP8266_IP[2],ESP8266_IP[3]);

			os_timer_disarm(&OS_Timer_1);
			wifi_set_opmode(0x01);
			ESP8266_Func_Cmd();
		}
		// ESP8266无法连接WIFI
			//------------------------------------------------------------------------------------------------
			else if(	S_WIFI_STA_Connect==STATION_NO_AP_FOUND 	||		// 未找到指定WIFI
						S_WIFI_STA_Connect==STATION_WRONG_PASSWORD 	||		// WIFI密码错误
						S_WIFI_STA_Connect==STATION_CONNECT_FAIL		)	// 连接WIFI失败
			{
				os_timer_disarm(&OS_Timer_1);			// 关闭定时器

				os_printf("\r\n---- ESP8266 Can't Connect to WIFI-----------\r\n");


				// 微信智能配网设置
				//…………………………………………………………………………………………………………………………
				//wifi_set_opmode(STATION_MODE);		// 设为STA模式							//【第①步】

				smartconfig_set_type(SC_TYPE_AIRKISS); 	// ESP8266配网方式【AIRKISS】			//【第②步】

				smartconfig_start(smartconfig_done);	// 进入【智能配网模式】,并设置回调函数	//【第③步】
				//…………………………………………………………………………………………………………………………

			}
}
