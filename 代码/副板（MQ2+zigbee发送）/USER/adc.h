#ifndef __ADC_H
#define __ADC_H	
#include "stm32f10x.h"
 

/*
#define ADC_CH0  0 //ͨ��0
#define ADC_CH1  1 //ͨ��1
#define ADC_CH2  2 //ͨ��2
#define ADC_CH3  3 //ͨ��3	   
*/
void Adc_GPIO_Config(void); 
void Adc_Config(void);
float GetMQ_2Res(void);
 
#endif 
