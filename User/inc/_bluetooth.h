#include "stm32g0xx.h"

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

void btInit(void);
void btIoPinsInit(void);
void btUartInit(void);
int txCommand(uint8_t* cmdArr,uint8_t cmdArrDim);
void rxResponse(uint8_t* buff);
void btRst(uint32_t rstDurMs,uint32_t delAfterRstMs);
uint8_t txData(uint8_t *txBuff, uint8_t txBytesNum);

