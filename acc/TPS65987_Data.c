#include "TPS65987_Data.h"

uint8_t active_region;
uint8_t inactive_region;
s32_t tps_file;
uint8_t outdata[MAX_BUF_BSIZE] = {0};
uint8_t indata[MAX_BUF_BSIZE] = {0};

spiffs_stat file_stat;

int32_t tpsFlashUpdate()
{
	int32_t retVal = 0;
	
	PreOpsForFlashUpdate();
	StartFlashUpdate();
	ResetPDController();
	return 1;
}

/**/
static int32_t PreOpsForFlashUpdate()
{
//	s_AppContext *const pCtx = &gAppCtx;
//	s_TPS_bootflag *p_bootflags = NULL;
//	s_TPS_portconfig *p_portconfig = NULL;
//	uint8_t outdata[MAX_BUF_BSIZE] = {0};
//	uint8_t indata[MAX_BUF_BSIZE] = {0};
	int32_t retVal = 0;
	/*
	* Read BootFlags (0x2D) register:
	* - Note #1: Applications shouldn't proceed w/ flash-update if the device's
	* boot didn't succeed
	* - Note #2: Flash-update shall be attempted on the inactive region first
	*/
//	retVal = ReadReg(REG_ADDR_BOOTFLAG, REG_LEN_BOOTFLAG, &outdata[0]);
	do{
		retVal = TPS65982_6_RW(TPS87, e_TPS65987_BootFlagsRegister, outdata, REG_LEN_BOOTFLAG+1, I2C_OP_READ,PreOpsForFlashUpdate);
	}while(!((retVal==e_FRS_Done)||(retVal==e_FRS_DoneError)));
//	RETURN_ON_ERROR(retVal);
	/*
	* Note #1
	* Error during patch load - Don't attempt flash-update as the device wouldn't
	* be able to process the 4CC commands
	*/
//	p_bootflags = (s_TPS_bootflag *)&outdata[1];
//	if(0 != p_bootflags->patchheadererr)
	if(0 != (outdata[1]&0x01)){
//		ERR_PRINT( p_bootflags->patchheadererr);
//		SignalEvent(APP_EVENT_ERROR);
//		retVal = 0; /* For the state-machine */
//		goto error;
		return -1;
	}
	/*
	* Note #2
	* Region1 = 0 indicates that device didn't attempt 'Region1',
	* which implicitly means that the content at Region0 is valid/active
	*/
	if(0 == (outdata[1]&0x20)){
//		pCtx->active_region = REGION_0;
//		pCtx->inactive_region = REGION_1;
		active_region = REGION_0;
		inactive_region = REGION_1;

	}
//	else if ( (1 == p_bootflags->region1) && \
//	(1 == p_bootflags->region0) && \
//	((0 == p_bootflags->region1crcfail) && \
//	(0 == p_bootflags->region1flasherr) && \
//	(0 == p_bootflags->region1invalid)) )
	else if ( (1 == (outdata[1]&0x20)) && \
	(1 == (outdata[1]&0x10)) && \
	((0 == (outdata[2]&0x20)) && \
	(0 == (outdata[2]&0x02)) && \
	(0 == (outdata[1]&0x80))) ){
		active_region = REGION_1;
		inactive_region = REGION_0;
	}
	/*
	* Keep the port disabled during the flash-update
	*/
//	retVal = ReadReg(REG_ADDR_PORTCONFIG, REG_LEN_PORTCONFIG, &outdata[0]);
	do{
		retVal = TPS65982_6_RW(TPS87, e_TPS65982_6_PortConfigurationRegister, outdata, REG_LEN_PORTCONFIG+1, I2C_OP_READ,PreOpsForFlashUpdate);
	}while(!((retVal==e_FRS_Done)||(retVal==e_FRS_DoneError)));
//	RETURN_ON_ERROR(retVal);
//	memcpy(&indata[0], &outdata[1], REG_LEN_PORTCONFIG); /* outdata[0] holds the register length
//		memcpy(indata, outdata, REG_LEN_PORTCONFIG); /* outdata[0] holds the register length
//	*/
//	p_portconfig = (s_TPS_portconfig *)&indata[0];
//	p_portconfig->typecstatemachine = DISABLE_PORT;
	outdata[1] |= 0x03;
//	retVal = WriteReg(REG_ADDR_PORTCONFIG, REG_LEN_PORTCONFIG, &indata[0]);
//	do{
//		retVal = TPS65982_6_RW(TPS87, e_TPS65982_6_PortConfigurationRegister, outdata, REG_LEN_PORTCONFIG+1, I2C_OP_WRITE);
//	}while(!((retVal==e_FRS_Done)||(retVal==e_FRS_DoneError)));
//	RETURN_ON_ERROR(retVal);
//	error:
//	return retVal;
	return 0;
}

