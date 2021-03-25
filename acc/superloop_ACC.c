/**
\file high level file for Control TPS65987, bq25703a, bq 28z610

For sleep mode:
used typedef enum  {e_PS_Work,e_PS_DontMindSleep,e_PS_ReadySleep} e_PowerState;
e_PowerState SLPl_GetPoewerState(void);
e_PowerState SLPl_SetSleepState(bool state);

using:
If all relevant modules have state e_PS_DontMindSleep or e_PS_ReadySleep -> 
send to all modules requires for sleep SLPl_SetSleepState(true)->
if all modules e_PS_ReadySleep-> sleep
if any modules e_PS_Work-> for all modules SLPl_SetSleepState(false)

\todo clear interrupt pending
\todo exit mainFSMfunction on only on steady state - > need check

*/

#include <stdint.h>
#include "superloop.h"

//#include "i2c_soft.h"
//#include "i2c_bus_pins.h"
#include "i2c_API.h"

#include "regBQ25703A.h"
#include "DriverBQ25703.h"
#include "DriverBQ28z610.h"
#include "DriverTPS65982_6.h"
#include "mainFSM.h"
#include "BoardSetup.h"
#include "I2c1.h"
#include "TPS65987_Data.h"

#include "board_PowerModes.h"

#include "BQ28z610_Data.h"


static e_SLAcc_BatStatus SLAcc_BatStatus;

__inline e_SLAcc_BatStatus Get_SLAcc_BatteryStatus(void)
{
	e_SLAcc_BatStatus ls;
	if (
			((e_FSM_ChargeOff==mainFMSstate)
		 ||(e_FSM_RestOff  ==mainFMSstate)
	   ) 
     )
  { 
		if (mFSM_BQ28z610_RSOC<10)
			{ls=SLAcc_batEmpty;}
		else
			if (mFSM_BQ28z610_RSOC<20)
			{ls=SLAcc_batLowLev;
			}
			else
				if (mFSM_BQ28z610_RSOC<50)
				{ls=SLAcc_batMedLev;
				}
				else
					if (mFSM_BQ28z610_RSOC<90)
					{ls=SLAcc_batHighLev;
					}
					else
					{ls=SLAcc_batFull;
					};
	}
	else
	{
		ls=SLAcc_batCharging;
	}
	SLAcc_BatStatus=ls;
	return SLAcc_BatStatus;
};



uint8_t maintaskstate=15;//15->skip debug
#define testkey (m_dcoff|m_sr82|m_p82|m_25703init|m_IinLow|m_hizOff|m_Iin82|m_IchAl|m_inhOff|m_DCon)

static uint16_t data_IIN_DPM;
static uint16_t data_ChargeCurrent;
static uint16_t data_ChargerStatus;
static uint16_t data_tmp;
static uint16_t pv_BQ28z610_Temperature;
static uint16_t I86;
static uint16_t V86;


//-------------------------------- loop -------------------------------------------------



typedef enum  
{SLA_FSM_WORK  								//e_PS_Work
,SLA_FSM_WORK2	              //e_PS_Work
,SLA_FSM_DontMindSleep  			//e_PS_DontMindSleep
,SLA_FSM_DontMindSleep2       //e_PS_DontMindSleep
,SLA_FSM_SleepTransition 			//e_PS_DontMindSleep
,SLA_FSM_Sleep 								//e_PS_ReadySleep
,SLA_FSM_WakeTransition 			//e_PS_Work
,SLA_FSM_NumOfEl	
} e_SLA_FSM;

/**
\brief Map e_SLA_FSM onto e_PowerState

e_PS_Work,e_PS_DontMindSleep,e_PS_ReadySleep
*/
const e_PowerState SLA_Encoder[SLA_FSM_NumOfEl]=
{e_PS_Work							//SLA_FSM_WORK  								//
,e_PS_Work	            //SLA_FSM_WORK2
,e_PS_DontMindSleep   	//SLA_FSM_DontMindSleep  		  	//
,e_PS_DontMindSleep   	//SLA_FSM_DontMindSleep2  		  	//	
,e_PS_DontMindSleep			//SLA_FSM_SleepTransition 			//
,e_PS_ReadySleep				//SLA_FSM_Sleep 								//
,e_PS_Work							//SLA_FSM_WakeTransition 			  //
};
//---------------------------------for power---------------------------------------------
static e_PowerState SLAcc_PowerState; 
static bool SLAcc_GoToSleep;

