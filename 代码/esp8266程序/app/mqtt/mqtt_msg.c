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
#include "user_interface.h"
#include <string.h>
#include "mqtt_msg.h"
#include "user_config.h"
#define MQTT_MAX_FIXED_HEADER_SIZE 3

#include "osapi.h"

// ��CONNECT�����Ʊ��ĵ�Flag��־λ
//---------------------------------------------------------------
enum mqtt_connect_flag
{
  MQTT_CONNECT_FLAG_USERNAME = 1 << 7,			// Username
  MQTT_CONNECT_FLAG_PASSWORD = 1 << 6,			// Password
  MQTT_CONNECT_FLAG_WILL_RETAIN = 1 << 5,		// Will Retain
  MQTT_CONNECT_FLAG_WILL = 1 << 2,				// Will Flag
  MQTT_CONNECT_FLAG_CLEAN_SESSION = 1 << 1		// Clean Session
};

// ��CONNECT�����Ʊ��ĵĿɱ�ͷ���� == 12Byte(v31)/10Byte(v311)
//-----------------------------------------------------------------
struct __attribute((__packed__)) mqtt_connect_variable_header
{
  uint8_t lengthMsb;					// Э����[MQlsdp/MQTT]����
  uint8_t lengthLsb;					// 6/4
#if defined(PROTOCOL_NAMEv31)
  uint8_t magic[6];						// MQIsdp
#elif defined(PROTOCOL_NAMEv311)
  uint8_t magic[4];						// "MQTT"
#else
#error "Please define protocol name"
#endif
  uint8_t version;						// Э��汾
  uint8_t flags;						// ���ӱ�־
  uint8_t keepaliveMsb;					// keepalive
  uint8_t keepaliveLsb;
};

// ��[�����ַ���]�ֶ���ӵ����Ļ����������ĳ���+=[�����ַ���]��ռ����	��ע���ַ���ǰ��������ֽڵ�ǰ׺��
//========================================================================================================
static int ICACHE_FLASH_ATTR append_string(mqtt_connection_t* connection, const char* string, int len)
{
  if(connection->message.length + len + 2 > connection->buffer_length)	// �жϱ����Ƿ����
  return -1;

  // �����ַ���ǰ�����ֽ�ǰ׺����ʾ���ַ����ĳ���
  //--------------------------------------------------------------------------
  connection->buffer[connection->message.length++] = len >> 8;		// �߰�λ
  connection->buffer[connection->message.length++] = len & 0xff;	// �Ͱ�λ

  memcpy(connection->buffer+connection->message.length, string, len);	// ��[�����ַ���]��ӵ����Ļ�����

  connection->message.length += len;	// ���ĳ��� += [�����ַ���]��ռ����

  return len + 2;	// ����[�����ַ���]��MQTT���Ʊ�������ռ����
}
//========================================================================================================


// ��ӡ����ı�ʶ�����ֶ�
//========================================================================================================
static uint16_t ICACHE_FLASH_ATTR append_message_id(mqtt_connection_t* connection, uint16_t message_id)
{
  // If message_id is zero then we should assign one, otherwise
  // we'll use the one supplied by the caller
  while(message_id == 0)
    message_id = ++connection->message_id;	// �����ı�ʶ����++

  if(connection->message.length + 2 > connection->buffer_length)	// ���Ĺ���
    return 0;

  connection->buffer[connection->message.length++] = message_id >> 8;	// ��ӡ����ı�ʶ�����ֶ�
  connection->buffer[connection->message.length++] = message_id & 0xff;	// 2�ֽ�

  return message_id;	// ���ء����ı�ʶ����
}
//========================================================================================================


