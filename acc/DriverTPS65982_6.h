/*
 * DriverTPS65982_6.h
 *
 *  Created on: Mar 8, 2019
 *      Author: RD
 */

#ifndef DRIVERTPS65982_6_H_
#define DRIVERTPS65982_6_H_

#include "i2c_API.h"



typedef enum {
				e_TPS65982_6_ActiveRDO,
				e_TPS65982_6_StatusRegister,
				e_TPS65982_6_PowerStatusRegister,
				e_TPS65982_6_ActivePDO,
				e_TPS65982_6_Cmd1,
				e_TPS65982_6_Data1,
				e_TPS65982_6_Cmd2,
				e_TPS65982_6_Data2,
				e_TPS65982_6_TXSourceCapabilities,
				e_TPS65982_6_TXSinkCapabilities,
				e_TPS65982_6_Mode,
				e_TPS65982_6_SystemConfigurationRegister,
	      e_TPS65987_IntClear1,
	      e_TPS65987_IntEvent1,
	      e_TPS65987_PowerStatusRegister,
				e_TPS65982_6_NumOfReg

} e_TPS65982_6_Registers;

typedef enum {
	e_TPS_CMD_SRDY,//ok
	e_TPS_CMD_SRYR,//ok, turns off the keys immediately
	e_TPS_CMD_SWSr,//ok
	e_TPS_CMD_SWSk,// ok,but reject by the tablet.
	e_TPS_CMD_DBfg,//ok
	e_TPS_CMD_NumOfEl
} e_TPS65982_6_CMD;

typedef struct {
					int8_t  CMD[5];
					uint8_t Data;
					uint8_t Data_qntByte;
               } t_TPS_CMD;



extern e_FunctionReturnState TPS65982_6_RDO_R(e_I2C_API_Devices device,  uint16_t* I, uint16_t* V); // mA, mV
extern e_FunctionReturnState TPS65982_6_RDO(e_I2C_API_Devices device,  uint16_t* I, uint16_t* V); // mA, mV
extern e_FunctionReturnState TPS65982_6_DISC(e_I2C_API_Devices device,uint8_t d);
extern e_FunctionReturnState
TPS65982_6_PSwap(e_I2C_API_Devices device,uint8_t sink,uint8_t sourse,uint16_t I);//0 - absent, else - present,mA,mV
extern e_FunctionReturnState TPS65982_6_CMD(e_I2C_API_Devices device,e_TPS65982_6_CMD CMD);

e_FunctionReturnState
TPS65982_6_RW(e_I2C_API_Devices device, e_TPS65982_6_Registers reg, uint8_t *data, uint8_t qntByte, uint8_t RW);

extern void TPS65982_6_DriverReset(void);


#endif /* DRIVERTPS65982_6_H_ */
