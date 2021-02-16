#ifndef __SPI1_H
#define __SPI1_H

#include "stm32g0xx.h"
#include "stdint.h"


void initSpi_1(void);

/**
* Initialize SPI1
*/
//void spi1_init(void);

/**
* Transfer byte via SPI1
* @param data Input byte
* @return Output byte
*/
uint8_t spi_transfer(uint8_t data);
extern void disableSpi_1(void);

/**
* Enable SPI-Flash CS (Set low level at PB3)
*/
void spi_cs_on(); 

/**
* Disable SPI-Flash CS (Set high level at PB3)
*/
void spi_cs_off();

#endif
