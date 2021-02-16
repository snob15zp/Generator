#include "stm32g0xx.h"
#include <stdbool.h>
#include "PowerModes_Defs.h"
#include "board_PowerModes.h"
#include "BoardSetup.h"
#include "superloop.h"
#include "superloopDisplay.h"
#include "superloop_Player.h"
#include "Spi1.h"
#include "SuperLoop_Comm2.h"

//#define SLP_WakeUpPause 500

//static systemticks_t SLP_LastUpdateTime;
static uint8_t SLP_state;
//static bool SLP_sleep;
//static bool SLP_WakeUP;

/*************************************************************************************************************************
*
*
**************************************************************************************************************************/
void EXTI4_15_IRQHandler(void)
{	
	GPIOB->BSRR = GPIO_BSRR_BR10;
	EXTI->RPR1 |= EXTI_RPR1_RPIF5;
	EXTI->FPR1 |= EXTI_FPR1_FPIF7;
	__NVIC_SetPendingIRQ(TIM3_IRQn);
}
//***********************************************************************************************************************

void bPM_FSMPower_Init(void);

void enterToStop(void);

void SuperLoop_PowerModes_Init(void)
	{
    RCC->IOPENR |= RCC_IOPENR_GPIOAEN;
		
    EXTI->IMR1 = 0;
    EXTI->EMR1 = 0;	
		
	GPIOA->MODER &= ~(GPIO_MODER_MODE5_Msk); /**< Power Button */
	EXTI->EXTICR[1] &= ~0xff00;
	//EXTI->FTSR1 |= EXTI_FTSR1_FT5;
	EXTI->RTSR1 |= EXTI_RTSR1_RT5;
	//EXTI->FPR1 |= EXTI_FPR1_FPIF5;
	EXTI->RPR1 |= EXTI_RPR1_RPIF5;
	EXTI->IMR1 |= EXTI_IMR1_IM5; // EXTI5 interrupts unmasked
	//EXTI->EMR1 |= EXTI_EMR1_EM5; // EXTI5 event unmasked
		
	GPIOA->MODER &= ~(GPIO_MODER_MODE7_Msk); /**< TPS		*/
	EXTI->EXTICR[1] &= ~0xff000000;
	EXTI->FTSR1 |= EXTI_FTSR1_FT7;
	//EXTI->RTSR1 |= EXTI_RTSR1_RT5;
	EXTI->FPR1 |= EXTI_FPR1_FPIF7;
	//EXTI->RPR1 |= EXTI_RPR1_RPIF5;
	EXTI->IMR1 |= EXTI_IMR1_IM7; // EXTI7 interrupts unmasked
	//EXTI->EMR1 |= EXTI_EMR1_EM7; // EXTI7 event unmasked
	
	
		
	//NVIC_SetPriority(EXTI4_15_IRQn, 5);
	//NVIC_EnableIRQ(EXTI4_15_IRQn);
		
		
		
  bPM_FSMPower_Init();
    
	}		
	
/**
\brief  control entering into sleep
	
	0: All 	DontMindSleep ? - > 	sleep request		-> 	goto 1
	1: All ready for sleep ?- > 	goto 2
	   Any in work mode ? 	- > 	goto 3
	2: Sleep/wakeup       	- > 	goto 2
	3: wakeup             	- > 	wake up request	->	goto 0
	
*/	
void SuperLoop_PowerModes(void)
	{
		e_PowerState rplayer,rdispl,racc,rcomm;

		rplayer=	SLPl_GetPowerState();
		rdispl=		SLD_GetPowerState();
		racc=			SLAcc_GetPowerState();
		rcomm=		SLC_GetPowerState();
		return; ///RDD debug
		switch (SLP_state)
		{
			case 0://work mode
				if ((rdispl&&racc&&rplayer&&rcomm))
					{
						SLPl_SetSleepState(true);
						SLD_SetSleepState(true);
						SLAcc_SetSleepState(true);
						SLP_state++;
					}
				break;
			case 1:	//wait transition for sleep
					if (!(rdispl&&racc&&rplayer&&rcomm)) SLP_state= 3;
					if ((e_PS_ReadySleep==rplayer)
						&&(e_PS_ReadySleep==rdispl)
					  &&(e_PS_ReadySleep==racc)
				     ) 
					  SLP_state++;
				break;
			case 2:
				    Communication_InSleep();
				    BoardSetup_InSleep();
			      //DBG->CR|= DBG_CR_DBG_STOP;
            enterToStop();
			      BoardSetup_OutSleep(); 
 			      Communication_OutSleep(); 
			      SLP_state=3;
              break;			
			case 3: //weakup
						SLAcc_SetSleepState(false);	
            SLD_SetSleepState(false);	
            SLPl_SetSleepState(false);	
			      SLP_state=0;
				break;	
			default: 	SLP_state=0;
	   };
	 }		
