#include "stm32f10x.h"
#include "stm32f10x_adc.h"
#include "adc.h"

void Adc_GPIO_Config(void)
{
  GPIO_InitTypeDef GPIO_InitStructure;
	/*ʹ��GPIO��ADC1ͨ��ʱ��*/
	RCC_APB2PeriphClockCmd(RCC_APB2Periph_GPIOA |RCC_APB2Periph_ADC1	, ENABLE );	  
 
	/*��PA0����Ϊģ������*/                         
	GPIO_InitStructure.GPIO_Pin = GPIO_Pin_0;
	/*��GPIO����Ϊģ������*/
	GPIO_InitStructure.GPIO_Mode = GPIO_Mode_AIN;		
	GPIO_Init(GPIOA, &GPIO_InitStructure);	
}


void  Adc_Config(void)
{ 	
	ADC_InitTypeDef ADC_InitStructure; 
	Adc_GPIO_Config();	
  /*72M/6=12,ADC���ʱ�䲻�ܳ���14M*/
	RCC_ADCCLKConfig(RCC_PCLK2_Div6);  
	/*������ ADC1 ��ȫ���Ĵ�������ΪĬ��ֵ*/
	ADC_DeInit(ADC1); 
  /*ADC����ģʽ:ADC1��ADC2�����ڶ���ģʽ*/
	ADC_InitStructure.ADC_Mode = ADC_Mode_Independent;	
	/*ģ��ת�������ڵ�ͨ��ģʽ*/
	ADC_InitStructure.ADC_ScanConvMode = DISABLE;	
	/*ģ��ת�������ڵ���ת��ģʽ*/
	ADC_InitStructure.ADC_ContinuousConvMode = DISABLE;
  /*ADCת��������������ⲿ��������*/	
	ADC_InitStructure.ADC_ExternalTrigConv = ADC_ExternalTrigConv_None;	
	/*ADC�����Ҷ���*/
	ADC_InitStructure.ADC_DataAlign = ADC_DataAlign_Right;	
	/*˳����й���ת����ADCͨ������Ŀ*/
	ADC_InitStructure.ADC_NbrOfChannel = 1;	
	/*����ADC_InitStruct��ָ���Ĳ�����ʼ������ADCx�ļĴ���*/   
	ADC_Init(ADC1, &ADC_InitStructure);	
 
  /*ʹ��ָ����ADC1*/
	ADC_Cmd(ADC1, ENABLE);	
	/*����ָ����ADC1��У׼�Ĵ���*/
	ADC_ResetCalibration(ADC1);	
	/*��ȡADC1����У׼�Ĵ�����״̬,����״̬��ȴ�*/
	while(ADC_GetResetCalibrationStatus(ADC1));
	/*��ʼָ��ADC1��У׼*/
	ADC_StartCalibration(ADC1);		
  /*��ȡָ��ADC1��У׼����,����״̬��ȴ�*/
	while(ADC_GetCalibrationStatus(ADC1));		
  /*ʹ��ָ����ADC1�����ת����������*/
	ADC_SoftwareStartConvCmd(ADC1, ENABLE);		
}	


#define  RL 10000.00   //��ѹ������ֹ
/*
1.STM32F103ZET6  ADC 12λ 4095
2.�ο���ѹ3300mV
3.ADC��ѹ = (3300/4095)*ADCֵ
4.ADC��ѹ = (0.8058608058608059)*ADCֵ
5.MQ_2��ֵ/RL= (5000-ADC��ѹ)/ADC��ѹ
6.MQ_2��ֵ = 5000-ADC��ѹ)/ADC��ѹ �� RL
*/
float GetMQ_2Res(void)
{
  int i;
	float res;
	float temp;
	float voltage;

	//���30�ε�ƽ��ֵ
	temp = 0;
	for(i = 0;i<30;i++)
	{
	/*ADC1,ADCͨ��3,�������˳��ֵΪ1,����ʱ��Ϊ239.5����*/	
	 ADC_RegularChannelConfig(ADC1, ADC_Channel_0, 1, ADC_SampleTime_239Cycles5 );	  			    
  /*ʹ��ָ����ADC1�����ת����������*/
	 ADC_SoftwareStartConvCmd(ADC1, ENABLE);			
	 /*�ȴ�ת������*/
	 while(!ADC_GetFlagStatus(ADC1, ADC_FLAG_EOC ));
	 /*�������һ��ADC1�������ת�����*/
	 temp += ADC_GetConversionValue(ADC1);			
	}
	temp = temp/30;
	
	 voltage = (0.8058608058608059)*temp;
	
	 temp = (5000.00-voltage)/voltage*RL;
	
	 res = temp;
	 
	 return(res);	
}


