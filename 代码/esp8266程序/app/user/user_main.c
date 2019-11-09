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




// ͷ�ļ�
//==============================

#include "func.h"

//=================================
//Ԥ����
//=================================

//==============================
// ���Ͷ���
//=================================
typedef unsigned long 		u32_t;

//=================================
// ȫ�ֱ���
//============================================================================
MQTT_Client mqttClient;			// MQTT�ͻ���_�ṹ�塾�˱����ǳ���Ҫ��
MQTT_Info mqtt_Info;
uint32 priv_param_start_sec;
ip_addr_t IP_Server;
struct espconn AP_NetCon;
struct espconn STA_NetCon;
os_timer_t OS_Timer_1;		// �����ʱ����ѯip����
os_timer_t OS_Timer_SNTP;
struct softap_config AP_Config;
struct station_config STA_Config;	// STA�����ṹ��

char waether_buf[1000]="";
uint8 times_rec =0;
//============================================================================
//�Զ���delay_ms
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
// DNS_������������_�ص�����������1�������ַ���ָ�� / ����2��IP��ַ�ṹ��ָ�� / ����3���������ӽṹ��ָ�롿
//============================================================================
void ICACHE_FLASH_ATTR DNS_Over_Cb(const char * name, ip_addr_t *ipaddr, void *arg)
{

}
//============================================================================
// STA TCP�������ӶϿ��ɹ��Ļص�����
//============================================================================
void ICACHE_FLASH_ATTR ESP8266_TCP_Disconnect_Cb(void *arg)
{

}
//============================================================================
// STA TCP & AP UDP�ɹ������������ݵĻص�����
//============================================================================
void ICACHE_FLASH_ATTR ESP8266_WIFI_Send_Cb(void *arg)
{
	/*os_printf("\nESP8266_WIFI_Send_OK\n");*/	// ��Է�����Ӧ��   ��������
}
//============================================================================
// STA TCP���ջص�����  ��ȡ������Ϣ
//============================================================================
void ICACHE_FLASH_ATTR ESP8266_Weather_Recv_Cb(void * arg, char * pdata, unsigned short len)
{

}
//============================================================================
// STA TCP �������ӽ����ɹ��Ļص�����
//============================================================================
void ICACHE_FLASH_ATTR ESP8266_TCP_Connect_Cb(void *arg)
{
	espconn_regist_sentcb((struct espconn *)arg, ESP8266_WIFI_Send_Cb);			// ע���������ݷ��ͳɹ��Ļص�����
	espconn_regist_recvcb((struct espconn *)arg, ESP8266_Weather_Recv_Cb);			// ע���������ݽ��ճɹ��Ļص�����
	espconn_regist_disconcb((struct espconn *)arg,ESP8266_TCP_Disconnect_Cb);	// ע��ɹ��Ͽ�TCP���ӵĻص�����

	os_printf("\n--------------- ESP8266_TCP_Connect_OK ---------------\n");
}
//============================================================================
// STA TCP�����쳣�Ͽ�ʱ�Ļص�����
//============================================================================
void ICACHE_FLASH_ATTR ESP8266_TCP_Break_Cb(void *arg,sint8 err)
{
	os_printf("\nESP8266_TCP_Break\n");
	espconn_connect(&STA_NetCon);	// ����TCP-server
}
//============================================================================
// ESP8266_STA_NetCon_Init �������ӳ�ʼ�� ����������
//============================================================================
void ICACHE_FLASH_ATTR ESP8266_STA_NetCon_Init()
{

}
//============================================================================
//UDP AP ���յĻص�����;arg��espconnָ��;pdata������ָ��;len�����ݳ���
//============================================================================
void ICACHE_FLASH_ATTR ESP8266_WIFI_Recv_Cb(void * arg, char * pdata, unsigned short len)
{

}

//============================================================================
//OS_Timer_1�����ʱ����ʼ��(ms����) ���ķǳ���Ҫ ������΢��������Ĭ������wifi
//============================================================================
void ICACHE_FLASH_ATTR OS_Timer_1_Init(u32 time_ms, u8 time_repetitive)
{
	os_timer_disarm(&OS_Timer_1);	// �رն�ʱ��
	os_timer_setfn(&OS_Timer_1,(os_timer_func_t *)OS_Timer_1_Cb, NULL);	// ���ö�ʱ��
	os_timer_arm(&OS_Timer_1, time_ms, time_repetitive);  // ʹ�ܶ�ʱ��
}


