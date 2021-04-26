#ifndef SLC2_H
#define SLC2_H

#include "board_PowerModes.h"
#include "PowerModes_Defs.h"
#include "spiffs.h"
#include "fs.h"
#include "GlobalKey.h"

//#define DLE 27 //esc
//#define DTD '$'
//#define DRD '%'



extern e_PS_Int PS_Int;
extern bool byte_TX_DLE;
extern bool isUSBint;

extern s32_t File_List;

void on_tx_done_cb(void);
void tps65987_cb(void);
void bq28z610_cb(void);

void SLC_init(void);
void SLC(void);


extern  e_PowerState SLC_GetPowerState(void);
extern  e_PowerState SLC_SetSleepState(bool state);
extern  bool SLC_FFSEnable(void);
extern  bool SLC_SPIFFS_State(void);

void Communication_InSleep();
void Communication_OutSleep();

extern spiffs fs;



#endif