static int32_t StartFlashUpdate()
{
//	s_AppContext *const pCtx = &gAppCtx;
	int32_t retVal = 0;
//	UART_PRINT("\n\rActive Region is [%d] - Region being updated is [%d]\n\r",\
	pCtx->active_region, pCtx->inactive_region);
	/*
	* Region-0 is currently active, hence update Region-1
	*/
//	retVal = UpdateAndVerifyRegion(pCtx->inactive_region);
	retVal = UpdateAndVerifyRegion(inactive_region);
//	if(0 != retVal){
////	UART_PRINT("Region[%d] update failed.! Next boot will happen from Region[%d]\n\r",\
////	pCtx->inactive_region, pCtx->active_region);
////	retVal = 0;
////	goto error;
//		return -1;
//	}
	/*
	* Region-1 is successfully updated.
	* To maintain a redundant copy for a fail-safe flash-update, copy the same
	* content at Region-0
	*/
//	retVal = UpdateAndVerifyRegion(pCtx->active_region);
	retVal = UpdateAndVerifyRegion(active_region);
//	if(0 != retVal)
////	{
//////		UART_PRINT("Region[%d] update failed.! Next boot will happen from Region[%d]\n\r",\
//////		pCtx->active_region, pCtx->inactive_region);
//////		retVal = 0;
//////		goto error;
////		return -1;
//	}
//	error:
//	SignalEvent(APP_EVENT_END_UPDATE);
//	return retVal;

	return 0;
}
	/**/
	uint32_t regAddr = 0;
	uint8_t bytesToRead=0;
	uint32_t bytesRemain=0;

