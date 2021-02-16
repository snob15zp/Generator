#ifndef SL_CommModbus_h
#define SL_CommModbus_h

#include <stdbool.h>
#include "BoardSetup.h"

#define USBcommPause 100000 //max
#define USBcommPauseErase 1000;

extern int  SL_CommModbus(void);
extern int SL_CommModbusInit(void);

extern systemticks_t USBcommLastTime;
//extern bool USBcommLastTimeSet;

#endif 