// ���ñ��ĳ��� = 3(��ʱ��Ϊ���̶���ͷ������(3)��֮����ӡ��ɱ䱨ͷ��������Ч�غɡ�)
//====================================================================================
static int ICACHE_FLASH_ATTR init_message(mqtt_connection_t* connection)
{
  connection->message.length = MQTT_MAX_FIXED_HEADER_SIZE;	// ���ĳ��� = 3(�̶���ͷ)
  return MQTT_MAX_FIXED_HEADER_SIZE;
}
//====================================================================================

// ������Ч
//====================================================================================
static mqtt_message_t* ICACHE_FLASH_ATTR fail_message(mqtt_connection_t* connection)
{
  connection->message.data = connection->buffer;	// ����ָ��ָ���Ļ������׵�ַ
  connection->message.length = 0;					// ���ĳ��� = 0
  return &connection->message;
}
//====================================================================================

// ���á�MQTT���Ʊ��ġ��Ĺ̶���ͷ
//------------------------------------------------------------------------------------------------------------------------------
// ע��	�ڡ�PUBLISH�������У��������ͱ�־λ���ظ��ַ���־[dup][Bit3]����������[qos][Bit2��1]�����ı�����־[retain][Bit1=0]��ɡ�
//		�������ͱ��ĵı������ͱ�־λ�ǹ̶��ġ�
//==============================================================================================================================
static mqtt_message_t* ICACHE_FLASH_ATTR fini_message(mqtt_connection_t* connection, int type, int dup, int qos, int retain)
{
  int remaining_length = connection->message.length - MQTT_MAX_FIXED_HEADER_SIZE;	// ��ȡ���ɱ䱨ͷ��+����Ч�غɡ�����

  // ���ù̶���ͷ(�̶�ͷ�е�ʣ�೤��ʹ�ñ䳤�ȱ��뷽����������ο�MQTTЭ���ֲ�)
  //----------------------------------------------------------------------------------------------------------
  if(remaining_length > 127)	// ���ɱ䱨ͷ��+����Ч�غɡ����ȴ���127��ʣ�೤��ռ2�ֽ�
  {
    connection->buffer[0] = ((type&0x0f)<<4)|((dup&1)<<3)|((qos&3)<<1)|(retain&1);	// �̶�ͷ�����ֽڸ�ֵ

    connection->buffer[1] = 0x80 | (remaining_length % 128);	// ʣ�೤�ȵĵ�һ���ֽ�  ���λ�̶�1����
    connection->buffer[2] = remaining_length / 128;				// ʣ�೤�ȵĵڶ����ֽ�

    connection->message.length = remaining_length + 3;			// ���ĵ���������
    connection->message.data = connection->buffer;				// MQTT����ָ�� -> ��վ���Ļ������׵�ַ
  }
  else	//if(remaining_length<= 127) // ʣ�೤��ռ1�ֽ�
  {
	// 			buffer[0] = ��
    connection->buffer[1] = ((type&0x0f)<<4)|((dup&1)<<3)|((qos&3)<<1)|(retain&1);	// �̶�ͷ�����ֽڸ�ֵ

    connection->buffer[2] = remaining_length;			// �̶�ͷ�е�[ʣ�೤��](�ɱ䱨ͷ+��������)

    connection->message.length = remaining_length + 2;	// ���ĵ���������  ���2���ǹ̶�+ʣ�೤��
    connection->message.data = connection->buffer + 1;	// MQTT����ָ�� -> ��վ���Ļ������׵�ַ+1
  }

  return &connection->message;		// ���ر����׵�ַ���������ݡ��������峤�ȡ�
}
//==============================================================================================================================

// ��ʼ��MQTT���Ļ�����
// mqtt_msg_init(&client->mqtt_state.mqtt_connection, client->mqtt_state.out_buffer, client->mqtt_state.out_buffer_length);
//==========================================================================================================================
void ICACHE_FLASH_ATTR mqtt_msg_init(mqtt_connection_t* connection, uint8_t* buffer, uint16_t buffer_length)
{
  memset(connection, 0, sizeof(mqtt_connection_t));	// mqttClient->mqtt_state.mqtt_connection = 0
  connection->buffer = buffer;							// buffer = (uint8_t *)os_zalloc(1024)			��������ָ�롿
  connection->buffer_length = buffer_length;			// buffer_length = 1024							�����������ȡ�
}
//==========================================================================================================================


