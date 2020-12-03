/*
Setting up shared resources that are used by multiple software modules
*/
#ifdef STM32G070xx   //STM32G070CBTx
#include "stm32g0xx.h"
#else
#include <stm32g030xx.h>
#endif

#include "BoardSetup.h"
#include "fpga.h"


/*************************************************************************************************************************
*
*                                             Board init 
*
**************************************************************************************************************************/

uint8_t SystemStatus;

volatile systemticks_t SystemTicks;

int BSInit(void)
{
	SystemCoreClock = setSystemClock(); 
  SysTick_Config(SystemCoreClock/1000); // for uGFX
	
	
  boardIoPinInit();


	return 0;
};

uint16_t button_sign;

//******************************************** for Display period= 1 ms ***************************************************

void SysTick_Handler(void) 
{
  static uint32_t ledTick = 0;
	static uint16_t ss;
	static uint8_t status=0;	
	static uint32_t button_new; 
	static uint16_t	button_old, button_stable_new, button_stable_old;
	
	SystemTicks++;
//	ledTick++;
//  if (ledTick >= 200)   
//		{ledTick = 0;
//		 switch (	status )   // Led Light according to SystemStatus
//		 {
//			 case 0: ss++;
//               GPIOB->ODR ^= GPIO_ODR_OD10;
//			         if (ss>=2*(SystemStatus+1))
//							 {ss=0;status=1;};
//							 break;
//			 case 1: ss++;
//               //GPIOB->ODR ^= GPIO_ODR_OD10;
//			         if (ss>=4)
//							 {ss=0;status=0;};
//							 break;
//       default: status=0;
//		 };       
//    }
  button_new =(GPIOA->IDR)& GPIO_IDR_ID5_Msk ;
	if  (((uint16_t)button_new)==button_old) 
		    button_stable_new =button_new;
	button_old=button_new;
	button_sign|=(~button_stable_old)&button_stable_new;
	button_stable_old=button_stable_new;
}
/*************************************************************************************************************************
*
*                                   Delays common
*
**************************************************************************************************************************/
void delayms(uint16_t count)// delays count  -0/+1 ms
	{
	 systemticks_t lt;	 
   lt=SystemTicks;                                                                                
   while ((SystemTicks-lt)<=count);                                                                                  
  }

/*************************************************************************************************************************
*
*                                   For uGFX Delays    
*	
**************************************************************************************************************************/

systemticks_t gfxSystemTicks(void)
{
	return SystemTicks;
}

systemticks_t gfxMillisecondsToTicks(delaytime_t ms)
{
	return ms;
}




/*************************************************************************************************************************
*
*                                          Setup PLL and system clocks
*
**************************************************************************************************************************/
uint32_t setSystemClock(void){
  uint32_t waitCycle = HSE_READY_DELAY;

  RCC->CR &= ~RCC_CR_PLLON;                                   // Disable the PLL by setting PLLON to 0
  while(RCC->CR & RCC_CR_PLLRDY){}                            // Wait until PLLRDY is cleared. The PLL is now fully stopped
    
  RCC->CR |= RCC_CR_HSEON;
  while (!(RCC->CR & RCC_CR_HSERDY) && (waitCycle > 0)){
    waitCycle--;
  }

  if (waitCycle){
    RCC->PLLCFGR |= RCC_PLLCFGR_PLLR_0 |                      // sets the PLL VCO division factor R to 2  
                    RCC_PLLCFGR_PLLP_0 |                      // sets the PLL VCO division factor P to 2
                    RCC_PLLCFGR_PLLN_4 |                      // sets the PLL frequency multiplication factor N to 16
                    RCC_PLLCFGR_PLLSRC_HSE;
  } else {
      RCC->PLLCFGR |= RCC_PLLCFGR_PLLR_0 |                    // sets the PLL VCO division factor R to 2  
                      RCC_PLLCFGR_PLLP_0 |                    // sets the PLL VCO division factor P to 2
                      RCC_PLLCFGR_PLLN_3 |                    // sets the PLL frequency multiplication factor N to 16
                      RCC_PLLCFGR_PLLSRC_HSI;
  }
    
  RCC->CR |= RCC_CR_PLLON;                                    // Enable the PLL again by setting PLLON to 1
  while(!(RCC->CR & RCC_CR_PLLRDY)){}                         // Wait until PLLRDY not is set  
  RCC->PLLCFGR |= RCC_PLLCFGR_PLLREN;                         // Enable the desired PLL outputs by configuring PLLPEN and PLLREN
                                                                                      
  FLASH->ACR |= FLASH_ACR_LATENCY_1;                          // set FLASH read access latency to 3 HCLK cycles
  
  RCC->CFGR &= ~(RCC_CFGR_HPRE_Msk | RCC_CFGR_PPRE_Msk);      // set AHB and APB prescalers to 1
  
  RCC->CFGR |= RCC_CFGR_SW_1;                                 // selects the clock for SYSCLK as PLLRCLK
  RCC->IOPENR |= RCC_IOPENR_GPIOAEN |
                 RCC_IOPENR_GPIOBEN |
                 RCC_IOPENR_GPIOCEN;
      
  return 64000000;
}

