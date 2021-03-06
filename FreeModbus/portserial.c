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
#include "SuperLoop_Comm2.h"
#include "GlobalKey.h"
//#include "Boardsetup.h"
#include "bluetooth.h"
#include "port.h"



/* ----------------------- Modbus includes ----------------------------------*/
#include "mb.h"
#include "mbport.h"


uint8_t USART1_RDR;

unsigned long Time_Cycle = 0;   

bool USART1_CR1_RXNEIE_Logic;

/* ----------------------- static functions ---------------------------------*/
static void prvvUARTTxReadyISR( void );
static void prvvUARTRxISR( void );

/* ----------------------- Start implementation -----------------------------*/
void vMBPortSerialEnable( BOOL xRxEnable, BOOL xTxEnable )
{
    /* If xRXEnable enable serial receive interrupts. If xTxENable enable
     * transmitter empty interrupts.
     */
	  switch (PS_Int)
		{
			case PS_Int_BLE:
			case PS_Int_BLE_No:	
					if( xRxEnable )
					{
						USART_CR1_RXNEIE_Logic=true;
						//USART2->CR1 |= USART_CR1_RXNEIE_RXFNEIE;
					}
					else
					{
						USART_CR1_RXNEIE_Logic=false; 
						//USART2->CR1 &= ~USART_CR1_RXNEIE_RXFNEIE;			
					}
					if ( xTxEnable )
					{
						USART2->CR1 |= USART_CR1_TXEIE_TXFNFIE;
					}
					else
					{
						USART2->CR1 &= ~USART_CR1_TXEIE_TXFNFIE;			
					}
				break;
			case PS_Int_USB:
			case PS_Int_USB_No:	
					if( xRxEnable )
					{
						USART1_CR1_RXNEIE_Logic=true;
					}
					else
					{
						USART1_CR1_RXNEIE_Logic=false;			
					}
					if ( xTxEnable )
					{
						USART1->CR1 |= USART_CR1_TXEIE_TXFNFIE;
					}
					else
					{
						USART1->CR1 &= ~USART_CR1_TXEIE_TXFNFIE;			
					}
					break;
			default:;
		}
}

BOOL xMBPortSerialInit( UCHAR ucPORT, ULONG ulBaudRate, UCHAR ucDataBits, eMBParity eParity )
{
#define USART1_PCLK 64000000UL
#define USART1_BAUDRATE 115200UL	
	
	
/*	
//#define USART1_ALT_FUNC 0x01
//#define USART1_DIV ()
	
	
	RCC->APBENR2 |= RCC_APBENR2_USART1EN;
	USART1->CR1 &= ~USART_CR1_UE;
	USART1->BRR = USART1_PCLK/USART1_BAUDRATE;		//sets UART1 baudrate 115200 baud
	USART1->CR3 |= USART_CR3_ONEBIT;
	USART1->CR1 |= //USART_CR1_FIFOEN |
//									USART_CR1_RXNEIE_RXFNEIE |
//									USART_CR1_TXEIE_TXFNFIE	 |
									USART_CR1_TE |
									USART_CR1_RE;
	NVIC_SetPriority(USART1_IRQn,1);
	NVIC_EnableIRQ(USART1_IRQn);
	USART1->CR1 |= USART_CR1_UE;
*/

	RCC->APBENR2 |= RCC_APBENR2_USART1EN;
	USART1->CR1 &= ~USART_CR1_UE;
	USART1->BRR = USART1_PCLK/USART1_BAUDRATE;	//sets UART1 baudrate 115200 baud
	USART1->CR3 |= USART_CR3_ONEBIT;
	USART1->CR3 |= USART_CR3_OVRDIS;
	USART1->CR1 |= USART_CR1_RXNEIE_RXFNEIE;
	USART1->CR1 |= USART_CR1_TE |
								 USART_CR1_RE;
//USART_CR1_FIFOEN |
//								USART_CR1_RXNEIE_RXFNEIE |
	NVIC_SetPriority(USART1_IRQn,1);
	NVIC_EnableIRQ(USART1_IRQn);
	USART1->CR1 |= USART_CR1_UE;
	return TRUE;
}

char btSendArr[256];
uint8_t txIrqCnt;
	
