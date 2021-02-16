#include "stm32g0xx.h"
#include <string.h>
#include <stdbool.h>
#include "bluetooth.h"
#include "tim3.h"
#include "SuperLoop_Comm2.h"
#include "port.h"
#include "mbport.h"

//#include "rn4870Model.h"

void btIoPinsInit(void);
void btUartInit(void);
void btOn(void);
void btOff(void);
void setComMode(void);
int txCommand(uint8_t* cmdArr,uint8_t cmdArrDim);
void rxResponse(uint8_t* buff);
void btHardRst(uint32_t rstDurMs,uint32_t delAfterRstMs);
uint8_t uart2Tx(char *txBuff, uint8_t txBytesNum);



typedef enum  
{SLBL_Int_init  		//work
,SLBL_Int_Work  						//work
,SLBL_Int_NumOfEl	
} e_SLBL_Int;

char USART2_RDR;

#define D_BufferSize 256

#define OFFSET 220
char btChRx;
uint8_t btChRxRdy;
char btRespArr[D_BufferSize];
uint8_t rxIrqCnt,rxIrqCntRead,rxIrqCntOld,rxIrqCntl;
uint8_t btState;


e_SLBL_Int btInterfaceState;
//uint32_t btCurTime;
uint32_t lastIrqTime;



static bool byte_DLE;

uint8_t debArr[10];

systemticks_t lastUSBTime;

#define BYTES_IN_PACK 10

#define D_BL_Packet_Pause 1000

void SLBL(void)
{			
	if ((lastUSBTime+D_BL_Packet_Pause)<SystemTicks)
	{ if (PS_Int_USB==PS_Int)
		PS_Int=PS_Int_No;
	}	
	else	
	{if (PS_Int_No==PS_Int)
		PS_Int=PS_Int_USB;
	};	
	
	if (0==btState) return;
	if ((PS_Int_No==PS_Int)&&(SLBL_Int_Work==btInterfaceState))
			  PS_Int=PS_Int_BLE; 
	rxIrqCntl=rxIrqCnt;
	if (((lastIrqTime+D_BL_Packet_Pause)<SystemTicks)&&(rxIrqCntRead!=rxIrqCntl))
	{
		if (('%'==btRespArr[rxIrqCntl-1]) && ('D'==btRespArr[rxIrqCntl-2]) )
			btInterfaceState = SLBL_Int_Work;
		if (('%'==btRespArr[rxIrqCntl-1]) && ('T'==btRespArr[rxIrqCntl-2]) )
		{ btInterfaceState = SLBL_Int_init;
			if (PS_Int_BLE==PS_Int)
				PS_Int=PS_Int_No;
		};
   rxIrqCntRead=rxIrqCntl;
	};

}

void USART2_IRQHandler(void)
{
	if(USART2->ISR & USART_ISR_RXNE_RXFNE)
	{
		USART2->ICR |= USART_ICR_ORECF;	
		btChRx=USART2->RDR;
		btRespArr[rxIrqCnt]=btChRx;
		rxIrqCnt++;//&0xff
		//if (rxIrqCnt>=D_BufferSize)
		switch(btState)
		{
			case 0:	//BLE init
				if((rxIrqCnt)>=0x42)
				{
//					btChRxRdy=1;
					btState=1;
					rxIrqCntRead=rxIrqCnt;
				}
				break;
			default:
				lastIrqTime=SystemTicks;
				if ((PS_Int_BLE==PS_Int)&&(btInterfaceState == SLBL_Int_Work))
				{
					if (DLE==btChRx)
					{	
						byte_DLE=true;
					}
					else
					{
						if (byte_DLE)
						{	USART2_RDR=btChRx+1;
							byte_DLE=false;
						};					 
						USART2_RDR=btChRx;
						pxMBFrameCBByteReceived();
					};
		    }

	    }
		}
    if ( (USART2->ISR & USART_ISR_TXE_TXFNF) && (USART2->CR1 & USART_CR1_TXEIE_TXFNFIE) )
    {
        USART2->ICR |= USART_ICR_TXFECF;             
        USART2->RQR |= USART_RQR_TXFRQ;
       switch (PS_Int)
			{
				case PS_Int_BLE:
					  if (byte_TX_DLE)
						{	byte_TX_DLE=false;
						}
						else
						{
							pxMBFrameCBTransmitterEmpty();
						};
			      break;
				default:;
					
			}
        return;
    }	
	
}

