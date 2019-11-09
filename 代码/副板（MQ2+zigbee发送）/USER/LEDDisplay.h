#include "stm32f10x.h"

void LEDDisplay_GPIO_Config(void);
void Delay_xms(unsigned int x);
void Write_Max7219_byte(unsigned char DATA);         
void Write_Max7219(unsigned char address,unsigned char dat);
void Init_MAX7219(void);
void LEDDisplayConfig(void);
void LEDDisplay(float value);


