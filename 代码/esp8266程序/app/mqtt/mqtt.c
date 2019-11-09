/* mqtt.c
*  Protocol: http://docs.oasis-open.org/mqtt/mqtt/v3.1.1/os/mqtt-v3.1.1-os.html
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

#include "user_interface.h"
#include "osapi.h"
#include "espconn.h"
#include "os_type.h"
#include "mem.h"
#include "mqtt_msg.h"
#include "debug.h"
#include "user_config.h"
#include "mqtt.h"
#include "queue.h"

#define MQTT_TASK_PRIO           	2			// MQTT�������ȼ�
#define MQTT_TASK_QUEUE_SIZE    	1			// MQTT������Ϣ���
#define MQTT_SEND_TIMOUT        	5			// MQTT���ͳ�ʱ

#ifndef QUEUE_BUFFER_SIZE
#define QUEUE_BUFFER_SIZE             2048		// ���л��� = 2048
#endif

unsigned char *default_certificate;			// Ĭ��֤��ָ��
unsigned int default_certificate_len = 0;		// Ĭ��֤�鳤��
unsigned char *default_private_key;			// Ĭ����Կָ��
unsigned int default_private_key_len = 0;		// Ĭ����Կ����

os_event_t mqtt_procTaskQueue[MQTT_TASK_QUEUE_SIZE];	// MQTT����ṹ�壨����Ϊ�ṹ���������ʽ��


// ���������ɹ�_�ص�
//=============================================================================================
LOCAL void ICACHE_FLASH_ATTR mqtt_dns_found(const char *name, ip_addr_t *ipaddr, void *arg)
{
    struct espconn *pConn = (struct espconn *)arg;		 // ��ȡTCP����ָ��

    MQTT_Client* client = (MQTT_Client *)pConn->reverse; // ��ȡmqttClientָ��

    if (ipaddr == NULL)		// ��������ʧ��
    {
        INFO("DNS: Found, but got no ip, try to reconnect\r\n");

        client->connState = TCP_RECONNECT_REQ;	// TCP��������(�ȴ�5��)

        return;
    }

    INFO("DNS: found ip %d.%d.%d.%d\n",		// ��ӡ������Ӧ��IP��ַ
         *((uint8 *) &ipaddr->addr),
         *((uint8 *) &ipaddr->addr + 1),
         *((uint8 *) &ipaddr->addr + 2),
         *((uint8 *) &ipaddr->addr + 3));

    // �ж�IP��ַ�Ƿ���ȷ(?=0)
    //----------------------------------------------------------------------------------------
    if (client->ip.addr == 0 && ipaddr->addr != 0)	// δ����IP��ַ��mqttClient->ip.addr == 0
    {
        os_memcpy(client->pCon->proto.tcp->remote_ip, &ipaddr->addr, 4);	// IP��ֵ

        // ���ݰ�ȫ���ͣ����ò�ͬ��TCP���ӷ�ʽ
        //-------------------------------------------------------------------------------------------------
        if (client->security)		// ��ȫ���� != 0
        {
#ifdef MQTT_SSL_ENABLE
            if(DEFAULT_SECURITY >= ONE_WAY_ANTHENTICATION )		// ������֤��ONE_WAY_ANTHENTICATION = 2��
            {
                espconn_secure_ca_enable(ESPCONN_CLIENT,CA_CERT_FLASH_ADDRESS);
            }

            if(DEFAULT_SECURITY >= TWO_WAY_ANTHENTICATION)		// ˫����֤��TWO_WAY_ANTHENTICATION = 3��
            {
                espconn_secure_cert_req_enable(ESPCONN_CLIENT,CLIENT_CERT_FLASH_ADDRESS);
            }

            espconn_secure_connect(client->pCon);				// ����֤��TLS_WITHOUT_AUTHENTICATION = 1��
#else
            INFO("TCP: Do not support SSL\r\n");
#endif
        }

        else	// ��ȫ���� = 0 = NO_TLS
        {
            espconn_connect(client->pCon);		// TCP����(��ΪClient����Server)
        }

        client->connState = TCP_CONNECTING;		// TCP��������

        INFO("TCP: connecting...\r\n");
    }

    system_os_post(MQTT_TASK_PRIO, 0, (os_param_t)client);		// ��������MQTT_Task
}
//=============================================================================================


// ESP8266��ȡ����ˡ�PUBLISH�����ĵġ����⡿������Ч�غɡ�
//===================================================================================================================================
LOCAL void ICACHE_FLASH_ATTR deliver_publish(MQTT_Client* client, uint8_t* message, int length)
{
    mqtt_event_data_t event_data;

    event_data.topic_length = length;	// ���������ȳ�ʼ��
    event_data.topic = mqtt_get_publish_topic(message, &event_data.topic_length);	// ��ȡ��PUBLISH�����ĵ�������(ָ��)������������

    event_data.data_length = length;	// ��Ч�غɳ��ȳ�ʼ��
    event_data.data = mqtt_get_publish_data(message, &event_data.data_length);	// ��ȡ��PUBLISH�����ĵ��غ�(ָ��)���غɳ���


    // ���롾����MQTT��[PUBLISH]���ݡ�����
    //-------------------------------------------------------------------------------------------------------------------------
    if (client->dataCb)
        client->dataCb((uint32_t*)client, event_data.topic, event_data.topic_length, event_data.data, event_data.data_length);
}
//===================================================================================================================================


// ��MQTT���������͡����������ģ����Ĳ�д����У�TCPֱ�ӷ��ͣ�
//===================================================================================================================================
void ICACHE_FLASH_ATTR mqtt_send_keepalive(MQTT_Client *client)
{
    INFO("\r\nMQTT: Send keepalive packet to %s:%d!\r\n", client->host, client->port);

    client->mqtt_state.outbound_message = mqtt_msg_pingreq(&client->mqtt_state.mqtt_connection);	// ���á�PINGREQ������
    client->mqtt_state.pending_msg_type = MQTT_MSG_TYPE_PINGREQ;
    client->mqtt_state.pending_msg_type = mqtt_get_type(client->mqtt_state.outbound_message->data);	// ��ȡ��������
    client->mqtt_state.pending_msg_id = mqtt_get_id(client->mqtt_state.outbound_message->data, client->mqtt_state.outbound_message->length);

    // TCP���ͳɹ�/5���ʱ���� => ���ķ��ͽ���(sendTimeout=0)
    //-------------------------------------------------------------------------
    client->sendTimeout = MQTT_SEND_TIMOUT;	// ����MQTT����ʱ��sendTimeout=5

    INFO("MQTT: Sending, type: %d, id: %04X\r\n", client->mqtt_state.pending_msg_type, client->mqtt_state.pending_msg_id);

    err_t result = ESPCONN_OK;

    if (client->security)	// ��ȫ���� != 0
    {
#ifdef MQTT_SSL_ENABLE
        result = espconn_secure_send(client->pCon, client->mqtt_state.outbound_message->data, client->mqtt_state.outbound_message->length);
#else
        INFO("TCP: Do not support SSL\r\n");
#endif
    }
    else 	// ��ȫ���� = 0 = NO_TLS
    {
        result = espconn_send(client->pCon, client->mqtt_state.outbound_message->data, client->mqtt_state.outbound_message->length);
    }

    client->mqtt_state.outbound_message = NULL;		// ���ķ�����������վ����ָ��


    if(ESPCONN_OK == result)	// �������ݷ��ͳɹ�
    {
        client->keepAliveTick = 0;		// �������� = 0

        client->connState = MQTT_DATA;	// ESP8266��ǰ״̬ = MQTT��������

        system_os_post(MQTT_TASK_PRIO, 0, (os_param_t)client);	// ��������MQTT_Task
    }

    else 	// ���������󡿷���ʧ��
    {
        client->connState = TCP_RECONNECT_DISCONNECTING;		// TCP��ʱ�Ͽ�(�Ͽ��������)

        system_os_post(MQTT_TASK_PRIO, 0, (os_param_t)client);	// ��������MQTT_Task
    }
}
//===================================================================================================================================


/**
  * @brief  Delete tcp client and free all memory
  * @param  mqttClient: The mqtt client which contain TCP client
  * @retval None
  */
