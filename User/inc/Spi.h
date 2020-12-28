#ifndef __SPI_H
#define __SPI_H

#include "BoardSetup.h"

typedef 
	enum {
		SPI_FLASH,
		SPI_DISPLAY   
  } spiMode_en;

typedef 
	enum {
    INC_DISABLE,
		INC_ENABLE 
  } incMode_en;  
  

  
extern spiMode_en spiMode;

void initSpi_1(void);
extern void initSpi_2(void);
extern void spiByteModeEnable(void);
extern void spiWordModeEnable(void);
extern uint16_t displayWriteCmdByte(uint8_t cmd);
extern uint16_t displayWriteDataBytes(uint8_t* pDataByte, uint32_t len);
extern uint16_t displayWriteDataWords(uint16_t* pDataWord, uint32_t len);
extern uint16_t displayWriteOneWord(uint16_t DataWord);
extern void flashModeEnable(void);
extern void sendByteToFpga(uint8_t byte);
extern void sendByteToFlash(uint8_t byte);
	
extern void spi1Receive(uint8_t *pData, uint16_t Size, uint32_t Timeout);
extern void spi1Transmit(uint8_t *pData, uint16_t Size, uint32_t Timeout);
extern void spi1TransmitReceive(uint8_t *pTxData, uint8_t *pRxData, uint16_t Size, uint32_t Timeout);
extern void spi1FifoClr(void);

extern void spi2Transmit(uint8_t *pData, uint16_t Size);

#endif