// ������յ������������У����ĵ�ʵ�ʳ���(ͨ����ʣ�೤�ȡ��õ�)
//===============================================================================================
int ICACHE_FLASH_ATTR mqtt_get_total_length(uint8_t* buffer, uint16_t length)
{
  int i;
  int totlen = 0;	// �����ܳ���(�ֽ���)

  // ����ʣ�೤�ȵ�ֵ
  //--------------------------------------------------------------------
  for(i = 1; i <length; ++i)	// ������ʣ�೤�ȡ��ֶ�(��buffer[1]��ʼ)
  {
	  totlen += (buffer[i]&0x7f)<<(7*(i-1));

	  // ���ʣ�೤�ȵĵ�ǰ�ֽڵ�ֵ<0x80�����ʾ���ֽ��������ֽ�
	  //-----------------------------------------------------------------------
	  if((buffer[i]&0x80) == 0)		// ��ǰ�ֽڵ�ֵ<0x80
	  {
		  ++i;		// i == �̶���ͷ���� == ��������(1�ֽ�) + ʣ�೤���ֶ�

		  break;	// ����ѭ��
	  }
  }

  totlen += i;		// �����ܳ��� = �̶���ͷ���� + ʣ�೤�ȵ�ֵ(���ɱ䱨ͷ��+����Ч�غɡ��ĳ���)

  return totlen;	// ���ر����ܳ���
}
//===============================================================================================


// ��ȡ��PUBLISH�������е�������(ָ��)������������
//=========================================================================================
const char* ICACHE_FLASH_ATTR mqtt_get_publish_topic(uint8_t* buffer, uint16_t* length)
{
	int i;
	int totlen = 0;
	int topiclen;

	// ����ʣ�೤�ȵ�ֵ
	//--------------------------------------------------------------------
	for(i = 1; i<*length; ++i)	// ������ʣ�೤�ȡ��ֶ�(��buffer[1]��ʼ)
	{
		totlen += (buffer[i]&0x7f)<<(7*(i-1));

		// ���ʣ�೤�ȵĵ�ǰ�ֽڵ�ֵ<0x80�����ʾ���ֽ��������ֽ�
		//-----------------------------------------------------------------
		if((buffer[i] & 0x80) == 0)		// ��ǰ�ֽڵ�ֵ<0x80
		{
			++i;	// i == �̶���ͷ���� == ��������(1�ֽ�) + ʣ�೤���ֶ�

			break;	// ����ѭ��
		}
	}

	totlen += i;	// �����ܳ��ȣ�����ɾ��û���ã�


	if(i + 2 >= *length)	// ���û���غɣ��򷵻�NULL
		return NULL;

	topiclen = buffer[i++] << 8;	// ��ȡ����������(2�ֽ�)
	topiclen |= buffer[i++];		// ǰ׺

	if(i + topiclen > *length)	// ���ĳ���(û��������)������NULL
		return NULL;

	*length = topiclen;		// ��������������

	return (const char*)(buffer+i);	// ����������(ָ��)
}
//=========================================================================================


