/*
 * fuc.h
 *
 *  Created on: 2019��6��2��
 *      Author: Administrator
 */

#ifndef APP_INCLUDE_FUNC_H_
#define APP_INCLUDE_FUNC_H_

#include "ets_sys.h"
#include "driver/uart.h"
#include "osapi.h"
#include "mqtt.h"
#include "wifi.h"
#include "config.h"
#include "debug.h"
#include "gpio.h"
#include "user_interface.h"
#include "mem.h"
#include "sntp.h"
#include "driver/dht11.h"
#include "espconn.h"
#include "mem.h"
#include "my_cJSON.h"
#include <string.h>
#include "os_type.h"
#include "ctype.h"
#include "user_config.h"
#include "smartconfig.h"		// ��������

void string_Packet_Publish(MQTT_Client* client,char *data);

void ICACHE_FLASH_ATTR smartconfig_done(sc_status status, void *pdata);


void ICACHE_FLASH_ATTR OS_Timer_1_Cb();

void ICACHE_FLASH_ATTR save_Mqtt_Info(MQTT_Info *pdata);

void ICACHE_FLASH_ATTR ESP8266_Func_Cmd();
extern os_timer_t OS_Timer_1;
extern MQTT_Client mqttClient;			// MQTT�ͻ���_�ṹ�塾�˱����ǳ���Ҫ��
extern struct softap_config AP_Config;
extern struct station_config STA_Config;	// STA�����ṹ��
extern struct espconn STA_NetCon;
extern struct espconn AP_NetCon;
extern MQTT_Info mqtt_Info;
#endif /* APP_INCLUDE_FUNC_H_ */
