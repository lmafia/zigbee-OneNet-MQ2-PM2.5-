

#include "usart1.h"
#include <stdarg.h>
#include "misc.h"



void USART1_Config(void)
{
	GPIO_InitTypeDef GPIO_InitStructure;
	USART_InitTypeDef USART_InitStructure;
	NVIC_InitTypeDef NVIC_InitStrue;
	
	/* ʹ�� USART1 ʱ��*/
	RCC_APB2PeriphClockCmd(RCC_APB2Periph_USART1 | RCC_APB2Periph_GPIOA, ENABLE); 

	/* USART1 ʹ��IO�˿����� */    
  GPIO_InitStructure.GPIO_Pin = GPIO_Pin_9;
  GPIO_InitStructure.GPIO_Mode = GPIO_Mode_AF_PP; //�����������
  GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;
  GPIO_Init(GPIOA, &GPIO_InitStructure);    
  
  GPIO_InitStructure.GPIO_Pin = GPIO_Pin_10;
  GPIO_InitStructure.GPIO_Mode = GPIO_Mode_IN_FLOATING;	//��������
  GPIO_Init(GPIOA, &GPIO_InitStructure);   //��ʼ��GPIOA
	  
	/* USART1 ����ģʽ���� */
	USART_InitStructure.USART_BaudRate = 9600;	//9600
	USART_InitStructure.USART_WordLength = USART_WordLength_8b;	//����λ�����ã�8λ
	USART_InitStructure.USART_StopBits = USART_StopBits_1; 	//ֹͣλ���ã�1λ
	USART_InitStructure.USART_Parity = USART_Parity_No ;  //�Ƿ���żУ�飺��
	USART_InitStructure.USART_HardwareFlowControl = USART_HardwareFlowControl_None;	//Ӳ��������ģʽ���ã�û��ʹ��
	USART_InitStructure.USART_Mode = USART_Mode_Rx | USART_Mode_Tx;//�����뷢�Ͷ�ʹ��
	USART_Init(USART1, &USART_InitStructure);  //��ʼ��USART1
	USART_Cmd(USART1, ENABLE);// USART1ʹ��
	USART_ITConfig(USART1,USART_IT_RXNE,ENABLE);         //���������ж�
	//�򿪴��ڽ��տ����ж�
	USART_ITConfig(USART1,USART_IT_IDLE,ENABLE);
	
	
	NVIC_InitStrue.NVIC_IRQChannel=USART1_IRQn;          //ָ��IRQͨ��
	NVIC_InitStrue.NVIC_IRQChannelCmd=ENABLE;            //�ж�ʹ��
	NVIC_InitStrue.NVIC_IRQChannelPreemptionPriority=1;  //ָ�����ȼ�
	NVIC_InitStrue.NVIC_IRQChannelSubPriority=1;         //
	NVIC_Init(&NVIC_InitStrue);    
}

 /*����һ���ֽ�����*/
 void UART1SendByte(unsigned char SendData)
{	   
        USART_SendData(USART1,SendData);
        while(USART_GetFlagStatus(USART1, USART_FLAG_TXE) == RESET);	    
}  

/*����һ���ֽ�����*/
unsigned char UART1GetByte(unsigned char* GetData)
{   	   
        if(USART_GetFlagStatus(USART1, USART_FLAG_RXNE) == RESET)
        {  return 0;//û���յ����� 
		}
        *GetData = USART_ReceiveData(USART1); 
        return 1;//�յ�����
}
/*����һ�����ݣ����Ϸ��ؽ��յ����������*/

volatile static u8 buff[200];
volatile static u8 count = 0;


void USART1_IRQHandler(void)
{
	if(USART_GetITStatus(USART1,USART_IT_RXNE))
	{
		
	   USART_ClearITPendingBit(USART1,USART_IT_RXNE);
    }
	
	if(USART_GetITStatus(USART1, USART_IT_PE) != RESET)
	{	
		USART_ClearITPendingBit(USART1, USART_IT_PE);
	}		
	if (USART_GetFlagStatus(USART1, USART_IT_LBD) != RESET) 
	{
    USART_ClearITPendingBit(USART1, USART_IT_LBD);           
  }		
	if(USART_GetFlagStatus(USART1, USART_FLAG_ORE) != RESET)
	{
		USART_ReceiveData(USART1);
		USART_ClearFlag(USART1, USART_FLAG_ORE);
	}		
	if(USART_GetFlagStatus(USART1, USART_FLAG_NE) != RESET)
	{
		USART_ClearFlag(USART1, USART_FLAG_NE);
	}
	if(USART_GetFlagStatus(USART1, USART_FLAG_FE) != RESET)
	{
		USART_ClearFlag(USART1, USART_FLAG_FE);
	}
	if(USART_GetFlagStatus(USART1, USART_FLAG_PE) != RESET)
	{
		USART_ClearFlag(USART1, USART_FLAG_PE);
	}	
	if (USART_GetITStatus(USART1, USART_IT_TC) != RESET) 
  {
    USART_ClearITPendingBit(USART1, USART_IT_TC);    
  } 
	
}