// ɾ��TCP���ӡ��ͷ�pCon�ڴ桢���TCP����ָ��
//=======================================================================
void ICACHE_FLASH_ATTR mqtt_tcpclient_delete(MQTT_Client *mqttClient)
{
    if (mqttClient->pCon != NULL)
    {
        INFO("Free memory\r\n");

        espconn_delete(mqttClient->pCon);			// ɾ����������

        if (mqttClient->pCon->proto.tcp)			// �����TCP����
            os_free(mqttClient->pCon->proto.tcp);	// �ͷ�TCP�ڴ�

        os_free(mqttClient->pCon);	// �ͷ�pCon�ڴ�

        mqttClient->pCon = NULL;	// ���mqttClient->pConָ��
    }
}
//=======================================================================


/**
  * @brief  Delete MQTT client and free all memory
  * @param  mqttClient: The mqtt client
  * @retval None
  */
// ɾ��MQTT�ͻ��ˣ����ͷ���������ڴ�
//=====================================================================================
void ICACHE_FLASH_ATTR mqtt_client_delete(MQTT_Client *mqttClient)
{
    mqtt_tcpclient_delete(mqttClient);	// ɾ��TCP���ӡ��ͷ�pCon�ڴ桢���TCP����ָ��

    if (mqttClient->host != NULL)
    {
        os_free(mqttClient->host);		// �ͷ�����(����/IP)�ڴ�
        mqttClient->host = NULL;
    }

    if (mqttClient->user_data != NULL)
    {
        os_free(mqttClient->user_data);	// �ͷ��û������ڴ�
        mqttClient->user_data = NULL;
    }

    if(mqttClient->connect_info.client_id != NULL)
    {
        os_free(mqttClient->connect_info.client_id);// �ͷ�client_id�ڴ�
        mqttClient->connect_info.client_id = NULL;
    }

    if(mqttClient->connect_info.username != NULL)
    {
        os_free(mqttClient->connect_info.username);	// �ͷ�username�ڴ�
        mqttClient->connect_info.username = NULL;
    }

    if(mqttClient->connect_info.password != NULL)
    {
        os_free(mqttClient->connect_info.password);	// �ͷ�password�ڴ�
        mqttClient->connect_info.password = NULL;
    }

    if(mqttClient->connect_info.will_topic != NULL)
    {
        os_free(mqttClient->connect_info.will_topic);// �ͷ�will_topic�ڴ�
        mqttClient->connect_info.will_topic = NULL;
    }

    if(mqttClient->connect_info.will_message != NULL)
    {
        os_free(mqttClient->connect_info.will_message);// �ͷ�will_message�ڴ�
        mqttClient->connect_info.will_message = NULL;
    }

    if(mqttClient->mqtt_state.in_buffer != NULL)
    {
        os_free(mqttClient->mqtt_state.in_buffer);	// �ͷ�in_buffer�ڴ�
        mqttClient->mqtt_state.in_buffer = NULL;
    }

    if(mqttClient->mqtt_state.out_buffer != NULL)
    {
        os_free(mqttClient->mqtt_state.out_buffer);	// �ͷ�out_buffer�ڴ�
        mqttClient->mqtt_state.out_buffer = NULL;
    }
}
//=============================================================================

/**
  * @brief  Client received callback function.
  * @param  arg: contain the ip link information
  * @param  pdata: received data
  * @param  len: the lenght of received data
  * @retval None
  */

