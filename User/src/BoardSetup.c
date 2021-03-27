/*
Setting up shared resources that are used by multiple software modules
*/

#include "stm32g0xx.h"
#include "BoardSetup.h"


//for power
void BoardSetup_InSleep(void)
{
	RCC->CR  &= ~(RCC_CR_HSION);                                  // OFF HSI
    while((RCC->CR & RCC_CR_HSIRDY)){};
		
	__DMB();
	SysTick->CTRL &= ~SysTick_CTRL_TICKINT_Msk;
	__DMB();
};

void BoardSetup_OutSleep(void)
{
	 SysTick->CTRL |= SysTick_CTRL_TICKINT_Msk;
	 BS_LastButtonPress = SystemTicks;
};


/*************************************************************************************************************************
*
*                                             Board init 
*
**************************************************************************************************************************/

uint8_t SystemStatus;

volatile systemticks_t SystemTicks;
volatile systemticks_t BS_LastButtonPress;

int BSInit(void)
{
  SystemCoreClock = setSystemClock(); 
  SysTick_Config(SystemCoreClock/1000); // for uGFX
	
	
  boardIoPinInit();
  return 0;
};

uint16_t button_sign;
extern uint32_t btCurTime;

//******************************************** for Display period= 1 ms ***************************************************
 
void SysTick_Handler(void) 
{
    static uint32_t btn_interval;
    SystemTicks++;
    
    if((GPIOA->IDR & GPIO_IDR_ID5_Msk) == GPIO_IDR_ID5_Msk) //button is pressed
    {
        BS_LastButtonPress = SystemTicks;
        btn_interval ++;
        
        if(btn_interval == 2000)
        {
            button_sign = 1;
        }
        
        if (btn_interval == 5000)
        {
            NVIC_SystemReset();
        }
    }
    else    // button released
    {
        btn_interval = 0;
    }
}
/*************************************************************************************************************************
*
*                                   Delays common
*
**************************************************************************************************************************/
void delayms(uint16_t count)// delays count  -0/+1 ms
{
   systemticks_t lt = SystemTicks;                                                                                
   while ((SystemTicks - lt) <= count);                                                                                  
}