void btInit(void)
{
	rxIrqCnt=0;
	btState=0;
	btIoPinsInit();
	btUartInit();
	btOn();
}

void btIoPinsInit(void)
{
	GPIOA->MODER &= ~(GPIO_MODER_MODE0_0 |  GPIO_MODER_MODE0_1 |
										GPIO_MODER_MODE1_0 |  GPIO_MODER_MODE1_1 |
										GPIO_MODER_MODE2_0 |  GPIO_MODER_MODE2_1 |
										GPIO_MODER_MODE3_0 |  GPIO_MODER_MODE3_1 |
										GPIO_MODER_MODE4_0 |  GPIO_MODER_MODE4_1);
	GPIOA->MODER |= GPIO_MODER_MODE0_0 | //output 
									GPIO_MODER_MODE1_0 | //output
									GPIO_MODER_MODE2_1 | // alternate
									GPIO_MODER_MODE3_1 | // alternate
									GPIO_MODER_MODE4_0;  // output
	GPIOA->OTYPER &= ~(GPIO_OTYPER_OT0 | // push-pull
										 GPIO_OTYPER_OT1 | // push-pull
										 GPIO_OTYPER_OT4); // push-pull
	GPIOA->PUPDR &= ~(GPIO_PUPDR_PUPD2_0 | GPIO_PUPDR_PUPD2_1);
	GPIOA->PUPDR |= GPIO_PUPDR_PUPD2_0; // pull up
	GPIOA->OSPEEDR |= GPIO_OSPEEDR_OSPEED2_0 | GPIO_OSPEEDR_OSPEED2_1;// high speed
	GPIOA->BSRR = GPIO_BSRR_BS0 | GPIO_BSRR_BS1 | GPIO_BSRR_BS4; //set
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

void btHardRst(uint32_t rstDurMs,uint32_t delAfterRstMs)
{
	GPIOA->BSRR = GPIO_BSRR_BR1;
	delay_ms(rstDurMs);
	GPIOA->BSRR = GPIO_BSRR_BS1;
	delay_ms(delAfterRstMs);
}

void btOn(void)
{
//	GPIOB->BSRR = GPIO_BSRR_BS2;
	GPIOA->BSRR = GPIO_BSRR_BR4;
	delay_ms(20);
	btHardRst(2,100);	//hard reset
//	delay_ms(100);
	uart2Tx("$$$",3);	//set Command Mode
	delay_ms(10);
	uart2Tx("+\r",2);	//echo on, for debug
	delay_ms(10);
	uart2Tx("SS,40\r",6);	//uart transparent mode
	delay_ms(10);
	uart2Tx("R,1\r",4);	//soft reset
	delay_ms(10);
}

void btOff(void)
{
	
}

void setComMode(void)
{
	char temp[]="$$$";
	uart2Tx(temp,3);
}

uint8_t uart2Tx(char *txBuff, uint8_t txBytesNum)
{
	int i;
	for(i=0;i<txBytesNum;i++){
		USART2->TDR = txBuff[i];
		while(!(USART2->ISR & USART_ISR_TC)){}
		USART2->ICR |= USART_ICR_TCCF;
	}
//	USART2->TDR=DATA_LAST_CHAR;
//	while(!(USART2->ISR & USART_ISR_TC)){}
//	USART2->ICR |= USART_ICR_TCCF;
//	USART2->CR1 |= USART_CR1_RXNEIE_RXFNEIE;
	return 0;
}