__inline e_PowerState SLAcc_GetPowerState(void)
{
	 	 return SLA_Encoder[maintaskstate];
};

__inline e_PowerState SLAcc_SetSleepState(bool state)
{
	SLAcc_GoToSleep=state;
	return SLA_Encoder[maintaskstate];
};
//----------------------------------------------------------------------------------------

static const e_SLA_FSM encoderPowerWork[8]=
{SLA_FSM_DontMindSleep //0
,SLA_FSM_DontMindSleep   //1
,SLA_FSM_WORK //2
,SLA_FSM_WORK //3
,SLA_FSM_WORK //4
,SLA_FSM_WORK //5
,SLA_FSM_WORK //6
,SLA_FSM_WORK //7
};

static const e_SLA_FSM encoderDontMindSleep[8]=
{SLA_FSM_DontMindSleep //0
,SLA_FSM_SleepTransition   //1
,SLA_FSM_WORK //2
,SLA_FSM_WORK //3
,SLA_FSM_WORK //4
,SLA_FSM_WORK //5
,SLA_FSM_WORK //6
,SLA_FSM_WORK //7
};

static const e_SLA_FSM encoderSleep[8]=
{SLA_FSM_WakeTransition //0
,SLA_FSM_WakeTransition   //1
,SLA_FSM_WakeTransition //2
,SLA_FSM_WakeTransition //3
,SLA_FSM_WakeTransition //4
,SLA_FSM_WakeTransition //5
,SLA_FSM_WakeTransition //6
,SLA_FSM_WakeTransition //7
};

e_FunctionReturnState A_FSM_WakeTransition(void);
e_FunctionReturnState A_FSM_SleepTransition(void);

e_SLA_FSM  GetNewPowerState(const e_SLA_FSM* encoder)
{
	uint8_t DataFor_SLAcc_PowerState; // into function?
			    DataFor_SLAcc_PowerState=0;
		      if (SLAcc_GoToSleep ) 										DataFor_SLAcc_PowerState|=(1<<0);
					if (TPSIRQ) 															DataFor_SLAcc_PowerState|=(1<<1);
	        if (!(
						     (e_FSM_ChargeOff==mainFMSstate)
					     ||(e_FSM_RestOff  ==mainFMSstate)
	             )
					   )	
			                              DataFor_SLAcc_PowerState|=(1<<2);
					return encoder[DataFor_SLAcc_PowerState];
};

uint8_t maintaskstate1;//debug

void LoopACC(void)
{ //e_FunctionReturnState returnstate;
  e_FunctionReturnState rstatel,wrstate;
  static uint16_t data;
 
	maintaskstate1=maintaskstate;
	
  switch(maintaskstate)
  {
	  case SLA_FSM_WORK:
			   PM_ClearPendingTPSIRQ;
		     maintaskstate= SLA_FSM_WORK2;
		  //break;
		case SLA_FSM_WORK2: 
		       rstatel=mainFSMfunction(); //work
		       if (e_FRS_Done==rstatel)
					 {maintaskstate= GetNewPowerState(encoderPowerWork);
					 };
		 break;
   case SLA_FSM_DontMindSleep: 
        PM_ClearPendingTPSIRQ;
	      maintaskstate=SLA_FSM_DontMindSleep2;
	   //break;
	 case  SLA_FSM_DontMindSleep2:
		       rstatel=mainFSMfunction(); //don't mind sleep
		       if (e_FRS_Done==rstatel)
					 {maintaskstate= GetNewPowerState(encoderDontMindSleep);
					 };
     break;
   case SLA_FSM_SleepTransition: 
          if (e_FRS_Done==A_FSM_SleepTransition())
		        {maintaskstate=SLA_FSM_Sleep;
						};
		 break;
	 case SLA_FSM_Sleep:
		 				maintaskstate= GetNewPowerState(encoderSleep);	
	   break;
	 case SLA_FSM_WakeTransition://
          if (e_FRS_Done==A_FSM_WakeTransition())
		        {maintaskstate=SLA_FSM_WORK;
						};
		 break;
	 
  default: maintaskstate=0;
  };
SystemStatus=mainFMSstate; //RDD debug
};