// TCP�ɹ������������ݣ��������->�ͻ���ESP8266��
//==================================================================================================================================================
void ICACHE_FLASH_ATTR mqtt_tcpclient_recv(void *arg, char *pdata, unsigned short len)
{
    uint8_t msg_type;	// ��Ϣ����
    uint8_t msg_qos;	// ��������
    uint16_t msg_id;	// ��ϢID

    struct espconn *pCon = (struct espconn*)arg;		// ��ȡTCP����ָ��

    MQTT_Client *client = (MQTT_Client *)pCon->reverse;	// ��ȡmqttClientָ��

    client->keepAliveTick = 0;	// ֻҪ�յ��������ݣ��ͻ���(ESP8266)�������� = 0

// ��ȡ���ݰ�
//----------------------------------------------------------------------------------------------------------------------------
READPACKET:
    INFO("TCP: data received %d bytes\r\n",len);

// ����С�¡����
//###############################################################################################################################
    INFO("TCP: data received %d,%d,%d,%d \r\n", *pdata,*(pdata+1),*(pdata+2),*(pdata+3));	// �鿴��ȷ���������󡿱��ĵľ���ֵ	#
//###############################################################################################################################

    if (len<MQTT_BUF_SIZE && len>0)		// ���յ������ݳ���������Χ��
    {
        os_memcpy(client->mqtt_state.in_buffer, pdata, len);	// ��ȡ�������ݣ����롾��վ���Ļ�������

        msg_type = mqtt_get_type(client->mqtt_state.in_buffer);	// ��ȡ���������͡�
        msg_qos = mqtt_get_qos(client->mqtt_state.in_buffer);	// ��ȡ����Ϣ������
        msg_id = mqtt_get_id(client->mqtt_state.in_buffer, client->mqtt_state.in_buffer_length);	// ��ȡ�����еġ����ı�ʶ����

        // ����ESP8266����״̬��ִ����Ӧ����
        //-------------------------------------------------------------------------
        switch (client->connState)
        {
        	case MQTT_CONNECT_SENDING:				// ��MQTT_CONNECT_SENDING��
            if (msg_type == MQTT_MSG_TYPE_CONNACK) 	// �ж���Ϣ����!=��CONNACK��
            {
            	// ESP8266���� != ��CONNECT������
            	//--------------------------------------------------------------
                if (client->mqtt_state.pending_msg_type!=MQTT_MSG_TYPE_CONNECT)
                {
                    INFO("MQTT: Invalid packet\r\n");	// �������ݳ���

                    if (client->security)	// ��ȫ���� != 0
                    {
#ifdef MQTT_SSL_ENABLE
                        espconn_secure_disconnect(client->pCon);// �Ͽ�TCP����
#else
                        INFO("TCP: Do not support SSL\r\n");
#endif
                    }
                    else 	// ��ȫ���� = 0 = NO_TLS
                    {
                        espconn_disconnect(client->pCon);	// �Ͽ�TCP����
                    }
                }

                // ESP8266���� == ��CONNECT������
                //---------------------------------------------------------------------------------
                else
                {
                    INFO("MQTT: Connected to %s:%d\r\n", client->host, client->port);

                    client->connState = MQTT_DATA;	// ESP8266״̬�ı䣺��MQTT_DATA��

                    if (client->connectedCb)		// ִ��[mqttConnectedCb]����(MQTT���ӳɹ�����)
                    client->connectedCb((uint32_t*)client);	// ���� = mqttClient
                }
            }
            break;


        // ��ǰESP8266״̬ == MQTT_DATA || MQTT_KEEPALIVE_SEND
        //-----------------------------------------------------
        case MQTT_DATA:
        case MQTT_KEEPALIVE_SEND:

            client->mqtt_state.message_length_read = len;	// ��ȡ�����������ݵĳ���

            // ������յ������������У����ĵ�ʵ�ʳ���(ͨ����ʣ�೤�ȡ��õ�)
            //------------------------------------------------------------------------------------------------------------------------------
            client->mqtt_state.message_length = mqtt_get_total_length(client->mqtt_state.in_buffer, client->mqtt_state.message_length_read);


            // ��ǰESP8266״̬ ==��MQTT_DATA || MQTT_KEEPALIVE_SEND�������ݽ��յı������ͣ�����ESP8266�������Ķ���
            //-----------------------------------------------------------------------------------------------------------------------
            switch (msg_type)
            {
            // ESP8266���յ���SUBACK�����ģ�����ȷ��
            //-----------------------------------------------------------------------------------------------------------------------
            case MQTT_MSG_TYPE_SUBACK:
            	// �ж�ESP8266֮ǰ���͵ı����Ƿ��ǡ��������⡿
            	//-------------------------------------------------------------------------------------------------------------
                if (client->mqtt_state.pending_msg_type==MQTT_MSG_TYPE_SUBSCRIBE && client->mqtt_state.pending_msg_id==msg_id)
                    INFO("MQTT: Subscribe successful\r\n");		//
                break;
            //-----------------------------------------------------------------------------------------------------------------------


            // ESP8266���յ���UNSUBACK�����ģ�ȡ������ȷ��
          	//-----------------------------------------------------------------------------------------------------------------------
            case MQTT_MSG_TYPE_UNSUBACK:
            	// �ж�ESP8266֮ǰ���͵ı����Ƿ��ǡ��������⡿
            	//------------------------------------------------------------------------------------------------------------------
                if (client->mqtt_state.pending_msg_type == MQTT_MSG_TYPE_UNSUBSCRIBE && client->mqtt_state.pending_msg_id == msg_id)
                    INFO("MQTT: UnSubscribe successful\r\n");
                break;
        	//-----------------------------------------------------------------------------------------------------------------------


         	// ESP8266���յ���PUBLISH�����ģ�������Ϣ
        	//--------------------------------------------------------------------------------------------------------------------------------
            case MQTT_MSG_TYPE_PUBLISH:

                if (msg_qos == 1)	// �������->�ͻ��ˡ�������Ϣ Qos=1
                    client->mqtt_state.outbound_message = mqtt_msg_puback(&client->mqtt_state.mqtt_connection, msg_id);	// ���á�PUBACK������
                else if (msg_qos == 2)	// �������->�ͻ��ˡ�������Ϣ Qos=2
                    client->mqtt_state.outbound_message = mqtt_msg_pubrec(&client->mqtt_state.mqtt_connection, msg_id);	// ���á�PUBREC������

                if (msg_qos == 1 || msg_qos == 2)
                {
                    INFO("MQTT: Queue response QoS: %d\r\n", msg_qos);

                    // ��ESP8266Ӧ����(��PUBACK����PUBREC��)��д�����
                    //-------------------------------------------------------------------------------------------------------------------------------
                    if (QUEUE_Puts(&client->msgQueue, client->mqtt_state.outbound_message->data, client->mqtt_state.outbound_message->length) == -1)
                    {
                        INFO("MQTT: Queue full\r\n");
                    }
                }

                /// ��ȡ����ˡ�PUBLISH�����ĵġ����⡿������Ч�غɡ�
                //---------------------------------------------------------------------------------------------
                deliver_publish(client, client->mqtt_state.in_buffer, client->mqtt_state.message_length_read);

                break;
           	//-----------------------------------------------------------------------------------------------------------------------

            // ESP8266���յ���PUBACK�����ģ�������ϢӦ��
            //-------------------------------------------------------------------------------------------------------------------
            case MQTT_MSG_TYPE_PUBACK:
                if (client->mqtt_state.pending_msg_type == MQTT_MSG_TYPE_PUBLISH && client->mqtt_state.pending_msg_id == msg_id)
                {
                    INFO("MQTT: received MQTT_MSG_TYPE_PUBACK, finish QoS1 publish\r\n");
                }
                break;
            //-------------------------------------------------------------------------------------------------------------------


            // Qos == 2
            //-------------------------------------------------------------------------------------------------------------------------------------
            case MQTT_MSG_TYPE_PUBREC:
                client->mqtt_state.outbound_message = mqtt_msg_pubrel(&client->mqtt_state.mqtt_connection, msg_id);
                if (QUEUE_Puts(&client->msgQueue, client->mqtt_state.outbound_message->data, client->mqtt_state.outbound_message->length) == -1) {
                    INFO("MQTT: Queue full\r\n");
                }
                break;
            case MQTT_MSG_TYPE_PUBREL:
                client->mqtt_state.outbound_message = mqtt_msg_pubcomp(&client->mqtt_state.mqtt_connection, msg_id);
                if (QUEUE_Puts(&client->msgQueue, client->mqtt_state.outbound_message->data, client->mqtt_state.outbound_message->length) == -1) {
                    INFO("MQTT: Queue full\r\n");
                }
                break;
            case MQTT_MSG_TYPE_PUBCOMP:
                if (client->mqtt_state.pending_msg_type == MQTT_MSG_TYPE_PUBLISH && client->mqtt_state.pending_msg_id == msg_id) {
                    INFO("MQTT: receive MQTT_MSG_TYPE_PUBCOMP, finish QoS2 publish\r\n");
                }
                break;
            //-------------------------------------------------------------------------------------------------------------------------------------


            // ESP8266���յ���PINGREQ�����ģ�������Ƿ���˷��͸��ͻ��˵ģ���������£�ESP8266�����յ��˱��ġ�
            //-------------------------------------------------------------------------------------------------------------------------------------
            case MQTT_MSG_TYPE_PINGREQ:
                client->mqtt_state.outbound_message = mqtt_msg_pingresp(&client->mqtt_state.mqtt_connection);
                if (QUEUE_Puts(&client->msgQueue, client->mqtt_state.outbound_message->data, client->mqtt_state.outbound_message->length) == -1) {
                    INFO("MQTT: Queue full\r\n");
                }
                break;
            //-------------------------------------------------------------------------------------------------------------------------------------


            // ESP8266���յ���PINGRESP�����ģ�����Ӧ��
            //-----------------------------------------
            case MQTT_MSG_TYPE_PINGRESP:
                // Ignore	// ����Ӧ���������
                break;
            //-----------------------------------------
            }


            // NOTE: this is done down here and not in the switch case above
            // because the PSOCK_READBUF_LEN() won't work inside a switch
            // statement due to the way protothreads resume.

            // ESP8266���յ���PUBLISH�����ģ�������Ϣ
            //-------------------------------------------------------------------------------------
            if (msg_type == MQTT_MSG_TYPE_PUBLISH)
            {
                len = client->mqtt_state.message_length_read;

                // ����ʵ�ʳ��� < �������ݳ���
                //------------------------------------------------------------------------------
                if (client->mqtt_state.message_length < client->mqtt_state.message_length_read)
                {
                    //client->connState = MQTT_PUBLISH_RECV;
                    //Not Implement yet
                    len -= client->mqtt_state.message_length;		// ����֮ǰ�������ı���
                    pdata += client->mqtt_state.message_length;		// ָ��֮��Ϊ��������������

                    INFO("Get another published message\r\n");

                    goto READPACKET;	// ���½�����������
                }
            }
            break;		// case MQTT_DATA:
            			// case MQTT_KEEPALIVE_SEND:
            //-------------------------------------------------------------------------------------
        }
    }

    else 	// ���յ��ı��ĳ���
    {
        INFO("ERROR: Message too long\r\n");
    }

    system_os_post(MQTT_TASK_PRIO, 0, (os_param_t)client);	// ��������MQTT_Task
}
//==================================================================================================================================================


/**
  * @brief  Client send over callback function.
  * @param  arg: contain the ip link information
  * @retval None
  */

// TCP���ͳɹ�_�ص�����
//================================================================================
void ICACHE_FLASH_ATTR mqtt_tcpclient_sent_cb(void *arg)
{
    struct espconn *pCon = (struct espconn *)arg;		// ��ȡTCP����ָ��

    MQTT_Client* client = (MQTT_Client *)pCon->reverse;	// ��ȡmqttClientָ��

    INFO("TCP: Sent\r\n");


    client->sendTimeout = 0;	// TCP���ͳɹ�/���ķ���5���ʱ���� => ���ķ��ͽ���(sendTimeout=0)

    client->keepAliveTick =0;	// ֻҪTCP���ͳɹ����ͻ���(ESP8266)�������� = 0


    // ��ǰ��MQTT�ͻ��� ������Ϣ/�������� ״̬
    //--------------------------------------------------------------------------
    if ((client->connState==MQTT_DATA || client->connState==MQTT_KEEPALIVE_SEND)
                && client->mqtt_state.pending_msg_type==MQTT_MSG_TYPE_PUBLISH)
    {
        if (client->publishedCb)					// ִ��[MQTT�����ɹ�]����
            client->publishedCb((uint32_t*)client);
    }

    system_os_post(MQTT_TASK_PRIO, 0, (os_param_t)client);	// ��������MQTT_Task
}
//================================================================================


