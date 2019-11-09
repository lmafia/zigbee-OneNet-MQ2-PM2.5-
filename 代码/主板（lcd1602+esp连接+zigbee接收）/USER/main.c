#include "delay.h"
#include "sys.h"
#include "usart.h"
#include "led.h"
#include "lcd1602.h"
#include "ds18b20.h"
#include "string.h"
#include "timer.h"
#include "usart3.h"
float pm25;
float smoke;
int level;
char temp[10];

char show_flag;
int main(void)
{
    char* start = 0;
	short  temperature = 0 ;	
	delay_init();
	NVIC_Configuration();
	TIM3_Int_Init(9999,7199); 
	TIM7_Int_Init(99,7199) ;
	uart_init(9600);
	GPIO_Configuration();
	LED_Init();
	usart3_init(9600);
	LCD1602_Init();
 	LCD1602_Show_Str(2, 0, "Starting...");
	delay_ms(100);
	while(DS18B20_Init())	//DS18B20???	
	{
		LCD1602_Show_Str(2,1, "DS18B20 Error   ");
		delay_ms(200);
	}						   
	LCD1602_Show_Str(2,1, "DS18B20 OK!");
	LCD1602_ClearScreen();
	LCD1602_Show_Str(0,0,"SecurityLevel:");
	while(1)
	{
		if(USART3_RX_STA&0x8000)
	{
		printf("\r\n\r\n 收到的信息：%s\r\n",USART3_RX_BUF) ;
		if(strncmp((const char*)USART3_RX_BUF, "PM25:",5) == 0)
			sscanf((const char*)USART3_RX_BUF, "PM25:%f\r\n",&pm25);
		else if(strncmp((const char*)USART3_RX_BUF, "SMOKE",5) == 0)
		{
			sscanf((const char*)USART3_RX_BUF, "SMOKE:%f\r\n",&smoke);
			start = strstr((const char*)USART3_RX_BUF, "L");
			if(strncmp((const char*)start, "LEVEL:",6) == 0)
			{
				sscanf((const char*)start, "LEVEL:%d\r\n",&level);
			}
		}
			

		USART3_RX_STA = 0;
	}		
		LCD1602_Show_Num(15, 0, level);
	switch(show_flag)
	{
		case 0:
	    sprintf(temp, "%.2f", pm25);
		u3_printf("PM25:%.2f\r\n",pm25)  ;
		LCD1602_Show_Str(0, 1, "PM2.5:                    ");
		LCD1602_Show_Str(6, 1, temp);
		break;
		
		case 1:
        LCD1602_Show_Str(0, 1, "                       "); 
		LCD1602_Show_Str(0, 1, "Temperature:              ");
		temperature=DS18B20_Get_Temp();
		u3_printf("TP:%.1f\r\n",(temperature/10.0))  ;			
		LCD1602_Show_Num(12,1,temperature/100%10);
        LCD1602_Write_Dat(temperature/10%10+'0');
		LCD1602_Write_Dat('.');
		LCD1602_Write_Dat(temperature%10+'0');
		break;
		
	    case 2:
		u3_printf("SMOKE:%.2f\r\n",smoke)  ;
        sprintf(temp, "%.2f", smoke);
		LCD1602_Show_Str(0, 1, "                       "); 
		LCD1602_Show_Str(0, 1, "Smoke:              ");
		LCD1602_Show_Str(6, 1, temp);
		u3_printf("LEVEL:%d\r\n",level)  ;
		break;
		
		default:
			break;
	}
	if(level > 0)
		PBout(9) = 1;
	else
		PBout(9) = 0;
	delay_ms(500);
	}
}

