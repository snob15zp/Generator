#ifndef TPS65987_H_
#define TPS65987_H_

#include "stm32g0xx.h"
#include "spiffs.h"
#include "SuperLoop_Comm2.h"
#include "DriverTPS65982_6.h"

#define MAX_BUF_BSIZE 65

#define REG_ADDR_BOOTFLAG 0x2D
#define REG_LEN_BOOTFLAG 12
#define REG_ADDR_PORTCONFIG 0x28
#define REG_LEN_PORTCONFIG 8

#define REGION_0 0
#define REGION_1 1

#define OUTPUT_LEN_FLRR 1
#define TOTAL_4kBSECTORS_FOR_PATCH 4 //todo from datasheet
#define TASK_RET_CODE_LEN 1
#define PATCH_BUNDLE_SIZE 64

//int32_t ReadReg(uint8_t addr, uint8_t len, uint8_t* buf);
//int32_t WriteReg(uint8_t addr, uint8_t len, uint8_t* buf);
//int32_t ExecCmd(char* cmd_ascii, uint8_t in_data_len, uint8_t* in_data, uint8_t out_data_len, uint8_t* out_data);

static int32_t PreOpsForFlashUpdate();
static int32_t UpdateAndVerifyRegion(uint8_t reg);
static int32_t StartFlashUpdate();
static int32_t ResetPDController();
int32_t tpsFlashUpdate();

#endif