#include "BoardSetup.h"
#include "SL_CommModbus.h"
#include "superloopDisplay.h"
#include "superloop_Player.h"
#include "mainFSM.h"
#include "bluetooth.h"
#include "BQ28z610_Data.h"
#include "mb.h"
#include "SuperLoop_Comm2.h"
#include "TPS65987_Data.h"

//------------------------------------for iteraction with MOFBUS-------------------------------

#define D_BL_Packet_Pause 50000
#define D_USB_Packet_Pause 50000

static systemticks_t MODBUScommLastTimel;
static systemticks_t lastIrqTime=-(2*D_BL_Packet_Pause);
static systemticks_t lastUSBTime=-(2*D_USB_Packet_Pause);

e_PS_Int PS_Int=PS_Int_USB;
bool byte_TX_DLE;
bool isUSBint;

//------------------------------------for Display----------------------------------------------
s32_t File_List;
//---------------------------------------------------------------------------------------------
extern int spiffs_init();

// ----------------------------------------for power-------------------------------------------

typedef enum  
{SLC_FSM_InitialWaitSupply  		//work
,SLC_FSM_InitComms  						//work
,SLC_FSM_Init28z610             //work
,SLC_FSM_Init65987              //work 
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
,e_PS_Work						//SLC_FSM_Init28z610 
,e_PS_Work	          //SLC_FSM_Init65987
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
,false            //SLC_FSM_Init28z610
,false            //SLC_FSM_Init65987
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
,false            //SLC_FSM_Init28z610
,false            //SLC_FSM_Init65987
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
//----------------------------------call backs----------------------------------------------------

bool b_UpdateFlag_28z610;
bool b_UpdateFlag_65987;

void on_tx_done_cb(void)
{
  
	switch (PS_Int)// swith Android/USB
	{
		case PS_Int_USB_No:
		case PS_Int_USB:
			 	lastUSBTime=SystemTicks-2*D_USB_Packet_Pause;
				isUSBint=false;	
			break;	
		 case PS_Int_BLE:
		 case PS_Int_BLE_No:	 
				lastIrqTime=SystemTicks-2*D_BL_Packet_Pause;
		    isBLEint=false;
			break;
		 default: ;
	 };
	
	 MODBUScommLastTimel=SystemTicks-2*USBcommPause;	//swith FilesExchainge/generation
  
}

/**
* TPS65987 write done callback 
* Called when tps65987.bin has been written and can be applied
*/
void tps65987_cb(void)
{
	  b_UpdateFlag_65987=true;
}



/**
* BQ28Z610 write done callback 
* Called when bq28z610.bin has been written and can be applied
*/
void bq28z610_cb(void)
{
    b_UpdateFlag_28z610=true;	
}




//---------------------------super loop--------------------------------------------------------------


extern void SLC_init(void)
{
	spiffs_on_write_tps65987_done(tps65987_cb);
  spiffs_on_write_bq28z610_done(bq28z610_cb);
};

//#define SL_CommModbus()
static uint16_t data=3300;
static e_FunctionReturnState res;

static bool br_28z610;
static bool br_65987;

extern void SLC(void)
{

	//systemticks_t SLD_LastButtonPress;
//	if ((!bVSYS))
//		state_inner=SLC_FSM_InitialWaitSupply;
	switch (state_inner)
	{
		case SLC_FSM_InitialWaitSupply: // initial on
			if (bVSYS) 
      {state_inner=SLC_FSM_InitComms;};
			break;
		case SLC_FSM_InitComms:
			if ((!bVSYS))
  		state_inner=SLC_FSM_InitialWaitSupply;
			PM_OnOffPWR(PM_Communication,true );
			spiffs_init();
		  SL_CommModbusInit();
			btInit();
		  state_inner=SLC_FSM_Init28z610;
			break;
		case SLC_FSM_Init28z610:
			if ((!bVSYS))
				state_inner=SLC_FSM_InitialWaitSupply;			
			if (b_UpdateFlag_28z610)
			{
				if (BQ28z610_DriverState())
					break;
				b_UpdateFlag_28z610=false;
				br_28z610=readDataFromFile();
				
				if (br_28z610)
				{ SetStatusString("Update 28z610 ok");
				}
        else				
				{ SetStatusString("Update 28z610 err");
				}
			}
      else
			{
				br_28z610=true;
				state_inner=SLC_FSM_Init65987;
			}			
 		case SLC_FSM_Init65987:
			if ((!bVSYS))
				state_inner=SLC_FSM_InitialWaitSupply;
			if (b_UpdateFlag_65987)
			{
				if (TPS6598x_DriverState())
					break;
				b_UpdateFlag_65987=false;
				br_65987=tpsFlashUpdate();
				if (br_65987)
				{ SetStatusString("Update 65987 ok");
				}
        else				
				{ SetStatusString("Update 65987 err");
				}
			}
      else
			{
				state_inner=SLC_FSM_InitFiles;
			}			
//		  do // debug instead readDataFromFile()
//			{res=BQ28z610_AltManufacturerAccessDFWrite(0x46c9, (uint8_t*)&data, 2,SLC);
//			}
//		  while (e_FRS_Done!=res);
//			readDataFromFile();
//			tpsFlashUpdate();
//			state_inner=SLC_FSM_InitFiles;
		case SLC_FSM_InitFiles:
			if ((!bVSYS))
  		state_inner=SLC_FSM_InitialWaitSupply;
			  File_List=SPIFFS_open(&fs,"freq.pls",SPIFFS_O_RDONLY,0);
				SLPl_InitFiles();
		    state_inner=SLC_FSM_CommAbsent;
			break;
		case SLC_FSM_CommAbsent: //
			if ((!bVSYS))
  		state_inner=SLC_FSM_InitialWaitSupply;

			  SL_CommModbus();
		    SLBL();//
				if (SLC_GoToSleep)
				   {
						SPIFFS_close(&fs, File_List);
					  PM_OnOffPWR(PM_Communication,false);
						//unmount
						state_inner=SLC_FSM_Sleep;
					 }						 
						 
				MODBUScommLastTimel=MODBUScommLastTime; //MODBUScommLastTime
				if ((SystemTicks-MODBUScommLastTimel)>(3*USBcommPause))
				   {   MODBUScommLastTime=SystemTicks-(2*USBcommPause);
					 }	 

				if ((SystemTicks-MODBUScommLastTimel)<USBcommPause)
				{
					  //SPIFFS_close(&fs, File_List);
					  state_inner=SLC_FSM_OffPlayerTransition;
				}
      break;
		case SLC_FSM_OffPlayerTransition: // on
			if ((!bVSYS))
  		state_inner=SLC_FSM_InitialWaitSupply;

			SL_CommModbus();
		    SLBL();
            if (SLPl_FFSFree())
			{
 				SPIFFS_close(&fs, File_List);
				state_inner=SLC_FSM_USBCommunication;
			};
			break;
		case SLC_FSM_USBCommunication: 
			if ((!bVSYS))
  		state_inner=SLC_FSM_InitialWaitSupply;

			SL_CommModbus();
		   SLBL();
		  MODBUScommLastTimel=MODBUScommLastTime;
      if ((SystemTicks-MODBUScommLastTimel)>(USBcommPause))				
				  state_inner=SLC_FSM_Init28z610;
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




void SLBL(void)
{			
	switch (PS_Int)
		{case PS_Int_USB_No:
        if ((SystemTicks-lastUSBTime) < D_USB_Packet_Pause)
				{ PS_Int=PS_Int_USB; 
				}	
				if ((SystemTicks-lastIrqTime) < D_BL_Packet_Pause)
				{	PS_Int=PS_Int_No;
					eMBDisable();
					PS_Int=PS_Int_BLE_No;
					eMBEnable();
				};					
			break;
		 case PS_Int_USB:
			 	if ((SystemTicks-lastUSBTime) > D_USB_Packet_Pause)
					PS_Int=PS_Int_USB_No;
			break;	
		 case PS_Int_BLE_No:
				if ((SystemTicks-lastIrqTime) < D_BL_Packet_Pause) // Exit BLE mode by pause
          PS_Int=PS_Int_BLE;
				if ((SystemTicks-lastUSBTime )< (D_USB_Packet_Pause))
				{	PS_Int=PS_Int_No;
					eMBDisable();
					PS_Int=PS_Int_USB_No;
					eMBEnable();
				};					
			break;
		 case PS_Int_BLE:
				if ((SystemTicks-lastIrqTime) > D_BL_Packet_Pause) // Exit BLE mode by pause
					PS_Int=PS_Int_BLE_No;
			break;
		 default: PS_Int=PS_Int_USB_No;
	 };
		
//----------------------------BLE times-----------------------------------
  if ((SystemTicks-lastIrqTime)>(3*D_BL_Packet_Pause))// Correction for cyclical time
	   {   lastIrqTime=SystemTicks-(2*D_BL_Packet_Pause);
		 }	 
					 
	if (isBLEint) //Interrupt time latch (approximately)
		{
			isBLEint=false;
			lastIrqTime=SystemTicks;  
		}
//----------------------------USB times-------------------------------------		
  if ((SystemTicks-lastUSBTime)>(3*D_USB_Packet_Pause))// Correction for cyclical time
	   {   lastUSBTime=SystemTicks-(2*D_USB_Packet_Pause);
		 }	 
					 
	if (isUSBint) //Interrupt time latch (approximately)
		{
			isUSBint=false;
			lastUSBTime=SystemTicks;  
		}
}