// ��ȡ��PUBLISH�����ĵ��غ�(ָ��)���غɳ���
//========================================================================================================
const char* ICACHE_FLASH_ATTR mqtt_get_publish_data(uint8_t* buffer, uint16_t* length)
{
	int i;
	int totlen = 0;
	int topiclen;
	int blength = *length;
	*length = 0;

	// ����ʣ�೤�ȵ�ֵ
	//--------------------------------------------------------------------
	for(i = 1; i<blength; ++i)	// ������ʣ�೤�ȡ��ֶ�(��buffer[1]��ʼ)
	{
		totlen += (buffer[i]&0x7f)<<(7*(i-1));

		// ���ʣ�೤�ȵĵ�ǰ�ֽڵ�ֵ<0x80�����ʾ���ֽ��������ֽ�
		//-----------------------------------------------------------------
		if((buffer[i] & 0x80) == 0)
		{
			++i;	// i == �̶���ͷ���� == ��������(1�ֽ�) + ʣ�೤���ֶ�

			break;	// ����ѭ��
		}
	}

	totlen += i;	// �����ܳ��� = ʣ�೤�ȱ�ʾ��ֵ + �̶�ͷ��ռ�ֽ�


	if(i + 2 >= blength)	// ���û���غɣ��򷵻�NULL
		return NULL;

	topiclen = buffer[i++] << 8;	// ��ȡ����������(2�ֽ�)
	topiclen |= buffer[i++];		// ǰ׺

	if(i+topiclen >= blength)		// ���ĳ���(û��������)������NULL
		return NULL;

	i += topiclen;	// i = ���̶���ͷ��+��������(����ǰ׺)��

	// Qos > 0
	//----------------------------------------------------------------------------
	if(mqtt_get_qos(buffer)>0)	// ��Qos>0ʱ�������������ֶκ����ǡ����ı�ʶ����
	{
		if(i + 2 >= blength)	// ���Ĵ������غɣ�
			return NULL;

		i += 2;			// i = ���̶���ͷ��+���ɱ䱨ͷ��
	}

	if(totlen < i)		// ���Ĵ��󣬷���NULL
		return NULL;

	if(totlen <= blength)		// �����ܳ��� <= ����������ݳ���
		*length = totlen - i;	// ����Ч�غɡ����� = ���ĳ���- (���̶���ͷ������+���ɱ䱨ͷ������)

	else						// �����ܳ��� > ����������ݳ��ȡ���ʧ����/δ������ϡ�
		*length = blength - i;	// ��Ч�غɳ��� = ����������ݳ��� - (���̶���ͷ������+���ɱ䱨ͷ������)

	return (const char*)(buffer + i);	// ������Ч�غ��׵�ַ
}
//========================================================================================================


// ��ȡ�����еġ����ı�ʶ����
//=========================================================================================
uint16_t ICACHE_FLASH_ATTR mqtt_get_id(uint8_t* buffer, uint16_t length)
{
  if(length < 1)	// ������Ч
  return 0;

  // �ж�Ŀ�걨�ĵ�����
  //----------------------------
  switch(mqtt_get_type(buffer))
  {
  	// ��PUBLISH������
	//��������������������������������������������������������������������������������
    case MQTT_MSG_TYPE_PUBLISH:
    {
      int i;
      int topiclen;

      for(i = 1; i < length; ++i)		// ���ҡ���Ч�غɡ��׵�ַ
      {
        if((buffer[i] & 0x80) == 0)		// �жϵ�ǰ�ֽ��Ƿ�Ϊ��ʣ�೤�ȡ���β�ֽ�
        {
          ++i;		// ָ����Ч�غ��׵�ַ
          break;
        }
      }

      if(i + 2 >= length)	// ���Ĵ�������������
        return 0;

      topiclen = buffer[i++] << 8;	// ����������
      topiclen |= buffer[i++];

      if(i + topiclen >= length)	// ���Ĵ���
        return 0;

      i += topiclen;				// ָ�򡾱��ı�ʶ����/����Ч�غɡ�

      // Qos > 0
      //----------------------------------------------------------------------------
      if(mqtt_get_qos(buffer) > 0)	// ��Qos>0ʱ�������������ֶκ����ǡ����ı�ʶ����
      {
        if(i + 2 >= length)		// ���Ĵ������غɣ�
          return 0;

        //i += 2;				// &buffer[i]ָ�򡾱��ı�ʶ����
      }

      else 	// Qos == 0
      {
    	  return 0;		// ��Qos==0ʱ��û�С����ı�ʶ����
      }

      return (buffer[i] << 8) | buffer[i + 1];		//��ȡ�����ı�ʶ����
	}
    //��������������������������������������������������������������������������������


    // �������͵ı��ģ������ı�ʶ�����̶��ǿ��Ʊ��ĵġ���2����3�ֽڡ�
    //--------------------------------------------------------------------------------
    case MQTT_MSG_TYPE_PUBACK:		// ��PUBACK������
    case MQTT_MSG_TYPE_PUBREC:		// ��PUBREC������
    case MQTT_MSG_TYPE_PUBREL:		// ��PUBREL������
    case MQTT_MSG_TYPE_PUBCOMP:		// ��PUBCOMP������
    case MQTT_MSG_TYPE_SUBACK:		// ��SUBACK������
    case MQTT_MSG_TYPE_UNSUBACK:	// ��UNSUBACK������
    case MQTT_MSG_TYPE_SUBSCRIBE:	// ��SUBSCRIBE������
    {
      // This requires the remaining length to be encoded in 1 byte,
      // which it should be.
      if(length >= 4 && (buffer[1] & 0x80) == 0)	// �жϡ��̶�ͷ���ɱ�ͷ���Ƿ���ȷ
        return (buffer[2] << 8) | buffer[3];		// ���ء����ı�ʶ����
      else
        return 0;
    }
    //--------------------------------------------------------------------------------


    // ��CONNECT����CONNACK����UNSUBSCRIBE����PINGREQ����PINGRESP����DISCONNECT��
    //---------------------------------------------------------------------------
    default:
      return 0;
  }
}
//=========================================================================================