/*************************************************************************************************************************
*
*                                            
*
**************************************************************************************************************************/
void boardIoPinInit(void){
  
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
  //i2c1
  GPIOB->OTYPER |= GPIO_OTYPER_OT9 | GPIO_OTYPER_OT8 |    // output open-drain for PB6 .. PB9
                   GPIO_OTYPER_OT7 | GPIO_OTYPER_OT6;
  GPIOB->OSPEEDR |= GPIO_OSPEEDR_OSPEED9_1 |
                    GPIO_OSPEEDR_OSPEED8_1 |
                    GPIO_OSPEEDR_OSPEED7_1 |
										GPIO_OSPEEDR_OSPEED6_1;		            // switch PB6 .. PB9 to High speed mode   
	
	GPIOB->AFR[0] &= ~(GPIO_AFRL_AFSEL6_Msk |           	// set PB6 as I2C2_SCL
                     GPIO_AFRL_AFSEL7_Msk);            	// set PB7 as I2C2_SDA 
	GPIOB->AFR[0] |= 6 << GPIO_AFRL_AFSEL6_Pos |           	// set PB6 as I2C2_SCL
                   6 << GPIO_AFRL_AFSEL7_Pos;
	GPIOB->AFR[1] = 6 << GPIO_AFRH_AFSEL8_Pos |           	// set PB8 as I2C2_SCL
                  6 << GPIO_AFRH_AFSEL9_Pos;            	// set PB9 as I2C2_SDA 
  
  GPIOB->OSPEEDR |= GPIO_OSPEEDR_OSPEED5_Msk | 
                    GPIO_OSPEEDR_OSPEED5_Msk;
										
	/*                   button                  */									
	GPIOA->MODER &= ~(GPIO_MODER_MODE5_Msk);                  // input 
  //GPIOA->IDR;
  /* SETTING GPIO FOR TOUCHPAD */ 
	//i2c2
  GPIOA->BSRR = GPIO_BSRR_BR12 | GPIO_BSRR_BR11;          // out LOW to PA11, PA12
  GPIOA->OTYPER |= GPIO_OTYPER_OT11 | GPIO_OTYPER_OT12;   // output open-drain for PA11, PA12
  GPIOA->OSPEEDR |= GPIO_OSPEEDR_OSPEED12_1 |
										GPIO_OSPEEDR_OSPEED11_1;		          // switch PA11, PA12 to High speed mode  
	GPIOA->AFR[1] |= 6 << GPIO_AFRH_AFSEL11_Pos |           // set PA11 as I2C2_SCL
                   6 << GPIO_AFRH_AFSEL12_Pos;            // set PA12 as I2C2_SDA  										
  //TP reset                 
  GPIOA->MODER &= ~GPIO_MODER_MODE6_1;							      // set TP_RST pin PA6 as output push-pull
   
//  GPIOC->MODER &= ~(GPIO_MODER_MODE13_Msk);						    // set TP_IRQ pin PC13 as Input mode	  
//  EXTI->EXTICR[3] = EXTI_EXTICR4_EXTI13_1;                // select PC13 for EXTI
//  EXTI->RTSR1 |= EXTI_RTSR1_RT13;                         // set EXTI13 trigger to rising front

  /* SETTING GPIO FOR DISPLAY */ 
  TFT_LED_OFF;
  GPIOD->MODER &= ~(GPIO_MODER_MODE0_1 |                  // TFT_LED pin as output  
                    GPIO_MODER_MODE1_1 |                  // TFT_RST pin as output 
                    GPIO_MODER_MODE2_1 |                  // TFT_DC pin as output 
                    GPIO_MODER_MODE3_1);                  // TFT_CS pin as output
                    
  GPIOB->MODER &= ~(GPIO_MODER_MODE3_1 |                  // TFT_SCL pin as output  
                    GPIO_MODER_MODE5_1);                  // TFT_MOSI pin as output
                    
  GPIOB->OSPEEDR |=  GPIO_OSPEEDR_OSPEED5_1 |
										 GPIO_OSPEEDR_OSPEED3_1;		          // switch PB3, PB5 to High speed mode 
  switchDisplayInterfacePinsToPwr(DISABLE);										 
  //SPI
  switchSPI1InterfacePinsToPwr(DISABLE);
	
	/* SETTING GPIO FOR FLASH */ 
	GPIOB->MODER &= ~(GPIO_MODER_MODE3_0 | GPIO_MODER_MODE3_1 |
										GPIO_MODER_MODE4_0 | GPIO_MODER_MODE4_1 |
										GPIO_MODER_MODE5_0 | GPIO_MODER_MODE5_1);
										
	GPIOA->MODER &= ~(GPIO_MODER_MODE15_0 | GPIO_MODER_MODE15_1);
	
	GPIOB->MODER |= GPIO_MODER_MODE3_1 |
									GPIO_MODER_MODE4_1 |
									GPIO_MODER_MODE5_1;
									
	GPIOB->AFR[0]&=~( (GPIO_AFRL_AFSEL3_Msk) |										//PB3 - SPI1 SCK
									  (GPIO_AFRL_AFSEL3_Msk) |										//PB4 - SPI1 MISO
									  (GPIO_AFRL_AFSEL3_Msk)										//PB5 - SPI1 MOSI	
                  );										
	GPIOB->AFR[0] |= (0<<GPIO_AFRL_AFSEL3_Pos) |										//PB3 - SPI1 SCK
									(0<<GPIO_AFRL_AFSEL4_Pos) |										//PB4 - SPI1 MISO
									(0<<GPIO_AFRL_AFSEL5_Pos);										//PB5 - SPI1 MOSI
									
	GPIOA->MODER |= GPIO_MODER_MODE15_0;	//PA15 - output CS
	
	GPIOD->MODER &= ~(GPIO_MODER_MODE3_0 | GPIO_MODER_MODE3_1);		//for debug
	GPIOD->MODER |= GPIO_MODER_MODE3_0;														//for debug
	TFT_CS_HI;																										//for debug
	
	/* SETTING GPIO FOR FPGA */
	GPIOB->MODER &= ~(GPIO_MODER_MODE12_0 | GPIO_MODER_MODE12_1 |	
										GPIO_MODER_MODE13_0 | GPIO_MODER_MODE13_1 |
										GPIO_MODER_MODE14_0 | GPIO_MODER_MODE14_1 |
										GPIO_MODER_MODE15_0 | GPIO_MODER_MODE15_1 |
										GPIO_MODER_MODE11_0 | GPIO_MODER_MODE11_1);
	GPIOB->MODER |= GPIO_MODER_MODE13_1 |
									GPIO_MODER_MODE14_1 |
									GPIO_MODER_MODE15_1;				
	GPIOB->AFR[1] |= (0<<GPIO_AFRH_AFSEL13_Pos)|										//PB13 - SPI2 SCK
									 (0<<GPIO_AFRH_AFSEL14_Pos)|										//PB14 - SPI2 MISO
									 (0<<GPIO_AFRH_AFSEL15_Pos);										//PB15 - SPI2 MOSI
	GPIOB->MODER |= GPIO_MODER_MODE12_0 |														//PB12 - output CS
									GPIO_MODER_MODE11_0;														//PB11 - output nCONFIG
	FPGA_CS_H;
	nCONFIG_L;
									 
	GPIOA->MODER &= ~(GPIO_MODER_MODE8_0 | GPIO_MODER_MODE8_1);
	GPIOA->MODER |= GPIO_MODER_MODE8_0;															//PA8 - output FPGA_START
	
	GPIOC->MODER &= ~(GPIO_MODER_MODE6_0 | GPIO_MODER_MODE6_1 |			//PC6 - input CONF_DONE
										GPIO_MODER_MODE7_0 | GPIO_MODER_MODE7_1);			//PC7 - input nSTATUS						
	
	/* SETTING GPIO FOR USB */
	GPIOA->MODER &= ~(GPIO_MODER_MODE9_0 |  GPIO_MODER_MODE9_1 |
										GPIO_MODER_MODE10_0 |  GPIO_MODER_MODE10_1);
	GPIOA->MODER |= GPIO_MODER_MODE9_1 | GPIO_MODER_MODE10_1;
	
	GPIOA->PUPDR &= ~(GPIO_PUPDR_PUPD10_0 | GPIO_PUPDR_PUPD10_1);

	GPIOA->OSPEEDR |= GPIO_OSPEEDR_OSPEED9_0 | GPIO_OSPEEDR_OSPEED9_1;
	
	GPIOA->AFR[1] = (0x01<<GPIO_AFRH_AFSEL9_Pos) | 
									(0x01<<GPIO_AFRH_AFSEL10_Pos);
									
  
}

