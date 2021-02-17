#include "BoardSetup.h"
#include "SuperLoop_Comm2.h"
#include "SL_CommModbus.h"
#include "bluetooth.h"
#include "BQ28z610_Data.h"

//------------------------------------for iteraction with MOFBUS


static systemticks_t USBcommLastTimel;


e_PS_Int PS_Int;
bool byte_TX_DLE;

//------------------------------------for Display----------------------------------------------
s32_t File_List;
//---------------------------------------------------------------------------------------------
extern int spiffs_init();

// ----------------------------------------for power-------------------------------------------

typedef enum  
{SLC_FSM_InitialWaitSupply  		//work
,SLC_FSM_InitComms  						//work
,SLC_FSM_InitFiles	            //work
,SLC_FSM_CommAbsent 		        //e_PS_DontMindSleep
,SLC_FSM_OffPlayerTransition 	  //work
,SLC_FSM_USBCommunication		    //e_PS_DontMindSleep
,SLC_FSM_AndroidConnected 	    //work
,SLC_FSM_Sleep           	      //  ready for sleep
,SLC_FSM_WakeTransition  	      //work
,SLC_FSM_NumOfEl	
} e_SLC_FSM;

/**
\brief Map e_SLC_FSM onto e_PowerState

e_PS_Work,e_PS_DontMindSleep,e_PS_ReadySleep
*/
const e_PowerState SLC_Encoder[SLC_FSM_NumOfEl]=
{e_PS_Work						//SLC_FSM_InitialWaitSupply
,e_PS_Work						//SLC_FSM_InitComms
,e_PS_Work						//SLC_FSM_InitFiles	
,e_PS_DontMindSleep		//SLC_FSM_CommAbsent
,e_PS_Work						//SLC_FSM_OnTransitionOffPlayer
,e_PS_Work						//SLC_FSM_USBCommunication 
,e_PS_Work	        	//SLC_FSM_AndroidConnected	
,e_PS_ReadySleep			//SLC_FSM_Sleep
,e_PS_Work						//SLC_FSM_WakeTransition
};

const bool SPIFFS_ReadyEncoder[SLC_FSM_NumOfEl]=
{false						//SLC_FSM_InitialWaitSupply
,false						//SLC_FSM_InitComms
,false						//SLC_FSM_InitFiles	
,true		          //SLC_FSM_CommAbsent
,false						  //SLC_FSM_OnTransitionOffPlayer
,false					  	//SLC_FSM_USBCommunication 
,false	          	//SLC_FSM_AndroidConnected	
,false		      	//SLC_FSM_Sleep
,false						//SLC_FSM_WakeTransition
};

const bool SLC_FFSEnable_Encoder[SLC_FSM_NumOfEl]=
{false						//SLC_FSM_InitialWaitSupply
,false						//SLC_FSM_InitComms
,false						//SLC_FSM_InitFiles		
,false		        //SLC_FSM_CommAbsent
,false				  	//SLC_FSM_OnTransitionOffPlayer
,true					  	//SLC_FSM_USBCommunication 
,true	          	//SLC_FSM_AndroidConnected	
,false		      	//SLC_FSM_Sleep
,false						//SLC_FSM_WakeTransition
};




static e_SLC_FSM state_inner;
//static bool USBcomm;
//static bool OffPlayer=true; //rdd debug

__inline bool SLC_SPIFFS_State(void)
{
	 return SPIFFS_ReadyEncoder[state_inner];
};
__inline bool SLC_FFSEnable(void)
{
	 return SLC_FFSEnable_Encoder[state_inner];
};


//---------------------------------for power sleep---------------------------------------------
//static e_PowerState SLD_PowerState; 
static bool SLC_GoToSleep;

__inline e_PowerState SLC_GetPowerState(void)
{
	 return SLC_Encoder[state_inner];
};

__inline e_PowerState SLC_SetSleepState(bool state)
{
	SLC_GoToSleep=state;
	return SLC_Encoder[state_inner];
};



extern void SLC_init(void)
{

};

//#define SL_CommModbus()

extern void SLC(void)
{

	//systemticks_t SLD_LastButtonPress;
	switch (state_inner)
	{
		case SLC_FSM_InitialWaitSupply: // initial on
			if (bVSYS) 
      {state_inner=SLC_FSM_InitComms;};
			break;
		case SLC_FSM_InitComms:
			PM_OnOffPWR(PM_Communication,true );
			spiffs_init();
		  SL_CommModbusInit();
			btInit();
		  state_inner=SLC_FSM_InitFiles;
			if ((!bVSYS))
				state_inner=SLC_FSM_InitialWaitSupply;
			break;
		case SLC_FSM_InitFiles:
			  File_List=SPIFFS_open(&fs,"freq.pls",SPIFFS_O_RDONLY,0);
				SLPl_InitFiles();
		    state_inner=SLC_FSM_CommAbsent;
		    if ((!bVSYS))
					state_inner=SLC_FSM_InitialWaitSupply;
			break;
		case SLC_FSM_CommAbsent: //
			  SL_CommModbus();
		    SLBL();//
				if (SLC_GoToSleep)
				   {
						SPIFFS_close(&fs, File_List);
						SPIFFS_unmount(&fs);//unmount
						state_inner=SLC_FSM_Sleep;
						PM_OnOffPWR(PM_Communication,false); 
					 }						 
				USBcommLastTimel=USBcommLastTime; //MODBUScommLastTime
				if ((SystemTicks-USBcommLastTimel)>(2*USBcommPause))
				   {   USBcommLastTime=SystemTicks-(2*USBcommPause);
					 }	 
				if ((SystemTicks-USBcommLastTimel)<USBcommPause)
				{
					  //SPIFFS_close(&fs, File_List);
					  state_inner=SLC_FSM_OffPlayerTransition;
				}
				if ((!bVSYS))
					state_inner=SLC_FSM_InitialWaitSupply;
      break;
		case SLC_FSM_OffPlayerTransition: // on
			SL_CommModbus();
		  SLBL();
  		if (SLPl_FFSFree())
			{
 				SPIFFS_close(&fs, File_List);
				state_inner=SLC_FSM_USBCommunication;
			};
			if ((!bVSYS))
				state_inner=SLC_FSM_InitialWaitSupply;
			break;
		case SLC_FSM_USBCommunication: 
			SL_CommModbus();
		  SLBL();
		  USBcommLastTimel=USBcommLastTime;
      if ((SystemTicks-USBcommLastTimel)>(USBcommPause))				
				  state_inner=SLC_FSM_InitFiles;
			if ((!bVSYS))
				state_inner=SLC_FSM_InitialWaitSupply;
		  break;	
		case SLC_FSM_Sleep:
				if ((!SLC_GoToSleep) ) 
	  			{state_inner=SLC_FSM_WakeTransition;
				  };
			break;
		case SLC_FSM_WakeTransition: //wake transition
		  state_inner=SLC_FSM_InitialWaitSupply;
		break;
    default: state_inner=SLC_FSM_InitialWaitSupply;		
	};
	//return 0;
};


void Communication_InSleep()
{
	RCC->APBENR2 |= RCC_APBENR2_USART1EN;
	USART1->CR1 &= ~USART_CR1_UE;
	RCC->APBENR2 &= ~RCC_APBENR2_USART1EN;
};
void Communication_OutSleep()
{
  //uart1Init();	
};

