/*
 * mainFSM.c
 *
 *  Created on: 8 мая 2019 г.
 *      Author: RD
 */

#include "regBQ25703A.h"
#include "DriverBQ25703.h"
#include "DriverBQ28z610.h"
#include "DriverTPS65982_6.h"

#include "mainFSM.h"
#include "battery.h"
#include "BoardSetup.h"

//#include "stm32l0xx_hal.h"
typedef uint32_t key_type;
typedef uint32_t t_TransitionFunctionType;

//e_FunctionReturnState BQ25703_ADCVSYSVBAT_Read(uint16_t *Vsys,uint16_t *Vbat)//V. mV
typedef enum  {e_TF_inh,e_TF_hiz,e_TF_25703init,e_TF_IIN200,e_TF_hizOff,e_TF_inhOff //6  //don't dependence form input
,e_TF_ReadTPSState,e_TF_BQ28z610_Reads //+2 =8
,e_TF_BQ25703_ADCIBAT_Read	//+1=11 //input
,e_TF_SignChargeOff,e_TF_SignCharge,e_TF_SignRestOff,e_TF_SignRest //+4=15  // to do desision
,e_TF_BQ25703_InputCurrentSet,e_TF_BQ25703_InputCurrentWrite,e_TF_BatteryFSM,e_TF_BQ25703_Charge_Check  //+3=18   //write calculated form input data value
,e_TF_BQ25703_VSYSVBAT_Read,e_TF_VsysAnaliz //+2=20
,e_TF_NumOfel} e_TransitionFunctionType;

#define m_inh (1<<e_TF_inh)
#define m_hiz (1<<e_TF_hiz)
#define m_25703init (1<<e_TF_25703init)
#define m_IIN200 (1<<e_TF_IIN200)
#define m_hizOff (1<<e_TF_hizOff)
#define m_inhOff (1<<e_TF_inhOff)

//#define m_ClrTPSInt (1<<e_TF_ClrTPSInt)
#define m_ReadTPSState (1<<e_TF_ReadTPSState)
#define m_BQ28z610_Reads (1<<e_TF_BQ28z610_Reads)
#define m_BQ25703_ADCIBAT_Read (1<<e_TF_BQ25703_ADCIBAT_Read)
#define m_BQ25703_VSYSVBAT_Read (1<<e_TF_BQ25703_VSYSVBAT_Read) 

#define m_SignChargeOff (1<<e_TF_SignChargeOff)
#define m_SignCharge (1<<e_TF_SignCharge)
#define m_SignRestOff (1<<e_TF_SignRestOff)
#define m_SignRest (1<<e_TF_SignRest)
#define m_VsysAnaliz (1<<e_TF_VsysAnaliz)

#define m_BQ25703_InputCurrentSet (1<<e_TF_BQ25703_InputCurrentSet)
#define m_BQ25703_InputCurrentWrite (1<<e_TF_BQ25703_InputCurrentWrite)
#define m_BatteryFSM (1<<e_TF_BatteryFSM)
#define m_BQ25703_Charge_Check (1<<e_TF_BQ25703_Charge_Check)	

#define testkey (m_25703init|m_hizOff|m_inhOff)

// to from
#define key_Charge_Charge \
(m_ReadTPSState|m_BQ28z610_Reads|m_BQ25703_ADCIBAT_Read\
|m_BQ25703_VSYSVBAT_Read  \
|m_SignCharge|m_VsysAnaliz   \
 |m_BQ25703_InputCurrentSet|m_BQ25703_InputCurrentWrite|m_BatteryFSM|m_BQ25703_Charge_Check|m_hizOff|m_inhOff)       //steady
#define key_Charge_Rest   (m_hizOff|m_inhOff)
#define key_Charge_OffCharge (m_IIN200|m_hizOff|m_inhOff)  
#define key_Charge_OffRest 0
#define key_Charge_Init 0