/*************************************************************************************************************************
*
*
**************************************************************************************************************************/
void switchDisplayInterfacePinsToPwr(FunctionalState pwrMode){
// Displey 7 lines
// CTP 3 lines	
  uint32_t tmp = GPIOA->MODER & ~(GPIO_MODER_MODE12_Msk | GPIO_MODER_MODE11_Msk); // clear PA11, PA12 mode bits //PA11 PA12 i2c2 CTP
 
  if (pwrMode == DISABLE){                                          // if mode is DISABLE 
    
//    EXTI->IMR1 &= EXTI_IMR1_IM13;                                   // EXTI13 interrupt masked                     //PC13  CTP_IRQ ??? - not used
//    EXTI->RPR1 = EXTI_RPR1_RPIF13;                                  // clear EXTI13 interrupt flag
//    NVIC_DisableIRQ(EXTI4_15_IRQn);                                 // disable EXTI4_15 interrupt
 
    GPIOA->BSRR = GPIO_BSRR_BR12 | GPIO_BSRR_BR11;                  // out LOW to PA11, PA12                          // PA11 PA12 i2c2 CTP
    tmp |= (GPIO_MODER_MODE11_0 | GPIO_MODER_MODE12_0);             // TP I2C pins set as general purpose output mode // PA11 PA12  i2c2  CTP
    GPIOA->MODER = tmp;    
                                                                                                                      
//    GPIOB->MODER |= (GPIO_MODER_MODE5_Msk |                         // PB3..PB5 switch to analog mode                 //PB3..PB5 SPI1 Flash+Displ
//                     GPIO_MODER_MODE4_Msk |    
//                     GPIO_MODER_MODE3_Msk);  
    
    GPIOD->MODER |= (GPIO_MODER_MODE3_Msk | GPIO_MODER_MODE2_Msk |  // PD0..PD3 mode bits switch to analog mode       //PD0 PD1 PD2 PD3 TFT_LED TFT_RST TFT_D/I TFT_CS
                    GPIO_MODER_MODE1_Msk | GPIO_MODER_MODE0_Msk);    
	/*                   button                  */									
	GPIOA->MODER &= ~(GPIO_MODER_MODE5_Msk);                  // input 
 
    
  } else { 
    
    tmp |= (GPIO_MODER_MODE11_1 |                                   // PA11 alternate function I2C2_SCL
            GPIO_MODER_MODE12_1);                                   // PA12 alternate function I2C2_SDA 
    
    GPIOA->MODER = tmp;
        
//    GPIOB->MODER &= ~(GPIO_MODER_MODE5_0 |                          // PB5 alternate function SPI1_MOSI 
//                     GPIO_MODER_MODE4_0 |                           // PB4 alternate function SPI1_MISO
//                     GPIO_MODER_MODE3_0);                           // PB3 alternate function SPI1_SCK
 
    GPIOD->BSRR = GPIO_BSRR_BR3 | GPIO_BSRR_BR2 |                   // out LOW to the PD0..PD3 pins
                  GPIO_BSRR_BR1 | GPIO_BSRR_BR0;
    GPIOD->MODER &= ~(GPIO_MODER_MODE3_1 | GPIO_MODER_MODE2_1 |
                    GPIO_MODER_MODE1_1 | GPIO_MODER_MODE0_1);       // PD0..PD3 switch to output mode  
			/*                   button                  */									
	GPIOA->MODER &= ~(GPIO_MODER_MODE5_Msk);                  // input 

    
//    EXTI->IMR1 |= EXTI_IMR1_IM13;                                   // EXTI13 interrupt unmasked
//    EXTI->RPR1 = EXTI_RPR1_RPIF13;                                  // clear EXTI13 interrupt flag
//    NVIC_EnableIRQ(EXTI4_15_IRQn);
  }   
 TFT_LED_OFF;                      
}

void switchSPI1InterfacePinsToPwr(FunctionalState pwrMode)
{
  if (pwrMode == DISABLE){                                          // if mode is DISABLE 
    GPIOB->MODER |= (GPIO_MODER_MODE5_Msk |                         // PB3..PB5 switch to analog mode                 //PB3..PB5 SPI1 Flash+Displ
                     GPIO_MODER_MODE4_Msk |    
                     GPIO_MODER_MODE3_Msk);  
    
   } else { 
    GPIOB->MODER &= ~(GPIO_MODER_MODE5_0 |                          // PB5 alternate function SPI1_MOSI 
                     GPIO_MODER_MODE4_0 |                           // PB4 alternate function SPI1_MISO
                     GPIO_MODER_MODE3_0);                           // PB3 alternate function SPI1_SCK
 
  }   
                    
}

void switchOUTStageInterfacePinsToPwr(FunctionalState pwrMode)
{
};