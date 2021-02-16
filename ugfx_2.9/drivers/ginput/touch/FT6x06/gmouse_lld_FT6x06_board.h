/*
 * This file is subject to the terms of the GFX License. If a copy of
 * the license was not distributed with this file, you can obtain one at:
 *
 *              http://ugfx.io/license.html
 */

#ifndef _GINPUT_LLD_MOUSE_BOARD_H
#define _GINPUT_LLD_MOUSE_BOARD_H

#include "I2C2.h"

#include "ft6x06.h"
//#include "stm32f4xx.h"
//#include "stm32f4xx_hal.h"

// Resolution and Accuracy Settings
#define GMOUSE_FT6x06_PEN_CALIBRATE_ERROR			40
#define GMOUSE_FT6x06_PEN_CLICK_ERROR				16
#define GMOUSE_FT6x06_PEN_MOVE_ERROR				14
#define GMOUSE_FT6x06_FINGER_CALIBRATE_ERROR		50
#define GMOUSE_FT6x06_FINGER_CLICK_ERROR			28
#define GMOUSE_FT6x06_FINGER_MOVE_ERROR				24

// How much extra data to allocate at the end of the GMouse structure for the board's use
#define GMOUSE_FT6x06_BOARD_DATA_SIZE				0

/* The FT6x06 I2C slave address */
#define FT6x06_SLAVE_ADDR 0x38  //rdd FT6x36

//I2C_HandleTypeDef i2cHandle;
/* Maximum speed (100kHz) */
#define CLOCKSPEED 100000;

static gBool init_board(GMouse* m, unsigned instance) {
	(void)m;
	(void)instance;
  initI2c2();

		return gTrue;

}

static GFXINLINE void aquire_bus(GMouse* m) {
    (void)m;
}

static GFXINLINE void release_bus(GMouse* m) {
    (void)m;
}

static void write_reg(GMouse* m, gU8 reg, gU8 val) {
    (void)m;
//val-> static?
    //HAL_I2C_Mem_Write(&i2cHandle, FT6x06_SLAVE_ADDR, (gU16)reg, I2C_MEMADD_SIZE_8BIT, &val, 1, 1000);
	  i2c2DataRW(FT6x06_SLAVE_ADDR, TRANSFER_WRITE, reg,  1, (uint8_t*) &val, 1 );
	  while (getI2c2Status()!=I2C_IDLE);
}

static gU8 read_byte(GMouse* m, gU8 reg) {
    (void)m;
    gU8 result;

	  i2c2DataRW(FT6x06_SLAVE_ADDR, TRANSFER_READ, reg,  1, (uint8_t*) &result, 1 );
	  while (getI2c2Status()!=I2C_IDLE);
	  ;
    return result;
}

static gU16 read_word(GMouse* m, gU8 reg) {
	(void)m;
	gU8 result[2];

	  i2c2DataRW(FT6x06_SLAVE_ADDR, TRANSFER_READ, reg,  1, (uint8_t*) &(result[0]), 2 );
	  while (getI2c2Status()!=I2C_IDLE);

	return (result[0]<<8 | result[1]);

}

#endif /* _GINPUT_LLD_MOUSE_BOARD_H */
