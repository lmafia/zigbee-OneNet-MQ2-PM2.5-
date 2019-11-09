#ifndef __USER_CONFIG_H__
#define __USER_CONFIG_H__

#define USE_OPTIMIZE_PRINTF		// �Ż���ӡ

#define		Sector_STA_INFO		0x99			// ��STA��������������


typedef struct
{
	uint8_t smoke[10];
	uint8_t temperature[10];
	uint8_t pm25[10];
	uint8_t level[10];
}MQTT_Info;

#define PM25 ",;PM25,%s;"
#define SMOKE ",;SMOKE,%s;"
#define TP ",;TP,%s;"
#define LEVEL ",;LEVEL.%s;"

#endif