void USART2_Config(void)
{
	GPIO_InitTypeDef GPIO_InitStructure;
	USART_InitTypeDef USART_InitStructure;
	NVIC_InitTypeDef NVIC_InitStrue;
	
	/* ʹ�� USART2 ʱ��*/
	RCC_APB1PeriphClockCmd(RCC_APB1Periph_USART2, ENABLE); 

	/* USART2 ʹ��IO�˿����� */    
  GPIO_InitStructure.GPIO_Pin = GPIO_Pin_2;
  GPIO_InitStructure.GPIO_Mode = GPIO_Mode_AF_PP; //�����������
  GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;
  GPIO_Init(GPIOA, &GPIO_InitStructure);    
  
  GPIO_InitStructure.GPIO_Pin = GPIO_Pin_3;
  GPIO_InitStructure.GPIO_Mode = GPIO_Mode_IN_FLOATING;	//��������
  GPIO_Init(GPIOA, &GPIO_InitStructure);   //��ʼ��GPIOA
	  
	/* USART2 ����ģʽ���� */
	USART_InitStructure.USART_BaudRate = 9600;	//9600
	USART_InitStructure.USART_WordLength = USART_WordLength_8b;	//����λ�����ã�8λ
	USART_InitStructure.USART_StopBits = USART_StopBits_1; 	//ֹͣλ���ã�1λ
	USART_InitStructure.USART_Parity = USART_Parity_No ;  //�Ƿ���żУ�飺��
	USART_InitStructure.USART_HardwareFlowControl = USART_HardwareFlowControl_None;	//Ӳ��������ģʽ���ã�û��ʹ��
	USART_InitStructure.USART_Mode = USART_Mode_Rx | USART_Mode_Tx;//�����뷢�Ͷ�ʹ��
	USART_Init(USART2, &USART_InitStructure);  //��ʼ��USART1
	USART_Cmd(USART2, ENABLE);// USART1ʹ��
	USART_ITConfig(USART2,USART_IT_RXNE,ENABLE);         //���������ж�
	//�򿪴��ڽ��տ����ж�
//	USART_ITConfig(USART2,USART_IT_IDLE,ENABLE);
	
	
	NVIC_InitStrue.NVIC_IRQChannel=USART2_IRQn;          //ָ��IRQͨ��
	NVIC_InitStrue.NVIC_IRQChannelCmd=ENABLE;            //�ж�ʹ��
	NVIC_InitStrue.NVIC_IRQChannelPreemptionPriority=2;  //ָ�����ȼ�
	NVIC_InitStrue.NVIC_IRQChannelSubPriority=1;         //
	NVIC_Init(&NVIC_InitStrue);    
}

 /*����һ���ֽ�����*/
 void UART2SendByte(unsigned char SendData)
{	   
        USART_SendData(USART2,SendData);
        while(USART_GetFlagStatus(USART2, USART_FLAG_TXE) == RESET);	    
}  

void USART2_IRQHandler(void)
{

	if(USART_GetITStatus(USART2,USART_IT_RXNE))
	{
		
	   USART_ClearITPendingBit(USART2,USART_IT_RXNE);
  }
 //�����ж�
 	if(USART_GetITStatus(USART2,USART_IT_IDLE))
 {
	 
	   USART_ClearITPendingBit(USART2,USART_IT_IDLE);		 
 }
	if(USART_GetITStatus(USART2, USART_IT_PE) != RESET)
	{	
		USART_ClearITPendingBit(USART2, USART_IT_PE);
	}		
	if (USART_GetFlagStatus(USART2, USART_IT_LBD) != RESET) 
	{
    USART_ClearITPendingBit(USART2, USART_IT_LBD);           
  }		
	if(USART_GetFlagStatus(USART2, USART_FLAG_ORE) != RESET)
	{
		USART_ReceiveData(USART2);
		USART_ClearFlag(USART2, USART_FLAG_ORE);
	}		
	if(USART_GetFlagStatus(USART2, USART_FLAG_NE) != RESET)
	{
		USART_ClearFlag(USART2, USART_FLAG_NE);
	}
	if(USART_GetFlagStatus(USART2, USART_FLAG_FE) != RESET)
	{
		USART_ClearFlag(USART2, USART_FLAG_FE);
	}
	if(USART_GetFlagStatus(USART2, USART_FLAG_PE) != RESET)
	{
		USART_ClearFlag(USART2, USART_FLAG_PE);
	}	
	if (USART_GetITStatus(USART2, USART_IT_TC) != RESET) 
  {
    USART_ClearITPendingBit(USART2, USART_IT_TC);    
	
  }
}







