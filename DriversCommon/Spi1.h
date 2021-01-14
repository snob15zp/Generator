#ifndef __SPI1_H
#define __SPI1_H

#include "stm32g0xx.h"
#include "w25qxx.h"

extern void initSpi_1(void);
extern void disableSpi_1(void);

extern void spi1Receive(uint8_t *pData, uint16_t Size, uint32_t Timeout);
extern void spi1Transmit(uint8_t *pData, uint16_t Size, uint32_t Timeout);
extern void spi1TransmitReceive(uint8_t *pTxData, uint8_t *pRxData, uint16_t Size, uint32_t Timeout);
extern void spi1FifoClr(void);
void spiByteModeEnable(void);


#endif
