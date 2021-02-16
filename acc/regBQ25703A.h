/*
 * regBQ25703A.h
 *
 *  Created on: Feb 19, 2019
 *      Author: 2382
 */

#ifndef REGBQ25703A_H_
#define REGBQ25703A_H_

//#include "DriverBQ25703.h"
#include "i2c_API.h"

typedef enum {
				ChargeOption0, //0
				ChargeCurrent,
				MaxChargeVoltage,
				ChargeOption1,
				ChargeOption2,
				ChargeOption3,
				ProchotOption0,
				ProchotOption1,
				ADCOption,
				ChargerStatus,
				ProchotStatus,//10
				IIN_DPM,
				ADCVBUSPSYS,
				ADCIBAT,
				ADCIINCMPIN,
				ADCVSYSVBAT,
				OTGVoltage,
				OTGCurrent,
				InputVoltage,
				MinSystemVoltage,
				IIN_HOST,//20
				ManufacturerID,
				DeviceID,
				eBQ25703_NumOfReg //23
} bq25703Registers;



extern const t_I2c16InitData bq25703InitData[eBQ25703_NumOfReg];

uint16_t BQ25703_ChargeCurrent_Eval(uint16_t I);//arg in mA
uint16_t BQ25703_ChargeVoltage_Eval(uint16_t V);//arg in mV
uint16_t BQ25703_MinSystemVoltage_Eval(uint16_t V);//arg in mV
uint16_t BQ25703_IIN_HOST_Eval(uint16_t I);//arg in mA
uint16_t BQ25703_InputVoltage_Eval(uint16_t V);//arg in mA
uint16_t BQ25703_IBAT_CH(uint16_t data16);//in mA
uint16_t BQ25703_IBAT_DCH(uint16_t data16);//in mA
uint16_t BQ25703_VSYS(uint16_t data16);//V, mV
uint16_t BQ25703_VBAT(uint16_t data16);//V, mV 

#endif /* REGBQ25703A_H_ */