e_FunctionReturnState A_FSM_SleepTransition(void)
{ static uint8_t state=0;
	e_FunctionReturnState rstatel,wrstate;
	static uint16_t data;
  rstatel=e_FRS_Processing;

  switch(state)
  {
	 case 0:
		      	data=0;
	        	wrstate=BQ25703_Wr_Check(       bq25703,
												bq25703InitData[ADCOption].I2cRecord,
												data,
												cPriorityDefault,
												voidfun8
	        									);
		 if (e_FRS_Done==wrstate)
		 {
		  state++;
		 }
		 if (e_FRS_DoneError==wrstate)
		 {
		  state=101;
		 }
		 break;
	 case 1:	 
		 data=(0b101<<8)+1;
		 wrstate=TPS65982_6_RW(TPS87,  e_TPS65987_SleepConfigurationRegister,(uint8_t*)&data,2,I2C_OP_WRITE,A_FSM_SleepTransition);
		 if (e_FRS_Done==wrstate)
		 {
		  state++;
		 }
		 if (e_FRS_DoneError==wrstate)
		 {
		  state=101;
		 }
		 break;
	 case 2:	 
		 wrstate=BQ28z610_AltManufacturerAccessCommand(BQ28z610_Command_Sleep,A_FSM_SleepTransition);
		 if (e_FRS_Done==wrstate)
		 {
		  state++;
		 }
		 if (e_FRS_DoneError==wrstate)
		 {
		  state=101;
		 }
		 break;				
   case 3:
		 I2c1InSleep();
		 B_ACC_PinsOnOff(DISABLE); 
	   state=0;
	   rstatel=e_FRS_Done;
	   break;
		case 101://error
	   state=0;
	   rstatel=e_FRS_DoneError;
     break;
   default: state=0;
	};
	
  return rstatel;	
};

e_FunctionReturnState A_FSM_WakeTransition(void)
{ static uint8_t state=0;
	e_FunctionReturnState rstatel,wrstate;
	static uint16_t data;
  rstatel=e_FRS_Processing;

  switch(state)
  {
   case 0:
		 B_ACC_PinsOnOff(ENABLE); 
     I2c1OutSleep();
	   state++;
	   //break;
	 case 1:
		 data=bq25703InitData[ADCOption].data;
	        	wrstate=BQ25703_Wr_Check(       bq25703,
												bq25703InitData[ADCOption].I2cRecord,
												data,
												cPriorityDefault,
												voidfun8
	        									);
		 if (e_FRS_Done==wrstate)
		 {rstatel=e_FRS_Done;
		  state=0;}
		 break;
   default: state=0;
	};
	
  return rstatel;	
};