/**

PWR_CR1->FPD_STOP: Flash memory powered down during Stop mode
PWR_CR4->VBE:	 0: VBAT battery charging disable
	 
*/	 
void enterToStop(void)
{
    while (GPIOA->IDR & GPIO_IDR_ID5);

  
  NVIC_DisableIRQ(TIM3_IRQn);
	NVIC_DisableIRQ(I2C2_IRQn);
	NVIC_DisableIRQ(I2C1_IRQn);
	NVIC_DisableIRQ(USART1_IRQn);
	

	
	RCC->CSR |= RCC_CSR_LSION;
  
//	SysTick->CTRL  &= ~(SysTick_CTRL_CLKSOURCE_Msk |
//                   SysTick_CTRL_TICKINT_Msk   |
//                   SysTick_CTRL_ENABLE_Msk);
	
	TIM3->CR1 &= ~TIM_CR1_CEN;
	TIM3->DIER = 0;
	
  //PM_ClearPendingButton;  
  //PM_ClearPendingTPSIRQ;
  

	TIM3->SR=0x0000;
		__DSB();
	__ISB();
	NVIC->ICPR[0]=0xffffffff;
		__DSB();
	__ISB();
	__NVIC_ClearPendingIRQ(TIM3_IRQn);
		__DSB();
	__ISB();
	
	//NVIC_SetPriority(EXTI4_15_IRQn, 5);
	//NVIC_EnableIRQ(EXTI4_15_IRQn);

    
	GPIOB->BSRR = GPIO_BSRR_BS10;
	
	SET_BIT(SCB->SCR, ((uint32_t)SCB_SCR_SEVONPEND_Msk));
	
	PWR->CR1 |= PWR_CR1_LPR 		// the regulator is switched from main mode (MR) to low-power mode
	         | PWR_CR1_FPD_STOP //RDD
	         | PWR_CR1_LPMS_0; 	// select Stop 1 low-power mode
	__DMB();
	SysTick->CTRL &= ~SysTick_CTRL_TICKINT_Msk;
	__DMB();

	SCB->SCR |= SCB_SCR_SLEEPDEEP_Msk; // Set SLEEPDEEP bit of Cortex System Control Register
//while (!(( EXTI->RPR1 & EXTI_RPR1_RPIF5) || (EXTI->FPR1 & EXTI_FPR1_FPIF7)))
{
	__DSB();
	__ISB();

  __SEV();
	

__WFE();
__WFE();	
}


   //__WFI();

	SCB->SCR &= ~SCB_SCR_SLEEPDEEP_Msk; // reset SLEEPDEEP bit of Cortex System Control Register
	PWR->CR1 &= ~(PWR_CR1_LPMS_Msk | PWR_CR1_LPR); // the regulator is switched from low-power mode to main mode (MR)
	SysTick->CTRL |= SysTick_CTRL_TICKINT_Msk;
	
//	NVIC_DisableIRQ(EXTI4_15_IRQn);
	
	setSystemClock();
	
	
	
	tim3Init();

	GPIOB->BSRR = GPIO_BSRR_BR10;
	NVIC_EnableIRQ(USART1_IRQn);
	NVIC_EnableIRQ(I2C2_IRQn);
	NVIC_EnableIRQ(I2C1_IRQn);
	NVIC_EnableIRQ(TIM3_IRQn);
    
}	 
	 
	
/**

  Display off/on:								PM_OnOffPWR(PM_Display,false/true );				
  Player off/on:								PM_OnOffPWR(PM_Player,false/true );					
	Communication(Flash) off/on:	PM_OnOffPWR(PM_Communication,false/true );	
*/