// ���á�CONNECT�����Ʊ���
// mqtt_msg_connect(&client->mqtt_state.mqtt_connection, client->mqtt_state.connect_info)
//================================================================================================================================
mqtt_message_t* ICACHE_FLASH_ATTR mqtt_msg_connect(mqtt_connection_t* connection, mqtt_connect_info_t* info)
{
  struct mqtt_connect_variable_header* variable_header;		// ��CONNECT�����ĵġ��ɱ䱨ͷ��ָ��

  init_message(connection);		// ���ñ��ĳ��� = 3(��ʱ��Ϊ���̶���ͷ������(3)��֮����ӡ��ɱ䱨ͷ��������Ч�غɡ�)

  // �ж���Ϣ�����Ƿ񳬹�����������						// ��ע��[message.length]��ָTCP���������MQTT���ĳ��ȡ�
  //------------------------------------------------------------------------------------------------------------
  if(connection->message.length + sizeof(*variable_header) > connection->buffer_length)		// �ж�MQTT���ĳ���
    return fail_message(connection);

  // �����˶ԡ��̶���ͷ���ĸ�ֵ��ֻΪ���̶���ͷ��������3���ֽڵĿռ䡣	ע��ʣ�೤�����ռ���ֽڡ�

  // ��ȡ���ɱ䱨ͷ��ָ�룬�����±��ĳ���
  //----------------------------------------------------------------------------------------------------------------------------
  variable_header = (void*)(connection->buffer + connection->message.length);	// ���ɱ䱨ͷ��ָ�� = ���Ļ�����ָ��+3(�̶���ͷ)
  connection->message.length += sizeof(*variable_header);						// ���ĳ��� == �̶���ͷ + �ɱ䱨ͷ


  // Э������Э�鼶��ֵ
  //-----------------------------------------------
  variable_header->lengthMsb = 0;	// lengthMsb
#if defined(PROTOCOL_NAMEv31)
  variable_header->lengthLsb = 6;	// lengthLsb
  memcpy(variable_header->magic, "MQIsdp", 6);
  variable_header->version = 3;		// v31�汾 = 3

#elif defined(PROTOCOL_NAMEv311)
  variable_header->lengthLsb = 4;	// lengthLsb
  memcpy(variable_header->magic, "MQTT", 4);
  variable_header->version = 4;		// v311�汾 = 4
#else
#error "Please define protocol name"
#endif


  //----------------------------------------------------------------------
  variable_header->flags = 0;	// ���ӱ�־�ֽ� = 0����ʱ��0�����ḳֵ��

  // ��������ʱ����ֵ
  //----------------------------------------------------------------------
  variable_header->keepaliveMsb = info->keepalive >> 8;		// ��ֵ���ֽ�
  variable_header->keepaliveLsb = info->keepalive & 0xff;	// ��ֵ���ֽ�


  // clean_session = 1���ͻ��˺ͷ���˱��붪��֮ǰ���κλỰ����ʼһ���µĻỰ
  //----------------------------------------------------------------------------
  if(info->clean_session)
    variable_header->flags |= MQTT_CONNECT_FLAG_CLEAN_SESSION; //clean_session=1


  // �ж��Ƿ����[client_id]������������[client_id]�ֶ�
  //----------------------------------------------------------------------------
  if(info->client_id != NULL && info->client_id[0] != '\0')
  {
	// ��[client_id]�ֶ���ӵ����Ļ����������ĳ���+=[client_id]��ռ����
	//--------------------------------------------------------------------------
	if(append_string(connection, info->client_id, strlen(info->client_id)) < 0)
	return fail_message(connection);
  }
  else
    return fail_message(connection);	// ���ĳ���

  // �ж��Ƿ����[will_topic]
  //---------------------------------------------------------------------------------
  if(info->will_topic != NULL && info->will_topic[0] != '\0')
  {
	// ��[will_topic]�ֶ���ӵ����Ļ����������ĳ���+=[will_topic]��ռ����
	//--------------------------------------------------------------------------
	if(append_string(connection, info->will_topic,strlen(info->will_topic))<0)
    return fail_message(connection);

	// ��[will_message]�ֶ���ӵ����Ļ����������ĳ���+=[will_message]��ռ����
	//----------------------------------------------------------------------------
	if(append_string(connection,info->will_message,strlen(info->will_message))<0)
    return fail_message(connection);

	// ���á�CONNECT�������е�Will��־λ��[Will Flag]��[Will QoS]��[Will Retain]
	//--------------------------------------------------------------------------
	variable_header->flags |= MQTT_CONNECT_FLAG_WILL;		// ������־λ = 1
	if(info->will_retain)
    variable_header->flags |= MQTT_CONNECT_FLAG_WILL_RETAIN;// WILL_RETAIN = 1
	variable_header->flags |= (info->will_qos & 3) << 3;	// will������ֵ
  }

  // �ж��Ƿ����[username]
  //----------------------------------------------------------------------------
  if(info->username != NULL && info->username[0] != '\0')
  {
	// ��[username]�ֶ���ӵ����Ļ����������ĳ���+=[username]��ռ����
	//--------------------------------------------------------------------------
    if(append_string(connection, info->username, strlen(info->username)) < 0)
      return fail_message(connection);

    // ���á�CONNECT�������е�[username]��־λ
    //--------------------------------------------------------------------------
    variable_header->flags |= MQTT_CONNECT_FLAG_USERNAME;	// username = 1
  }

  // �ж��Ƿ����[password]
  //----------------------------------------------------------------------------
  if(info->password != NULL && info->password[0] != '\0')
  {
	// ��[password]�ֶ���ӵ����Ļ����������ĳ���+=[password]��ռ����
	//--------------------------------------------------------------------------
    if(append_string(connection, info->password, strlen(info->password)) < 0)
      return fail_message(connection);

    // ���á�CONNECT�������е�[password]��־λ
    //--------------------------------------------------------------------------
    variable_header->flags |= MQTT_CONNECT_FLAG_PASSWORD;	// password = 1
  }

  // ���á�CONNECT�����Ĺ̶�ͷ������[Bit7��4=1]��[Bit3=0]��[Bit2��1=0]��[Bit1=0]
  //----------------------------------------------------------------------------
  return fini_message(connection, MQTT_MSG_TYPE_CONNECT, 0, 0, 0);
}
//================================================================================================================================

