#include "stm32f10x.h"
#include "LEDDisplay.h"


/*定义Max7219端口  
	CLK	E14    推挽
	CS	B10    推挽
	DIN	B11    推挽
	VCC	3.3V
	GND	GND
*/
#define Max7219_pinCLK_1 GPIO_SetBits(GPIOE,GPIO_Pin_14);    //高电平
#define Max7219_pinCLK_0 GPIO_ResetBits(GPIOE,GPIO_Pin_14);  //低电平

#define Max7219_pinCS_1 GPIO_SetBits(GPIOB,GPIO_Pin_10);     //高电平
#define Max7219_pinCS_0 GPIO_ResetBits(GPIOB,GPIO_Pin_10);   //低电平

#define Max7219_pinDIN_1 GPIO_SetBits(GPIOB,GPIO_Pin_11);    //高电平
#define Max7219_pinDIN_0 GPIO_ResetBits(GPIOB,GPIO_Pin_11);  //低电平

/*
	CLK	E14    推挽
	CS	B10    推挽
	DIN	B11    推挽
*/
void LEDDisplay_GPIO_Config(void)
{
		/*定义一个GPIO_InitTypeDef类型的结构体*/
	 GPIO_InitTypeDef GPIO_InitStructure;

	
	/*E14*/
	 RCC_APB2PeriphClockCmd(RCC_APB2Periph_GPIOE, ENABLE);

	/*选择要控制的GPIOC引脚*/															   
   GPIO_InitStructure.GPIO_Pin = GPIO_Pin_14;	

	/*设置引脚模式为通用推挽输出*/
  GPIO_InitStructure.GPIO_Mode = GPIO_Mode_Out_PP;   

	/*设置引脚速率为50MHz */   
  GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz; 

	/*调用库函数，初始化*/
  GPIO_Init(GPIOE, &GPIO_InitStructure);
	
	
		/*B10 B11*/
	 RCC_APB2PeriphClockCmd(RCC_APB2Periph_GPIOB, ENABLE);

	/*选择要控制的GPIOC引脚*/															   
   GPIO_InitStructure.GPIO_Pin = GPIO_Pin_10|GPIO_Pin_11;	

	/*设置引脚模式为通用推挽输出*/
  GPIO_InitStructure.GPIO_Mode = GPIO_Mode_Out_PP;   

	/*设置引脚速率为50MHz */   
  GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz; 

	/*调用库函数，初始化*/
  GPIO_Init(GPIOB, &GPIO_InitStructure);
}


void Delay_xms(unsigned int x)
{
 unsigned int i,j;
 for(i=0;i<x;i++)
  for(j=0;j<112;j++);
}

//--------------------------------------------
//功能：向MAX7219(U3)写入字节
//入口参数：DATA 
//出口参数：无
//说明：
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
//功能：向MAX7219写入数据
//入口参数：address、dat
//出口参数：无
//说明：
void Write_Max7219(unsigned char address,unsigned char dat)
{ 
   Max7219_pinCS_0;
	 Write_Max7219_byte(address);           //写入地址，即数码管编号
   Write_Max7219_byte(dat);               //写入数据，即数码管显示数字 
	 Max7219_pinCS_1;                        
}

void Init_MAX7219(void)
{
 Write_Max7219(0x09, 0xff);       //译码方式：BCD码
 Write_Max7219(0x0a, 0x03);       //亮度
 Write_Max7219(0x0b, 0x07);       //扫描界限；4个数码管显示
 Write_Max7219(0x0c, 0x01);       //掉电模式：0，普通模式：1
 Write_Max7219(0x0f, 0x01);       //显示测试：1；测试结束，正常显示：0
}
/*
1.显示方向以右边为第一位，高位没有则用-填充
2.显示数据以浮点形式，但仅到小数点后2位
3.实例：100.8999   实际显示----100.89
*/
void LEDDisplay(float value)
{

 unsigned long int V;
 unsigned long int temp;
 unsigned long int temp1;	
	
	unsigned long int flag;
	
	flag = 0;
	
 //放大100被
 V = value*100;
 //取数据,从最高到最低
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
 Write_Max7219(0x0f, 0x00);       //显示测试：1；测试结束，正常显示：0
	
 LEDDisplay(0);
}
