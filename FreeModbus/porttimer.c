/*
 * FreeModbus Libary: BARE Port
 * Copyright (C) 2006 Christian Walter <wolti@sil.at>
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2.1 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA  02110-1301  USA
 *
 * File: $Id$
 */

/* ----------------------- Platform includes --------------------------------*/
#include "port.h"

/* ----------------------- Modbus includes ----------------------------------*/
#include "mb.h"
#include "mbport.h"
#include "stm32g0xx.h"

/* ----------------------- static functions ---------------------------------*/
static void prvvTIMERExpiredISR( void );

/* ----------------------- Start implementation -----------------------------*/
//BOOL xMBPortTimersInit( USHORT usTim1Timerout50us )
//{
//    return FALSE;
//}
BOOL xMBPortTimersInit( USHORT usTim1Timerout50us )
{
/*	
    NVIC_InitTypeDef NVIC_InitStructure;
    TIM_TimeBaseInitTypeDef base_timer;

    RCC_APB1PeriphClockCmd( RCC_APB1Periph_TIM2, ENABLE );

    TIM_TimeBaseStructInit( &base_timer );

    // 84000 ??? // 4200 = 20 ??? ( 50 ??? ) 
    base_timer.TIM_Prescaler = 4200 - 1;
    base_timer.TIM_Period = ( (uint32_t) usTim1Timerout50us ) - 1;
    base_timer.TIM_ClockDivision = 0;
    base_timer.TIM_CounterMode = TIM_CounterMode_Up;
    TIM_TimeBaseInit( TIM2, &base_timer );

    TIM_ClearITPendingBit( TIM2, TIM_IT_Update );

    // ????????? ?????????? ?? ?????????? (? ?????? ?????? -  ?? ????????????) ????????-??????? TIM2.  
    TIM_ITConfig( TIM2, TIM_IT_Update, ENABLE );

    NVIC_InitStructure.NVIC_IRQChannel = TIM2_IRQn;
    NVIC_InitStructure.NVIC_IRQChannelPreemptionPriority = 0;
    NVIC_InitStructure.NVIC_IRQChannelSubPriority = 1;
    NVIC_InitStructure.NVIC_IRQChannelCmd = ENABLE;
    NVIC_Init( &NVIC_InitStructure );

    TIM_Cmd( TIM2, ENABLE );

    return TRUE;
*/
	RCC->APBENR1 |= RCC_APBENR1_TIM6EN;
	TIM6->SR = 0;
	TIM6->PSC = 63;												//sets TIM1 clk 1 MHz
	TIM6->ARR = 400;											//sets TIM1 count interval 1 ms
	TIM6->DIER = TIM_DIER_UIE;
	NVIC_SetPriority(TIM6_IRQn,14);
	NVIC_EnableIRQ(TIM6_IRQn);
	TIM6->CR1 |= TIM_CR1_CEN;

  return TRUE;
}

inline void vMBPortTimersEnable(  )
{
    /* Enable the timer with the timeout passed to xMBPortTimersInit( ) */
	TIM6->CNT = 0;
  RCC->APBENR1 |= RCC_APBENR1_TIM6EN;
//    TIM_SetCounter( TIM2, 0 );
//    TIM_Cmd( TIM2, ENABLE );
}

inline void vMBPortTimersDisable(  )
{
    /* Disable any pending timers. */
  RCC->APBENR1 &= ~RCC_APBENR1_TIM6EN;
//    TIM_Cmd( TIM2, DISABLE );
}

inline void vMBPortTimersDelay( USHORT usTimeOutMS )
{
};

/* Create an ISR which is called whenever the timer has expired. This function
 * must then call pxMBPortCBTimerExpired( ) to notify the protocol stack that
 * the timer has expired.
 */
//static void prvvTIMERExpiredISR( void )
//{
//    ( void )pxMBPortCBTimerExpired(  );
//}



void TIM6_IRQHandler(void)
{
	if(TIM6->SR & TIM_SR_UIF)
	{
		TIM6->SR = ~TIM_SR_UIF;
    pxMBPortCBTimerExpired();
	}
}











