#include "led.h"
//////////////////////////////////////////////////////////////////////////////////	 
//本程序只供学习使用，未经作者许可，不得用于其它任何用途
//ALIENTEK精英STM32F103开发板V1
//LED驱动代码	   
//正点原子@ALIENTEK
//技术论坛:www.openedv.com
//创建日期:2015/1/9
//版本：V1.0
//版权所有，盗版必究。
//Copyright(C) 广州市星翼电子科技有限公司 2009-2019
//All rights reserved									  
////////////////////////////////////////////////////////////////////////////////// 

//初始化PB5和PB0为输出口.并使能PORTB时钟		    
//LED IO初始化
void LED_Init(void)
{
	RCC->APB2ENR|=1<<3;    //使能PORTB时钟	   	 
	 	 
	GPIOB->CRL&=0XFF0FFFFF; 
	GPIOB->CRL|=0X00300000;//PB.5 推挽输出   	 
    GPIOB->ODR|=1<<5;      //PB.5 输出高
												  
	GPIOB->CRL&=0XFFFFFFF0;
	GPIOB->CRL|=0X00000003;//PB.0 推挽输出（C8T6适配）
	GPIOB->ODR|=1<<0;      //PB.0 输出高
}