// MQTT��ʱ(1��)�����ܣ�������ʱ��������ʱ��TCP���ͼ�ʱ��
//================================================================================
void ICACHE_FLASH_ATTR mqtt_timer(void *arg)
{
    MQTT_Client* client = (MQTT_Client*)arg;

    // �����ǰ��[MQTT_DATA]״̬������д���ʱ����
    //--------------------------------------------------------------------------
    if (client->connState == MQTT_DATA)		// MQTT_DATA
    {
        client->keepAliveTick ++;	// �ͻ���(ESP8266)��������++

        // �жϿͻ���(ESP8266)�������� ?> �������ӵ�1/2ʱ��
        //--------------------------------------------------------------------------------------------------------------------------
        if (client->keepAliveTick >= (client->mqtt_state.connect_info->keepalive)-10)	//��ע���ٷ��������ǣ��ж��Ƿ񳬹���������ʱ����
        {
            client->connState = MQTT_KEEPALIVE_SEND;	// MQTT_KEEPALIVE_SEND �����Ƿ���һ��������

            system_os_post(MQTT_TASK_PRIO,0,(os_param_t)client);// ��������
        }
    }

    // �����ȴ���ʱ����������������״̬����ȴ�5�룬֮�������������
    //--------------------------------------------------------------------------
    else if (client->connState == TCP_RECONNECT_REQ)	// TCP��������(�ȴ�5��)
    {
        client->reconnectTick ++;	// ������ʱ++

        if (client->reconnectTick > MQTT_RECONNECT_TIMEOUT)	// �������󳬹���
        {
            client->reconnectTick = 0;	// ������ʱ = 0

            client->connState = TCP_RECONNECT;	// TCP��������

            system_os_post(MQTT_TASK_PRIO, 0, (os_param_t)client);	// ��������

            // �����ȴ���ʱ����
            //-----------------------------------------------------------------
            if (client->timeoutCb)							// δ������ʱ�ص����� ����û��д�ص�����
                client->timeoutCb((uint32_t*)client);
        }
    }

    // TCP���ͳɹ�/���ķ���5���ʱ���� => ���ķ��ͽ���(sendTimeout=0)
    //----------------------------------------------------------------
    if (client->sendTimeout>0)		// ����MQTT����ʱ��sendTimeout=5
        client->sendTimeout --;		// sendTimeoutÿ��ݼ�(ֱ��=0)
}
//================================================================================


// TCP�Ͽ��ɹ�_�ص�����
//================================================================================
void ICACHE_FLASH_ATTR mqtt_tcpclient_discon_cb(void *arg)
{
    struct espconn *pespconn = (struct espconn *)arg;	  // ��ȡTCP����ָ��

    MQTT_Client*client = (MQTT_Client*)pespconn->reverse; // ��ȡmqttClientָ��

    INFO("TCP: Disconnected callback\r\n");

    // ��ִ����������
    //����������������������������������������������������������������������������
    if(TCP_DISCONNECTING == client->connState)	// ��ǰ״̬����ǣ�TCP���ڶϿ�
    {
        client->connState = TCP_DISCONNECTED;	// ESP8266״̬�ı䣺TCP�ɹ��Ͽ�
    }

    else if(MQTT_DELETING == client->connState)// ��ǰ״̬����ǣ�MQTT����ɾ��
    {
        client->connState = MQTT_DELETED;		// ESP8266״̬�ı䣺MQTTɾ��
    }
    //����������������������������������������������������������������������������


    // ֻҪ������������״̬����ô��TCP�Ͽ����Ӻ󣬽��롾TCP��������(�ȴ�5��)��
    //����������������������������������������������������������������������������
    else
    {
        client->connState = TCP_RECONNECT_REQ;	// TCP��������(�ȴ�5��)
    }

    if (client->disconnectedCb)					// ����[MQTT�ɹ��Ͽ�����]����
        client->disconnectedCb((uint32_t*)client);
    //����������������������������������������������������������������������������


    system_os_post(MQTT_TASK_PRIO, 0, (os_param_t)client);	// ��������MQTT_Task
}
//================================================================================


/**
  * @brief  Tcp client connect success callback function.
  * @param  arg: contain the ip link information
  * @retval None
  */

// TCP���ӳɹ��Ļص�����
//============================================================================================================================================
void ICACHE_FLASH_ATTR mqtt_tcpclient_connect_cb(void *arg)
{
    struct espconn *pCon = (struct espconn *)arg;		 // ��ȡTCP����ָ��
    MQTT_Client* client = (MQTT_Client *)(pCon->reverse);// ��ȡmqttClientָ��

    // ע��ص�����
    //--------------------------------------------------------------------------------------
    espconn_regist_disconcb(client->pCon, mqtt_tcpclient_discon_cb);	// TCP�Ͽ��ɹ�_�ص�
    espconn_regist_recvcb(client->pCon, mqtt_tcpclient_recv);			// TCP���ճɹ�_�ص�
    espconn_regist_sentcb(client->pCon, mqtt_tcpclient_sent_cb);		// TCP���ͳɹ�_�ص�

    INFO("MQTT: Connected to broker %s:%d\r\n", client->host, client->port);

    // ��CONNECT�����ķ���׼��
    //����������������������������������������������������������������������������������������������������������������������������������������
    // ��ʼ��MQTT���Ļ�����
    //-----------------------------------------------------------------------------------------------------------------------
	mqtt_msg_init(&client->mqtt_state.mqtt_connection, client->mqtt_state.out_buffer, client->mqtt_state.out_buffer_length);

	// ���á�CONNECT�����Ʊ��ģ�����ȡ��CONNECT������[ָ��]��[����]
    //----------------------------------------------------------------------------------------------------------------------------
    client->mqtt_state.outbound_message = mqtt_msg_connect(&client->mqtt_state.mqtt_connection, client->mqtt_state.connect_info);

    // ��ȡ�����͵ı�������(�˴��ǡ�CONNECT������)
    //----------------------------------------------------------------------------------------------
    client->mqtt_state.pending_msg_type = mqtt_get_type(client->mqtt_state.outbound_message->data);

    // ��ȡ�����ͱ����еġ����ı�ʶ��������CONNECT��������û�У�
    //-------------------------------------------------------------------------------------------------------------------------------------
    client->mqtt_state.pending_msg_id = mqtt_get_id(client->mqtt_state.outbound_message->data,client->mqtt_state.outbound_message->length);

    // TCP���ͳɹ�/���ķ���5���ʱ���� => ���ķ��ͽ���(sendTimeout=0)
    //-------------------------------------------------------------------------
    client->sendTimeout = MQTT_SEND_TIMOUT;	// ����MQTT����ʱ��sendTimeout=5

    INFO("MQTT: Sending, type: %d, id: %04X\r\n", client->mqtt_state.pending_msg_type, client->mqtt_state.pending_msg_id);
    //����������������������������������������������������������������������������������������������������������������������������������������

    // TCP�����͡�CONNECT������
    //-----------------------------------------------------------------------------------------------------------------------------
    if (client->security)	// ��ȫ���� != 0
    {
#ifdef MQTT_SSL_ENABLE
        espconn_secure_send(client->pCon, client->mqtt_state.outbound_message->data, client->mqtt_state.outbound_message->length);
#else
        INFO("TCP: Do not support SSL\r\n");
#endif
    }
    else	// ��ȫ���� = 0 = NO_TLS
    {
    	// TCP���ͣ�����=[client->mqtt_state.outbound_message->data]������=[client->mqtt_state.outbound_message->length]
    	//-----------------------------------------------------------------------------------------------------------------
        espconn_send(client->pCon, client->mqtt_state.outbound_message->data, client->mqtt_state.outbound_message->length);
    }
    //-----------------------------------------------------------------------------------------------------------------------------

    client->mqtt_state.outbound_message = NULL;		// ���ķ�����������վ����ָ��

    client->connState = MQTT_CONNECT_SENDING;		// ״̬��Ϊ��MQTT��CONNECT�����ķ����С�MQTT_CONNECT_SENDING��

    system_os_post(MQTT_TASK_PRIO, 0, (os_param_t)client);	// ��������MQTT_Task

}
//============================================================================================================================================


