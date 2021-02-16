#ifndef BQ28Z610_DATA_H_
#define BQ28Z610_DATA_H_

#include "stm32g0xx.h"
#include "spiffs.h"
#include "SuperLoop_Comm2.h"
#include <string.h>
#include "DriverBQ28z610.h"

e_FunctionReturnState readDataFromFile(void);
void dataProcessing(void);
uint8_t char2hex(char ch);

#endif