#include "Spi1.h"
#include "gfx.h"
#include "GlobalKey.h"

uint32_t txallowed = 1U;
extern uint8_t spiDispCapture;


void initSpi_1(void){
    NVIC_DisableIRQ(SPI1_IRQn); 
    RCC->APBENR2 |= RCC_APBENR2_SPI1EN;                     // enable SPI1 clk
  
    SPI1->CR1 = 0x0000;        
    SPI1->CR2 = 0x0000;        
    SPI1->CR1 |= SPI_CR1_MSTR | SPI_CR1_SSM | SPI_CR1_SSI ;// SPI_CR1_BR_0 | SPI_CR1_BR_1 | SPI_CR1_BR_2   ;     
    SPI1->CR2 |= SPI_CR2_DS_0| SPI_CR2_DS_1|SPI_CR2_DS_2| SPI_CR2_FRXTH;    
    
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
static void delay_us(uint32_t i)
{
    // 255 -> 52.917us ~ 4.81
    volatile uint32_t j = i * 5;
    while(j)
    {
        j--;
    }
}

uint8_t spi_transfer(uint8_t data)
{
    if(SPI1->SR & SPI_SR_OVR)
    {
        (void)SPI1->DR;
    }
    
    while (!(SPI1->SR & SPI_SR_TXE));     
    *(__IO uint8_t *) (&SPI1->DR) = data;    
    while (!(SPI1->SR & SPI_SR_RXNE));     
    uint8_t val = *(__IO uint8_t *) (&SPI1->DR);
    while (SPI1->SR & SPI_SR_BSY); 
    
    for(uint8_t i = 0; i< 4; i++)
    {
        (void)SPI1->DR;
    }
    
    return val;
}

void spi_cs_on()
{  
	while (SPI1->SR & SPI_SR_FTLVL_Msk){}										//  Wait until FTLVL[1:0] = 00 (no more data to transmit)
	while (SPI1->SR & SPI_SR_BSY){}
    while(spiDispCapture)
    {
        Error("spi_cs_on spiDispCapture");
        gfxYield();
    }
    
    GPIOA->BSRR = GPIO_BSRR_BR15; //GPIOD->BSRR = GPIO_BSRR_BR3
}

void spi_cs_off()
{ 
    while (SPI1->SR & SPI_SR_BSY); 
    GPIOA->BSRR = GPIO_BSRR_BS15; //GPIOD->BSRR = GPIO_BSRR_BS3
    while(SPI1->SR & SPI_SR_FRLVL)
    {
        (void)SPI1->DR;
    }
    delay_us(25);
}



