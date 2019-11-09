/*
/* config.c
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
#include "ets_sys.h"
#include "os_type.h"
#include "mem.h"
#include "osapi.h"
#include "user_interface.h"

#include "mqtt.h"
#include "config.h"
#include "user_config.h"
#include "debug.h"


// Flash����
//=========================================================================
// ������Ox79����ϵͳ���������_0
// ������Ox7A����ϵͳ���������_1

// ������Ox7C������0��ϵͳ���������0x79����  !0��ϵͳ���������0x7A������
//=========================================================================


// ȫ�ֱ���
//================================================================================================
SYSCFG sysCfg;			// ϵͳ���û���

SAVE_FLAG saveFlag;		// ����������־	��0��ϵͳ���������0x79����	!0��ϵͳ���������0x7A������
//================================================================================================


// �����º��ϵͳ������¼��Flash��
//==================================================================================================
void ICACHE_FLASH_ATTR CFG_Save()
{
	// ��Flash��0x7C����������ŵ���saveFlag��������֮ǰ�ĳ����˱�ʶ��
	//--------------------------------------------------------------------------------------------
	 spi_flash_read((CFG_LOCATION+3)*SPI_FLASH_SEC_SIZE, (uint32 *)&saveFlag, sizeof(SAVE_FLAG));

	 //���ݡ�����������־������ϵͳ������¼����Ӧ����
	//---------------------------------------------------------------------------------------
	if (saveFlag.flag == 0)		//��0��ϵͳ������0x79������
	{
		spi_flash_erase_sector(CFG_LOCATION + 1);					// ����0x7A����
		spi_flash_write((CFG_LOCATION + 1) * SPI_FLASH_SEC_SIZE,	// ϵͳ����д��0x7A����
						(uint32 *)&sysCfg, sizeof(SYSCFG));

		saveFlag.flag = 1;											// ����������־ = 1
		spi_flash_erase_sector(CFG_LOCATION + 3);					// ����0x7C����
		spi_flash_write((CFG_LOCATION + 3) * SPI_FLASH_SEC_SIZE,	// ����������־д��0x7C
						(uint32 *)&saveFlag, sizeof(SAVE_FLAG));
	}

	else //saveFlag.flag != 0	//��!0��ϵͳ������0x7A������
	{
		spi_flash_erase_sector(CFG_LOCATION + 0);					// ����0x79����
		spi_flash_write((CFG_LOCATION + 0) * SPI_FLASH_SEC_SIZE,	// ϵͳ����д��0x79����
						(uint32 *)&sysCfg, sizeof(SYSCFG));

		saveFlag.flag = 0;											// ����������־ = 0
		spi_flash_erase_sector(CFG_LOCATION + 3);					// ����0x7C����
		spi_flash_write((CFG_LOCATION + 3) * SPI_FLASH_SEC_SIZE,	// ����������־д��0x7C
						(uint32 *)&saveFlag, sizeof(SAVE_FLAG));
	}
	//---------------------------------------------------------------------------------------
}
//==================================================================================================


// ����/����ϵͳ������WIFI������MQTT������(�ɳ����˱�ʶ����)
//===================================================================================================================================
void ICACHE_FLASH_ATTR CFG_Load()
{
	INFO("\r\nload ...\r\n");

	// ��Flash��0x7C����������ŵ���saveFlag��������֮ǰ�ĳ����˱�ʶ��
	//----------------------------------------------------------------------------------------------
	spi_flash_read((CFG_LOCATION+3)*SPI_FLASH_SEC_SIZE,(uint32 *)&saveFlag, sizeof(SAVE_FLAG));


	//���ݡ�����������־������ȡ��Ӧ������ϵͳ������0��ϵͳ������0x79����	!0��ϵͳ������0x7A������
	//-------------------------------------------------------------------------------------------------------------------------
	if (saveFlag.flag == 0)
	{
		spi_flash_read((CFG_LOCATION+0)*SPI_FLASH_SEC_SIZE,	(uint32 *)&sysCfg, sizeof(SYSCFG));		// ����ϵͳ����(1����0x79)
	}
	else //saveFlag.flag != 0
	{
		spi_flash_read((CFG_LOCATION+1)*SPI_FLASH_SEC_SIZE,	(uint32 *)&sysCfg, sizeof(SYSCFG));		// ����ϵͳ����(2����0x7A)
	}


	// ֻ���ڡ������˱�ʶ��֮ǰ��ͬ��������£��Ż����ϵͳ�������޸�ϵͳ����ʱ��һ��Ҫ�ǵ��޸ĳ����˱�ʶ��ֵ��
	//------------------------------------------------------------------------------------------------------------------------
	//if(sysCfg.cfg_holder != CFG_HOLDER)		// �����˱�ʶ��ͬ
	{
		os_memset(&sysCfg, 0x00, sizeof sysCfg);	// ��������=0

		sysCfg.cfg_holder = CFG_HOLDER;		// ���³����˱�ʶ

		os_strcpy(sysCfg.device_id, MQTT_CLIENT_ID);		// ��MQTT_CLIENT_ID��MQTT�ͻ��˱�ʶ��
		sysCfg.device_id[sizeof(sysCfg.device_id) - 1] = '\0';					// �����'\0'����ֹ�ַ����������飬ָ�������

/*
		os_strncpy(sysCfg.sta_ssid, STA_SSID, sizeof(sysCfg.sta_ssid)-1);		// ��STA_SSID��WIFI����
		os_strncpy(sysCfg.sta_pwd, STA_PASS, sizeof(sysCfg.sta_pwd)-1);			// ��STA_PASS��WIFI����
		sysCfg.sta_type = STA_TYPE;												// ��STA_TYPE��WIFI����
*/

		os_strncpy(sysCfg.mqtt_host, MQTT_HOST, sizeof(sysCfg.mqtt_host)-1);	// ��MQTT_HOST��MQTT���������/IP��ַ
		sysCfg.mqtt_port = MQTT_PORT;											// ��MQTT_PORT���������Ӷ˿ں�
		os_strncpy(sysCfg.mqtt_user, MQTT_USER, sizeof(sysCfg.mqtt_user)-1);	// ��MQTT_USER��MQTT�û���
		os_strncpy(sysCfg.mqtt_pass, MQTT_PASS, sizeof(sysCfg.mqtt_pass)-1);	// ��MQTT_PASS��MQTT����

		sysCfg.security = DEFAULT_SECURITY;		/* default non ssl */			// ��DEFAULT_SECURITY��Ĭ�ϰ�ȫ�ȼ�(Ĭ��=0��������)

		sysCfg.mqtt_keepalive = MQTT_KEEPALIVE;		// ��MQTT_KEEPALIVE����������ʱ��(�궨��==120)

		INFO(" default configuration\r\n");

		CFG_Save();		// �����º��ϵͳ������¼��Flash��
	}
}
//===================================================================================================================================