// ���á�PUBLISH�����ģ�����ȡ��PUBLISH������[ָ��]��[����]
// ������2�������� / ����3��������Ϣ����Ч�غ� / ����4����Ч�غɳ��� / ����5������Qos / ����6��Retain / ����7�����ı�ʶ��ָ�롿
//===================================================================================================================================================
mqtt_message_t* ICACHE_FLASH_ATTR
mqtt_msg_publish(mqtt_connection_t* connection, const char* topic, const char* data, int data_length, int qos, int retain, uint16_t* message_id)
{
  init_message(connection);		// ���ñ��ĳ��� = 3

  if(topic == NULL || topic[0] == '\0')		// û��"topic"�����
    return fail_message(connection);

  if(append_string(connection, topic, strlen(topic)) < 0)	// ��ӡ����⡿�ֶ�
    return fail_message(connection);

  if(qos > 0)	// ����QoS>0������Ҫ�����ı�ʶ����
  {
    if((*message_id = append_message_id(connection, 0)) == 0)	// ��ӡ����ı�ʶ�����ֶ�
      return fail_message(connection);
  }
  else	// if(qos = 0)	// ����QoS=0��������Ҫ�����ı�ʶ����
    *message_id = 0;

  // �жϱ��ĳ����Ƿ���ڻ�����
  //----------------------------------------------------------------------------
  if(connection->message.length + data_length > connection->buffer_length)
    return fail_message(connection);

  memcpy(connection->buffer + connection->message.length, data, data_length);	// ��ӡ���Ч�غɡ��ֶ�
  connection->message.length += data_length;	// ���ñ��ĳ���


  // ���á�PUBLISH�����Ĺ̶�ͷ������[Bit7��4=1]��[Bit3=0]��[Bit2��1=0]��[Bit1=0]
  //----------------------------------------------------------------------------
  return fini_message(connection, MQTT_MSG_TYPE_PUBLISH, 0, qos, retain);
}
//===================================================================================================================================================

