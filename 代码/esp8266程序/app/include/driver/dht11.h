
#ifndef __DHT11_H
#define __DHT11_H

// ͷ�ļ�����
//==================================================================================
#include "os_type.h"			// os_XXX
#include "osapi.h"  			// os_XXX�������ʱ��
#include "c_types.h"			// ��������
#include "user_interface.h" 	// ϵͳ�ӿڡ�system_param_xxx�ӿڡ�WIFI��RateContro
//==================================================================================


// ȫ�ֱ���
//=====================================================
extern u8 DHT11_Data_Array[6];		// DHT11��������

extern u8 DHT11_Data_Char[2][10];	// DHT11�����ַ���
//=====================================================


// ��������
//==================================================================================================
void ICACHE_FLASH_ATTR Dht11_delay_ms(u32 C_time);			// ������ʱ����

void ICACHE_FLASH_ATTR DHT11_Signal_Output(u8 Value_Vol);	// DHT11�ź���(IO5)���������ƽ

void ICACHE_FLASH_ATTR DHT11_Signal_Input(void);			// DHT11�ź���(IO5) ��Ϊ����

u8 ICACHE_FLASH_ATTR DHT11_Start_Signal_JX(void);			// DHT11�������ʼ�źţ���������Ӧ�ź�

u8 ICACHE_FLASH_ATTR DHT11_Read_Bit(void);					// ��ȡDHT11һλ����

u8 ICACHE_FLASH_ATTR DHT11_Read_Byte(void);				// ��ȡDHT11һ���ֽ�

u8 ICACHE_FLASH_ATTR DHT11_Read_Data_Complete(void);		// �����Ķ�ȡDHT11���ݲ���

void ICACHE_FLASH_ATTR DHT11_NUM_Char(void);				// DHT11����ֵת���ַ���
//==================================================================================================


#endif /* __DHT11_H */
