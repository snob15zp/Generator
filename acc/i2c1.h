#ifndef __I2C1_H 
#define __I2C1_H

#include "BoardSetup.h"


typedef 
	enum {
		TPS65987_CHIP,
		BQ25703_CHIP,
    BQ28Z610_CHIP
} slaveChip_e;
 
typedef 
	enum {
		TRANSFER_WRITE,
		TRANSFER_READ
} transferMode_e;

typedef 
	enum {
		I2C_IDLE,
		I2C_BUSY,
    I2C_SUBADDR_TRANSFERED,
		I2C_TRANSACTION_OK,
		I2C_TRANSACTION_ERROR
} i2cState_e;
	
typedef
   struct { 
			transferMode_e direct;
      uint8_t slaveAddr;
      uint8_t subAddr[2];
		  uint8_t numOfDataBytes;
		  uint8_t numOfSubAddrBytes;
    } i2cPacket_t;

/* functions prototypes */
extern void initI2c1(void);
//extern void i2cStart(void);
//extern void i2cRepStart(void);
extern uint16_t i2cDataRW(slaveChip_e chipID, transferMode_e tMode, uint8_t slaveAddr, uint16_t subAddr,  uint8_t subAddrLen, uint8_t* pDataBuff, uint8_t len );
//extern void i2cStateMachine(void);
extern i2cState_e getI2cStatus(void);
extern uint16_t getI2cError(void);
extern void getI2cReset(void);
		
extern void I2c1InSleep(void);
extern void I2c1OutSleep(void);

		
#endif