//============================================================================
// ESP8266 ģʽ���õĳ�ʼ��
//============================================================================
void ICACHE_FLASH_ATTR ESP8266_AP_STA_Init()
{
	os_memset(&STA_Config, 0, sizeof(struct station_config));	// STA�����ṹ�� = 0
	spi_flash_read(Sector_STA_INFO*4096,(uint32 *)&STA_Config, 96);	// ������STA������(SSID/PASS)
	//д����smartconnect�ǿ�
	STA_Config.ssid[31] = 0;		// SSID�����'\0'
	STA_Config.password[63] = 0;	// APSS�����'\0'
	wifi_set_opmode(0x01);
	if(wifi_station_set_config(&STA_Config) == true)	// ����STA�����������浽Flash
	{
		OS_Timer_1_Init(1000, 1);
	}
}
//============================================================================
// MQTT�ѳɹ����ӣ�ESP8266���͡�CONNECT���������յ���CONNACK��
//============================================================================
void mqttConnectedCb(uint32_t *args)
{
    MQTT_Client* client = (MQTT_Client*)args;	// ��ȡmqttClientָ��
    INFO("MQTT: Connected\r\n");

}
//============================================================================
// MQTT�ɹ��Ͽ�����
//============================================================================
void mqttDisconnectedCb(uint32_t *args)
{
    MQTT_Client* client = (MQTT_Client*)args;
    INFO("MQTT: Disconnected\r\n");
}
//============================================================================
// MQTT�ɹ�������Ϣ
//============================================================================
void mqttPublishedCb(uint32_t *args)
{
    MQTT_Client* client = (MQTT_Client*)args;
    INFO("MQTT: Published\r\n");
}
//============================================================================
// ������MQTT��[PUBLISH]���ݡ�����		������1������ / ����2�����ⳤ�� / ����3����Ч�غ� / ����4����Ч�غɳ��ȡ�
//===============================================================================================================
void mqttDataCb(uint32_t *args, const char* topic, uint32_t topic_len, const char *data, uint32_t data_len)
{
    char *topicBuf = (char*)os_zalloc(topic_len+1);		// ���롾���⡿�ռ�
    char *dataBuf  = (char*)os_zalloc(data_len+1);		// ���롾��Ч�غɡ��ռ�
    char *str;
    MQTT_Client* client = (MQTT_Client*)args;	// ��ȡMQTT_Clientָ��
    os_memcpy(topicBuf, topic, topic_len);	// ��������
    topicBuf[topic_len] = 0;				// �����'\0'
    os_memcpy(dataBuf, data, data_len);		// ������Ч�غ�
    dataBuf[data_len] = 0;					// �����'\0'
    os_printf("Receive topic: %s, data: %s \r\n", topicBuf, dataBuf);	// ���ڴ�ӡ�����⡿����Ч�غɡ�
    os_printf("Topic_len = %d, Data_len = %d\r\n", topic_len, data_len);	// ���ڴ�ӡ�����ⳤ�ȡ�����Ч�غɳ��ȡ�

//########################################################################################
    // ���ݽ��յ���������/��Ч�غɣ�����LED����/��
    //-----------------------------------------------------------------------------------
    if( os_strncmp(topicBuf,"$creq",5) == 0 )			// ���� == "SW_LED"
    {
    	os_printf("MQTT_GET:%s\n",dataBuf);

    }
//########################################################################################
    os_free(str);
    os_free(topicBuf);	// �ͷš����⡿�ռ�
    os_free(dataBuf);	// �ͷš���Ч�غɡ��ռ�
}
//===============================================================================================================


// user_init��entry of user application, init user function here
//===================================================================================================================
void user_init(void)
{
	uart_init(9600,0);// ���ڲ�������Ϊ115200
	delay_ms(10);
	os_printf("\r\n=======================================\r\n");
	ESP8266_AP_STA_Init();
	//��GPIO����
	PIN_FUNC_SELECT(PERIPHS_IO_MUX_GPIO2_U, FUNC_GPIO2);
	GPIO_OUTPUT_SET(GPIO_ID_PIN(2),0);
	//���ڴ��wifi �˺�����
	os_memset(&STA_Config,0,sizeof(struct station_config));			// STA_INFO = 0
	spi_flash_read(Sector_STA_INFO*4096,(uint32 *)&STA_Config, 96);	// ������STA������(SSID/PASS)
	spi_flash_read(0x98*4096,(uint32 *)&mqtt_Info,sizeof(MQTT_Info));
	STA_Config.ssid[31] = 0;		// SSID�����'\0'
	STA_Config.password[63] = 0;	// APSS�����'\0'

    CFG_Load();	// ����/����ϵͳ������WIFI������MQTT������

    // �������Ӳ�����ֵ�������������mqtt_test_jx.mqtt.iot.gz.baidubce.com�����������Ӷ˿ڡ�1883������ȫ���͡�0��NO_TLS��
	//-------------------------------------------------------------------------------------------------------------------
	MQTT_InitConnection(&mqttClient, sysCfg.mqtt_host, sysCfg.mqtt_port, sysCfg.security);

	// MQTT���Ӳ�����ֵ���ͻ��˱�ʶ����..����MQTT�û�����..����MQTT��Կ��..������������ʱ����120s��������Ự��1��clean_session��
	//----------------------------------------------------------------------------------------------------------------------------
	MQTT_InitClient(&mqttClient, sysCfg.device_id, sysCfg.mqtt_user, sysCfg.mqtt_pass, sysCfg.mqtt_keepalive, 1);

	// ����MQTT��غ���
	//--------------------------------------------------------------------------------------------------
	MQTT_OnConnected(&mqttClient, mqttConnectedCb);			// ���á�MQTT�ɹ����ӡ���������һ�ֵ��÷�ʽ
	MQTT_OnDisconnected(&mqttClient, mqttDisconnectedCb);	// ���á�MQTT�ɹ��Ͽ�����������һ�ֵ��÷�ʽ
	MQTT_OnPublished(&mqttClient, mqttPublishedCb);			// ���á�MQTT�ɹ���������������һ�ֵ��÷�ʽ
	MQTT_OnData(&mqttClient, mqttDataCb);					// ���á�����MQTT���ݡ���������һ�ֵ��÷�ʽ


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