// ���á�PUBACK������
//=====================================================================================================
mqtt_message_t* ICACHE_FLASH_ATTR mqtt_msg_puback(mqtt_connection_t* connection, uint16_t message_id)
{
  init_message(connection);		// ���ñ��ĳ��� = 3

  if(append_message_id(connection, message_id) == 0)	// ��ӡ����ı�ʶ�����ֶ�
    return fail_message(connection);

  return fini_message(connection, MQTT_MSG_TYPE_PUBACK, 0, 0, 0);	// ���ù̶���ͷ
}
//=====================================================================================================


// ���á�PUBREC������
//=====================================================================================================
mqtt_message_t* ICACHE_FLASH_ATTR mqtt_msg_pubrec(mqtt_connection_t* connection, uint16_t message_id)
{
  init_message(connection);		// ���ñ��ĳ��� = 3

  if(append_message_id(connection, message_id) == 0)	// ��ӡ����ı�ʶ�����ֶ�
    return fail_message(connection);

  return fini_message(connection, MQTT_MSG_TYPE_PUBREC, 0, 0, 0);	// ���ù̶���ͷ
}
//=====================================================================================================


// ���á�PUBREL������
//=====================================================================================================
mqtt_message_t* ICACHE_FLASH_ATTR mqtt_msg_pubrel(mqtt_connection_t* connection, uint16_t message_id)
{
  init_message(connection);		// ���ñ��ĳ��� = 3

  if(append_message_id(connection, message_id) == 0)	// ��ӡ����ı�ʶ�����ֶ�
    return fail_message(connection);

  return fini_message(connection, MQTT_MSG_TYPE_PUBREL, 0, 1, 0);	// ���ù̶���ͷ
}
//=====================================================================================================