/*************************************************************************************************************************
*
*                                          Setup PLL and system clocks
*
**************************************************************************************************************************/
uint32_t setSystemClock(void)
{
  uint32_t waitCycle = HSE_READY_DELAY;	
	RCC->CR |= (RCC_CR_HSION);                                  // ON HSI
  while(!(RCC->CR & (RCC_CR_HSIRDY))){};
	RCC->CFGR&=~(RCC_CFGR_SW_Msk);                           // togle on HSI
	while((RCC->CFGR & RCC_CFGR_SWS_Msk)){};  
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
	GPIOB->AFR[1] &= ~(GPIO_AFRH_AFSEL8_Msk |           	// set PB8 as I2C2_SCL
                     GPIO_AFRH_AFSEL9_Msk);            	// set PB9 as I2C2_SDA 
  GPIOB->AFR[1] = 6 << GPIO_AFRH_AFSEL8_Pos |           	// set PB8 as I2C2_SCL
                  6 << GPIO_AFRH_AFSEL9_Pos;            	// set PB9 as I2C2_SDA 
  
  GPIOB->OSPEEDR |= GPIO_OSPEEDR_OSPEED5_Msk | 
                    GPIO_OSPEEDR_OSPEED5_Msk;
										
	/*                   button                  */									
	GPIOA->MODER &= ~(GPIO_MODER_MODE5_Msk);                  // input 
	/*                   TPS                       */
	GPIOA->MODER &= ~(GPIO_MODER_MODE7_Msk);                  // input
  //GPIOA->IDR;
  /* SETTING GPIO FOR TOUCHPAD */ 
	//i2c2
  GPIOA->BSRR = GPIO_BSRR_BR12 | GPIO_BSRR_BR11;          // out LOW to PA11, PA12
  GPIOA->OTYPER |= GPIO_OTYPER_OT11 | GPIO_OTYPER_OT12;   // output open-drain for PA11, PA12  //1 bit
 
  GPIOA->OSPEEDR &= ~(GPIO_OSPEEDR_OSPEED12_Msk |
										GPIO_OSPEEDR_OSPEED11_Msk);
  GPIOA->OSPEEDR |= GPIO_OSPEEDR_OSPEED12_1 |
										GPIO_OSPEEDR_OSPEED11_1;		          // switch PA11, PA12 to High speed mode   //2 bits

  GPIOA->AFR[1] &= ~(GPIO_AFRH_AFSEL11_Msk |           // set PA11 as I2C2_SCL
                   GPIO_AFRH_AFSEL12_Msk);            // set PA12 as I2C2_SDA  										
  GPIOA->AFR[1] |= 6 << GPIO_AFRH_AFSEL11_Pos |           // set PA11 as I2C2_SCL
                   6 << GPIO_AFRH_AFSEL12_Pos;            // set PA12 as I2C2_SDA  
									 
  //TP reset 
	GPIOA->MODER |= GPIO_MODER_MODE6_Msk;	//2 bits, Analog
  GPIOA->MODER &= ~GPIO_MODER_MODE6_1;							      // set TP_RST pin PA6 as output push-pull //2 bits
   
//  GPIOC->MODER &= ~(GPIO_MODER_MODE13_Msk);						    // set TP_IRQ pin PC13 as Input mode	  
//  EXTI->EXTICR[3] = EXTI_EXTICR4_EXTI13_1;                // select PC13 for EXTI
//  EXTI->RTSR1 |= EXTI_RTSR1_RT13;                         // set EXTI13 trigger to rising front

  /* SETTING GPIO FOR DISPLAY */ 
  TFT_LED_OFF;
	GPIOD->MODER |= (GPIO_MODER_MODE0_Msk |     // 2 bits 
                    GPIO_MODER_MODE1_Msk |    // Analog 
                    GPIO_MODER_MODE2_Msk |                   
                    GPIO_MODER_MODE3_Msk);
  GPIOD->MODER &= ~(GPIO_MODER_MODE0_1 |                  // TFT_LED pin as output  
                    GPIO_MODER_MODE1_1 |                  // TFT_RST pin as output 
                    GPIO_MODER_MODE2_1 |                  // TFT_DC pin as output 
                    GPIO_MODER_MODE3_1);                  // TFT_CS pin as output
  
  GPIOB->MODER |= (GPIO_MODER_MODE3_Msk |  // 2 bits                 
                    GPIO_MODER_MODE5_Msk); // Analog
  GPIOB->MODER &= ~(GPIO_MODER_MODE3_1 |                  // TFT_SCL pin as output  
                    GPIO_MODER_MODE5_1);                  // TFT_MOSI pin as output
                    
  GPIOB->OSPEEDR |=  GPIO_OSPEEDR_OSPEED5_0 | GPIO_OSPEEDR_OSPEED5_1 | 
                     GPIO_OSPEEDR_OSPEED4_0 | GPIO_OSPEEDR_OSPEED4_1 | 
                     GPIO_OSPEEDR_OSPEED3_0 | GPIO_OSPEEDR_OSPEED3_1;		          // switch PB3, PB5 to High speed mode 
										 
										 
  switchDisplayInterfacePinsToPwr(DISABLE);										 
  //SPI
  switchSPI1InterfacePinsToPwr(DISABLE);
	
	/* SETTING GPIO FOR FLASH */ 
	GPIOB->MODER &= ~(GPIO_MODER_MODE3_Msk|   //B3 !  //2 bits
										GPIO_MODER_MODE4_Msk|   //B4 !
										GPIO_MODER_MODE5_Msk);  //B5 !
										
	GPIOA->MODER &= ~(GPIO_MODER_MODE15_Msk); //A15 ! 
	
	GPIOB->MODER |= GPIO_MODER_MODE3_1 | //B3 ?
									GPIO_MODER_MODE4_1 | //B4?
									GPIO_MODER_MODE5_1;  //B5?
									
	GPIOB->AFR[0]&=~(GPIO_AFRL_AFSEL3_Msk |									  	//PB3 - SPI1 SCK
									  GPIO_AFRL_AFSEL3_Msk |										//PB4 - SPI1 MISO
									  GPIO_AFRL_AFSEL3_Msk);										//PB5 - SPI1 MOSI											
	GPIOB->AFR[0] |=(0<<GPIO_AFRL_AFSEL3_Pos) |								//PB3 - SPI1 SCK
									(0<<GPIO_AFRL_AFSEL4_Pos) |										//PB4 - SPI1 MISO
									(0<<GPIO_AFRL_AFSEL5_Pos);										//PB5 - SPI1 MOSI
									
	GPIOA->MODER |= GPIO_MODER_MODE15_0;	//PA15 ?- output FLASH CS
	
	GPIOD->MODER &= ~(GPIO_MODER_MODE3_Msk);		//for debug
	GPIOD->MODER |= GPIO_MODER_MODE3_0;														//for debug
	TFT_CS_HI;																										//for debug
	
	/* SETTING GPIO FOR FPGA */
	GPIOB->MODER &= ~(GPIO_MODER_MODE12_Msk |	//input
										GPIO_MODER_MODE13_Msk | // MODER 12 13 14 15 11
										GPIO_MODER_MODE14_Msk |
										GPIO_MODER_MODE15_Msk |
										GPIO_MODER_MODE11_Msk);
	GPIOB->MODER |= GPIO_MODER_MODE13_1 |      // MODER 13 14 15
									GPIO_MODER_MODE14_1 |
									GPIO_MODER_MODE15_1;			

  GPIOB->AFR[1]&= ~(GPIO_AFRH_AFSEL13_Msk|			//4 bits  	//PB13 - SPI2 SCK
									 GPIO_AFRH_AFSEL14_Msk|				//AF0				//PB14 - SPI2 MISO
									 GPIO_AFRH_AFSEL15_Msk);
	GPIOB->AFR[1] |= (0<<GPIO_AFRH_AFSEL13_Pos)|	//double					//PB13 - SPI2 SCK
									 (0<<GPIO_AFRH_AFSEL14_Pos)|										//PB14 - SPI2 MISO
									 (0<<GPIO_AFRH_AFSEL15_Pos);	
									 //PB15 - SPI2 MOSI
									 
	GPIOB->MODER |= GPIO_MODER_MODE12_0 |		// MODER 12   11				//PB12 - output CS
									GPIO_MODER_MODE11_0;														//PB11 - output nCONFIG
	FPGA_CS_H;
	nCONFIG_L;
									 
	GPIOA->MODER &= ~(GPIO_MODER_MODE8_Msk);// input
	GPIOA->MODER |= GPIO_MODER_MODE8_0;//2 bit											//PA8 - output FPGA_START
	
	GPIOC->MODER &= ~(GPIO_MODER_MODE6_Msk  |			                  //PC6 - input CONF_DONE
										GPIO_MODER_MODE7_Msk);		                  	//PC7 - input nSTATUS						
	
	/* SETTING GPIO FOR USB */
	GPIOA->MODER &= ~(GPIO_MODER_MODE9_Msk|GPIO_MODER_MODE10_Msk); //input mode
	GPIOA->MODER |= GPIO_MODER_MODE9_1 | GPIO_MODER_MODE10_1;//2bits
	
  GPIOA->PUPDR |=  (GPIO_PUPDR_PUPD10_Msk | GPIO_PUPDR_PUPD10_Msk);//2bits
  GPIOA->PUPDR &= ~(GPIO_PUPDR_PUPD10_0 | GPIO_PUPDR_PUPD10_1);//2bits

  GPIOA->OSPEEDR&= ~(GPIO_OSPEEDR_OSPEED9_Msk | GPIO_OSPEEDR_OSPEED9_Msk);
	GPIOA->OSPEEDR |= GPIO_OSPEEDR_OSPEED9_0 | GPIO_OSPEEDR_OSPEED9_1; //2 bits
	
	GPIOA->AFR[1]  &= ~(GPIO_AFRH_AFSEL9_Msk) |  //4 bits
									(GPIO_AFRH_AFSEL10_Msk);
	GPIOA->AFR[1] |= (0x01<<GPIO_AFRH_AFSEL9_Pos) | 
									(0x01<<GPIO_AFRH_AFSEL10_Pos);
									
  
}

