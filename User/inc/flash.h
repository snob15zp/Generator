#ifndef fileInfo_H
#define fileInfo_H

#include "stm32g0xx.h"
#include "w25qxx.h"

//#define __flashInit() W25qxx_Init()
#define PAGE_SIZE 			256
#define SECTOR_SIZE 		4096
#define SECTORS_NUM			512
#define MAX_FILES_NUM 	50
#define FILE_NAME_SHIFT 6
#define FILE_NAME_BYTES 8

#define FIRST_PLAY_SECT 0
#define LAST_PLAY_SECT 	49
#define FIRST_CONF_SECT 50
#define FIRST_CONF_BYTE 204800
#define LAST_CONF_SECT 	205
#define CONF_FILE_SIZE  718569

#define WR_CONF_FILE		0x31
#define WR_PLAY_FILES		0x32
#define ER_CONF_FILE 		0x33
#define ER_PLAY_FILES 	0x34
#define RD_CONF_FILE		0x35
#define RD_PLAY_FILES		0x36

//typedef struct{
//	uint8_t 	fileId;
//	uint8_t 	fileName[13];
//	uint32_t 	fileSize;
//}fileInfo;

//extern volatile fileInfo fileList[MAX_FILES_NUM];

extern uint32_t nonEmptyBytes;

void procCmdFromUsb(void);
void wrPage(void);
void wrPlayFiles(void);
void wrConfFile(void);
void rdPlayFile(uint8_t fid);
void rdPlayFiles(void);
void rdConfFile(void);
void getFileList(void);
void setFile(uint8_t fid);
uint16_t findEmptySector(void);
void delFile(uint8_t fid);
void erFlash(uint8_t firstSectAddr, uint8_t lastSectAddr);
void endCmd(void);
void errorCmd(void);
extern uint32_t isFlashClear(void);
void eraseFlash(void);
void rdFlash(void);

void loadFlashForDebug(void);

#endif //fileInfo_H