/**
  * @brief  Tcp client connect repeat callback function.
  * @param  arg: contain the ip link information
  * @retval None
  */

// TCP�쳣�ж�_�ص�
//==============================================================================
void ICACHE_FLASH_ATTR mqtt_tcpclient_recon_cb(void *arg, sint8 errType)
{
    struct espconn *pCon = (struct espconn *)arg;		// ��ȡTCP����ָ��
    MQTT_Client* client = (MQTT_Client *)pCon->reverse;	// ��ȡmqttClientָ��

    INFO("TCP: Reconnect to %s:%d\r\n", client->host, client->port);

    client->connState = TCP_RECONNECT_REQ;		// TCP��������(�ȴ�5��)

    system_os_post(MQTT_TASK_PRIO, 0, (os_param_t)client);	// ��������MQTT_Task
}
//==============================================================================

/**
  * @brief  MQTT publish function.
  * @param  client:     MQTT_Client reference
  * @param  topic:         string topic will publish to
  * @param  data:         buffer data send point to
  * @param  data_length: length of data
  * @param  qos:        qos
  * @param  retain:        retain
  * @retval TRUE if success queue
  */

// ESP8266�����ⷢ����Ϣ��������2�������� / ����3��������Ϣ����Ч�غ� / ����4����Ч�غɳ��� / ����5������Qos / ����6��Retain��
//============================================================================================================================================
BOOL ICACHE_FLASH_ATTR MQTT_Publish(MQTT_Client *client, const char* topic, const char* data, int data_length, int qos, int retain)
{
    uint8_t dataBuffer[MQTT_BUF_SIZE];	// �������Ļ���(1204�ֽ�)
    uint16_t dataLen;					// �������ĳ���

    // ���á�PUBLISH�����ģ�����ȡ��PUBLISH������[ָ��]��[����]
    //------------------------------------------------------------------------------------------
    client->mqtt_state.outbound_message = mqtt_msg_publish(&client->mqtt_state.mqtt_connection,
                                          topic, data, data_length,
                                          qos, retain,
                                          &client->mqtt_state.pending_msg_id);

    if (client->mqtt_state.outbound_message->length == 0)	// �жϱ����Ƿ���ȷ
    {
        INFO("MQTT: Queuing publish failed\r\n");
        return FALSE;
    }

    // ���ڴ�ӡ����PUBLISH�����ĳ���,(����װ������/���д�С)
    //--------------------------------------------------------------------------------------------------------------------------------------------------------------------
    INFO("MQTT: queuing publish, length: %d, queue size(%d/%d)\r\n", client->mqtt_state.outbound_message->length, client->msgQueue.rb.fill_cnt, client->msgQueue.rb.size);

    // ������д����У�������д���ֽ���(����������)
    //----------------------------------------------------------------------------------------------------------------------------------
    while (QUEUE_Puts(&client->msgQueue, client->mqtt_state.outbound_message->data, client->mqtt_state.outbound_message->length) == -1)
    {
        INFO("MQTT: Queue full\r\n");	// ��������

        // ���������е����ݰ�
        //-----------------------------------------------------------------------------------------------
        if (QUEUE_Gets(&client->msgQueue, dataBuffer, &dataLen, MQTT_BUF_SIZE) == -1)	// ����ʧ�� = -1
        {
            INFO("MQTT: Serious buffer error\r\n");

            return FALSE;
        }
    }

    system_os_post(MQTT_TASK_PRIO, 0, (os_param_t)client);	// ��������

    return TRUE;
}
//============================================================================================================================================


/**
  * @brief  MQTT subscibe function.
  * @param  client:     MQTT_Client reference
  * @param  topic:		string topic will subscribe
  * @param  qos:        qos
  * @retval TRUE if success queue
  */

// ESP8266�������⡾����2����������� / ����3������Qos��
//============================================================================================================================================
BOOL ICACHE_FLASH_ATTR MQTT_Subscribe(MQTT_Client *client, char* topic, uint8_t qos)
{
    uint8_t dataBuffer[MQTT_BUF_SIZE];		// �������Ļ���(1204�ֽ�)
    uint16_t dataLen;						// �������ĳ���

    // ���á�SUBSCRIBE�����ģ�����ȡ��SUBSCRIBE������[ָ��]��[����]
    //----------------------------------------------------------------------------------------------------------------------------------------
    client->mqtt_state.outbound_message=mqtt_msg_subscribe(&client->mqtt_state.mqtt_connection,topic, qos,&client->mqtt_state.pending_msg_id);

    INFO("MQTT: queue subscribe, topic\"%s\", id: %d\r\n", topic, client->mqtt_state.pending_msg_id);


    // ������д����У�������д���ֽ���(����������)
    //----------------------------------------------------------------------------------------------------------------------------------
    while(QUEUE_Puts(&client->msgQueue, client->mqtt_state.outbound_message->data, client->mqtt_state.outbound_message->length) == -1)
    {
        INFO("MQTT: Queue full\r\n");

        // ���������еı���
        //-----------------------------------------------------------------------------------------------
        if (QUEUE_Gets(&client->msgQueue, dataBuffer, &dataLen, MQTT_BUF_SIZE) == -1)	// ����ʧ�� = -1
        {
            INFO("MQTT: Serious buffer error\r\n");

            return FALSE;
        }
    }

    system_os_post(MQTT_TASK_PRIO, 0, (os_param_t)client);	// ��������MQTT_Task

    return TRUE;
}
//============================================================================================================================================

/**
  * @brief  MQTT un-subscibe function.
  * @param  client:     MQTT_Client reference
  * @param  topic:   String topic will un-subscribe
  * @retval TRUE if success queue
  */

// ESP8266ȡ����������
//============================================================================================================================================
BOOL ICACHE_FLASH_ATTR MQTT_UnSubscribe(MQTT_Client *client, char* topic)
{
    uint8_t dataBuffer[MQTT_BUF_SIZE];	// �������Ļ���(1204�ֽ�)
    uint16_t dataLen;					// �������ĳ���

    // ���á�UNSUBSCRIBE������	������2��ȡ���������� /  ����3�����ı�ʶ����
    //----------------------------------------------------------------------------------------------------------------------------------------
    client->mqtt_state.outbound_message = mqtt_msg_unsubscribe(&client->mqtt_state.mqtt_connection,topic,&client->mqtt_state.pending_msg_id);

    INFO("MQTT: queue un-subscribe, topic\"%s\", id: %d\r\n", topic, client->mqtt_state.pending_msg_id);

    // ������д����У�������д���ֽ���(����������)
	//----------------------------------------------------------------------------------------------------------------------------------
    while (QUEUE_Puts(&client->msgQueue, client->mqtt_state.outbound_message->data, client->mqtt_state.outbound_message->length) == -1)
    {
        INFO("MQTT: Queue full\r\n");

        // ���������еı���
        //-----------------------------------------------------------------------------------------------
        if (QUEUE_Gets(&client->msgQueue, dataBuffer, &dataLen, MQTT_BUF_SIZE) == -1)	// ����ʧ�� = -1
        {
            INFO("MQTT: Serious buffer error\r\n");
            return FALSE;
        }
    }
    system_os_post(MQTT_TASK_PRIO, 0, (os_param_t)client);	// ��������MQTT_Task

    return TRUE;
}
//============================================================================================================================================

/**
  * @brief  MQTT ping function.
  * @param  client:     MQTT_Client reference
  * @retval TRUE if success queue
  */

