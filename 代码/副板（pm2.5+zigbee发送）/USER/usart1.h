#ifndef __USART1_H
#define	__USART1_H

#include "stm32f10x.h"
#include <stdio.h>

void USART1_Config(void);
void UART1Test(void);

void USART2_Config(void);
 void UART2SendByte(unsigned char SendData);
  void UART1SendByte(unsigned char SendData);
void UART2SendByte(unsigned char SendData);
extern volatile float PMPulseRate ;
#endif /* __USART1_H */
