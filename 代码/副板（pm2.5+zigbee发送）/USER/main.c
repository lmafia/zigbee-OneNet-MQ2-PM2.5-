
#include "string.h"
#include "stm32f10x.h"
#include "usart1.h"
volatile float PMPulseRate = 0;
void delay_ms(unsigned int x);
void sendStr2(const char* str);
/* 
 USART1���ڽ���PM2.5������  ������9600 
 USART2���ڷ������ݵ�zigbee��zigbee������9600 
*/
int main(void)
{  
	u32 i ;  
	char data[20] = "";  
	float pm_num = 0;
	static float pre_rate, ppre_rate;
	SystemInit();	//����ϵͳʱ��Ϊ 72M 
	USART1_Config(); //USART1 ���� 		
    USART2_Config();
   
	while (1)
	{	
		
		delay_ms(300);
//		sprintf(data, "PM25:%.2f\r\n",PMPulseRate);
//		sendStr2(data);
		
		pm_num = (PMPulseRate+pre_rate+ppre_rate) /3/0.05;     //�ҳ�Ũ��
        sprintf(data, "PM25:%.2f\r\n",pm_num);
		USART_ITConfig(USART1,USART_IT_IDLE,DISABLE);
		sendStr2(data);
		USART_ITConfig(USART1,USART_IT_IDLE,ENABLE);
		ppre_rate = pre_rate;
		pre_rate = PMPulseRate;
	}
}

void sendStr2(const char* str)
{
	while(*str != 0)
	{
		UART2SendByte(*str);
		str++;
	}


}


void delay_ms(unsigned int x)
{
 unsigned int i,j;
 for(i=0;i<x;i++)
   for(j=0;j<1220;j++);
}