// ��MQTT���������͡����������ģ�������д����У��������з��ͣ�
//============================================================================================================================================
BOOL ICACHE_FLASH_ATTR MQTT_Ping(MQTT_Client *client)
{
    uint8_t dataBuffer[MQTT_BUF_SIZE];
    uint16_t dataLen;
    client->mqtt_state.outbound_message = mqtt_msg_pingreq(&client->mqtt_state.mqtt_connection);	// ���á�PINGREQ������

    if(client->mqtt_state.outbound_message->length == 0)	// ���Ĵ���
    {
        INFO("MQTT: Queuing publish failed\r\n");
        return FALSE;
    }

    INFO("MQTT: queuing publish, length: %d, queue size(%d/%d)\r\n", client->mqtt_state.outbound_message->length, client->msgQueue.rb.fill_cnt, client->msgQueue.rb.size);


    // ������д����У�������д���ֽ���(����������)
	//----------------------------------------------------------------------------------------------------------------------------------
    while(QUEUE_Puts(&client->msgQueue, client->mqtt_state.outbound_message->data, client->mqtt_state.outbound_message->length) == -1)
    {
        INFO("MQTT: Queue full\r\n");

        // ���������еı���
        //-----------------------------------------------------------------------------------------------
        if(QUEUE_Gets(&client->msgQueue, dataBuffer, &dataLen, MQTT_BUF_SIZE) == -1)	// ����ʧ�� = -1
        {
            INFO("MQTT: Serious buffer error\r\n");
            return FALSE;
        }
    }

    system_os_post(MQTT_TASK_PRIO, 0, (os_param_t)client);	// ��������MQTT_Task

    return TRUE;
}
//============================================================================================================================================


// MQTT�����������񣺸���ESP8266����״̬��ִ����Ӧ������
//--------------------------------------------------------------------------------------------
// TCP_RECONNECT_REQ			TCP��������(�ȴ�5��)	�˳�Tsak(5��󣬽���TCP_RECONNECT״̬)
//--------------------------------------------------------------------------------------------
// TCP_RECONNECT				TCP��������				ִ��MQTT����׼����������ESP8266״̬
//--------------------------------------------------------------------------------------------
// MQTT_DELETING				MQTT����ɾ��			TCP�Ͽ�����
// TCP_DISCONNECTING			TCP���ڶϿ�
// TCP_RECONNECT_DISCONNECTING	TCP��ʱ�Ͽ�(�Ͽ��������)
//--------------------------------------------------------------------------------------------
// TCP_DISCONNECTED				TCP�ɹ��Ͽ�				ɾ��TCP���ӣ����ͷ�pCon�ڴ�
//--------------------------------------------------------------------------------------------
// MQTT_DELETED					MQTT��ɾ��				ɾ��MQTT�ͻ��ˣ����ͷ�����ڴ�
//--------------------------------------------------------------------------------------------
// MQTT_KEEPALIVE_SEND			MQTT����				�������������������
//--------------------------------------------------------------------------------------------
// MQTT_DATA					MQTT���ݴ���			TCP���Ͷ����еı���
//====================================================================================================================================
void ICACHE_FLASH_ATTR MQTT_Task(os_event_t *e)	// ���ж���Ϣ����
{
	INFO("\r\n------------- MQTT_Task -------------\r\n");

    MQTT_Client* client = (MQTT_Client*)e->par;		// ��e->par�� == ��mqttClientָ���ֵ��,����������ת��

    uint8_t dataBuffer[MQTT_BUF_SIZE];	// ���ݻ�����(1204�ֽ�)

    uint16_t dataLen;					// ���ݳ���

    if (e->par == 0)		// û��mqttClientָ�룬����
    return;


    // ����ESP8266����״̬��ִ����Ӧ����
    //������������������������������������������������������������������������������������������������������������������������������
    switch (client->connState)
    {
    	// TCP��������(�ȴ�5��)���˳�Tsak
    	//---------------------------------
    	case TCP_RECONNECT_REQ:		break;
    	//---------------------------------


    	// TCP�������ӣ�ִ��MQTT����׼����������ESP8266״̬
    	//--------------------------------------------------------------------------------
    	case TCP_RECONNECT:

    		mqtt_tcpclient_delete(client);	// ɾ��TCP���ӡ��ͷ�pCon�ڴ桢���TCP����ָ��

    		MQTT_Connect(client);			// MQTT����׼����TCP���ӡ�����������

    		INFO("TCP: Reconnect to: %s:%d\r\n", client->host, client->port);

    		client->connState = TCP_CONNECTING;		// TCP��������

    		break;
    	//--------------------------------------------------------------------------------


    	// MQTT����ɾ����TCP���ڶϿ������������󡿱��ķ���ʧ�ܣ�TCP�Ͽ�����
    	//------------------------------------------------------------------
    	case MQTT_DELETING:
    	case TCP_DISCONNECTING:
    	case TCP_RECONNECT_DISCONNECTING:
    		if (client->security)	// ��ȫ���� != 0
    		{
#ifdef MQTT_SSL_ENABLE
    			espconn_secure_disconnect(client->pCon);
#else
    			INFO("TCP: Do not support SSL\r\n");
#endif
    		}
    		else 	// ��ȫ���� = 0 = NO_TLS
    		{
    			espconn_disconnect(client->pCon);	// TCP�Ͽ�����
    		}

    		break;
    	//------------------------------------------------------------------


    	// TCP�ɹ��Ͽ�
    	//--------------------------------------------------------------------------------
    	case TCP_DISCONNECTED:
    		INFO("MQTT: Disconnected\r\n");
    		mqtt_tcpclient_delete(client);	// ɾ��TCP���ӡ��ͷ�pCon�ڴ桢���TCP����ָ��
    		break;
    	//--------------------------------------------------------------------------------


    	// MQTT��ɾ����ESP8266��״̬Ϊ[MQTT��ɾ��]�󣬽�MQTT����ڴ��ͷ�
    	//--------------------------------------------------------------------
    	case MQTT_DELETED:
    		INFO("MQTT: Deleted client\r\n");
    		mqtt_client_delete(client);		// ɾ��MQTT�ͻ��ˣ����ͷ�����ڴ�
    		break;


    	// MQTT�ͻ��˴���
    	//--------------------------------------------------------------------
    	case MQTT_KEEPALIVE_SEND:
    		mqtt_send_keepalive(client);	// ��MQTT���������͡�����������
    		break;


    	// MQTT��������״̬
    	//-------------------------------------------------------------------------------------------------------------------------------
    	case MQTT_DATA:
    		if (QUEUE_IsEmpty(&client->msgQueue) || client->sendTimeout != 0)
    		{
    			break;	// ������Ϊ�� || ����δ����������ִ�в���
    		}

    		// �����зǿ� && ���ͽ����������������� �����еı���
    		//--------------------------------------------------------------------------------------------------------
    		if (QUEUE_Gets(&client->msgQueue, dataBuffer, &dataLen, MQTT_BUF_SIZE) == 0)	// �����ɹ� = 0
    		{
				client->mqtt_state.pending_msg_type = mqtt_get_type(dataBuffer);		// ��ȡ�����еġ��������͡�
				client->mqtt_state.pending_msg_id = mqtt_get_id(dataBuffer, dataLen);	// ��ȡ�����еġ����ı�ʶ����

				client->sendTimeout = MQTT_SEND_TIMOUT;	// ����MQTT����ʱ��sendTimeout=5

				INFO("MQTT: Sending, type: %d, id: %04X\r\n", client->mqtt_state.pending_msg_type, client->mqtt_state.pending_msg_id);


				// ���ͱ���
				//-----------------------------------------------------------------------
				if (client->security)	// ��ȫ���� != 0
				{
#ifdef MQTT_SSL_ENABLE
					espconn_secure_send(client->pCon, dataBuffer, dataLen);
#else
					INFO("TCP: Do not support SSL\r\n");
#endif
				}
				else	// ��ȫ���� = 0 = NO_TLS
				{
					espconn_send(client->pCon, dataBuffer, dataLen);	// TCP�������ݰ�
				}

				client->mqtt_state.outbound_message = NULL;		// ���ķ�����������վ����ָ��

				break;
    		}
        break;
    }
    //������������������������������������������������������������������������������������������������������������������������������

}	// ������MQTT_Task������
//====================================================================================================================================


