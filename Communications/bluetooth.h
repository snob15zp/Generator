#ifndef _bluethooth_H
#define _bluethooth_H

#include "stm32g0xx.h"
#include <stdint.h>
#include <stdbool.h>
#include "BoardSetup.h"

#define USART2_ALT_FUNC 0x01
#define USART2_DIV (USART2_PCLK/USART2_BAUDRATE)
#define USART2_PCLK 64000000U
#define USART2_BAUDRATE 115200U

#define USART2_FIFO_1_8 0x00
#define USART2_FIFO_1_4 0x01
#define USART2_FIFO_1_2 0x02
#define USART2_FIFO_3_4 0x03
#define USART2_FIFO_7_8 0x04
#define USART2_FIFO_FULL_EMPTY 0x05

#define NEW_NAME "BLE_001"

#define SET_NAME "SN,"
#define EN_COM_MODE "$$$"
#define DISCONNECT "K,1"
#define REBOOT "R,1"

extern char USART2_RDR;
extern bool USART_CR1_RXNEIE_Logic;
extern bool isBLEint;

void btInit(void);
void SLBL(void);
//void btIoPinsInit(void);
//void btUartInit(void);
//void btOn(void);
//void btOff(void);
//void setComMode(void);
//int txCommand(uint8_t* cmdArr,uint8_t cmdArrDim);
//void rxResponse(uint8_t* buff);
//void btHardRst(uint32_t rstDurMs,uint32_t delAfterRstMs);
//uint8_t uart2Tx(char *txBuff, uint8_t txBytesNum);

#endif