// ���á�PUBCOMP������
//=====================================================================================================
mqtt_message_t* ICACHE_FLASH_ATTR mqtt_msg_pubcomp(mqtt_connection_t* connection, uint16_t message_id)
{
  init_message(connection);		// ���ñ��ĳ��� = 3

  if(append_message_id(connection, message_id) == 0)	// ��ӡ����ı�ʶ�����ֶ�
    return fail_message(connection);

  return fini_message(connection, MQTT_MSG_TYPE_PUBCOMP, 0, 0, 0);	// ���ù̶���ͷ
}
//=====================================================================================================


// ���á�SUBSCRIBE�����ģ�������2���������� / ����3���������� / ����4�����ı�ʶ����
//=======================================================================================================================================
mqtt_message_t* ICACHE_FLASH_ATTR mqtt_msg_subscribe(mqtt_connection_t* connection, const char* topic, int qos, uint16_t* message_id)
{
  init_message(connection);		// ���ĳ��� = 3(�̶�ͷ)

  if(topic == NULL || topic[0] == '\0')		// ������Ч
    return fail_message(connection);

  if((*message_id = append_message_id(connection, 0)) == 0)	// ���[���ı�ʶ��]
    return fail_message(connection);

  if(append_string(connection, topic, strlen(topic)) < 0)	// ���[����]
    return fail_message(connection);

  if(connection->message.length + 1 > connection->buffer_length)// �жϱ��ĳ���
    return fail_message(connection);

  connection->buffer[connection->message.length++] = qos;	// ������Ϣ����QoS


  // ���á�SUBSCRIBE�����ĵĹ̶���ͷ
  //-----------------------------------------------------------------
  return fini_message(connection, MQTT_MSG_TYPE_SUBSCRIBE, 0, 1, 0);
}
//=======================================================================================================================================


// ���á�UNSUBSCRIBE�����ģ�������2��ȡ���������� /  ����3�����ı�ʶ����
//===============================================================================================================================
mqtt_message_t* ICACHE_FLASH_ATTR mqtt_msg_unsubscribe(mqtt_connection_t* connection, const char* topic, uint16_t* message_id)
{
	init_message(connection);		// ���ĳ��� = 3(�̶�ͷ)

	if(topic == NULL || topic[0] == '\0')		// ������Ч
		return fail_message(connection);

	if((*message_id = append_message_id(connection, 0)) == 0)		// ���[���ı�ʶ��]
		return fail_message(connection);

	if(append_string(connection, topic, strlen(topic)) < 0)		// ���[����]
		return fail_message(connection);

	// ���á�UNSUBSCRIBE�����ĵĹ̶���ͷ
	//-----------------------------------------------------------------
	return fini_message(connection, MQTT_MSG_TYPE_UNSUBSCRIBE, 0, 1, 0);
}
//===============================================================================================================================


// ���á�PINGREQ������
//=================================================================================
mqtt_message_t* ICACHE_FLASH_ATTR mqtt_msg_pingreq(mqtt_connection_t* connection)
{
  init_message(connection);
  return fini_message(connection, MQTT_MSG_TYPE_PINGREQ, 0, 0, 0);
}
//=================================================================================


// ���á�PINGRESP������
//=================================================================================
mqtt_message_t* ICACHE_FLASH_ATTR mqtt_msg_pingresp(mqtt_connection_t* connection)
{
  init_message(connection);
  return fini_message(connection, MQTT_MSG_TYPE_PINGRESP, 0, 0, 0);
}
//=================================================================================

// ���á�DISCONNECT������
//====================================================================================
mqtt_message_t* ICACHE_FLASH_ATTR mqtt_msg_disconnect(mqtt_connection_t* connection)
{
  init_message(connection);
  return fini_message(connection, MQTT_MSG_TYPE_DISCONNECT, 0, 0, 0);
}
//====================================================================================