/**
  * @brief  MQTT initialization connection function
  * @param  client:     MQTT_Client reference
  * @param  host:     Domain or IP string
  * @param  port:     Port to connect
  * @param  security:        1 for ssl, 0 for none
  * @retval None
  */

// �������Ӳ�����ֵ�������������mqtt_test_jx.mqtt.iot.gz.baidubce.com�����������Ӷ˿ڡ�1883������ȫ���͡�0��NO_TLS��
//====================================================================================================================
void ICACHE_FLASH_ATTR MQTT_InitConnection(MQTT_Client *mqttClient, uint8_t* host, uint32_t port, uint8_t security)
{
    uint32_t temp;

    INFO("MQTT_InitConnection\r\n");

    os_memset(mqttClient, 0, sizeof(MQTT_Client));		// ��MQTT�ͻ��ˡ��ṹ�� = 0

    temp = os_strlen(host);								// ���������/IP���ַ�������
    mqttClient->host = (uint8_t*)os_zalloc(temp+1);		// ����ռ䣬��ŷ��������/IP��ַ�ַ���

    os_strcpy(mqttClient->host, host);					// �ַ�������
    mqttClient->host[temp] = 0;							// ���'\0'

    mqttClient->port = port;							// ����˿ں� = 1883

    mqttClient->security = security;					// ��ȫ���� = 0 = NO_TLS
}
//====================================================================================================================

/**
  * @brief  MQTT initialization mqtt client function
  * @param  client:     MQTT_Client reference
  * @param  clientid: 	MQTT client id
  * @param  client_user:MQTT client user
  * @param  client_pass:MQTT client password
  * @param  client_pass:MQTT keep alive timer, in second
  * @retval None
  */

// MQTT���Ӳ�����ֵ���ͻ��˱�ʶ����..����MQTT�û�����..����MQTT��Կ��..������������ʱ����120s��������Ự��1��clean_session��
//======================================================================================================================================================
void ICACHE_FLASH_ATTR
MQTT_InitClient(MQTT_Client *mqttClient, uint8_t* client_id, uint8_t* client_user, uint8_t* client_pass, uint32_t keepAliveTime, uint8_t cleanSession)
{
    uint32_t temp;

    INFO("MQTT_InitClient\r\n");

    // MQTT��CONNECT�����ĵ����Ӳ��� ��ֵ
    //---------------------------------------------------------------------------------------------------------------
    os_memset(&mqttClient->connect_info, 0, sizeof(mqtt_connect_info_t));		// MQTT��CONNECT�����ĵ����Ӳ��� = 0

    temp = os_strlen(client_id);
    mqttClient->connect_info.client_id = (uint8_t*)os_zalloc(temp + 1);			// ���롾�ͻ��˱�ʶ�����Ĵ���ڴ�
    os_strcpy(mqttClient->connect_info.client_id, client_id);					// ��ֵ���ͻ��˱�ʶ����
    mqttClient->connect_info.client_id[temp] = 0;								// ���'\0'

    if (client_user)	// �ж��Ƿ��С�MQTT�û�����
    {
        temp = os_strlen(client_user);
        mqttClient->connect_info.username = (uint8_t*)os_zalloc(temp + 1);
        os_strcpy(mqttClient->connect_info.username, client_user);				// ��ֵ��MQTT�û�����
        mqttClient->connect_info.username[temp] = 0;
    }

    if (client_pass)	// �ж��Ƿ��С�MQTT���롿
    {
        temp = os_strlen(client_pass);
        mqttClient->connect_info.password = (uint8_t*)os_zalloc(temp + 1);
        os_strcpy(mqttClient->connect_info.password, client_pass);				// ��ֵ��MQTT���롿
        mqttClient->connect_info.password[temp] = 0;
    }

    mqttClient->connect_info.keepalive = keepAliveTime;							// �������� = 120s
    mqttClient->connect_info.clean_session = cleanSession;						// ����Ự = 1 = clean_session
    //--------------------------------------------------------------------------------------------------------------

    // ����mqtt_state���ֲ���
    //------------------------------------------------------------------------------------------------------------------------------------------------------------
    mqttClient->mqtt_state.in_buffer = (uint8_t *)os_zalloc(MQTT_BUF_SIZE);		// ����in_buffer�ڴ桾��վ���Ļ�������
    mqttClient->mqtt_state.in_buffer_length = MQTT_BUF_SIZE;					// ����in_buffer��С
    mqttClient->mqtt_state.out_buffer = (uint8_t *)os_zalloc(MQTT_BUF_SIZE);	// ����out_buffer�ڴ桾��վ���Ļ�������
    mqttClient->mqtt_state.out_buffer_length = MQTT_BUF_SIZE;					// ����out_buffer��С
    mqttClient->mqtt_state.connect_info = &(mqttClient->connect_info);			// MQTT��CONNECT�����ĵ����Ӳ���(ָ��)����ֵ��mqttClient->mqtt_state.connect_info


    // ��ʼ��MQTT��վ���Ļ�����
    //----------------------------------------------------------------------------------------------------------------------------------
    mqtt_msg_init(&mqttClient->mqtt_state.mqtt_connection, mqttClient->mqtt_state.out_buffer, mqttClient->mqtt_state.out_buffer_length);

    QUEUE_Init(&mqttClient->msgQueue, QUEUE_BUFFER_SIZE);	// ��Ϣ���г�ʼ�������п��Դ��һ��/���MQTT���ġ�


    // ����������������MQTT_Task�������ȼ���2��������ָ�롾mqtt_procTaskQueue������Ϣ��ȡ�1��
    //---------------------------------------------------------------------------------------------
    system_os_task(MQTT_Task, MQTT_TASK_PRIO, mqtt_procTaskQueue, MQTT_TASK_QUEUE_SIZE);

    // �������񣺲���1=����ȼ� / ����2=��Ϣ���� / ����3=��Ϣ����
    //-----------------------------------------------------------------------------------------------
    system_os_post(MQTT_TASK_PRIO, 0, (os_param_t)mqttClient);	// ����3�����ͱ���Ϊ��os_param_t����
}
//======================================================================================================================================================


// �����������������⡾...����������Ϣ��...��������������Will_Qos=0�����������֡�Will_Retain=0��
//====================================================================================================================
void ICACHE_FLASH_ATTR
MQTT_InitLWT(MQTT_Client *mqttClient, uint8_t* will_topic, uint8_t* will_msg, uint8_t will_qos, uint8_t will_retain)
{
    uint32_t temp;
    temp = os_strlen(will_topic);
    mqttClient->connect_info.will_topic = (uint8_t*)os_zalloc(temp + 1);		// ���롾�������⡿�Ĵ���ڴ�
    os_strcpy(mqttClient->connect_info.will_topic, will_topic);					// ��ֵ���������⡿
    mqttClient->connect_info.will_topic[temp] = 0;								// ���'\0'

    temp = os_strlen(will_msg);
    mqttClient->connect_info.will_message = (uint8_t*)os_zalloc(temp + 1);
    os_strcpy(mqttClient->connect_info.will_message, will_msg);					// ��ֵ��������Ϣ��
    mqttClient->connect_info.will_message[temp] = 0;

    mqttClient->connect_info.will_qos = will_qos;			// ����������Will_Qos=0��
    mqttClient->connect_info.will_retain = will_retain;		// �������֡�Will_Retain=0��
}
//====================================================================================================================

/**
  * @brief  Begin connect to MQTT broker
  * @param  client: MQTT_Client reference
  * @retval None
  */

