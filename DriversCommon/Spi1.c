#include "Spi1.h"

uint32_t txallowed = 1U;

void initSpi_1(void){
	NVIC_DisableIRQ(SPI1_IRQn); 
  RCC->APBENR2 |= RCC_APBENR2_SPI1EN;                     // enable SPI1 clk
  SPI1->CR1 &= ~SPI_CR1_SPE;                              // disable SPI1 perif
	SPI1->CR1 &= ~(
						 SPI_CR1_CPOL |															// SPI_CPOL_High
						 SPI_CR1_CPHA 													// SPI_CPHA_2Edge
						) ;                            
  SPI1->CR1 |= /*SPI_CR1_BR_2 | SPI_CR1_BR_1 |*/ SPI_CR1_BR_0 |       
               SPI_CR1_SSM |															// SPI_NSS_Soft
               SPI_CR1_SSI |
//               SPI_CR1_CPOL |															// SPI_CPOL_High
//               SPI_CR1_CPHA |															// SPI_CPHA_2Edge
               SPI_CR1_MSTR;                              // master mode select
	SPI1->CR2 |= SPI_CR2_FRXTH;															// RXNE event is generated if the FIFO level is greater than or equal to 1/4 (8-bit)
  SPI1->CR1 |= SPI_CR1_SPE;                               // enable SPI1 perif
}


/**
*			The correct disable procedure is (except when receive only mode is used):
*			1. Wait until FTLVL[1:0] = 00 (no more data to transmit).
*			2. Wait until BSY=0 (the last data frame is processed).
*			3. Disable the SPI (SPE=0).
*			4. Read data until FRLVL[1:0] = 00 (read all the received data).
*
**************************************************************************************************************************/
void disableSpi_1(void){

	while (SPI1->SR & SPI_SR_FTLVL_Msk){}										//  Wait until FTLVL[1:0] = 00 (no more data to transmit)
	while (SPI1->SR & SPI_SR_BSY){}													//	Wait until BSY=0 (the last data frame is processed)	
		
	NVIC_DisableIRQ(SPI1_IRQn);										 
  SPI1->CR1 &= ~SPI_CR1_SPE;                              // disable SPI1 perif
		
	while (SPI1->SR & SPI_SR_FRE_Msk){ SPI1->DR; }					// Read data until FRLVL[1:0] = 00 (read all the received data)				
	RCC->APBENR2 &= ~RCC_APBENR2_SPI1EN;                    // disable SPI1 clk
              
}

/*************************************************************************************************************************
*
*
**************************************************************************************************************************/
void spi1Receive(uint8_t *pData, uint16_t Size, uint32_t Timeout)
{
	uint16_t RxXferCount=Size;
	uint16_t Temp;
	for(int i=0;i<2;i++){
		Temp=SPI1->DR;
	}
	*(__IO uint8_t *)&SPI1->DR = W25QXX_DUMMY_BYTE;
	while (RxXferCount > 0U){
//		*(__IO uint8_t *)&SPI1->DR = W25QXX_DUMMY_BYTE;
		if (SPI1->SR & SPI_SR_RXNE){
			/* read the received data */
			(* (uint8_t *)pData) = SPI1->DR;
			pData += sizeof(uint8_t);
			RxXferCount--;
			if(RxXferCount>0){
				*(__IO uint8_t *)&SPI1->DR = W25QXX_DUMMY_BYTE;
			}
		}
	}
	while(SPI1->SR & SPI_SR_BSY){}
}
/*************************************************************************************************************************
*
*
**************************************************************************************************************************/

void spi1Transmit(uint8_t *pData, uint16_t Size, uint32_t Timeout)
{
	uint16_t TxXferCount=Size;
	uint16_t Temp;
	  /* Transmit data in 8 Bit mode */
	while (TxXferCount > 0U){
		/* Wait until TXE flag is set to send data */
		if (SPI1->SR & SPI_SR_TXE){
			if (TxXferCount > 1U){
				/* write on the data register in packing mode */
				SPI1->DR = *((uint16_t *)pData);
				pData += sizeof(uint16_t);
				TxXferCount -= 2U;
			}
			else{
				*(__IO uint8_t *)&SPI1->DR = *pData;
				pData++;
				TxXferCount--;
			}
		}
	}
	while(SPI1->SR & SPI_SR_BSY){}
	
	for(int i=0;i<2;i++){
		Temp=SPI1->DR;
	}
}

/*************************************************************************************************************************
*
*
**************************************************************************************************************************/

void spi1TransmitReceive(uint8_t *pTxData, uint8_t *pRxData, uint16_t Size, uint32_t Timeout)
{
	uint16_t TxXferCount=Size;
	uint16_t RxXferCount=Size;
	uint16_t Temp;
	spiByteModeEnable();
	while ((TxXferCount > 0U) || (RxXferCount > 0U)){
      /* Check TXE flag */
		if ((SPI1->SR & SPI_SR_TXE) && (TxXferCount > 0U) && (txallowed == 1U))
			{
//			if (TxXferCount > 1U){
//				SPI1->DR = *((uint16_t *)pTxData);
//				pTxData += sizeof(uint16_t);
//				TxXferCount -= 2U;
//			}
//			else
			{
				*(__IO uint8_t *)&SPI1->DR = *pTxData;
				pTxData++;
				TxXferCount--;
			}
			/* Next Data is a reception (Rx). Tx not allowed */
			txallowed = 0U;
		}
			/* Wait until RXNE flag is reset */
		if ((SPI1->SR & SPI_SR_RXNE) && (RxXferCount > 0U))
			{
//			if (RxXferCount > 1U){
//				*((uint16_t *)pRxData) = SPI1->DR;
//				pRxData += sizeof(uint16_t);
//				RxXferCount -= 2U;
//				if (RxXferCount <= 1U){
//					/* Set RX Fifo threshold before to switch on 8 bit data size */
//					spiByteModeEnable();
//				}
//			}
//		else
		{
			*((uint16_t *)pRxData) = SPI1->DR;
			pRxData++;
			RxXferCount--;
		}
		/* Next Data is a Transmission (Tx). Tx is allowed */
			txallowed = 1U;
		}
	}
}
/*************************************************************************************************************************
*
*
**************************************************************************************************************************/

void spi1FifoClr(void)
{
	uint16_t Temp;
	
	for(int i=0;i<4;i++){
		Temp=SPI1->DR;
	}
}

/*************************************************************************************************************************
*
*
**************************************************************************************************************************/
void spiByteModeEnable(void)
{
  SPI1->CR1 &= ~SPI_CR1_SPE;                              // disable SPI1 perif
  SPI1->CR2 &= ~SPI_CR2_DS_Msk;
  SPI1->CR2 |= (0x7 << SPI_CR2_DS_Pos);
  SPI1->CR2 |= SPI_CR2_FRXTH;														  // RXNE event is generated if the FIFO level is greater than or equal to 1/4 (8-bit)
  SPI1->CR1 |= SPI_CR1_SPE;                               // enable SPI1 perif

}