uint8_t dataforTPS[65];
extern uint8_t outdata[MAX_BUF_BSIZE];
extern uint8_t indata[MAX_BUF_BSIZE];
/**
\brief test acc procedure

sleep state and other
*/
e_FunctionReturnState testACC(void)
{ //e_FunctionReturnState returnstate;
  e_FunctionReturnState rstatel,wrstate;
  rstatel=e_FRS_Processing;
  static uint16_t data;
  switch(maintaskstate)
  {
   case 0:if (e_FRS_Done==BQ25703_Init_Check())
                 {maintaskstate++;};
	  	  break;
   case 1://if (e_FRS_Done==BQ25703_IIN_Check(100))
           {maintaskstate++;};
  	  	  break;
   case 2: //if (e_FRS_Done==BQ25703_SetBits_Check(ChargeOption3,ChargeOption3_EN_HIZ,0))
                  {maintaskstate++;};
 	  	  break;
   case 3: //if (e_FRS_Done==BQ25703_SetBits_Check(ChargeOption0,0,ChargeOption0_CHRG_INHIBIT))
		        {maintaskstate++;};
						break;
   case 4: 	      	data=0;
	        	wrstate=BQ25703_Wr_Check(       bq25703,
												bq25703InitData[ADCOption].I2cRecord,
												data,
												cPriorityDefault,
												voidfun8
	        									);
		 if (e_FRS_Done==wrstate)
		 {
		  maintaskstate++;
		 }
		 break;
	 case 5: 		 data=bq25703InitData[ADCOption].data;
	        	wrstate=BQ25703_Wr_Check(       bq25703,
												bq25703InitData[ADCOption].I2cRecord,
												data,
												cPriorityDefault,
												voidfun8
	        									);
		 if (e_FRS_Done==wrstate)
		 {
		  maintaskstate++;
		 }
		 break;
	case 6: if (e_FRS_Done==BQ25703_Read(ADCOption, &data))
		        {maintaskstate++;};
						break;	 
	 case 7:  data=0;
	        	wrstate=BQ25703_Wr_Check(       bq25703,
												bq25703InitData[ADCOption].I2cRecord,
												data,
												cPriorityDefault,
												voidfun8
	        									);
		 if (e_FRS_Done==wrstate)
		 {
		  maintaskstate++;
		 }
		 break;
		 case 8:  data=(bq25703InitData[ChargeOption0].data)&(~0x8000);
	        	wrstate=BQ25703_Wr_Check(       bq25703,
												bq25703InitData[ChargeOption0].I2cRecord,
												data,
												cPriorityDefault,
												voidfun8
	        									);
		 if (e_FRS_Done==wrstate)
		 {
		  maintaskstate++;
		 }
		 break;
	 case 9: 
     data=bq25703InitData[ChargeOption0].data|(0x8000);
	        	wrstate=BQ25703_Wr_Check(       bq25703,
												bq25703InitData[ChargeOption0].I2cRecord,
												data,
												cPriorityDefault,
												voidfun8
	        									);
		 if (e_FRS_Done==wrstate)
		 {
		  maintaskstate++;
		 }
	   break;
   case 10: wrstate=BQ25703_Read(ChargeOption0, &data);
		        if (e_FRS_Done==wrstate)
		        {maintaskstate++;};
						break;
				
   case 11: if (e_FRS_Done==BQ28z610_Read(e_BQ28z610_Temperature,&pv_BQ28z610_Temperature,testACC))
		        {maintaskstate++;};
  	  	  	  break; 		 
   case 12: data=3300;
						if (e_FRS_Done==BQ28z610_AltManufacturerAccessCommand(BQ28z610_Command_Sleep,testACC))
		        //if (e_FRS_Done==BQ28z610_AltManufacturerAccessDFWrite(0x46c9, (uint8_t*)&data, 2,testACC))
            {maintaskstate++;};
            break;
   case 13: //data=4400;
		        //if (e_FRS_Done==BQ28z610_AltManufacturerAccessDFWrite(0x46c9, (uint8_t*)&data, 2,testACC))
	                     //#define I2C_OP_READ         ((unsigned short)(0))
//	          if (e_FRS_Done==TPS65982_6_RW(TPS87, e_TPS65987_BootFlagsRegister, outdata, 13, I2C_OP_READ))
//						tpsFlashUpdate();
//						indata[0]=0;
//		if (e_FRS_Done==TPS65982_6_CMD_U(TPS87,e_TPS_CMD_FLrr,indata,1,outdata,4))
            {maintaskstate++;};
            break;
   case 14: //readDataFromFile();
		         {maintaskstate++;};
//						if (e_FRS_Done==TPS65982_6_RDO_R(TPS87,  &I86, &V86))
//		         {maintaskstate++;};
//	       rstatel=MainTransition(testkey);
//	       if (e_FRS_Done==rstatel)
//                                 {maintaskstate++;};
//	       if (e_FRS_DoneError==rstatel)
//                                 {maintaskstate++;};
           break;
  default: maintaskstate=0;rstatel=e_FRS_Done;
  };
return rstatel;
};



void SuperLoopACC(void)
{
	uint16_t data;
	
//	readDataFromFile();	//for debug
	
	e_FunctionReturnState wrstate;
	static uint8_t state=0;
	switch (state)
	{ 
		case 0: if (e_FRS_Done==testACC())
            {maintaskstate=0;
							state++;
					  };
       break;
		case 1: LoopACC();
			 break;
		default:state=0;	
	};	
}


void SuperLoopACC_init(void)
{
  SLAcc_GoToSleep=false;	
	SLAcc_PowerState=e_PS_Work;
	
	initI2c1();
  I2C_API_INIT();
  BQ25703_DriverReset();
  //while ((get_sysTick_ms()-time) <500) {;};
};
