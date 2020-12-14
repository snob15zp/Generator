#include "stm32g0xx.h"

#define USART1_DIV (USART1_PCLK/USART1_BAUDRATE)
#define USART1_PCLK 64000000
#define USART1_BAUDRATE 115200
#define USB_RX_BUFF_SIZE 256

//void uart1Init(void);
//void dmaForUart1Init(uint8_t uartRxBuffAddr,uint16_t uartRxBytesNum);
void rdPage(void);
void usbCmdDetect(void);
