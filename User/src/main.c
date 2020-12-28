#include "GlobalKey.h"

//#define debug1

#include <stdio.h>
#include <string.h>
#include "stm32g0xx.h"
#include "BoardSetup.h"

#ifdef PowerUSE
#include "board_PowerModes.h"
#endif

#ifdef LCDUSE
#include "superloopDisplay.h"
#endif

#ifdef ACCUSE
#include "superloop.h"
#endif

#ifdef COMMS
#include "SuperLoop_Comm.h"
#include "bluetooth.h"
//#include "rn4870Model.h"
#include "uart.h"
#include "flash.h"
#include "Spi.h"
#include "w25qxx.h"
#include "tim3.h"
#endif

#ifdef PLAYER
#include "fpga.h"
#include "flash.h"
#include "Spi.h"
#include "w25qxx.h"
#include "tim3.h"
#endif

#ifdef MODBUS
#include "SL_CommModbus.h"
#endif

#ifdef SPIFFS
#include <spiffs.h>
  
extern spiffs fs;

int spiffs_init();
 
#endif

#ifdef RELEASE
#define APPLICATION_ADDRESS (uint32_t)0x08001800 /**  offset start address */

void _ttywrch(int ch)
{
    (void)ch;
}

void _sys_exit(int return_code)
{
    (void) return_code;
label:  goto label;  /* endless loop */
}

void _sys_command_string(char *cmd, int len)
{
    (void) cmd;
    (void) len;
}

#endif

extern bool bVSYS;
int main(void)
{
#ifdef RELEASE
    __asm(".global __use_no_semihosting\n\t");
    SCB->VTOR = APPLICATION_ADDRESS;
    __enable_irq();
#endif    
    
#ifdef debug1	
__disable_irq();	
  RCC->IOPENR |= RCC_IOPENR_GPIOAEN |                     // enable clock for GPIO 
                 RCC_IOPENR_GPIOBEN |
                 RCC_IOPENR_GPIOCEN	|
								 RCC_IOPENR_GPIODEN;
  
  /* SETTING GPIO FOR POWER ON */
  GPIOB->BSRR = GPIO_BSRR_BS0 |                           // out Hi to PWR_ON_Outstage pin for Outstage power off                           
                GPIO_BSRR_BS1 |                           // out Hi to PWR_ON_LCD pin for DISPLAY and TOUCH power off 
                GPIO_BSRR_BS2 ;                           // out Hi to PWR_ON pin for device power ON

  GPIOB->MODER &= ~(
	                  GPIO_MODER_MODE0_1 |                  // PWR_ON_Outstage pin as output  
                    GPIO_MODER_MODE1_1 |                  // PWR_ON_LCD pin as output 
                    GPIO_MODER_MODE2_1 |                  // PWR_ON pin as output 
                    GPIO_MODER_MODE10_1                    // LED_TEST pin as output
	                                       ); 	
	
GPIOB->BSRR = GPIO_BSRR_BS10;  
#endif	
  BSInit();
#ifdef debug1	
GPIOB->BSRR = GPIO_BSRR_BS10;
__enable_irq();	
delayms(1000);
__disable_irq();
#endif


#ifdef PowerUSE
 SuperLoop_PowerModes_Init();	//must be call brefore other board functions
#endif

#ifdef ACCUSE
SuperLoopACC_init();
#else
 bVSYS = 1;
#endif

#ifdef LCDUSE
    SLD_init();
#endif

#if defined COMMS || defined PLAYER
	tim3Init();
	initSpi_1();
	SLC_init();
	SLP_init();
    __flashInit();
#endif	

#ifdef SPIFFS
spiffs_init();
#endif


#ifdef 	COMMS 
SLC_init();
#endif

#ifdef 	PLAYER 
SLP_init();
#endif

#ifdef MODBUS
SL_CommModbusInit();
#endif


//test flash
//uint8_t temp_pBuffer;
//W25qxx_EraseSector(0);
//W25qxx_WriteByte(0x55, 0);
//W25qxx_ReadByte(&temp_pBuffer, 0);
//end test flash

//debug FPGA config
//fpgaConfig();
//fpgaConfig();
//end debug FPGA config	
	PM_OnOffPWR_Pl(false);	
  while(1){

#ifdef 	COMMS 
SLC();
#endif

#ifdef 	PLAYER 
SLP();
#endif
		
#ifdef LCDUSE
SLD();

#endif
			
//GPIOB->ODR ^= GPIO_ODR_OD10; 
//GPIOB->BSRR = GPIO_BSRR_BS10;		
			
#ifdef ACCUSE
SuperLoopACC();
#endif	
			
//GPIOB->BSRR = GPIO_BSRR_BR10;	
			
#ifdef PowerUSE
SuperLoop_PowerModes();			
#endif			

#ifdef MODBUS
SL_CommModbus();
#endif
		
  }
}

