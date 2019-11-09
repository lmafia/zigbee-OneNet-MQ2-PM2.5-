#include "stm32f10x.h"
#include "LED.h"
#include "LEDDisplay.h"
#include "adc.h"
#include "math.h"
#include "stm32f10x_tim.h"
#include "misc.h"
#include "usart1.h"
#include "string.h"
void delay(unsigned int count);
void sendStr(const char* str);
/*
 * ��������TIM2_NVIC_Configuration
 * ����  ��TIM2�ж����ȼ�����
 * ����  ����
 * ���  ����	
 */
void TIME_NVIC_Configuration(void)
{
	NVIC_InitTypeDef NVIC_InitStructure;
	/*����NVIC�жϷ���2:2λ��ռ���ȼ���2λ��Ӧ���ȼ�*/
	NVIC_PriorityGroupConfig(NVIC_PriorityGroup_2);	
	/*TIM3�ж�*/
  NVIC_InitStructure.NVIC_IRQChannel = TIM3_IRQn; 
  /*��ռ���ȼ�0��*/	
	NVIC_InitStructure.NVIC_IRQChannelPreemptionPriority = 0; 
  /*�����ȼ�3��*/	
	NVIC_InitStructure.NVIC_IRQChannelSubPriority = 3; 
  /*IRQͨ����ʹ��*/	
	NVIC_InitStructure.NVIC_IRQChannelCmd = ENABLE; 
	/*����NVIC_InitStruct��ָ���Ĳ�����ʼ������NVIC�Ĵ���*/
	NVIC_Init(&NVIC_InitStructure);  
	
}


void TIME_Configuration(void)
{
  TIM_TimeBaseInitTypeDef  TIM_TimeBaseStructure;
	/*��ʱ��TIM3ʱ��ʹ��*/
  RCC_APB1PeriphClockCmd(RCC_APB1Periph_TIM3, ENABLE); 
  /*��������һ�������¼�װ�����Զ���װ�ؼĴ������ڵ�ֵ,������1Ϊ0.1ms 10KHz*/
	TIM_TimeBaseStructure.TIM_Period = 1; 
	/*����������ΪTIMxʱ��Ƶ�ʳ�����Ԥ��Ƶֵ*/
	TIM_TimeBaseStructure.TIM_Prescaler =(7200-1);
  /*����ʱ�ӷָ�:TDTS = Tck_tim*/  
	TIM_TimeBaseStructure.TIM_ClockDivision = 0; 
	/*TIM���ϼ���ģʽ*/
	TIM_TimeBaseStructure.TIM_CounterMode = TIM_CounterMode_Up; 
  /*����TIM_TimeBaseInitStruct��ָ���Ĳ�����ʼ��TIMx��ʱ�������λ*/
	TIM_TimeBaseInit(TIM3, &TIM_TimeBaseStructure); 
  /*ʹ�ܻ���ʧ��ָ����TIM�ж�*/
	TIM_ITConfig(TIM3,TIM_IT_Update|TIM_IT_Trigger,ENABLE);

}

#define CAL_PPM 20   //У׼������ú����Ũ�� PPMֵ
// ������У׼����
float MQ2_PPM_Calibration(float RS_1)
{
		float R0_1; 
	
    R0_1 = RS_1 / pow(CAL_PPM / 613.9f, 1 / -2.074f);
	  return(R0_1);
}
/*
ú��Ũ�ȼ��㹫ʽ���£�
Rs/R0  1.651428   1.437143   1.257143   1.137143   1      0.574393   0.415581   0.305119   0.254795
ppm    200        300        400        500        600    2000       4000       7000       10000
�������϶�Ӧֵ�������Rs/R0��ppm�ļ��㹫ʽ�����£�ʹ��Excel���ɵĹ�ʽ����
ppm = 613.9*pow(RS/R0,-2.074)
ppm��Ϊ��ȼ�����Ũ�ȡ�
Rs�������ڲ�ͬ���壬��ͬŨ���µĵ���ֵ��
R0�������ڽྻ�����еĵ���ֵ��
*/	



#define   ALARM_VALUE    100.00

float R0;
float Res;
float ppm;
float temp;
int main(void)
{
	int k;
	char data[20] = "";
	SystemInit();	//����ϵͳʱ��Ϊ 72M 
	Adc_GPIO_Config();
	Adc_Config();
	TIME_NVIC_Configuration();
	LEDDisplayConfig();
    GPIO_ResetBits(GPIOE,GPIO_Pin_15);
	USART2_Config()  ;
	//�ȴ�30S���ϵȴ������ȶ���,ʱ��Խ������ȡŨ��Խ�ȶ�
	for(k = 0;k<10;k++)
	{
		delay(0x4fffff);	
	  temp	= k;	
		temp = temp/100.0
		;		LEDDisplay(temp);	
	}
	
 
	//��ȡ�ྻ��������ֵ
	//
	Res = GetMQ_2Res();		
	R0 = MQ2_PPM_Calibration(Res);
	

	while(1)
	{	
	   delay(0x4fffff);	
		
    //����ppm	
		Res = GetMQ_2Res();
		ppm = 613.9*pow(Res/R0,-2.074);
		sprintf(data, "SMOKE:%.2f\r\n",ppm);
	    sendStr(data);

		if(ppm > 100 && ppm <=500)
			sendStr("LEVEL:1\r\n");
		else if (ppm >500 && ppm <=1000 )
			sendStr("LEVEL:2\r\n");
		else if (ppm>1000)
			sendStr("LEVEL:3\r\n");
		else
			sendStr("LEVEL:0\r\n");
//		LEDDisplay(ppm);	
//		if( ppm>= ALARM_VALUE)
//		{

//			GPIO_SetBits(GPIOE,GPIO_Pin_15);
//		}
//		else
//		{
//			
//			GPIO_ResetBits(GPIOE,GPIO_Pin_15);
//		}
		
	}
}



void delay(unsigned int count)
{
int i;
for(i=0;i<count;i++);
}

void sendStr(const char* str)
{
	while(*str != 0)
	{
		UART2SendByte(*str);
		str++;
	}

}
