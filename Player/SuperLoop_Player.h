#ifndef superloopPlayer_h
#define superloopPlayer_h

#include <stdbool.h>
#include <math.h>
#include <stm32g0xx.h>
#include "GlobalKey.h"
#include "PowerModes_Defs.h"
#include "SuperLoop_Comm2.h"
//#include "superloopDisplay.h"
#include "I2C_COMMON1.h"




//for player


#define FREQ_CW 			0x09
#define MULT_REG1_CW 	0x05
#define MULT_REG2_CW 	0x0A
#define CRC_CW				0xA0

#define CONF_BUFF_SIZE 1000
#define MULT_VAL_1 21000
#define MULT_VAL_2 21000

typedef enum  
{SLPl_FSM_InitialWait  		//work
,SLPl_FSM_off  						//e_PS_ReadySleep
,SLPl_FSM_OnTransition 		//work
,SLPl_FSM_On 							//work
,SLPl_FSM_OffTransition 	//work
,SLPl_FSM_NumOfElements	
} e_SLPl_FSM;

typedef struct {
	uint32_t playStart						:1;
	uint32_t playBegin						:1;
	uint32_t fpgaConfig						:1;
	uint32_t playStop							:1;
	uint32_t fpgaConfigComplete		:1;
	uint32_t fileListUpdate				:1;
	uint32_t labelsUpdate					:1;
	uint32_t clockStart						:1;
	uint32_t nextFreq							:1;
	uint32_t endOfFile						:1;
	uint32_t addListItem					:1;
	uint32_t addNewListItem				:1;
	uint32_t clearList						:1;
	uint32_t timeUpdate						:1;
} t_fpgaFlags;

//extern volatile t_fpgaFlags fpgaFlags;

extern uint16_t SLPl_ui16_NumOffiles;


//extern char SLPl_filenameWOE[D_FileNameLength+1];

void SLPl_Start(uint32_t nof);
void SLPl_Stop();


void timeToString(uint8_t* timeArr);

void getTimers(void);
extern void SecToHhMmSs(uint32_t timeInSec);

void tim3Init(void);

extern volatile uint32_t playClk;
extern volatile uint32_t progBarClk;

//for power
extern  e_PowerState SLPl_GetPowerState(void);
extern  e_PowerState SLPl_SetSleepState(bool state);
extern  e_SLPl_FSM Get_SLPl_FSM_State(void);
extern bool SLPl_FFSFree(void);

// For main
void SLP_init(void);
void SLP(void);

//For display
extern uint8_t curState;
extern uint16_t playFileSector;

//For superloopComm

void SLPl_InitFiles(void);


#endif