#define  key_Rest_Charge (m_hizOff|m_inh)
#define key_Rest_Rest \
(m_ReadTPSState|m_BQ28z610_Reads|m_BQ25703_ADCIBAT_Read \
|m_BQ25703_VSYSVBAT_Read  \
|m_SignRest|m_VsysAnaliz \
|m_BQ25703_InputCurrentSet|m_BQ25703_InputCurrentWrite) //steady
#define key_Rest_OffCharge 0
#define key_Rest_OffRest (m_hizOff|m_inh)
#define key_Rest_Init (0)

#define key_OffCharge_Charge  (m_inh)
#define key_OffCharge_Rest 0
#define key_OffCharge_OffCharge ( \
m_ReadTPSState|m_BQ28z610_Reads|m_BQ25703_ADCIBAT_Read\
|m_BQ25703_VSYSVBAT_Read  \
|m_SignChargeOff|m_VsysAnaliz|m_BQ25703_InputCurrentSet|m_BQ25703_InputCurrentWrite )  //steady
#define key_OffCharge_OffRest 0
#define key_OffCharge_Init 0

#define key_OffRest_Charge  0
#define key_OffRest_Rest (m_inh)
#define key_OffRest_OffCharge 0
#define key_OffRest_OffRest ( \
m_ReadTPSState|m_BQ28z610_Reads|m_BQ25703_ADCIBAT_Read\
|m_BQ25703_VSYSVBAT_Read  \
|m_SignRestOff|m_VsysAnaliz |m_BQ25703_InputCurrentSet|m_BQ25703_InputCurrentWrite)  //steady
#define key_OffRest_Init (m_25703init)


//static uint16_t mtProchotStatus;
//static uint16_t mtChargerStatus;
//static uint16_t mtIIN_DPM;
//static uint16_t mtIIN_HOST;
//static uint16_t mte_BQ28z610_Temperature;
//static uint16_t mte_BQ28z610_Voltage;

//static uint16_t mtChargeCurrent;
//static uint16_t mtChargeOption3;
//static uint16_t mtChargeOption0;
//static uint16_t mtADCIBAT;
//static uint16_t mtADCOption;
//static uint16_t mtADCVSYSVBAT;

#define CurrentAdditional 100 //mA to LDO

 bool bVSYS;
 
 uint16_t I87;
 uint16_t V87;
 
 uint16_t mFSM_BQ28z610_Temperature;
 uint16_t pv_BQ28z610_Voltage;
 uint16_t mFSM_BQ28z610_RSOC;
 uint16_t mFSM_BQ28z610_BatteryStatus;

 uint16_t pvIcharge;
 uint16_t pvIdescharge;
 uint16_t pvVSYS;
 uint16_t pvVBAT;

 int ChargeCurr;
 int InCurrent;



const key_type TransitionKeys[4][5]=  //int a[ROWS][COLS] = 
{
{key_Charge_Charge ,key_Charge_Rest,key_Charge_OffCharge,key_Charge_OffRest,key_Charge_Init},
{key_Rest_Charge ,key_Rest_Rest,key_Rest_OffCharge,key_Rest_OffRest,key_Rest_Init},
{key_OffCharge_Charge ,key_OffCharge_Rest,key_OffCharge_OffCharge,key_OffCharge_OffRest,key_OffCharge_Init},
{key_OffRest_Charge ,key_OffRest_Rest,key_OffRest_OffCharge,key_OffRest_OffRest,key_OffRest_Init},
//{key_Init_Charge ,key_Init_Rest,key_Init_OffCharge,key_Init_OffRest,key_Init_Init},
};



e_FunctionReturnState TransitionFunction(uint8_t state);
e_FunctionReturnState  MainTransition(key_type key);
e_FunctionReturnState Readbq28z610(void);


e_FSM_State mainFMSstate=e_FSM_Init;
static e_FSM_State sign=e_FSM_RestOff;