BOOL xMBPortSerialPutByte( CHAR ucByte )
{
    /* Put a byte in the UARTs transmit buffer. This function is called
     * by the protocol stack if pxMBFrameCBTransmitterEmpty( ) has been
     * called. */
	char ucBytel;
	  switch (PS_Int)
		{
			case PS_Int_BLE:
			case PS_Int_BLE_No:	
				  ucBytel=ucByte;
				  if ((DTD==ucByte)||(DLE==ucByte))
				  {	byte_TX_DLE = true;
						USART2->TDR = DLE;
#ifdef D_TestOuputToUart1
 						USART1->TDR = DLE;
#endif						
						btSendArr[txIrqCnt++]=DLE;
						while(!(USART2->ISR&USART_ISR_TXE_TXFNF));
						ucBytel=ucBytel-1;
					};
					USART2->TDR = ucBytel;
#ifdef D_TestOuputToUart1
					USART1->TDR = ucBytel;
#endif						
					btSendArr[txIrqCnt++]=ucBytel;
				break;
			case PS_Int_USB:
			case PS_Int_USB_No:	
					USART1->TDR = ucByte;
			break;
	  	default:;
		}
    return TRUE;
}

BOOL xMBPortSerialGetByte( CHAR *pucByte )
{
    /* Return the byte in the UARTs receive buffer. This function is called
     * by the protocol stack after pxMBFrameCBByteReceived( ) has been called.
     */
	
	 switch (PS_Int)
		{
			case PS_Int_BLE:
			case PS_Int_BLE_No:	
				  *pucByte = USART2_RDR;// todo ??????
				break;
			case PS_Int_USB:
			case PS_Int_USB_No:	
					*pucByte = USART1_RDR;				
      break;			
			default:;
		}
    return TRUE;
}

/* Create an interrupt handler for the transmit buffer empty interrupt
 * (or an equivalent) for your target processor. This function should then
 * call pxMBFrameCBTransmitterEmpty( ) which tells the protocol stack that
 * a new character can be sent. The protocol stack will then call 
 * xMBPortSerialPutByte( ) to send the character.
 */
static void prvvUARTTxReadyISR( void )
{
    pxMBFrameCBTransmitterEmpty(  );
}

/* Create an interrupt handler for the receive interrupt for your target
 * processor. This function should then call pxMBFrameCBByteReceived( ). The
 * protocol stack will then call xMBPortSerialGetByte( ) to retrieve the
 * character.
 */
static void prvvUARTRxISR( void )
{
    pxMBFrameCBByteReceived();
}

/*
void USART1_IRQHandler(void)
{
CHAR data;  		
		if ( USART1->ISR & USART_ISR_RXNE_RXFNE )
		{
				USART1->ICR |= USART_ICR_ORECF;
				USART1->RQR |= USART_RQR_RXFRQ;
			
				data = (CHAR)USART1->RDR;			
			
//        pxMBFrameCBByteReceived();
		}
		
		if ( USART1->ISR & USART_ISR_TXE_TXFNF )
		{
			USART1->RQR |= USART_RQR_TXFRQ;		
      pxMBFrameCBTransmitterEmpty();			
		}
}
*/



uint8_t usbChRx;
uint8_t usbRxArr[256];
uint8_t rxUSBCnt;


#ifdef MODBUS
void USART1_IRQHandler(void)
{
    if  (USART1->ISR & USART_ISR_TXE_TXFNF) 
		{	USART1->ICR |= USART_ICR_TXFECF; 
			USART1->RQR |= USART_RQR_TXFRQ;
			if (USART1->CR1 & USART_CR1_TXEIE_TXFNFIE) 
			{
        USART1->RQR |= USART_RQR_TXFRQ;
				switch (PS_Int)
				{
          case PS_Int_BLE:
					case PS_Int_BLE_No:						
						break;
			case PS_Int_USB:
			case PS_Int_USB_No:	
						pxMBFrameCBTransmitterEmpty();
			     break;
					default:;
				}
					return;
      }	
		}
	
		if (USART1->ISR & USART_ISR_RXNE_RXFNE) 
		{ isUSBint =true;
			usbChRx=USART1->RDR;
		  usbRxArr[rxUSBCnt]=usbChRx;
		  rxUSBCnt++;//&0xff
			USART1->ICR |= USART_ICR_ORECF;
			if (USART1->CR1 & USART_CR1_RXNEIE_RXFNEIE)
			{
			//	USART1->RQR |= USART_RQR_RXFRQ;
				switch (PS_Int)
				{
					case PS_Int_BLE:
					case PS_Int_BLE_No:
						break;
					case PS_Int_USB:
					case PS_Int_USB_No:	
						if (USART1_CR1_RXNEIE_Logic)
						{
							USART1_RDR=usbChRx;
					    pxMBFrameCBByteReceived();
						}
						break;
					default:;
				}
			}
		}
}
#endif
