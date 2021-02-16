#ifndef superloopDisplay_h
#define superloopDisplay_h

#include <stdbool.h>
//#include "stm32g0xx.h"
#include "gfx.h"
//#include "SuperLoop_Player.h"
//#include "SuperLoop_Comm.h"
#include "PowerModes_Defs.h"

typedef struct {
	uint32_t playStart						:1;
	uint32_t playStop							:1;
	uint32_t fileListUp   				:1;
	uint32_t fileListDown					:1;
} t_DisplayFlags;

void SetStatusString(char* s);
//extern t_DisplayFlags DisplayFlags;

extern uint8_t spiDispCapture;
//extern GHandle	ghList1, ghLabel3, ghLabel4, ghLabel5, ghLabel6, ghLabel7;

// for PowerControl
e_PowerState SLD_GetPowerState(void);
e_PowerState SLD_SetSleepState(bool state);
bool SLD_PWRState(void);


//For main
extern int SLD_init(void);
extern int SLD(void);


#endif
