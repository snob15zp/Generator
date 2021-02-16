#include "bluetooth.h"
#include "stm32g0xx.h"
#include "tim3.h"
#include <string.h>
//#include "rn4870Model.h"

uint8_t debArr[10];

void btInit(void)
{
	btIoPinsInit();
	btUartInit();
}

void btIoPinsInit(void)
{
	GPIOA->MODER &= ~(GPIO_MODER_MODE0_0 |  GPIO_MODER_MODE0_1 |
										GPIO_MODER_MODE1_0 |  GPIO_MODER_MODE1_1 |
										GPIO_MODER_MODE2_0 |  GPIO_MODER_MODE2_1 |
										GPIO_MODER_MODE3_0 |  GPIO_MODER_MODE3_1 |
										GPIO_MODER_MODE4_0 |  GPIO_MODER_MODE4_1);
	GPIOA->MODER |= GPIO_MODER_MODE0_0 |
									GPIO_MODER_MODE1_0 |
									GPIO_MODER_MODE2_1 |
									GPIO_MODER_MODE3_1 |
									GPIO_MODER_MODE4_0;
	GPIOA->OTYPER &= ~(GPIO_OTYPER_OT0 |
										 GPIO_OTYPER_OT1 |
										 GPIO_OTYPER_OT4);
	GPIOA->PUPDR &= ~(GPIO_PUPDR_PUPD2_0 | GPIO_PUPDR_PUPD2_1);
	GPIOA->PUPDR |= GPIO_PUPDR_PUPD2_0;
	GPIOA->OSPEEDR |= GPIO_OSPEEDR_OSPEED2_0 | GPIO_OSPEEDR_OSPEED2_1;
	GPIOA->BSRR = GPIO_BSRR_BS0 | GPIO_BSRR_BS1 | GPIO_BSRR_BS4;
	GPIOA->AFR[0] = (USART2_ALT_FUNC<<GPIO_AFRL_AFSEL2_Pos) | 
									(USART2_ALT_FUNC<<GPIO_AFRL_AFSEL3_Pos);
}

void btUartInit(void)
{
	RCC->APBENR1 |= RCC_APBENR1_USART2EN;
	USART2->BRR = USART2_DIV;		//sets UART2 baudrate 115200 baud
	USART2->CR1 |= //USART_CR1_FIFOEN |
									USART_CR1_RXNEIE_RXFNEIE |
									USART_CR1_TE |
									USART_CR1_RE |
									USART_CR1_UE;
	NVIC_SetPriority(USART2_IRQn,1);
	NVIC_EnableIRQ(USART2_IRQn);
}

void btRst(uint32_t rstDurMs,uint32_t delAfterRstMs)
{
	GPIOA->BSRR = GPIO_BSRR_BR1;
	delay_ms(rstDurMs);
	GPIOA->BSRR = GPIO_BSRR_BS1;
	delay_ms(delAfterRstMs);
}

uint8_t txData(uint8_t *txBuff, uint8_t txBytesNum)
{
	int i;
	for(i=0;i<txBytesNum+1;i++){
		USART2->TDR = txBuff[i];
		while(!(USART2->ISR & USART_ISR_TC)){}
		USART2->ICR |= USART_ICR_TCCF;
	}
	USART2->TDR=DATA_LAST_CHAR;
	while(!(USART2->ISR & USART_ISR_TC)){}
	USART2->ICR |= USART_ICR_TCCF;
//	USART2->CR1 |= USART_CR1_RXNEIE_RXFNEIE;
	return 0;
}

