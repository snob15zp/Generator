/*
 * This file is subject to the terms of the GFX License. If a copy of
 * the license was not distributed with this file, you can obtain one at:
 *
 *              http://ugfx.io/license.html
 */

#ifndef _GDISP_LLD_BOARD_H
#define _GDISP_LLD_BOARD_H

#include "BoardSetup.h"
//#include "uGFXport.h"
#include "superloopDisplay.h"

static GFXINLINE void init_board(GDisplay *g) {
	(void) g;
//	NVIC_DisableIRQ(SPI1_IRQn); 

  PWR_TFT_ON;
  delayms(1);
  TP_RST_HI;
  delayms(1);
  TP_RST_LOW;
	TFT_RST_LOW;
  switchDisplayInterfacePinsToPwr(ENABLE);
  delayms(10);  
	TFT_RST_HI;
  TP_RST_HI;
  	
	delayms(1); 
	TFT_CS_HI;
}

static GFXINLINE void post_init_board(GDisplay *g) {
	(void) g;
}

static GFXINLINE void setpin_reset(GDisplay *g, gBool state) {
	(void) g;

	if (state) {
		 TFT_RST_LOW;
	} else {
		TFT_RST_HI;
	}
}

static GFXINLINE void set_backlight(GDisplay *g, gU8 percent) {
	(void) g;
	// TODO: can probably pwm this
	if(percent) {
		// turn back light on
		TFT_LED_ON;
	} else {
		// turn off
		TFT_LED_OFF;
	}
}

static GFXINLINE void acquire_bus(GDisplay *g) {
	(void) g;
	while (SPI1->SR & SPI_SR_FTLVL_Msk){}										//  Wait until FTLVL[1:0] = 00 (no more data to transmit)
	while (SPI1->SR & SPI_SR_BSY){}
	TFT_CS_LOW;
	spiDispCapture=1;
}

static GFXINLINE void release_bus(GDisplay *g) {
	(void) g;
	
	while (SPI1->SR & SPI_SR_FTLVL_Msk){}										//  Wait until FTLVL[1:0] = 00 (no more data to transmit)
	while (SPI1->SR & SPI_SR_BSY){}

	TFT_CS_HI;
	spiDispCapture=0;
}

static GFXINLINE void send_data(gU16 data) {
// http://forum.easyelectronics.ru/viewtopic.php?p=262122#p262122
  while (!(SPI1->SR & SPI_SR_TXE)); // ToDo check - ok
  *(__IO uint8_t*)&SPI1->DR = data; // загрузили в SPI_DR код команды
	//*(__IO uint16_t*)&SPI1->DR = data;

}

static GFXINLINE void write_index(GDisplay *g, gU16 index) {
	(void) g;
	spiDispCapture=1;
  while (SPI1->SR & SPI_SR_BSY);//ToDo check -  ok
 	TFT_CS_HI;
  TFT_DC_CMD; // переводим дисплей в режим команд
  TFT_CS_LOW;
  send_data(index);
  while (SPI1->SR & SPI_SR_BSY); //ToDo check 
  TFT_DC_DATA; 
}	

/**
 * SPI configuration structure.
 * Speed 12 MHz, CPHA=0, CPOL=0, 8bits frames, MSb transmitted first.
 * Soft slave select.
 */

//ToDo check SPI setup 
static GFXINLINE void write_data(GDisplay *g, gU16 data) {
	(void) g;
	send_data(data);
}

static GFXINLINE void setreadmode(GDisplay *g) {
	(void) g;
}

static GFXINLINE void setwritemode(GDisplay *g) {
	(void) g;
}

static GFXINLINE gU16 read_data(GDisplay *g) {
	(void) g;
	return 0;
}

#endif /* _GDISP_LLD_BOARD_H */
