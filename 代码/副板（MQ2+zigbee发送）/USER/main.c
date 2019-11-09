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
 * 函数名：TIM2_NVIC_Configuration
 * 描述  ：TIM2中断优先级配置
 * 输入  ：无
 * 输出  ：无	
 */
void TIME_NVIC_Configuration(void)
{
	NVIC_InitTypeDef NVIC_InitStructure;
	/*设置NVIC中断分组2:2位抢占优先级，2位响应优先级*/
	NVIC_PriorityGroupConfig(NVIC_PriorityGroup_2);	
	/*TIM3中断*/
  NVIC_InitStructure.NVIC_IRQChannel = TIM3_IRQn; 
  /*先占优先级0级*/	
	NVIC_InitStructure.NVIC_IRQChannelPreemptionPriority = 0; 
  /*从优先级3级*/	
	NVIC_InitStructure.NVIC_IRQChannelSubPriority = 3; 
  /*IRQ通道被使能*/	
	NVIC_InitStructure.NVIC_IRQChannelCmd = ENABLE; 
	/*根据NVIC_InitStruct中指定的参数初始化外设NVIC寄存器*/
	NVIC_Init(&NVIC_InitStructure);  
	
}


void TIME_Configuration(void)
{
  TIM_TimeBaseInitTypeDef  TIM_TimeBaseStructure;
	/*定时器TIM3时钟使能*/
  RCC_APB1PeriphClockCmd(RCC_APB1Periph_TIM3, ENABLE); 
  /*设置在下一个更新事件装入活动的自动重装载寄存器周期的值,计数到1为0.1ms 10KHz*/
	TIM_TimeBaseStructure.TIM_Period = 1; 
	/*设置用来作为TIMx时钟频率除数的预分频值*/
	TIM_TimeBaseStructure.TIM_Prescaler =(7200-1);
  /*设置时钟分割:TDTS = Tck_tim*/  
	TIM_TimeBaseStructure.TIM_ClockDivision = 0; 
	/*TIM向上计数模式*/
	TIM_TimeBaseStructure.TIM_CounterMode = TIM_CounterMode_Up; 
  /*根据TIM_TimeBaseInitStruct中指定的参数初始化TIMx的时间基数单位*/
	TIM_TimeBaseInit(TIM3, &TIM_TimeBaseStructure); 
  /*使能或者失能指定的TIM中断*/
	TIM_ITConfig(TIM3,TIM_IT_Update|TIM_IT_Trigger,ENABLE);

}

#define CAL_PPM 20   //校准环境中煤气的浓度 PPM值
// 传感器校准函数
float MQ2_PPM_Calibration(float RS_1)
{
		float R0_1; 
	
    R0_1 = RS_1 / pow(CAL_PPM / 613.9f, 1 / -2.074f);
	  return(R0_1);
}
/*
煤气浓度计算公式见下：
Rs/R0  1.651428   1.437143   1.257143   1.137143   1      0.574393   0.415581   0.305119   0.254795
ppm    200        300        400        500        600    2000       4000       7000       10000
根据以上对应值可以求出Rs/R0与ppm的计算公式，如下（使用Excel生成的公式）：
ppm = 613.9*pow(RS/R0,-2.074)
ppm：为可燃气体的浓度。
Rs：器件在不同气体，不同浓度下的电阻值。
R0：器件在洁净空气中的电阻值。
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
	SystemInit();	//配置系统时钟为 72M 
	Adc_GPIO_Config();
	Adc_Config();
	TIME_NVIC_Configuration();
	LEDDisplayConfig();
    GPIO_ResetBits(GPIOE,GPIO_Pin_15);
	USART2_Config()  ;
	//等待30S以上等传感器稳定后,时间越长，读取浓度越稳定
	for(k = 0;k<10;k++)
	{
		delay(0x4fffff);	
	  temp	= k;	
		temp = temp/100.0
		;		LEDDisplay(temp);	
	}
	
 
	//获取洁净空气电阻值
	//
	Res = GetMQ_2Res();		
	R0 = MQ2_PPM_Calibration(Res);
	

	while(1)
	{	
	   delay(0x4fffff);	
		
    //计算ppm	
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
