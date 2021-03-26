#ifndef __BOARDSETUP_H
#define __BOARDSETUP_H

#include "stm32g0xx.h" 
#include <stdint.h>
//#include "fpga.h"
//#include "gfx.h"

#define HSE_READY_DELAY   (uint32_t)1600000;
#define CLK_TICK_FOR_1MS  64000U;          

//DEFINE POWER SWITCHES PIN IO
//#define	PWR_OUTSTAGE			0				// OUT	PB0
#define	PWR_UTSTAGE_ON    GPIOB->BSRR = GPIO_BSRR_BR0
#define	PWR_UTSTAGE_OFF   GPIOB->BSRR = GPIO_BSRR_BS0

//#define	PWR_TFT			      1				// OUT	PB1
#define	PWR_TFT_ON        GPIOB->BSRR = GPIO_BSRR_BR1
#define	PWR_TFT_OFF       GPIOB->BSRR = GPIO_BSRR_BS1

//#define	PWR_GLOBAL			  2				// OUT	PB2
#define	PWR_GLOBAL_ON     GPIOB->BSRR = GPIO_BSRR_BS2
#define	PWR_GLOBAL_OFF    GPIOB->BSRR = GPIO_BSRR_BR2
 
//DEFINE TOUCHPAD PIN IO
#define	TP_IRQ						13			// EXTI	PC13

#define	TP_RST						6				// OUT	PA6  ( reset pulse width should be more than 1 ms )
#define	TP_RST_LOW        GPIOA->BSRR = GPIO_BSRR_BR6
#define	TP_RST_HI         GPIOA->BSRR = GPIO_BSRR_BS6

#define	TP_SCL					  11			// I2C2_SCL	PA11
#define	TP_SDA					  12			// I2C2_SDA	PA12


//DEFINE LCD PIN IO
#define TFT_LED					  0			// PD0 LED SIGNAL
#define TFT_LED_ON			  GPIOD->BSRR = GPIO_BSRR_BR0
#define TFT_LED_OFF			  GPIOD->BSRR = GPIO_BSRR_BS0
#define TFT_RST					  1			// PD1 RESET SIGNAL
#define TFT_RST_LOW			  GPIOD->BSRR = GPIO_BSRR_BR1	
#define TFT_RST_HI			  GPIOD->BSRR = GPIO_BSRR_BS1	
#define TFT_DC					  2			// PD2 	DATA/COMMAND SELECT   
#define TFT_DC_CMD			  GPIOD->BSRR = GPIO_BSRR_BR2	
#define TFT_DC_DATA				GPIOD->BSRR = GPIO_BSRR_BS2	
#define TFT_CS					  3			// PD3  CHIP SELECT
#define TFT_CS_LOW			  GPIOD->BSRR = GPIO_BSRR_BR3
#define TFT_CS_HI				  GPIOD->BSRR = GPIO_BSRR_BS3
#define TFT_CS_H 					GPIOD->BSRR=GPIO_BSRR_BS3
#define TFT_CS_L 					GPIOD->BSRR=GPIO_BSRR_BR3


#define TFT_SCL					  3			// PB3 	SERIAL CLOCK 
#define TFT_SCL_LOW			  GPIOB->BSRR = GPIO_BSRR_BR3	
#define TFT_SCL_HI			  GPIOB->BSRR = GPIO_BSRR_BS3	
#define TFT_MISO				  4	    // PB4 SERIAL DATA OUTPUT
#define TFT_MOSI				  5	    // PB5 SERIAL DATA INPUT
#define TFT_MOSI_LOW		  GPIOB->BSRR = GPIO_BSRR_BR5	
#define TFT_MOSI_HI			  GPIOB->BSRR = GPIO_BSRR_BS5

////DEFINE FLASH PIN IO
//#define FLASH_CS					  15	// PA15  CHIP SELECT
//#define FLASH_CS_LOW			  GPIOA->BSRR = GPIO_BSRR_BR15
//#define FLASH_CS_HI				  GPIOA->BSRR = GPIO_BSRR_BS15
//#define FLASH_MISO				  4	  // PB4 SERIAL DATA OUTPUT
//#define FLASH_MOSI				  5	  // PB5 SERIAL DATA INPUT

//for FPGA (player)
#define FPGA_Reserv_H		GPIOB->BSRR = GPIO_BSRR_BS10
#define FPGA_Reserv_L		GPIOB->BSRR = GPIO_BSRR_BR10
#define nCONFIG_H				GPIOB->BSRR = GPIO_BSRR_BS11
#define nCONFIG_L 			GPIOB->BSRR = GPIO_BSRR_BR11
#define FPGA_CS_H				GPIOB->BSRR = GPIO_BSRR_BS12
#define FPGA_CS_L				GPIOB->BSRR = GPIO_BSRR_BR12
#define FPGA_START_H 		GPIOA->BSRR = GPIO_BSRR_BS8
#define FPGA_START_L 		GPIOA->BSRR = GPIO_BSRR_BR8

#define FLASH_CS_H 		  GPIOA->BSRR = GPIO_BSRR_BS15
#define FLASH_CS_L 		  GPIOA->BSRR = GPIO_BSRR_BR15

//For Charger
#define TPSIRQ (!((GPIOA->IDR)& GPIO_IDR_ID7_Msk))

typedef uint32_t systemticks_t;

		
extern volatile   systemticks_t SystemTicks;	
extern volatile   systemticks_t BS_LastButtonPress;
extern uint16_t button_sign;		
extern uint8_t  SystemStatus;

/**
* De-initialize USART1 module
* Set usart1 registers to default values (0)
*/
void usart1_deinit();
		
//for power
extern void BoardSetup_InSleep(void);
extern void BoardSetup_OutSleep(void);
		
extern int BSInit(void);		
extern uint32_t setSystemClock(void);
extern void boardIoPinInit(void);

extern void switchDisplayInterfacePinsToPwr(FunctionalState pwrMode);
extern void switchOUTStageInterfacePinsToPwr(FunctionalState pwrMode);
extern void switchSPI1InterfacePinsToPwr(FunctionalState pwrMode);
extern void B_ACC_PinsOnOff(FunctionalState pwrMode);

extern void delayms(uint16_t tt);
extern void delay_x10ms(uint32_t tensMs);


#endif
