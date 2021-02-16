/*
 * DriverBQ25703.h
 *
 *  Created on: Feb 13, 2019
 *      Author: 2382
 */

#ifndef DRIVERBQ25703_H_
#define DRIVERBQ25703_H_

#include "i2c_API.h"
#include "regBQ25703A.h"

typedef enum {e_DR703_Sleep,e_DR703_HiZ,e_DR703_Inhibit,e_DR703_Work,e_DR703_NoS} t_eDR703_mode;
//typedef enum {eWait,eProcessing,eDone,eDoneError} t_eDriverState;

#define ChargeOption0_EN_LWPWR 0x8000
#define ChargeOption0_CHRG_INHIBIT 0x0001
#define ChargeOption3_EN_HIZ 0x8000

extern e_FunctionReturnState
BQ25703_Wr_Check( e_I2C_API_Devices d,	t_I2cRecord i2cRecord, uint16_t data, unsigned char priority,void (*fun)(uint8_t));
//extern e_FunctionReturnState BQ25703_Init();
extern e_FunctionReturnState BQ25703_Init_Check(void);//hiZ, currents=>minimum.
extern e_FunctionReturnState BQ25703_Write_Check(bq25703Registers reg, uint16_t data);
extern e_FunctionReturnState BQ25703_Charge_Check(uint16_t I);//charge, mA
extern e_FunctionReturnState BQ25703_IIN_Check(uint16_t I);//charge, mA

extern e_FunctionReturnState BQ25703_SetBits_Check(bq25703Registers reg, uint16_t set, uint16_t reset);
extern e_FunctionReturnState BQ25703_SetMode_Check(t_eDR703_mode mode);//sleep

extern e_FunctionReturnState BQ25703_ADCIBAT_Read(uint16_t *Ich,uint16_t *Idch);	//I, mA
extern e_FunctionReturnState BQ25703_ADCVSYSVBAT_Read(uint16_t *Vsys,uint16_t *Vbat);	//V, mV


extern e_FunctionReturnState BQ25703_Read(bq25703Registers reg, uint16_t *data);

extern void BQ25703_DriverReset(void);


#endif /* DRIVERBQ25703_H_ */