extern __inline e_FSM_State SLAcc_FSMState(void)
{
	return mainFMSstate;
};




e_FunctionReturnState  mainFSMfunction(void)
{
 static uint8_t state=0;

 e_FunctionReturnState rstate;
 e_FunctionReturnState rstatel;
 rstate=e_FRS_Processing;
 switch (state)
 {
  case 0: rstatel=MainTransition(TransitionKeys[sign][mainFMSstate]); //TransitionKeys
          if (e_FRS_Done==rstatel)
             { mainFMSstate=sign;
               state++;
//               rstate=e_FRS_Done;
             };
          if (e_FRS_DoneError==rstatel)   //if error on transition, go to previos state ????
             { state++;
//               rstate=e_FRS_Done;
             };
          break;
  case 1: rstatel=MainTransition(TransitionKeys[mainFMSstate][mainFMSstate]);    //Steady state
          if ((e_FRS_Done==rstatel)||(e_FRS_DoneError==rstatel))                     //sign is renewed here
          { if (mainFMSstate!=sign)                                                  
            {state++;
            };
            rstate=rstatel;
          };
          break;
  default: state=0;
 };

 return rstate;

}




t_TransitionFunctionType mFSM_Error;

e_FunctionReturnState  MainTransition(key_type key)
{
	static uint8_t state=0;
	static uint8_t state1=0;
	e_FunctionReturnState rstate;
	e_FunctionReturnState rstatel;
	rstate=e_FRS_Processing;

	switch (state1)
	{
	
	case 0:	if (((key>>state)&1)!=0)
		    { rstatel=TransitionFunction(state);
              if (e_FRS_Done==rstatel)
                                      {state++;state1++;};
              if (e_FRS_DoneError==rstatel)
									{ mFSM_Error|=1<<state;  // Tries to do everything regardless of mistakes
										state++;state1++;
									};  
	        }
	        else
	        {state++;state1++;};
	        break;
	case 1:// rstatel=readStatus();
          	//if (e_FRS_Done==rstatel)
          	                          {state1++;};
          	//if (e_FRS_DoneError==rstatel)
          	//                          {state1++;};
	        break;
	default: state1=0;
		     if (state>=e_TF_NumOfel)  
					{ 
						if (mFSM_Error==0) 
						{rstate=e_FRS_Done;}
						else
						{rstate=e_FRS_DoneError;};
					state=0;
					mFSM_Error=0;	
					};   //exit
	};
	return rstate;
}


//e_TF_inh,e_TF_hiz,e_TF_25703init,e_TF_hizOff,e_TF_inhOff
//,e_TF_SignRestI,e_TF_SignChargeI,e_TF_SignOffHiZ,e_TF_SignWorkHiZ
//,e_TF_ReadTPSState,e_TF_BQ28z610_Read_Temperature,e_TF_BQ28z610_Read_Voltage,e_TF_BQ25703_ADCIBAT_Read	
//,e_TF_BQ25703_InputCurrent,e_TF_BatteryFSM,e_TF_BQ25703_Charge_Check

static unsigned char u8_11_ff[11]={0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff};//ToDo do const
static bool bADCVSYSVBAT;
static bool bAccAvailability;
static systemticks_t SystemTicksOld;

e_FunctionReturnState WriteBQ25703(void);

#define D_incCurPause 1000
#define D_BQ28z610_BatteryStatus_INITmask (1<<7)
#define D_BQ28z610_BatteryStatus_TDAmask (1<<11)
e_FunctionReturnState TransitionFunction(uint8_t state)
{   e_FunctionReturnState rstate;
	switch (state)
	{ int currl;
	  case e_TF_inh: rstate=BQ25703_SetBits_Check(ChargeOption0,ChargeOption0_CHRG_INHIBIT,0); break;//0
	  case e_TF_inhOff: rstate=BQ25703_SetBits_Check(ChargeOption0,0,ChargeOption0_CHRG_INHIBIT); break;//1
	  case e_TF_hiz: rstate=BQ25703_SetBits_Check(ChargeOption3,ChargeOption3_EN_HIZ,0); break;//2
	  case e_TF_hizOff: rstate=BQ25703_SetBits_Check(ChargeOption3,0,ChargeOption3_EN_HIZ); break;//3
  	case e_TF_25703init: rstate=BQ25703_Init_Check(); break;//4
		case e_TF_IIN200: //currl=200;
			                //if (currl>I87) currl=I87;
			                //rstate=BQ25703_IIN_Check(currl);
		                  rstate=e_FRS_Done;
                      break;    //5
		case e_TF_ReadTPSState:              rstate=ReadTPSState();  break;//6
//		case e_TF_BQ28z610_Read_Temperature: rstate=BQ28z610_Read(e_BQ28z610_Temperature,&mFSM_BQ28z610_Temperature,mainFSMfunction);break;//7  
//		case e_TF_BQ28z610_Read_Voltage:     rstate=BQ28z610_Read(e_BQ28z610_Voltage,&pv_BQ28z610_Voltage,mainFSMfunction);   break;//8
		case e_TF_BQ25703_ADCIBAT_Read:      rstate=BQ25703_ADCIBAT_Read(&pvIcharge,&pvIdescharge);   break;//9
		case e_TF_BQ28z610_Reads:            rstate=Readbq28z610();
//		                                   rstate=BQ28z610_Read(e_BQ28z610_RelativeStateOfCharge,&mFSM_BQ28z610_RSOC,mainFSMfunction);
                                         break;
		case e_TF_BQ25703_VSYSVBAT_Read:     rstate=BQ25703_ADCVSYSVBAT_Read(&pvVSYS,&pvVBAT);bADCVSYSVBAT=true;   break;//9
		
	  case e_TF_SignChargeOff:
			                     InCurrent=0;
		                       sign=mainFMSstate;  
			                     if (!((I87==0)||(0==V87))) 
			                        sign=e_FSM_Charge;
													 rstate=e_FRS_Done;
			                     break;//10
	  case e_TF_SignCharge:  sign=mainFMSstate;
			                     if ((VOLTAGE_CHARGE_END<pv_BQ28z610_Voltage/2)&&(BatteryIChargeCutOff>pvIcharge))
			                         sign=e_FSM_Rest;
													 if (0!=(mFSM_Error &(m_BQ28z610_Reads
														                   |m_BQ25703_ADCIBAT_Read))) 
			                        sign=e_FSM_Rest;
			                     if ((I87==0)||(0==V87)||(0!=(mFSM_Error&(m_ReadTPSState)))) 
			                        sign=e_FSM_ChargeOff;
													 rstate=e_FRS_Done;
			                     break;//11
	  case e_TF_SignRestOff: InCurrent=0;
			                     sign=mainFMSstate; 
			                     if (!((I87==0)||(0==V87))) 
			                        sign=e_FSM_Rest;
													 rstate=e_FRS_Done;
			                     break;//12
	  case e_TF_SignRest:    
			                     sign=mainFMSstate;
			                     if (VOLTAGE_CHARGE_RENEWAL>pv_BQ28z610_Voltage/2)
			                         sign=e_FSM_Charge;
			                     if ((I87==0)||(0==V87)) 
			                        sign=e_FSM_RestOff;
													 rstate=e_FRS_Done;
			                     break; 
													 
                           			
		case e_TF_VsysAnaliz:  if ((0==(mFSM_Error&(m_BQ25703_VSYSVBAT_Read)))&&bADCVSYSVBAT)  
		                          { 
                               if (((e_FSM_RestOff==mainFMSstate)||(e_FSM_ChargeOff==mainFMSstate))
																 &&(0==(mFSM_Error&(m_BQ28z610_Reads))))
															 {
																  bVSYS=(pvVSYS>6000)
																      &&(pv_BQ28z610_Voltage>6000)
																      &&(0==(mFSM_BQ28z610_BatteryStatus&
																             (D_BQ28z610_BatteryStatus_TDAmask)
																            )
																        );
																  bAccAvailability=bVSYS;
															 };
                               if ((e_FSM_Rest==mainFMSstate)||(e_FSM_Charge==mainFMSstate))
															 {
																  bVSYS=(pvVSYS>6000)
																   &&((InCurrent>300)
																 ||    bAccAvailability
																     );
															 };
															 //if (!((e_FSM_Rest==mainFMSstate)||(e_FSM_Charge==mainFMSstate)))//debug
															 //	 bVSYS=false;                                                  //debug
															};	
														bADCVSYSVBAT=false;
														rstate=e_FRS_Done;
		                       break;
															
		
		case e_TF_BQ25703_InputCurrentSet: 
			                            if (0!=(mFSM_Error&(m_ReadTPSState)))
																			{rstate=e_FRS_Done;}
																		else 
																		{	
																			 
																		if ((SystemTicksOld+D_incCurPause)<SystemTicks)
																		{
																			SystemTicksOld=SystemTicks;   
																			InCurrent+=100;
																		}	
													          if (InCurrent>(I87-CurrentAdditional))
																			           InCurrent=I87-CurrentAdditional; 
													          if  (InCurrent<0)
																			  InCurrent=0;
																		if  (InCurrent>2900)
																			  InCurrent=2900;
																		 rstate=e_FRS_Done;
																	  };
																		break;
    case e_TF_BQ25703_InputCurrentWrite:	
                              			if (0!=(mFSM_Error&(m_ReadTPSState)))
																			{rstate=e_FRS_Done;}
																		else 	
																		  {
                                      rstate=BQ25703_IIN_Check(InCurrent);
																   	  };																			
			                              break;
		case e_TF_BatteryFSM:           ChargeCurr=fChargeCurrent(mFSM_BQ28z610_Temperature,pv_BQ28z610_Voltage/2);
													          rstate=e_FRS_Done;
													          break;
		case e_TF_BQ25703_Charge_Check: rstate=WriteBQ25703();
//			                                if (
//			                                 (
//			                                 (0!=(mFSM_Error &m_BQ28z610_Reads))
//		                                    )||
//		                                   ( 
//		                                   (0==mFSM_BQ28z610_Temperature)  //not power from 25703
//														         &&(0==pv_BQ28z610_Voltage)
//													           &&(0==mFSM_BQ28z610_RSOC)
//		                                    )
//		                                    )
//																				{rstate=BQ25703_Charge_Check(300); }  //If something is wrong, we will provide a charge
//																				else if (0!=(mFSM_Error &(
//														                    m_BQ28z610_Reads
//													                     |m_BQ25703_ADCIBAT_Read)))
//																			         {rstate=e_FRS_Done;}
//                                             else 																			
//																							{rstate=BQ25703_Charge_Check(ChargeCurr); };
																		break;
		
  	default: 	rstate=e_FRS_DoneError;
	}
	return rstate;
}

static uint16_t ChargeVolt;

e_FunctionReturnState WriteBQ25703(void)
{ static uint8_t state;
	//static uint8_t buf[20];
	
	e_FunctionReturnState returnstate,returnstatel;
	  returnstate=e_FRS_Processing;
	  switch(state)
	  {
	  case 0:  
			   if (mFSM_BQ28z610_RSOC<50)
				 {ChargeVolt=4200;}
				 else
				 {ChargeVolt=4200;}
	       state++;
//						 break;
	  case 1: 
			   
      	 returnstatel=BQ25703_Write_Check(MaxChargeVoltage, ((ChargeVolt*2)&0x7ff0));
	
			   if (e_FRS_Done==returnstatel)
	           {state++;};
			   if (e_FRS_DoneError==returnstatel)
	           {state=101;};
						 break;
		case 2:
                                    if (
			                                 (
			                                 (0!=(mFSM_Error &m_BQ28z610_Reads))
		                                    )||
		                                   ( 
		                                   (0==mFSM_BQ28z610_Temperature)  //not power from 25703
														         &&(0==pv_BQ28z610_Voltage)
													           &&(0==mFSM_BQ28z610_RSOC)
		                                    )
		                                    )
																				{returnstatel=BQ25703_Charge_Check(300); }  //If something is wrong, we will provide a charge
																				else if (0!=(mFSM_Error &(
														                    m_BQ28z610_Reads
													                     |m_BQ25703_ADCIBAT_Read)))
																			         {state=100;}
                                             else 																			
																							{returnstatel=BQ25703_Charge_Check(ChargeCurr); };
				 if (e_FRS_Done==returnstatel)
	           {state=100;};
			   if (e_FRS_DoneError==returnstatel)
	           {state=101;};
						 break;
		case 100:
      			returnstate=e_FRS_Done;//Normal exit
						state=0;
			      break;
		case 101: 
						returnstate=e_FRS_DoneError;		//Error
            state=0;						
						break;
	  default:  state=0;
	  }
	  return returnstate;
}




/**

e_BQ28z610_BatteryStatus
*/
#define Readbq28z610_Error_State 101
#define Readbq28z610_Done_State 100
static uint16_t tempdata;
e_FunctionReturnState Readbq28z610(void)
{ static uint8_t state;
//  uint16_t mFSM_BQ28z610_RSOC;
	//static uint8_t buf[20];
	
	e_FunctionReturnState returnstate,returnstatel;
	  returnstate=e_FRS_Processing;
	  switch(state)
	  {
	  case 0:  
      	 returnstatel=BQ28z610_Read(e_BQ28z610_BatteryStatus,&mFSM_BQ28z610_BatteryStatus,mainFSMfunction);
			   if (e_FRS_Done==returnstatel)
	           {state++;};
			   if (e_FRS_DoneError==returnstatel)
	           {state=Readbq28z610_Error_State;};
						 break;
	  case 1:  
      	 returnstatel=BQ28z610_Read(e_BQ28z610_Temperature,&mFSM_BQ28z610_Temperature,mainFSMfunction);
			   if (e_FRS_Done==returnstatel)
	           {state++;};
			   if (e_FRS_DoneError==returnstatel)
	           {state=Readbq28z610_Error_State;};
						 break;
	  case 2:  
      	 returnstatel=BQ28z610_Read(e_BQ28z610_Voltage,&pv_BQ28z610_Voltage,mainFSMfunction);
			   if (e_FRS_Done==returnstatel)
	           {state++;};
			   if (e_FRS_DoneError==returnstatel)
	           {state=Readbq28z610_Error_State;};
						 break;
	  case 3:  
      	 returnstatel=BQ28z610_Read(e_BQ28z610_RelativeStateOfCharge,&tempdata,mainFSMfunction);
			   if (e_FRS_Done==returnstatel)
	           { mFSM_BQ28z610_RSOC=tempdata;
						   state++;
						 };
			   if (e_FRS_DoneError==returnstatel)
	           {state=Readbq28z610_Error_State;};
						 break;
		case 4://rstate=BQ25703_ADCIBAT_Read(&pvIcharge,&pvIdescharge);				 
      	 returnstatel=BQ25703_Read(IIN_DPM, &tempdata);
			   if (e_FRS_Done==returnstatel)
	           {state=Readbq28z610_Done_State;};
			   if (e_FRS_DoneError==returnstatel)
	           {state=Readbq28z610_Error_State;};
						 break;
						 
		case Readbq28z610_Done_State:
      			returnstate=e_FRS_Done;//Normal exit
						state=0;
			      break;
		case Readbq28z610_Error_State: 
						returnstate=e_FRS_DoneError;		//Error
            state=0;						
						break;
	  default:  state=0;
	  }
	  return returnstate;
}






