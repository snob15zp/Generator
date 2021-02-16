/*
 * superloop.h
 *
 *  Created on: Feb 13, 2019
 *      Author: 2382
 */

#ifndef SUPERLOOP_H_
#define SUPERLOOP_H_

#include <stdbool.h>
#include "PowerModes_Defs.h"

typedef enum  
{SLAcc_Initial  	
,SLAcc_batEmpty	
,SLAcc_batLowLev  						
,SLAcc_batMedLev 		
,SLAcc_batHighLev 							
,SLAcc_batFull
,SLAcc_batCharging 	
,SLAcc_NumOfElements	
} e_SLAcc_BatStatus;

extern e_SLAcc_BatStatus Get_SLAcc_BatteryStatus(void);

//----------------------------------for main---------------------------------------------
extern void SuperLoopACC(void);
extern void SuperLoopACC_init(void);

//---------------------------------for power---------------------------------------------
//typedef enum  {e_AS_NoSysPwr,e_AS_IsSysPwr,e_AS_NumOfel} e_Acc_State;

e_PowerState SLAcc_GetPowerState(void);
e_PowerState SLAcc_SetSleepState(bool state);
 

#endif /* SUPERLOOP_H_ */
