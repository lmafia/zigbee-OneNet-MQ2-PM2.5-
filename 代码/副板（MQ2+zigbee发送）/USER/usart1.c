

#include "usart1.h"
#include <stdarg.h>
#include "misc.h"



void USART1_Config(void)
{
	GPIO_InitTypeDef GPIO_InitStructure;
	USART_InitTypeDef USART_InitStructure;
	NVIC_InitTypeDef NVIC_InitStrue;
	
	/* 使能 USART1 时钟*/
	RCC_APB2PeriphClockCmd(RCC_APB2Periph_USART1 | RCC_APB2Periph_GPIOA, ENABLE); 

	/* USART1 使用IO端口配置 */    
  GPIO_InitStructure.GPIO_Pin = GPIO_Pin_9;
  GPIO_InitStructure.GPIO_Mode = GPIO_Mode_AF_PP; //复用推挽输出
  GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;
  GPIO_Init(GPIOA, &GPIO_InitStructure);    
  
  GPIO_InitStructure.GPIO_Pin = GPIO_Pin_10;
  GPIO_InitStructure.GPIO_Mode = GPIO_Mode_IN_FLOATING;	//浮空输入
  GPIO_Init(GPIOA, &GPIO_InitStructure);   //初始化GPIOA
	  
	/* USART1 工作模式配置 */
	USART_InitStructure.USART_BaudRate = 9600;	//9600
	USART_InitStructure.USART_WordLength = USART_WordLength_8b;	//数据位数设置：8位
	USART_InitStructure.USART_StopBits = USART_StopBits_1; 	//停止位设置：1位
	USART_InitStructure.USART_Parity = USART_Parity_No ;  //是否奇偶校验：无
	USART_InitStructure.USART_HardwareFlowControl = USART_HardwareFlowControl_None;	//硬件流控制模式设置：没有使能
	USART_InitStructure.USART_Mode = USART_Mode_Rx | USART_Mode_Tx;//接收与发送都使能
	USART_Init(USART1, &USART_InitStructure);  //初始化USART1
	USART_Cmd(USART1, ENABLE);// USART1使能
	USART_ITConfig(USART1,USART_IT_RXNE,ENABLE);         //开启接收中断
	//打开串口接收空闲中断
	USART_ITConfig(USART1,USART_IT_IDLE,ENABLE);
	
	
	NVIC_InitStrue.NVIC_IRQChannel=USART1_IRQn;          //指定IRQ通道
	NVIC_InitStrue.NVIC_IRQChannelCmd=ENABLE;            //中断使能
	NVIC_InitStrue.NVIC_IRQChannelPreemptionPriority=1;  //指定优先级
	NVIC_InitStrue.NVIC_IRQChannelSubPriority=1;         //
	NVIC_Init(&NVIC_InitStrue);    
}

 /*发送一个字节数据*/
 void UART1SendByte(unsigned char SendData)
{	   
        USART_SendData(USART1,SendData);
        while(USART_GetFlagStatus(USART1, USART_FLAG_TXE) == RESET);	    
}  

/*接收一个字节数据*/
unsigned char UART1GetByte(unsigned char* GetData)
{   	   
        if(USART_GetFlagStatus(USART1, USART_FLAG_RXNE) == RESET)
        {  return 0;//没有收到数据 
		}
        *GetData = USART_ReceiveData(USART1); 
        return 1;//收到数据
}
/*接收一个数据，马上返回接收到的这个数据*/

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
	
	/* 使能 USART2 时钟*/
	RCC_APB1PeriphClockCmd(RCC_APB1Periph_USART2, ENABLE); 

	/* USART2 使用IO端口配置 */    
  GPIO_InitStructure.GPIO_Pin = GPIO_Pin_2;
  GPIO_InitStructure.GPIO_Mode = GPIO_Mode_AF_PP; //复用推挽输出
  GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;
  GPIO_Init(GPIOA, &GPIO_InitStructure);    
  
  GPIO_InitStructure.GPIO_Pin = GPIO_Pin_3;
  GPIO_InitStructure.GPIO_Mode = GPIO_Mode_IN_FLOATING;	//浮空输入
  GPIO_Init(GPIOA, &GPIO_InitStructure);   //初始化GPIOA
	  
	/* USART2 工作模式配置 */
	USART_InitStructure.USART_BaudRate = 9600;	//9600
	USART_InitStructure.USART_WordLength = USART_WordLength_8b;	//数据位数设置：8位
	USART_InitStructure.USART_StopBits = USART_StopBits_1; 	//停止位设置：1位
	USART_InitStructure.USART_Parity = USART_Parity_No ;  //是否奇偶校验：无
	USART_InitStructure.USART_HardwareFlowControl = USART_HardwareFlowControl_None;	//硬件流控制模式设置：没有使能
	USART_InitStructure.USART_Mode = USART_Mode_Rx | USART_Mode_Tx;//接收与发送都使能
	USART_Init(USART2, &USART_InitStructure);  //初始化USART1
	USART_Cmd(USART2, ENABLE);// USART1使能
	USART_ITConfig(USART2,USART_IT_RXNE,ENABLE);         //开启接收中断
	//打开串口接收空闲中断
//	USART_ITConfig(USART2,USART_IT_IDLE,ENABLE);
	
	
	NVIC_InitStrue.NVIC_IRQChannel=USART2_IRQn;          //指定IRQ通道
	NVIC_InitStrue.NVIC_IRQChannelCmd=ENABLE;            //中断使能
	NVIC_InitStrue.NVIC_IRQChannelPreemptionPriority=2;  //指定优先级
	NVIC_InitStrue.NVIC_IRQChannelSubPriority=1;         //
	NVIC_Init(&NVIC_InitStrue);    
}

 /*发送一个字节数据*/
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
 //空闲中断
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