e_FunctionReturnState ReadTPSState(void)
{ static uint8_t state;
	static uint8_t buf[20];
	e_FunctionReturnState returnstate,returnstatel;
	  returnstate=e_FRS_Processing;
	  switch(state)
	  {
	  case 0:  //read interrupt    //debug
      	  returnstatel=TPS65982_6_RW(TPS87,  e_TPS65987_IntEvent1, buf,  11,  I2C_OP_READ);
			   if (e_FRS_Done==returnstatel)
	           {state++;};
			   if (e_FRS_DoneError==returnstatel)
	           {state=101;};
						 break;
    case 1:			// clear interrupt
			       returnstatel=TPS65982_6_RW(TPS87,  e_TPS65987_IntClear1, u8_11_ff,  11,  I2C_OP_WRITE);
			   if (e_FRS_Done==returnstatel)
	           {state++;};
			   if (e_FRS_DoneError==returnstatel)
	           {state=101;};
						 break;
		case 2:				 //read interrupt    //debug
		      	  returnstatel=TPS65982_6_RW(TPS87,  e_TPS65987_IntEvent1, buf,  11,  I2C_OP_READ);
			   if (e_FRS_Done==returnstatel)
	           {state++;};
			   if (e_FRS_DoneError==returnstatel)
	           {state=101;};
						 break;
				 
		case 3:	
	          {returnstatel=TPS65982_6_RDO_R(TPS87,  &I87, &V87);
			   if (e_FRS_Done==returnstatel)
	           {state++;};
			   if (e_FRS_DoneError==returnstatel)
	           {state=101;};
	          }
			  break;
		case 4://				e_TPS65987_PortControl
	          returnstatel=TPS65982_6_RW(TPS87,  e_TPS65987_PowerStatusRegister, buf,  255,  I2C_OP_READ);
			   if (e_FRS_Done==returnstatel)
	           {state=100;};
			   if (e_FRS_DoneError==returnstatel)
	           {state=101;};
			  break;
		case 100:	returnstate=e_FRS_Done;//Normal exit
						state=0;
			      break;
		case 101: I87=0; V87=0;
						returnstate=e_FRS_DoneError;		//Error
            state=0;						
						break;
	  default:  state=0;
	  }
	  return returnstate;
}



//  ----------------------- not used functions -------------------------------------------------



//uint8_t BatteryStatus=1;
//e_FunctionReturnState ControlBattery()
//{
//	static uint16_t I;
//	static uint8_t state=0;
//	static uint16_t pause;
//	e_FunctionReturnState rstate;
//	rstate=e_FRS_Processing;
//	switch(state)
//	  { case 0:if (e_FRS_Done==BQ28z610_Read(e_BQ28z610_Temperature,&mFSM_BQ28z610_Temperature))
//               {state++;
//								pause=1000; 
////                if (0==BatteryStatus) state=4;
//               };


//							 break;
//		  case 1:if (pause--==0) 	
//			           {state++;};				
//						 break;		 
//	    case 2:if (e_FRS_Done==BQ28z610_Read(e_BQ28z610_Voltage,&pv_BQ28z610_Voltage))
//               {state++;pause=1000;
////                if (0==BatteryStatus) state=4;
//               };
//	           break;
//		  case 3:if (pause--==0) 	
//			           {state++;};				
//						 break;		 