// WIFI���ӡ�SNTP�ɹ��� => MQTT����׼��(����TCP���ӡ���������)
//============================================================================================================================================
void ICACHE_FLASH_ATTR MQTT_Connect(MQTT_Client *mqttClient)
{
    //espconn_secure_set_size(0x01,6*1024);	 // SSL˫����֤ʱ����ʹ��	// try to modify memory size 6*1024 if ssl/tls handshake failed

	// ��ʼMQTT����ǰ���ж��Ƿ����MQTT��TCP���ӡ�����У������֮ǰ��TCP����
	//------------------------------------------------------------------------------------
    if (mqttClient->pCon)
    {
        // Clean up the old connection forcefully - using MQTT_Disconnect
        // does not actually release the old connection until the
        // disconnection callback is invoked.

        mqtt_tcpclient_delete(mqttClient);	// ɾ��TCP���ӡ��ͷ�pCon�ڴ桢���TCP����ָ��
    }

    // TCP�������� �Ѷ�ȡ�ڴ�����Ĳ���������
    //------------------------------------------------------------------------------------------------------
    mqttClient->pCon = (struct espconn *)os_zalloc(sizeof(struct espconn));	// ����pCon�ڴ�
    mqttClient->pCon->type = ESPCONN_TCP;										// ��ΪTCP����
    mqttClient->pCon->state = ESPCONN_NONE;
    mqttClient->pCon->proto.tcp = (esp_tcp *)os_zalloc(sizeof(esp_tcp));		// ����esp_tcp�ڴ�
    mqttClient->pCon->proto.tcp->local_port = espconn_port();					// ��ȡESP8266���ö˿�
    mqttClient->pCon->proto.tcp->remote_port = mqttClient->port;				// ���ö˿ں�
    mqttClient->pCon->reverse = mqttClient;										// mqttClient->pCon->reverse ���� mqttClientָ��

    espconn_regist_connectcb(mqttClient->pCon, mqtt_tcpclient_connect_cb);		// ע��TCP���ӳɹ��Ļص�����
    espconn_regist_reconcb(mqttClient->pCon, mqtt_tcpclient_recon_cb);			// ע��TCP�쳣�жϵĻص�����


    //---------------------------------------------------------------------------------------------------
    mqttClient->keepAliveTick = 0;	// MQTT�ͻ���(ESP8266)��������
    mqttClient->reconnectTick = 0;	// �����ȴ���ʱ����������������״̬����ȴ�5�룬֮�������������

    // ����MQTT��ʱ(1��)�����ܣ�������ʱ��������ʱ��TCP���ͼ�ʱ��
    //---------------------------------------------------------------------------------------------------
    os_timer_disarm(&mqttClient->mqttTimer);
    os_timer_setfn(&mqttClient->mqttTimer, (os_timer_func_t *)mqtt_timer, mqttClient);	// mqtt_timer
    os_timer_arm(&mqttClient->mqttTimer, 1000, 1);										// 1�붨ʱ(�ظ�)


    // ��ӡSSL���ã���ȫ����[NO_TLS == 0]
    //--------------------------------------------------------------------------------------------------------------------------------------------------------------
    os_printf("your ESP SSL/TLS configuration is %d.[0:NO_TLS\t1:TLS_WITHOUT_AUTHENTICATION\t2ONE_WAY_ANTHENTICATION\t3TWO_WAY_ANTHENTICATION]\n",DEFAULT_SECURITY);


    // �������ʮ������ʽ��IP��ַ
    //------------------------------------------------------------------------------------------------------------------
    if (UTILS_StrToIP(mqttClient->host, &mqttClient->pCon->proto.tcp->remote_ip))	// ����IP��ַ(���ʮ�����ַ�����ʽ)
    {
        INFO("TCP: Connect to ip  %s:%d\r\n", mqttClient->host, mqttClient->port);	// ��ӡIP��ַ

        // ���ݰ�ȫ���ͣ����ò�ͬ��TCP���ӷ�ʽ
        //-------------------------------------------------------------------------------------------------
        if (mqttClient->security)	// ��ȫ���� != 0
        {
#ifdef MQTT_SSL_ENABLE

            if(DEFAULT_SECURITY >= ONE_WAY_ANTHENTICATION )		// ������֤��ONE_WAY_ANTHENTICATION = 2��
            {
                espconn_secure_ca_enable(ESPCONN_CLIENT,CA_CERT_FLASH_ADDRESS);
            }

            if(DEFAULT_SECURITY >= TWO_WAY_ANTHENTICATION)		// ˫����֤��TWO_WAY_ANTHENTICATION = 3��
            {
                espconn_secure_cert_req_enable(ESPCONN_CLIENT,CLIENT_CERT_FLASH_ADDRESS);
            }

            espconn_secure_connect(mqttClient->pCon);			// ����֤��TLS_WITHOUT_AUTHENTICATION = 1��
#else
            INFO("TCP: Do not support SSL\r\n");
#endif
        }

        else	// ��ȫ���� = 0 = NO_TLS
        {
            espconn_connect(mqttClient->pCon);	// TCP����(��ΪClient����Server)
        }
    }

    // ��������
    //----------------------------------------------------------------------------------------------
    else
    {
        INFO("TCP: Connect to domain %s:%d\r\n", mqttClient->host, mqttClient->port);

        espconn_gethostbyname(mqttClient->pCon, mqttClient->host, &mqttClient->ip, mqtt_dns_found);
    }

    mqttClient->connState = TCP_CONNECTING;		// TCP��������
}
//============================================================================================================================================


// TCP�Ͽ����ӣ�Clean up the old connection forcefully��
//==============================================================================
void ICACHE_FLASH_ATTR MQTT_Disconnect(MQTT_Client *mqttClient)
{
    mqttClient->connState = TCP_DISCONNECTING;	// ESP8266״̬�ı䣺TCP���ڶϿ�

    system_os_post(MQTT_TASK_PRIO, 0, (os_param_t)mqttClient);	// ��������

    os_timer_disarm(&mqttClient->mqttTimer);	// ȡ��MQTT��ʱ
}
//==============================================================================


// ɾ��MQTT�ͻ���
//==============================================================================
void ICACHE_FLASH_ATTR MQTT_DeleteClient(MQTT_Client *mqttClient)
{
    mqttClient->connState = MQTT_DELETING;	// ESP8266״̬�ı䣺MQTT����ɾ��

    system_os_post(MQTT_TASK_PRIO, 0, (os_param_t)mqttClient);	// ��������

    os_timer_disarm(&mqttClient->mqttTimer);	// ȡ��MQTT��ʱ
}
//==============================================================================


// ���������ض���
//������������������������������������������������������������������������������������������������������
// ִ�� mqttClient->connectedCb(...) => mqttConnectedCb(...)
//------------------------------------------------------------------------------------------
void ICACHE_FLASH_ATTR MQTT_OnConnected(MQTT_Client*mqttClient, MqttCallback connectedCb)
{
    mqttClient->connectedCb = connectedCb;	// ��������mqttConnectedCb��
}

// ִ�� mqttClient->disconnectedCb(...) => mqttDisconnectedCb(...)
//-------------------------------------------------------------------------------------------------
void ICACHE_FLASH_ATTR MQTT_OnDisconnected(MQTT_Client *mqttClient, MqttCallback disconnectedCb)
{
    mqttClient->disconnectedCb = disconnectedCb;	// ��������mqttDisconnectedCb��
}

// ִ�� mqttClient->dataCb(...) => mqttDataCb(...)
//------------------------------------------------------------------------------------
void ICACHE_FLASH_ATTR MQTT_OnData(MQTT_Client *mqttClient, MqttDataCallback dataCb)
{
    mqttClient->dataCb = dataCb;	// ��������mqttDataCb��
}

// ִ�� mqttClient->publishedCb(...) => mqttPublishedCb(...)
//-------------------------------------------------------------------------------------------
void ICACHE_FLASH_ATTR MQTT_OnPublished(MQTT_Client *mqttClient, MqttCallback publishedCb)
{
    mqttClient->publishedCb = publishedCb;	// ��������mqttPublishedCb��
}

// ִ�� mqttClient->timeoutCb(...) => ��...��δ���庯��
//--------------------------------------------------------------------------------------
void ICACHE_FLASH_ATTR MQTT_OnTimeout(MQTT_Client *mqttClient, MqttCallback timeoutCb)
{
    mqttClient->timeoutCb = timeoutCb;
}
//������������������������������������������������������������������������������������������������������
