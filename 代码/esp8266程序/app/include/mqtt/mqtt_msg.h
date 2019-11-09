/* 
 * File:   mqtt_msg.h
 * Author: Minh Tuan
 *
 * Created on July 12, 2014, 1:05 PM
 */

#ifndef MQTT_MSG_H
#define	MQTT_MSG_H
#include "mqtt_config.h"
#include "c_types.h"
#ifdef	__cplusplus
extern "C" {
#endif

/*
* Copyright (c) 2014, Stephen Robinson
* All rights reserved.
*
* Redistribution and use in source and binary forms, with or without
* modification, are permitted provided that the following conditions
* are met:
*
* 1. Redistributions of source code must retain the above copyright
* notice, this list of conditions and the following disclaimer.
* 2. Redistributions in binary form must reproduce the above copyright
* notice, this list of conditions and the following disclaimer in the
* documentation and/or other materials provided with the distribution.
* 3. Neither the name of the copyright holder nor the names of its
* contributors may be used to endorse or promote products derived
* from this software without specific prior written permission.
*
* THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
* AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
* IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
* ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE
* LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
* CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
* SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
* INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
* CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
* ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
* POSSIBILITY OF SUCH DAMAGE.
*
*/
/* 7			6			5			4			3			2			1			0*/
/*|      --- Message Type----			|  DUP Flag	|	   QoS Level		|	Retain	|
/*										Remaining Length								 */


// MQTTЭ�飺���Ʊ��ĵ�����
//-----------------------------------------------------------------------------
enum mqtt_message_type
{
  MQTT_MSG_TYPE_CONNECT = 1,		// �ͻ����������ӷ����	���ͣ�������
  MQTT_MSG_TYPE_CONNACK = 2,		// ���ӱ���ȷ��			���������͡�
  MQTT_MSG_TYPE_PUBLISH = 3,		// ������Ϣ
  MQTT_MSG_TYPE_PUBACK = 4,			// ������Ϣȷ��
  MQTT_MSG_TYPE_PUBREC = 5,			//
  MQTT_MSG_TYPE_PUBREL = 6,			//
  MQTT_MSG_TYPE_PUBCOMP = 7,		//
  MQTT_MSG_TYPE_SUBSCRIBE = 8,		// �ͻ��˶�������		���ͣ�������
  MQTT_MSG_TYPE_SUBACK = 9,			// ����������ȷ��		���������͡�
  MQTT_MSG_TYPE_UNSUBSCRIBE = 10,	// �ͻ���ȡ����������	���ͣ�������
  MQTT_MSG_TYPE_UNSUBACK = 11,		// ȡ�����ı���ȷ��		���������͡�
  MQTT_MSG_TYPE_PINGREQ = 12,		// ��������				���ͣ�������
  MQTT_MSG_TYPE_PINGRESP = 13,		// ������Ӧ				���������͡�
  MQTT_MSG_TYPE_DISCONNECT = 14		// �ͻ��˶Ͽ�����		���ͣ�������
};

// MQTT���Ʊ���[ָ��]��[����]
//------------------------------------------------------------------------
typedef struct mqtt_message
{
  uint8_t* data;	// MQTT���Ʊ���ָ��
  uint16_t length;	// MQTT���Ʊ��ĳ���(���ñ���ʱ����Ϊ����ָ���ƫ��ֵ)

} mqtt_message_t;
//------------------------------------------------------------------------

// MQTT����
//------------------------------------------------------------------------
typedef struct mqtt_connection
{
  mqtt_message_t message;	// MQTT���Ʊ���[ָ��]��[����]

  uint16_t message_id;		// ���ı�ʶ��
  uint8_t* buffer;			// ��վ���Ļ�����ָ��	buffer = os_zalloc(1024)
  uint16_t buffer_length;	// ��վ���Ļ���������	1024

} mqtt_connection_t;
//------------------------------------------------------------------------

// MQTT��CONNECT�����ĵ����Ӳ���
//---------------------------------------
typedef struct mqtt_connect_info
{
  char* client_id;		// �ͻ��˱�ʶ��
  char* username;		// MQTT�û���
  char* password;		// MQTT����
  char* will_topic;		// ��������
  char* will_message;  	// ������Ϣ
  int keepalive;		// ��������ʱ��
  int will_qos;			// ������Ϣ����
  int will_retain;		// ��������
  int clean_session;	// ����Ự
} mqtt_connect_info_t;
//---------------------------------------


static inline int ICACHE_FLASH_ATTR mqtt_get_type(uint8_t* buffer) { return (buffer[0] & 0xf0) >> 4; }
static inline int ICACHE_FLASH_ATTR mqtt_get_dup(uint8_t* buffer) { return (buffer[0] & 0x08) >> 3; }
static inline int ICACHE_FLASH_ATTR mqtt_get_qos(uint8_t* buffer) { return (buffer[0] & 0x06) >> 1; }
static inline int ICACHE_FLASH_ATTR mqtt_get_retain(uint8_t* buffer) { return (buffer[0] & 0x01); }

void ICACHE_FLASH_ATTR mqtt_msg_init(mqtt_connection_t* connection, uint8_t* buffer, uint16_t buffer_length);
int ICACHE_FLASH_ATTR mqtt_get_total_length(uint8_t* buffer, uint16_t length);
const char* ICACHE_FLASH_ATTR mqtt_get_publish_topic(uint8_t* buffer, uint16_t* length);
const char* ICACHE_FLASH_ATTR mqtt_get_publish_data(uint8_t* buffer, uint16_t* length);
uint16_t ICACHE_FLASH_ATTR mqtt_get_id(uint8_t* buffer, uint16_t length);

mqtt_message_t* ICACHE_FLASH_ATTR mqtt_msg_connect(mqtt_connection_t* connection, mqtt_connect_info_t* info);
mqtt_message_t* ICACHE_FLASH_ATTR mqtt_msg_publish(mqtt_connection_t* connection, const char* topic, const char* data, int data_length, int qos, int retain, uint16_t* message_id);
mqtt_message_t* ICACHE_FLASH_ATTR mqtt_msg_puback(mqtt_connection_t* connection, uint16_t message_id);
mqtt_message_t* ICACHE_FLASH_ATTR mqtt_msg_pubrec(mqtt_connection_t* connection, uint16_t message_id);
mqtt_message_t* ICACHE_FLASH_ATTR mqtt_msg_pubrel(mqtt_connection_t* connection, uint16_t message_id);
mqtt_message_t* ICACHE_FLASH_ATTR mqtt_msg_pubcomp(mqtt_connection_t* connection, uint16_t message_id);
mqtt_message_t* ICACHE_FLASH_ATTR mqtt_msg_subscribe(mqtt_connection_t* connection, const char* topic, int qos, uint16_t* message_id);
mqtt_message_t* ICACHE_FLASH_ATTR mqtt_msg_unsubscribe(mqtt_connection_t* connection, const char* topic, uint16_t* message_id);
mqtt_message_t* ICACHE_FLASH_ATTR mqtt_msg_pingreq(mqtt_connection_t* connection);
mqtt_message_t* ICACHE_FLASH_ATTR mqtt_msg_pingresp(mqtt_connection_t* connection);
mqtt_message_t* ICACHE_FLASH_ATTR mqtt_msg_disconnect(mqtt_connection_t* connection);


#ifdef	__cplusplus
}
#endif

#endif	/* MQTT_MSG_H */