//			case 4:if (e_FRS_Done==BQ25703_ADCIBAT_Read(&pvIcharge,&pvIdescharge) )
//               {state++;pause=1000;
// //               if (0==BatteryStatus) state=4;
//               };
//               break;
//		  case 5:if (pause--==0) 	
//			           {state++;};				
//						 break;		 
//					 
//	    case 6:I=BatteryFSM(mFSM_BQ28z610_Temperature,pv_BQ28z610_Voltage,pvIcharge);
//	           if (e_FRS_Done==BQ25703_Charge_Check(I))
//               {state++;pause=1000;
// //               if (0==BatteryStatus) state=4;
//	           rstate=e_FRS_Done;
//               };
//	    	   break;
//		  case 7:if (pause--==0) 	
//			           {state=0;};				
//						 break;		 
//							 
///*	    case 4:if (e_FRS_Done==BQ25703_Charge_Check(0))
//               {
//               if (1==BatteryStatus) state=0;
//               rstate=e_FRS_Done;
//               };
//	    	   break;
//*/	    default: state=0;//rstate=e_FRS_Done;
//	  }
//	return rstate;
//}








//e_FunctionReturnState SetModes()
//{
//	static uint8_t modenum=0;
//	static uint8_t phase=0;
//	e_FunctionReturnState rstate;
//	rstate=e_FRS_Processing;
//	switch(phase)
//	{
//	case 0:
////		if (e_FRS_Done==BQ25703_SetMode_Check((modenum&0x3)))
//		           {phase++;};
//			       break;
//	case 1:
//		//if (e_FRS_Done==BQ25703_SetMode_Check((modenum>>2)&0x3))
//		//if (e_FRS_Done==BQ25703_Read(ChargeCurrent,&mtChargeCurrent))
//				           {phase++;};
//					       break;
//	case 2:
//			//if (e_FRS_Done==TPS65982_6_PSwap(TPS86,modenum&0x1,modenum&0x2,500*(modenum&3)))//0 - absent, else - present,mA
//		  // if (e_FRS_Done==BQ25703_Charge_Check(400))
//					           {phase++;};
//						       break;

//	case 3:
//		//if (e_FRS_Done==readStatus())
//				           {phase=0;modenum++;};
//		                   rstate=e_FRS_Done;
//					       break;


//	};
//	return rstate;
//}



/*
e_FunctionReturnState readStatus()
{ static uint8_t state=0;
e_FunctionReturnState rstate;
rstate=e_FRS_Processing;

switch(state)
  { case 2:{ if (e_FRS_Done==BQ25703_Read(ProchotStatus,&mtProchotStatus))
             {state++;};
  	  	  	  break;
  	  	   };
  case 3:{ if (e_FRS_Done==BQ25703_Read(ChargerStatus,&mtChargerStatus))
           {state++;};
  	  	  break;
   	   	 };
  case 0: { if (e_FRS_Done==BQ25703_Read(IIN_DPM,&mtIIN_DPM))
           {state++;};
	       break;
  	      };
  case 1: { if (e_FRS_Done==BQ25703_Read(ChargeCurrent,&mtChargeCurrent))
           {state++;};
	       break;
  	      };
  case 4: { if (e_FRS_Done==BQ28z610_Read(e_BQ28z610_Temperature,&mte_BQ28z610_Temperature))
           {state++;};
	       break;
  	      };
  case 5: { if (e_FRS_Done==BQ28z610_Read(e_BQ28z610_Voltage,&mte_BQ28z610_Voltage))
           {state++;};
	       break;
          };
  case 6: { if (e_FRS_Done==BQ25703_Read(ChargeOption3,&mtChargeOption3))
           {state++;};
	       break;
          };
  case 7: { if (e_FRS_Done==BQ25703_Read(ChargeOption0,&mtChargeOption0))
           {state++;};
	       break;

  	      };
  case 8:  if (e_FRS_Done==BQ25703_Read(ADCIBAT,&mtADCIBAT))
              {state++;};
            break;

  case 9:  if (e_FRS_Done==BQ25703_Read(ADCOption,&mtADCOption))
              {state++;};
            break;
  case 10:  if (e_FRS_Done==BQ25703_Read(ADCVSYSVBAT,&mtADCVSYSVBAT))
              {state++;};
            break;

  default:state=0;rstate=e_FRS_Done;
  };

return rstate;
}

*/

