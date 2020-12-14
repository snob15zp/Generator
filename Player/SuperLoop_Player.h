#ifndef superloopPlayer_h
#define superloopPlayer_h

#include <stdbool.h>
#include <math.h>
#include "stm32g0xx.h"
#include "SuperLoop_Comm.h"
#include "superloopDisplay.h"

// for interraction with PWR
typedef enum  {e_FSMS_SLPl_Off,e_FSMS_SLPl_On,e_FSMS_SLPl_NumOfEl} e_FSMState_SuperLoopPlayer;
extern __inline e_FSMState_SuperLoopPlayer SLPl_FSMState(void);


// For main
extern void SLP_init(void);
extern void SLP(void);

//for power
extern bool SuperLoop_Player_SleepIn(void);
extern bool SuperLoop_Player_SleepOut(void);

//for player


#define FREQ_CW 			0x09
#define MULT_REG1_CW 	0x05
#define MULT_REG2_CW 	0x0A
#define CRC_CW				0xA0

#define CONF_BUFF_SIZE 1000
#define MULT_VAL_1 21000
#define MULT_VAL_2 21000

typedef struct {
	uint16_t playStart						:1;
	uint16_t playBegin						:1;
	uint16_t fpgaConfig						:1;
	uint16_t playStop							:1;
	uint16_t fpgaConfigComplete		:1;
	uint16_t fileListUpdate				:1;
	uint16_t labelsUpdate					:1;
	uint16_t clockStart						:1;
	uint16_t nextFreq							:1;
	uint16_t endOfFile						:1;
	uint16_t addListItem					:1;
} t_fpgaFlags;

extern volatile t_fpgaFlags fpgaFlags;

void fpgaConfig(void);
void getFileList(void);
void timeToString(uint8_t* timeArr);
void getControlParam(uint16_t fileSect);
//void loadDataToFpga(uint16_t fileSect);
uint32_t calcFreq(uint32_t val);
void getCrc(void);
void spi2FifoClr(void);
void loadMultToFpga(void);
void loadFreqToFpga(uint16_t addr);
void startFpga(void);

extern void setTotalTimer(void);
extern void setFileTimer(void);
extern void SecToHhMmSs(uint32_t timeInSec);

//for SPI2
void initSpi_2(void);
void spi2Transmit(uint8_t *pData, uint16_t Size);
void loadDataToFpga(uint16_t addr);

//for TIM3
//extern volatile uint32_t playClk;
void tim3Init(void);
void delay_ms(uint32_t delayTime);
//uint32_t getTick(void);
extern volatile uint32_t playClk;

#endif