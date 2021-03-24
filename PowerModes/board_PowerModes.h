#ifndef _board_PowerModes_H
#define _board_PowerModes_H

//#include "mainFSM.h"
//#include "superloopDisplay.h"
//#include "superloop_Player.h"
#include <stdint.h>
#include <stdbool.h>


//moduls
#define PM_Display 2
#define PM_Player 1
#define PM_Communication 4

#define PM_ClearPendingButton  EXTI->RPR1 |= EXTI_RPR1_RPIF5
#define PM_ClearPendingTPSIRQ  EXTI->FPR1 |= EXTI_FPR1_FPIF7

extern void PM_OnOffPWR(uint8_t modul, bool newstate);

extern void SuperLoop_PowerModes_Init(void);
extern void SuperLoop_PowerModes(void);

/**
* Put device to sleep
*/
void sleep();

#endif

