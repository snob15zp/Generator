#include "Spi1.h"

uint32_t txallowed = 1U;
extern uint8_t spiDispCapture;


void initSpi_1(void){
    NVIC_DisableIRQ(SPI1_IRQn); 
    RCC->APBENR2 |= RCC_APBENR2_SPI1EN;                     // enable SPI1 clk
  
    SPI1->CR1 = 0x0000;        
    SPI1->CR2 = 0x0000;        
    SPI1->CR1 |= SPI_CR1_MSTR | SPI_CR1_SSM | SPI_CR1_SSI | SPI_CR1_BR_0 | SPI_CR1_BR_1 | SPI_CR1_BR_2   ;     
    SPI1->CR2 |= SPI_CR2_DS_0| SPI_CR2_DS_1|SPI_CR2_DS_2| SPI_CR2_FRXTH;    
    
    SPI1->CR1 |= SPI_CR1_SPE;                               // enable SPI1 perif
}


static void delay_us(uint32_t i)
{
    // 255 -> 52.917us
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
    return *(__IO uint8_t *) (&SPI1->DR);
}

void spi_cs_on()
{
    while(spiDispCapture)
    {
        gfxYield();
    }
    
    GPIOA->BSRR = GPIO_BSRR_BR15; //GPIOD->BSRR = GPIO_BSRR_BR3
}

void spi_cs_off()
{ 
    while (SPI1->SR & SPI_SR_BSY); 
    GPIOA->BSRR = GPIO_BSRR_BS15; //GPIOD->BSRR = GPIO_BSRR_BS3
    delay_us(5);
}