static int32_t UpdateAndVerifyRegion(uint8_t region_number)
{
//	s_TPS_flrr flrrInData = {0};
//	s_TPS_flem flemInData = {0};
//	s_TPS_flad fladInData = {0};
//	s_TPS_flvy flvyInData = {0};
//	s32_t patchBundleSize = 0;
//	uint32_t regAddr = 0;
//	int32_t idx = -1;
	int32_t retVal;
	uint8_t byteBuff[PATCH_BUNDLE_SIZE];
//	uint8_t bytesToRead=0;
//	uint32_t bytesRemain=0;
	
	tps_file=SPIFFS_open(&fs,"tps65987.bin",SPIFFS_O_RDONLY,0);
//	patchBundleSize = sizeof(tps6598x_lowregion_array);
	SPIFFS_fstat(&fs,tps_file,&file_stat);
//	patchBundleSize = file_stat.size;
	/*
	* Get the location of the region 'region_number'
	*/
	indata[0] = 1;
	indata[1] = region_number;
//	retVal = ExecCmd(FLrr, sizeof(flrrInData), (int8_t *)&flrrInData, \
//	OUTPUT_LEN_FLRR, &outdata[0]);
	do{
		retVal=TPS65982_6_CMD_U(TPS87,e_TPS_CMD_FLrr,indata,2,outdata,5,UpdateAndVerifyRegion);
	}while(!((retVal==e_FRS_Done)||(retVal==e_FRS_DoneError)));
//	RETURN_ON_ERROR(retVal);
	regAddr = (outdata[4] << 24) | (outdata[3] << 16) | (outdata[2] << 8) | (outdata[1] << 0);
	/*
	* Erase #'numof4ksector' sectors at address 'regAddr' of the sFLASH
	* - Note: The below snippet assumes that the total number of 4kB segments
	* required to hold the maximum size of the patch-bundle is 4.
	* Ensure its validity for the TPS6598x being used for your
	* application.
	*/
	indata[0]=5;
	indata[1]=(regAddr&0x000000FF);
	indata[2]=((regAddr>>8)&0x0000FF);
	indata[3]=((regAddr>>16)&0x00FF);
	indata[4]=(regAddr>>24);
	indata[5]=TOTAL_4kBSECTORS_FOR_PATCH;
//	retVal = ExecCmd(FLem, sizeof(flemInData), (int8_t *)&flemInData, \
//	TASK_RET_CODE_LEN, &outdata[0]);
	do{
		retVal=TPS65982_6_CMD_U(TPS87,e_TPS_CMD_FLem,indata,6,outdata,2,UpdateAndVerifyRegion);
	}while(!((retVal==e_FRS_Done)||(retVal==e_FRS_DoneError)));

//	RETURN_ON_ERROR(retVal);
	/*
	* Set the start address for the next write
	*/
	indata[0]=4;
	indata[1]=(regAddr&0x000000FF);
	indata[2]=((regAddr>>8)&0x0000FF);
	indata[3]=((regAddr>>16)&0x00FF);
	indata[4]=(regAddr>>24);
//	retVal = ExecCmd(FLad, sizeof(fladInData), (int8_t *)&fladInData, \
//	TASK_RET_CODE_LEN, &outdata[0]);
	do{
		retVal=TPS65982_6_CMD_U(TPS87,e_TPS_CMD_FLad,indata,5,outdata,2,UpdateAndVerifyRegion);
	}while(!((retVal==e_FRS_Done)||(retVal==e_FRS_DoneError)));
//	RETURN_ON_ERROR(retVal);
	/**/
//	UART_PRINT("Updating [%d] 4k chunks starting @ 0x%x \n\r", flemInData.numof4ksector, regAddr);
//	for (idx = 0; idx < patchBundleSize/PATCH_BUNDLE_SIZE; idx++)
//	{
	bytesToRead=PATCH_BUNDLE_SIZE;
	bytesRemain=file_stat.size;
	do{
		retVal=SPIFFS_read(&fs, tps_file, &byteBuff, bytesToRead);
		if (retVal<bytesToRead)
		{	break;
		}
		
		indata[0]=bytesToRead;
		int j=1;
		for(int i=0;i<bytesToRead;i++){
			indata[j]=byteBuff[i];
			j++;
		}
		
	//	UART_PRINT(".");
		/*
		* Execute FLwd with PATCH_BUNDLE_SIZE bytes of patch-data
		* in each iteration
		*/
	//	retVal = ExecCmd(FLwd, PATCH_BUNDLE_SIZE,\
	//	(int8_t *)&tps6598x_lowregion_array[idx * PATCH_BUNDLE_SIZE],
	//	TASK_RET_CODE_LEN, &outdata[0]);
	//	RETURN_ON_ERROR(retVal);
		do{
			retVal=TPS65982_6_CMD_U(TPS87,e_TPS_CMD_FLwd,indata,bytesToRead+1,outdata,2,UpdateAndVerifyRegion);
		}while(!((retVal==e_FRS_Done)||(retVal==e_FRS_DoneError)));
//		delayms(100);
		bytesRemain-=bytesToRead;
		if(bytesRemain>=PATCH_BUNDLE_SIZE)
			bytesToRead=PATCH_BUNDLE_SIZE;
		else
			bytesToRead=bytesRemain;
		/*
		* 'outdata[1]' will contain the command's return code
		*/
//		if(0 != outdata[1])
//		{
//	//	UART_PRINT("\n\r");
//	//	UART_PRINT("Flash Write FAILED.!\n\r");
//	//	retVal = -1;
//	//	goto error;
//			return -1;
//		}
//	}
	}while(bytesRemain!=0);
//	UART_PRINT("\n\r");
	/*
	* Write is through. Now verify if the content/copy is valid
	*/
	indata[0]=4;
	indata[1]=(regAddr&0x000000FF);
	indata[2]=((regAddr>>8)&0x0000FF);
	indata[3]=((regAddr>>16)&0x00FF);
	indata[4]=(regAddr>>24);
//	retVal = ExecCmd(FLvy, sizeof(flvyInData), (int8_t *)&flvyInData, \
//	TASK_RET_CODE_LEN, &outdata[0]);
	do{
		retVal=TPS65982_6_CMD_U(TPS87,e_TPS_CMD_FLvy,indata,5,outdata,2,UpdateAndVerifyRegion);
	}while(!((retVal==e_FRS_Done)||(retVal==e_FRS_DoneError)));

	if(0 != outdata[1])
	{
//	UART_PRINT("Flash Verify FAILED.!\n\r");
//	retVal = -1;
//	goto error;
		SPIFFS_close(&fs,tps_file);
		SPIFFS_remove(&fs,"tps65987.bin");
		return -1;
	}
//	error:
//	return retVal;
	SPIFFS_close(&fs,tps_file);
	SPIFFS_remove(&fs,"tps65987.bin");
	return 0;
}
	
/**/
static int32_t ResetPDController()
{
	int32_t retVal = 0;
	/*
	* Execute GAID, and wait for reset to complete
	*/
//	ExecCmd(GAID, 0, NULL, 0, NULL);
	do{
		retVal=TPS65982_6_CMD_U(TPS87,e_TPS_CMD_GAID,indata,0,outdata,0,UpdateAndVerifyRegion);
	}while(!((retVal==e_FRS_Done)||(retVal==e_FRS_DoneError)));
//	Board_IF_Delay(1000);
//	retVal = ReadMode();
//	RETURN_ON_ERROR(retVal);
//	retVal = ReadVersion();
//	RETURN_ON_ERROR(retVal);
//	retVal = ReadBootStatus();
//	RETURN_ON_ERROR(retVal);
	return 0;
}

int32_t ReadReg(uint8_t addr, uint8_t len, uint8_t* buf)
{
	int32_t retVal = 0;
	
	TPS65982_6_RW(TPS87, addr, buf, len, I2C_OP_READ, ReadReg);
	if(retVal==-1)
		return -1;
	return 0;
}

int32_t WriteReg(uint8_t addr, uint8_t len, uint8_t* buf)
{
	int32_t retVal = 0;
	
	TPS65982_6_RW(TPS87, addr, buf, len, I2C_OP_WRITE,WriteReg);
	if(retVal==-1)
		return -1;
	return 0;
}
