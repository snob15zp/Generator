/*
 * DriverBQ28z610.h
 *
 *  Created on: Feb 21, 2019
 *      Author: 2382
 */

#ifndef DRIVERBQ28Z610_H_
#define DRIVERBQ28Z610_H_

#include "i2c_API.h"

#define BQ28z610_Command_Sleep 0x0011

#define BQ28z610_BatteryStatus_FullyDischarged (1<<4)

typedef enum {
				 e_BQ28z610_Temperature
				,e_BQ28z610_Voltage
	      ,e_BQ28z610_RelativeStateOfCharge
	      ,e_BQ28z610_AltManufacturerAccess
	      ,e_BQ28z610_MACDataSum
	      ,e_BQ28z610_BatteryStatus //BatteryStatus()
				,e_BQ28z610_NumOfReg
} e_BQ28z610_Registers;


extern e_FunctionReturnState BQ28z610_Read(e_BQ28z610_Registers reg, uint16_t *data,void* key);
extern e_FunctionReturnState BQ28z610_AltManufacturerAccessDFWrite(uint16_t address, uint8_t * data, uint8_t size,void* key);
extern e_FunctionReturnState BQ28z610_AltManufacturerAccessCommand(uint16_t command,void* key);

extern void BQ28z610_DriverReset(void);
extern e_FunctionReturnState BQ28z610_DriverState(void);


#endif /* DRIVERBQ28Z610_H_ */
