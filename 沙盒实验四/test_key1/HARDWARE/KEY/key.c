#include "key.h"
#include "delay.h"
//////////////////////////////////////////////////////////////////////////////////	 
//本程序只供学习使用，未经作者许可，不得用于其它任何用途
//ALIENTEK精英STM32F103开发板V1
//按键驱动代码	   
//正点原子@ALIENTEK
//技术论坛:www.openedv.com
//创建日期:2015/1/10
//版本：V1.0
//版权所有，盗版必究。
//Copyright(C) 广州市星翼电子科技有限公司 2009-2019
//All rights reserved								  
//////////////////////////////////////////////////////////////////////////////////  
								    
//按键初始化函数
void KEY_Init(void)
{
	RCC->APB2ENR|=1<<2;     //使能PORTA时钟
	RCC->APB2ENR|=1<<3;     //使能PORTB时钟
	GPIOA->CRL&=0XFFFFFF0F;	//PA0设置成输入（WK_UP）	  
	GPIOA->CRL|=0X00000080; 
	  
	GPIOB->CRL&=0XFFFFFF0F;	//PB1(KEY1)设置成输入	  
	GPIOB->CRL|=0X00000080; 					   
	GPIOB->ODR|=1<<1;	   	//PB1 上拉

	GPIOB->CRH&=0XFFFF0FFF;	//PB11(KEY0)设置成输入	  
	GPIOB->CRH|=0X00008000; 					   
	GPIOB->ODR|=1<<11;	   	//PB11 上拉
} 
//按键处理函数
//返回按键值
//mode:0,不支持连续按;1,支持连续按;
//返回值:
//0，没有任何按键按下
//1，KEY0按下
//2，KEY1按下 
//3，KEY_UP按下 即WK_UP
//注意此函数有响应优先级,KEY0>KEY1>KEY_UP!!
u8 KEY_Scan(u8 mode)
{	 
	static u8 key_up=1;//按键按松开标志
	if(mode)key_up=1;  //支持连按		  
	if(key_up&&(KEY0==0||KEY1==0||WK_UP==1))
	{
//		delay_ms(10);//去抖动###在LiteOS程序中不能再使用原来裸机程序中的delay_ms()等延时函数，因为这些延时函数用到了Systick定时器 
		key_up=0;
		if(KEY0==0)return 1;
		else if(KEY1==0)return 2; 
		else if(WK_UP==1)return 3;
	}else if(KEY0==1&&KEY1==1&&WK_UP==0)key_up=1; 	    
 	return 0;// 无按键按下
}
