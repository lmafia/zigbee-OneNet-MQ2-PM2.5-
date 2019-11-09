#ifndef __ADC_H
#define __ADC_H	
#include "stm32f10x.h"
 

/*
#define ADC_CH0  0 //通道0
#define ADC_CH1  1 //通道1
#define ADC_CH2  2 //通道2
#define ADC_CH3  3 //通道3	   
*/
void Adc_GPIO_Config(void); 
void Adc_Config(void);
float GetMQ_2Res(void);
 
#endif 
