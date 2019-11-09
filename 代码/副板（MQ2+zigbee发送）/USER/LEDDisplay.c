#include "stm32f10x.h"
#include "LEDDisplay.h"


/*����Max7219�˿�  
	CLK	E14    ����
	CS	B10    ����
	DIN	B11    ����
	VCC	3.3V
	GND	GND
*/
#define Max7219_pinCLK_1 GPIO_SetBits(GPIOE,GPIO_Pin_14);    //�ߵ�ƽ
#define Max7219_pinCLK_0 GPIO_ResetBits(GPIOE,GPIO_Pin_14);  //�͵�ƽ

#define Max7219_pinCS_1 GPIO_SetBits(GPIOB,GPIO_Pin_10);     //�ߵ�ƽ
#define Max7219_pinCS_0 GPIO_ResetBits(GPIOB,GPIO_Pin_10);   //�͵�ƽ

#define Max7219_pinDIN_1 GPIO_SetBits(GPIOB,GPIO_Pin_11);    //�ߵ�ƽ
#define Max7219_pinDIN_0 GPIO_ResetBits(GPIOB,GPIO_Pin_11);  //�͵�ƽ

/*
	CLK	E14    ����
	CS	B10    ����
	DIN	B11    ����
*/
void LEDDisplay_GPIO_Config(void)
{
		/*����һ��GPIO_InitTypeDef���͵Ľṹ��*/
	 GPIO_InitTypeDef GPIO_InitStructure;

	
	/*E14*/
	 RCC_APB2PeriphClockCmd(RCC_APB2Periph_GPIOE, ENABLE);

	/*ѡ��Ҫ���Ƶ�GPIOC����*/															   
   GPIO_InitStructure.GPIO_Pin = GPIO_Pin_14;	

	/*��������ģʽΪͨ���������*/
  GPIO_InitStructure.GPIO_Mode = GPIO_Mode_Out_PP;   

	/*������������Ϊ50MHz */   
  GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz; 

	/*���ÿ⺯������ʼ��*/
  GPIO_Init(GPIOE, &GPIO_InitStructure);
	
	
		/*B10 B11*/
	 RCC_APB2PeriphClockCmd(RCC_APB2Periph_GPIOB, ENABLE);

	/*ѡ��Ҫ���Ƶ�GPIOC����*/															   
   GPIO_InitStructure.GPIO_Pin = GPIO_Pin_10|GPIO_Pin_11;	

	/*��������ģʽΪͨ���������*/
  GPIO_InitStructure.GPIO_Mode = GPIO_Mode_Out_PP;   

	/*������������Ϊ50MHz */   
  GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz; 

	/*���ÿ⺯������ʼ��*/
  GPIO_Init(GPIOB, &GPIO_InitStructure);
}


void Delay_xms(unsigned int x)
{
 unsigned int i,j;
 for(i=0;i<x;i++)
  for(j=0;j<112;j++);
}

//--------------------------------------------
//���ܣ���MAX7219(U3)д���ֽ�
//��ڲ�����DATA 
//���ڲ�������
//˵����
void Write_Max7219_byte(unsigned char DATA)         
{
    unsigned char i;    
		Max7219_pinCS_0;		
	  for(i=8;i>=1;i--)
     {		  
         Max7219_pinCLK_0;
			   if( DATA&0x80)
				 {
					Max7219_pinDIN_1;
				 }
				 else
				 {
					 Max7219_pinDIN_0;
				 }
          DATA=DATA<<1;
          Max7219_pinCLK_1;
     }                                 
}
//-------------------------------------------
//���ܣ���MAX7219д������
//��ڲ�����address��dat
//���ڲ�������
//˵����
void Write_Max7219(unsigned char address,unsigned char dat)
{ 
   Max7219_pinCS_0;
	 Write_Max7219_byte(address);           //д���ַ��������ܱ��
   Write_Max7219_byte(dat);               //д�����ݣ����������ʾ���� 
	 Max7219_pinCS_1;                        
}

void Init_MAX7219(void)
{
 Write_Max7219(0x09, 0xff);       //���뷽ʽ��BCD��
 Write_Max7219(0x0a, 0x03);       //����
 Write_Max7219(0x0b, 0x07);       //ɨ����ޣ�4���������ʾ
 Write_Max7219(0x0c, 0x01);       //����ģʽ��0����ͨģʽ��1
 Write_Max7219(0x0f, 0x01);       //��ʾ���ԣ�1�����Խ�����������ʾ��0
}
/*
1.��ʾ�������ұ�Ϊ��һλ����λû������-���
2.��ʾ�����Ը�����ʽ��������С�����2λ
3.ʵ����100.8999   ʵ����ʾ----100.89
*/
void LEDDisplay(float value)
{

 unsigned long int V;
 unsigned long int temp;
 unsigned long int temp1;	
	
	unsigned long int flag;
	
	flag = 0;
	
 //�Ŵ�100��
 V = value*100;
 //ȡ����,����ߵ����
 temp =  V/10000000;
 temp1 =  V%10000000;	
	if(temp)		
	{
		Write_Max7219(8,temp);
		flag = 1;
	}
	else
	{
		Write_Max7219(8,10);		
	}
	
	
	 temp =  temp1/1000000;
	 temp1 =  temp1%1000000;
	if(temp || flag)		
	{
		Write_Max7219(7,temp);
		flag = 1;
	}
	else
	{
		Write_Max7219(7,10);		
	}
	
	
	 temp =  temp1/100000;
	 temp1 =  temp1%100000;
	if(temp || flag)		
	{
		Write_Max7219(6,temp);
		flag = 1;
	}
	else
	{
		Write_Max7219(6,10);		
	}		
	
	
	 temp =  temp1/10000;
	 temp1 =  temp1%10000;
	if(temp ||flag)		
	{
		Write_Max7219(5,temp);
				flag = 1;
	}
	else
	{
		Write_Max7219(5,10);		
	}		

	
	 temp =  temp1/1000;
	 temp1 =  temp1%1000;
	if(temp || flag)		
	{
		Write_Max7219(4,temp);
				flag = 1;
	}
	else
	{
		Write_Max7219(4,10);		
	}	
	
	
	 temp =  temp1/100;
	 temp1 =  temp1%100;
	if(temp || flag)		
	{
		Write_Max7219(3,temp|0xf0);
				flag = 1;
	}
	else
	{
		Write_Max7219(3,10|0xf0);		
	}		
	
	
	 temp =  temp1/10;
	 temp1 =  temp1%10;
	if(temp || flag)		
	{
		Write_Max7219(2,temp);
				flag = 1;
	}
	else
	{
		Write_Max7219(2,10);		
	}	
	 
	if(temp1 || flag)		
	{
		Write_Max7219(1,temp1);
				flag = 1;
	}
	else
	{
		Write_Max7219(1,10);		
	}			
}

void LEDDisplayConfig(void)
{
 Delay_xms(50000);
 Init_MAX7219();
 Delay_xms(2000);
 Write_Max7219(0x0f, 0x00);       //��ʾ���ԣ�1�����Խ�����������ʾ��0
	
 LEDDisplay(0);
}
