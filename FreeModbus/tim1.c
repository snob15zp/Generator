#include "stm32g0xx.h"
#include "tim1.h"
#include "BoardSetup.h"


unsigned short mbTick_1sec = 0;
extern unsigned long Time_Cycle;  
static unsigned long tick_my = 0;

void tim6Init(void)
{
	RCC->APBENR1 |= RCC_APBENR1_TIM6EN;
	TIM6->SR = 0;
	TIM6->PSC = 63;												//sets TIM1 clk 1 MHz
//	TIM6->ARR = 1000;											//sets TIM1 count interval 1 ms
	TIM6->ARR = 50;											//sets TIM1 count interval 1 ms	
	TIM6->DIER = TIM_DIER_UIE;
	NVIC_SetPriority(TIM6_IRQn,14);
	NVIC_EnableIRQ(TIM6_IRQn);
	TIM6->CR1 |= TIM_CR1_CEN;
}

/*
void TIM6_IRQHandler(void)
{
	if(TIM6->SR & TIM_SR_UIF)
	{
		TIM6->SR = ~TIM_SR_UIF;
//		tick_my++;
	}
}
*/


unsigned long get_myTime(void)
{
//	return tick_my;
	return SystemTicks;
}


void task_modbus(void)
{
static unsigned long value = 0;
static unsigned long timeStart = 0;
unsigned long timeTek;
	
	  Time_Cycle = 500;
		timeTek = get_myTime();
		if (  (timeTek - timeStart) >= Time_Cycle )
		{
			timeStart = timeTek;
			value++;
			if ( value & 0x0001 )
			{
				mbTick_1sec++;
				GPIOB->BSRR = GPIO_BSRR_BS10;				
			}
			else
				GPIOB->ODR = 0;//GPIO_ODR_OD10; 		
		}
}