void usart1_deinit()
{
    USART1->CR1 = 0;
    USART1->CR2 = 0;
    USART1->CR3 = 0;
    USART1->BRR = 0;
    USART1->GTPR = 0;
    USART1->RTOR = 0;
    USART1->RQR = 0;
    //isr skiped
    USART1->ICR = 0;
    USART1->RDR = 0;
    USART1->TDR = 0;
    USART1->PRESC = 0;
}

/**
***********************************************************************************************************************
*RST				PD1 	39	A	
*LED_PWM		PD0		38	A
*SPI_D_C		PD2		40	A
TFT_CS			PD3		41	A
SPI1_CLK		PB3		42	A
SPI1_MISO		PB4		43	A
SPI1_MOSI		PB5		44	A
CTP_IRQ			PC13	1		A
CTP_RST			PA6		17	O
I2C2_SCL		PA11	33	O
I2CI_SDA		PA12	34	O

**************************************************************************************************************************/
void switchDisplayInterfacePinsToPwr(FunctionalState pwrMode){
// Displey 7 lines
// CTP 3 lines	
 
  if (pwrMode == DISABLE){                                          // if mode is DISABLE 
    
//    EXTI->IMR1 &= EXTI_IMR1_IM13;                                   // EXTI13 interrupt masked                     //PC13  CTP_IRQ ??? - not used
//    EXTI->RPR1 = EXTI_RPR1_RPIF13;                                  // clear EXTI13 interrupt flag
//    NVIC_DisableIRQ(EXTI4_15_IRQn);                                 // disable EXTI4_15 interrupt
 
    GPIOA->BSRR = GPIO_BSRR_BR12 | GPIO_BSRR_BR11;                  // out LOW to PA11, PA12                          // PA11 PA12 i2c2 CTP
		
		GPIOA->MODER &=~(GPIO_MODER_MODE12_Msk |GPIO_MODER_MODE11_Msk);
    GPIOA->MODER |= (GPIO_MODER_MODE11_0 | GPIO_MODER_MODE12_0); // TP I2C pins set as general purpose output mode // PA11 PA12  i2c2  CTP   
                                                                                                                      
    
    GPIOD->MODER |= (GPIO_MODER_MODE3_Msk | GPIO_MODER_MODE2_Msk |  // PD0..PD3 mode bits switch to analog mode       //PD0 PD1 PD2 PD3 TFT_LED TFT_RST TFT_D/I TFT_CS
                    GPIO_MODER_MODE1_Msk | GPIO_MODER_MODE0_Msk);    
	/*                   button                  */									
	GPIOA->MODER &= ~(GPIO_MODER_MODE5_Msk);                  // input 
		
    TP_RST_LOW;// out LOW CTP_RST
		
    PWR_TFT_OFF;
  } else { 
		PWR_TFT_ON;
    TP_RST_LOW;
		TFT_RST_LOW;
		
    GPIOA->MODER &=~(GPIO_MODER_MODE12_Msk |GPIO_MODER_MODE11_Msk);// PA11 alternate function I2C2_SCL
    GPIOA->MODER |= (GPIO_MODER_MODE11_1 | GPIO_MODER_MODE12_1);// PA12 alternate function I2C2_SDA
        

 
    GPIOD->BSRR = GPIO_BSRR_BR3 | GPIO_BSRR_BR2 |                   // out LOW to the PD0..PD3 pins
                  GPIO_BSRR_BR1 | GPIO_BSRR_BR0;
		
    GPIOD->MODER |= (GPIO_MODER_MODE3_Msk | GPIO_MODER_MODE2_Msk |
                    GPIO_MODER_MODE1_Msk | GPIO_MODER_MODE0_Msk);		//Analog mode
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


void BS_BLE_PinsOnOff(FunctionalState pwrMode);
/**
\brief  Global Power + FLASH CS+ SPI1 (for flash and display)

*/
void switchSPI1InterfacePinsToPwr(FunctionalState pwrMode)
{
  if (pwrMode == DISABLE){                                          // if mode is DISABLE 
   

		GPIOB->MODER |= (GPIO_MODER_MODE5_Msk |                         // PB3..PB5 switch to analog mode                 //PB3..PB5 SPI1 Flash+Displ
                     GPIO_MODER_MODE4_Msk |    
                     GPIO_MODER_MODE3_Msk);  
    
		FLASH_CS_L;
		
    GPIOA->PUPDR &=  ~(GPIO_PUPDR_PUPD9_Msk | GPIO_PUPDR_PUPD10_Msk);//2bits
    GPIOA->PUPDR |=  (GPIO_PUPDR_PUPD9_0 | GPIO_PUPDR_PUPD10_0);//2bits
		GPIOA->MODER &= ~(GPIO_MODER_MODE9_Msk |                         // switch to analog mode       USB-COM
                      GPIO_MODER_MODE10_Msk);
		
		BS_BLE_PinsOnOff( pwrMode);
		
		TFT_LED_OFF;
		PWR_GLOBAL_OFF;
		PWR_TFT_ON;
		PWR_UTSTAGE_ON;
		
   } else { 
		 
		PWR_UTSTAGE_OFF; 
		PWR_TFT_OFF; 
		PWR_GLOBAL_ON; 
		 
		BS_BLE_PinsOnOff( pwrMode); 
		 
		 
		FLASH_CS_H; 
		 
		GPIOB->MODER |= (GPIO_MODER_MODE5_Msk |                         // PB3..PB5 switch to analog mode                 //PB3..PB5 SPI1 Flash+Displ
                     GPIO_MODER_MODE4_Msk |    
                     GPIO_MODER_MODE3_Msk);  
    GPIOB->MODER &= ~(GPIO_MODER_MODE5_0 |                          // PB5 alternate function SPI1_MOSI 
                     GPIO_MODER_MODE4_0 |                           // PB4 alternate function SPI1_MISO
                     GPIO_MODER_MODE3_0);                           // PB3 alternate function SPI1_SCK
		 
    GPIOA->PUPDR &=  ~(GPIO_PUPDR_PUPD9_Msk | GPIO_PUPDR_PUPD10_Msk);//2bits
    GPIOA->PUPDR |=  (GPIO_PUPDR_PUPD9_0 | GPIO_PUPDR_PUPD10_0);//2bits
		GPIOA->MODER &= ~(GPIO_MODER_MODE9_Msk |                     // switch to analog mode       USB-COM
                      GPIO_MODER_MODE10_Msk);
 		GPIOA->MODER |= (GPIO_MODER_MODE9_1 |                       // alternate function       USB-COM
                     GPIO_MODER_MODE10_1);

 
  }   
                    
}

/**

\brief switch OUTStage Interface Pins To Power state ENABLE or DISABLE AND POWER

  FPGA_CS 					PB12  output	FPGA_CS_L/H
	SPI2_SCK 					PB13  spi
	SPI2_MISO					PB14	spi
	SPI2_MOSI 				PB15	spi mosi
	CONF_DONE 				PC6		input
	nSTATUS 					PC7		input
	nCONFIG 					PB11	output 	nCONFIG_L/H
	Reserv=LED_TEST 	PB10	output 	FPGA_Reserv_L/H
	FPGA_START 				PA8		output	FPGA_START_L/H

*/
void switchOUTStageInterfacePinsToPwr(FunctionalState pwrMode)
{
	//SPI
  if (pwrMode == DISABLE){  
		// if mode is DISABLE 
   
    
		GPIOB->MODER |= (GPIO_MODER_MODE13_Msk |                         // PB3..PB5 switch to analog mode                 //PB3..PB5 SPI1 Flash+Displ
                     GPIO_MODER_MODE14_Msk |    
                     GPIO_MODER_MODE15_Msk);  
    PWR_UTSTAGE_OFF;
   } else { 
		PWR_UTSTAGE_ON;
		GPIOB->MODER |= (GPIO_MODER_MODE13_Msk |                         // PB3..PB5 switch to analog mode                 //PB3..PB5 SPI1 Flash+Displ
                     GPIO_MODER_MODE14_Msk |    
                     GPIO_MODER_MODE15_Msk);  
		 
		 
		GPIOB->MODER |= (GPIO_MODER_MODE13_Msk |                         // PB3..PB5 switch to analog mode                 //PB3..PB5 SPI1 Flash+Displ
                     GPIO_MODER_MODE14_Msk |    
                     GPIO_MODER_MODE15_Msk); 
    GPIOB->MODER &= ~(GPIO_MODER_MODE15_0 |                          // PB5 alternate function SPI1_MOSI 
                     GPIO_MODER_MODE14_0 |                           // PB4 alternate function SPI1_MISO
                     GPIO_MODER_MODE13_0);                           // PB3 alternate function SPI1_SCK
 
  }   

 //and other
 FPGA_CS_L;
 nCONFIG_L;
 FPGA_Reserv_L;
 FPGA_START_L;	
	
 if (pwrMode == DISABLE)
 {
  PWR_UTSTAGE_OFF;	
 } 
 else
 {
	PWR_UTSTAGE_ON;		
 };
	
};
/**

I2C1_SCL	PB8	47
I2C1_SDA	PB9 48
*/
void B_ACC_PinsOnOff(FunctionalState pwrMode)
{
	
  if (pwrMode == DISABLE)
		{  
			GPIOB->MODER &=~(GPIO_MODER_MODE8_Msk |GPIO_MODER_MODE9_Msk);//Charger I2C pins set as general purpose input mode 
		}
		else
		{
			GPIOB->MODER &=~(GPIO_MODER_MODE8_Msk |GPIO_MODER_MODE9_Msk);
			GPIOB->MODER |= (GPIO_MODER_MODE8_1 | GPIO_MODER_MODE9_1); // Charger I2C pins set as alternative    
		};
};


#define USART2_ALT_FUNC 0x01
void BS_BLE_PinsOnOff(FunctionalState pwrMode)
{
	
  if (pwrMode == DISABLE)
		{  
	GPIOA->MODER |= (GPIO_MODER_MODE0_Msk |
										GPIO_MODER_MODE1_Msk |
										GPIO_MODER_MODE2_Msk |  
										GPIO_MODER_MODE3_Msk |  
										GPIO_MODER_MODE4_Msk   );		
		}
		else
		{

//			GPIOB->MODER &=~(GPIO_MODER_MODE8_Msk |GPIO_MODER_MODE9_Msk);//Charger I2C pins set as general purpose input mode 
	GPIOA->MODER &= ~(GPIO_MODER_MODE0_0 |  GPIO_MODER_MODE0_1 |
										GPIO_MODER_MODE1_0 |  GPIO_MODER_MODE1_1 |
										GPIO_MODER_MODE2_0 |  GPIO_MODER_MODE2_1 |
										GPIO_MODER_MODE3_0 |  GPIO_MODER_MODE3_1 |
										GPIO_MODER_MODE4_0 |  GPIO_MODER_MODE4_1);
	GPIOA->MODER |= GPIO_MODER_MODE0_0 | //output 
									GPIO_MODER_MODE1_0 | //output
									GPIO_MODER_MODE2_1 | // alternate
									GPIO_MODER_MODE3_1 | // alternate
									GPIO_MODER_MODE4_0;  // output
	GPIOA->OTYPER &= ~(GPIO_OTYPER_OT0 | // push-pull
										 GPIO_OTYPER_OT1 | // push-pull
										 GPIO_OTYPER_OT4); // push-pull
	GPIOA->PUPDR &= ~(GPIO_PUPDR_PUPD2_0 | GPIO_PUPDR_PUPD2_1);
	GPIOA->PUPDR |= GPIO_PUPDR_PUPD2_0; // pull up
	GPIOA->OSPEEDR |= GPIO_OSPEEDR_OSPEED2_0 | GPIO_OSPEEDR_OSPEED2_1;// high speed
	GPIOA->BSRR = GPIO_BSRR_BS0 | GPIO_BSRR_BS1 | GPIO_BSRR_BS4; //set
	GPIOA->AFR[0] = (USART2_ALT_FUNC<<GPIO_AFRL_AFSEL2_Pos) | 
									(USART2_ALT_FUNC<<GPIO_AFRL_AFSEL3_Pos);
			
		};
};