//------------------------- FSM data  ---------------------------------------------

typedef uint32_t key_type;
	
typedef
   struct { 
			key_type mFSM_Error;
		  uint8_t state;
    } s_FSM_Data;
	 
typedef enum  {e_P_SPI1Y,e_P_DY,e_P_PlY // 3 interface pins
	            ,e_P_PlN,e_P_DN,e_P_SPI1N
	//,e_P_GLOBAL_ON,e_P_TFT_ON, e_P_UTSTAGE_ON ,e_P_UTSTAGE_OFF  ,e_P_TFT_OFF  ,e_P_GLOBAL_OFF //PWR 4+6=10
,e_P_NumOfel} e_TransitionFunctionType;// 13+1=14

#define dxx (1<<e_P_DN)   // digital pins and PWR
#define Dxx (1<<e_P_DY) 
#define xpx (1<<e_P_PlN)
#define xPx (1<<e_P_PlY)

#define xxc (1<<e_P_SPI1N)

#define G_P_On (1<<e_P_GLOBAL_ON)
#define D_P_On  (1<<e_P_TFT_ON)
#define Pl_P_On (1<<e_P_UTSTAGE_ON)
#define Pl_P_Off (1<<e_P_UTSTAGE_OFF)
#define D_P_Off (1<<e_P_TFT_OFF)
#define G_P_Off (1<<e_P_GLOBAL_OFF)

#define xxC (1<<e_P_SPI1Y)


/**

Display Player / communication (geberal 3.3V)

Capital letter - enabled, lowercase letter - disabled, x - does not matter
D/d - display
P/p - player
C/c - communication

*/
static const key_type PWR2_TransitionKeys[5][5]=  //int a[ROWS][COLS] // NEW OLD 
{//     dpc            dPx       Dpx      DPx   dpC
/*dpc*/{xxc|xpx|dxx,      0,        0,    		0,   	0 			},
/*dPx*/{xxC|xPx|Dxx,   		0,        dxx|xPx,	Dxx, 	xPx 		},
/*Dpx*/{xxC|    Dxx,   	  Dxx|xpx, 	0,    		xpx,  Dxx 		},
/*DPx*/{xxC|xPx|Dxx,   		0,        xPx,    	0,   	xPx|Dxx	},
/*dpC*/{xxC|Dxx    ,   		Dxx|xpx,  Dxx,    	xpx,  0 			},
};
		
//------------------------- FSM functions  ---------------------------------------

		
s_FSM_Data FDMD_Power;
e_FunctionReturnState  FSM_MainTransition_P(s_FSM_Data * pFDMD_Power,key_type key);
e_FunctionReturnState TransitionFunction_P(uint8_t state);
e_FunctionReturnState  MainTransition_Po(uint8_t NEW);
		
//------------------------------power on off----------------------------------------
static uint8_t PWR_STATE;

//--------------------------interface functions-----------------------------------------------
//!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!
void PM_OnOffPWR(uint8_t modul, bool newstate)
{ 
	      uint8_t PWR_STATE_NEW;
	      if (newstate) 
        {PWR_STATE_NEW=(PWR_STATE | modul)&7;}
				else
				{PWR_STATE_NEW=(PWR_STATE & (~modul))&7;}
				MainTransition_Po(PWR_STATE_NEW);
				PWR_STATE=	PWR_STATE_NEW;
};

//----------------------------------------------------------------------------
const static uint8_t inner_PWR_state_encoder[8]={0,1,2,3,4,1,2,3};
e_FunctionReturnState  MainTransition_Po(uint8_t NEW)
{
	      uint8_t inner_PWR_state_new	= inner_PWR_state_encoder[NEW];
	      uint8_t inner_PWR_state			= inner_PWR_state_encoder[PWR_STATE];
				while (e_FRS_Done!=FSM_MainTransition_P(&FDMD_Power, PWR2_TransitionKeys[inner_PWR_state_new][inner_PWR_state]))
                {}
				while (e_FRS_Done!=FSM_MainTransition_P(&FDMD_Power, PWR2_TransitionKeys[inner_PWR_state_new][inner_PWR_state_new]))
                {}
	      return e_FRS_Done;
};



