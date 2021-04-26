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
//				e_TPS65982_6_SystemConfigurationRegister,
				e_TPS65982_6_PortConfigurationRegister,
	      e_TPS65987_IntClear1,
	      e_TPS65987_IntEvent1,
	      e_TPS65987_PowerStatusRegister,
	      e_TPS65987_BootFlagsRegister,
				e_TPS65987_SleepConfigurationRegister,
				e_TPS65982_6_NumOfReg

} e_TPS65982_6_Registers;

typedef enum {
	e_TPS_CMD_SRDY,//ok
	e_TPS_CMD_SRYR,//ok, turns off the keys immediately
	e_TPS_CMD_SWSr,//ok
	e_TPS_CMD_SWSk,// ok,but reject by the tablet.
	e_TPS_CMD_DBfg,//ok
	e_TPS_CMD_FLrr,
	e_TPS_CMD_FLem,
	e_TPS_CMD_FLad,
	e_TPS_CMD_FLwd,
	e_TPS_CMD_FLvy,
	e_TPS_CMD_GAID,
	e_TPS_CMD_NumOfEl
} e_TPS65982_6_CMD;

typedef struct {
					int8_t  CMD[5];
					uint8_t Data;
					uint8_t Data_qntByte;
               } t_TPS_CMD;



extern e_FunctionReturnState TPS65982_6_RDO_R(e_I2C_API_Devices device,  uint16_t* I, uint16_t* V, void* key); // mA, mV
extern e_FunctionReturnState TPS65982_6_RDO(e_I2C_API_Devices device,  uint16_t* I, uint16_t* V, void* key); // mA, mV
extern e_FunctionReturnState TPS65982_6_DISC(e_I2C_API_Devices device,uint8_t d, void* key);
extern e_FunctionReturnState
TPS65982_6_PSwap(e_I2C_API_Devices device,uint8_t sink,uint8_t sourse,uint16_t I, void* key);//0 - absent, else - present,mA,mV
extern e_FunctionReturnState 
TPS65982_6_CMD(e_I2C_API_Devices device,e_TPS65982_6_CMD CMD, void* key);
extern e_FunctionReturnState 
TPS65982_6_CMD_U(e_I2C_API_Devices device,e_TPS65982_6_CMD CMD, uint8_t *dataWR, 
                                      uint8_t qntByteWR,uint8_t *dataRD, uint8_t qntByteRD, void* key);
extern e_FunctionReturnState  //#define I2C_OP_READ         ((unsigned short)(0))
TPS65982_6_RW(e_I2C_API_Devices device, e_TPS65982_6_Registers reg, uint8_t *data, 
                                                               uint8_t qntByte, uint8_t RW, void* key);
extern void TPS65982_6_DriverReset(void);
extern e_FunctionReturnState TPS6598x_DriverState(void);

#endif /* DRIVERTPS65982_6_H_ */