void bPM_FSMPower_Init(void)
{
	PM_OnOffPWR(PM_Display,false );
	PM_OnOffPWR(PM_Player,false );
	PM_OnOffPWR(PM_Communication,false );
	PM_OnOffPWR(PM_Display,true );
	PM_OnOffPWR(PM_Player,true );
	PM_OnOffPWR(PM_Communication,true );
	PM_OnOffPWR(PM_Display,false );
	PM_OnOffPWR(PM_Player,false );
	PM_OnOffPWR(PM_Communication,false );
};


//TransitionKeys[mainFMSstate][mainFMSstate]); 
//#define MStateP(d,p) ((((d)<<1)|((p)<<0))&0x3)



e_FunctionReturnState TransitionFunction_P(uint8_t state)
{   e_FunctionReturnState rstate;
	switch (state)
	{ int currl;
	  case e_P_DN:	 				switchDisplayInterfacePinsToPwr(DISABLE);	rstate=e_FRS_Done; break;//0
	  case e_P_DY: 					switchDisplayInterfacePinsToPwr(ENABLE);	rstate=e_FRS_Done; break;//1
	  case e_P_PlN: 				switchOUTStageInterfacePinsToPwr(DISABLE);rstate=e_FRS_Done; break;//2
	  case e_P_PlY: 				switchOUTStageInterfacePinsToPwr(ENABLE);	rstate=e_FRS_Done; break;//2
		
	  case e_P_SPI1N: 			//disableSpi_1();
			                    switchSPI1InterfacePinsToPwr(DISABLE);		
		
		                                                                 rstate=e_FRS_Done; break;//3
		
//		case e_P_GLOBAL_ON:   PWR_GLOBAL_ON;														rstate=e_FRS_Done; break;
//		case e_P_TFT_ON:			PWR_TFT_ON;																rstate=e_FRS_Done; break;
//		case e_P_UTSTAGE_ON:	PWR_UTSTAGE_ON;														rstate=e_FRS_Done; break;
//		case e_P_UTSTAGE_OFF:	PWR_UTSTAGE_OFF;													rstate=e_FRS_Done; break;
//		case e_P_TFT_OFF:			PWR_TFT_OFF;															rstate=e_FRS_Done; break;
//		case e_P_GLOBAL_OFF:	PWR_GLOBAL_OFF;														rstate=e_FRS_Done; break;
		
  	case e_P_SPI1Y: 			switchSPI1InterfacePinsToPwr(ENABLE);			
		                      //initSpi_1();
		                                                                 rstate=e_FRS_Done; break;//4
		
//    case e_P_sleep: SLP_sleep=true ;	SLP_WakeUP=false;									rstate=e_FRS_Done; break;
//    case e_P_wakeup:SLP_sleep=false ;	SLP_WakeUP=true;									rstate=e_FRS_Done; break;

  	default: 																												rstate=e_FRS_DoneError;
	}
	return rstate;
}


e_FunctionReturnState  FSM_MainTransition_P(s_FSM_Data * pFDMD_Power,key_type key)
{
//	static uint8_t state=0;
//	static uint8_t state1=0;
	e_FunctionReturnState rstate,rstatel;
	
	rstate=e_FRS_Processing;
while(1)
	{	
		if (((key>>pFDMD_Power->state)&1)!=0)
			{ rstatel=TransitionFunction_P(pFDMD_Power->state);
              if (e_FRS_Done==rstatel)   //switch ?
                                      {pFDMD_Power->state++;};
              if (e_FRS_DoneError==rstatel)
									{ pFDMD_Power->mFSM_Error|=1<<pFDMD_Power->state;  // Tries to do everything regardless of mistakes
										pFDMD_Power->state++;
									};
              if (e_FRS_Processing==rstatel)
								{ break;
								};
	        }
	        else
	        {pFDMD_Power->state++;};
		     if (pFDMD_Power->state>=e_P_NumOfel)  
					{ 
						if (pFDMD_Power->mFSM_Error==0) 
						{rstate=e_FRS_Done;}
						else
						{rstate=e_FRS_DoneError;};
					pFDMD_Power->state=0;
					pFDMD_Power->mFSM_Error=0;
					break;	//exit
					};   
	};
	return rstate;
}





