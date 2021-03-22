/**
\file


\todo on/off PWR

*/
#include "I2C_COMMON.h"
#include "GlobalKey.h"
#include "Spi1.h"
#include "SuperLoop_Player.h"
#include "board_PowerModes.h"
#include "BoardSetup.h"
#include "spiffs.h"
#include "SuperLoop_Comm2.h"

uint16_t SLPl_ui16_NumOffiles;///\todo global variable one time initialiseted

uint16_t freqStartByte;
uint32_t freq;

volatile uint32_t tim3TickCounter;
extern volatile uint32_t playClk;
volatile uint32_t durTimeMs;
volatile uint32_t durTimeS;
uint16_t steps;
uint16_t startSectAddr;

extern uint16_t playFileInList;
uint16_t playFileSectorBegin;
uint8_t i=0;
uint16_t frstChMult=0;
uint16_t scndChMult=0;
static uint32_t durTimeSLast;

#define  playParamArr_size 8
#define FPGA_GAIN 21620

uint32_t playParamArr[playParamArr_size];
/*************************************
playParamArr[0] - frequencies
playParamArr[1] - offset
playParamArr[2] - onset
playParamArr[3] - duration
playParamArr[4] - negative
playParamArr[5] - up
playParamArr[6] - inverse
playParamArr[7] - out voltage
**************************************/
uint32_t playFreqArr_1[100];
uint32_t playFreqArr_2[100];
uint32_t playFreqArr[200];
uint8_t timeArr[3];
/*******************************
[0] - hours
[1] - min
[2] - sec
*******************************/
uint8_t totalSec=0;
uint8_t totalMin=0;
uint8_t totalHour=0;
uint8_t fileSec=0;
uint8_t fileMin=0;
uint8_t fileHour=0;

//uint8_t totalTimeArr[];
//uint8_t fileTimeArr[];

//fileTimeArr={'0','0',':','0','0',':','0','0',0};
//totalTimeArr={'0','0',':','0','0',':','0','0',0};

volatile uint32_t playClk;
volatile uint32_t progBarClk;


//-------------------------for SPI2-----------------------------------------------
void initSpi_2(void)
{
  RCC->APBENR1 |= RCC_APBENR1_SPI2EN;                     // enable SPI2 clk
  SPI2->CR1 &= ~SPI_CR1_SPE;                              // disable SPI2 perif
  SPI2->CR1 |= /*SPI_CR1_BR_2 | SPI_CR1_BR_1 |*/ SPI_CR1_BR_0 |  
               SPI_CR1_SSM |															// SPI_NSS_Soft
               SPI_CR1_SSI |
							 SPI_CR1_LSBFIRST |
//               SPI_CR1_CPOL |															// SPI_CPOL_High
//               SPI_CR1_CPHA |															// SPI_CPHA_2Edge
               SPI_CR1_MSTR;                              // master mode select
	SPI2->CR2 |= SPI_CR2_FRXTH;															// RXNE event is generated if the FIFO level is greater than or equal to 1/4 (8-bit)
//							 SPI_CR2_RXNEIE;														// enable SPI2 RXNE interrupt
  SPI2->CR1 |= SPI_CR1_SPE;                               // enable SPI2 perif
}

/**
*			The correct disable procedure is (except when receive only mode is used):
*			1. Wait until FTLVL[1:0] = 00 (no more data to transmit).
*			2. Wait until BSY=0 (the last data frame is processed).
*			3. Disable the SPI (SPE=0).
*			4. Read data until FRLVL[1:0] = 00 (read all the received data).
*
**************************************************************************************************************************/

void disableSpi_2(void){

	while (SPI2->SR & SPI_SR_FTLVL_Msk){}										//  Wait until FTLVL[1:0] = 00 (no more data to transmit)
	while (SPI2->SR & SPI_SR_BSY){}													//	Wait until BSY=0 (the last data frame is processed)	
		
	NVIC_DisableIRQ(SPI2_IRQn);										 
  SPI2->CR1 &= ~SPI_CR1_SPE;                              // disable SPI1 perif
		
	while (SPI2->SR & SPI_SR_FRE_Msk){ SPI2->DR; }					// Read data until FRLVL[1:0] = 00 (read all the received data)				
	RCC->APBENR2 &= ~RCC_APBENR2_SPI1EN;                    // disable SPI1 clk
              
}


void spi2Transmit(uint8_t *pData, uint16_t Size)
{
	uint16_t TxXferCount=Size;
	uint16_t Temp;
	  /* Transmit data in 8 Bit mode */
	while (TxXferCount > 0U){
		/* Wait until TXE flag is set to send data */
		while(!(SPI2->SR & SPI_SR_TXE)){}
//		if (SPI2->SR & SPI_SR_TXE)
			{
			if (TxXferCount > 1U){
				/* write on the data register in packing mode */
				SPI2->DR = *((uint16_t *)pData);
				pData += sizeof(uint16_t);
				TxXferCount -= 2U;
			}
			else{
				*(__IO uint8_t *)&SPI2->DR = *pData;
				pData++;
				TxXferCount--;
			}
		}
	}
	while(SPI2->SR & SPI_SR_BSY){}
	
	for(int i=0;i<2;i++){
		Temp=SPI2->DR;
	}
}

void spi2FifoClr(void)
{
	uint16_t Temp;
	
	for(int i=0;i<4;i++){
		Temp=SPI2->DR;
	}
}

//-----------------------------------for TIM3---------------------------------------

void tim3Init(void)
{
	RCC->APBENR1 |= RCC_APBENR1_TIM3EN;
	TIM3->PSC = 63;												//sets TIM3 clk 1 MHz
	TIM3->ARR = 1000;											//sets TIM3 count interval 1 ms
	TIM3->DIER = TIM_DIER_UIE;
	NVIC_SetPriority(TIM3_IRQn,15);
	NVIC_EnableIRQ(TIM3_IRQn);
	TIM3->CR1 = TIM_CR1_CEN;
}

void delay_ms(uint32_t delayTime){
	tim3TickCounter = delayTime;
	while(tim3TickCounter){}
}

void TIM3_IRQHandler(void)
{
	static uint8_t x;
	if(TIM3->SR & TIM_SR_UIF){
		TIM3->SR = ~TIM_SR_UIF;
		tim3TickCounter--;
		if(fpgaFlags.clockStart==1){
			playClk++;
			durTimeMs++;
		}
		else{
			playClk=0;
			durTimeMs=0;
		}
	}
}

//-----------------------------------for player---------------------------------------
/**
\brief Passive Serial FPGA configuration

  FPGA_CS 					PB12  output	FPGA_CS_L/H
	SPI2_SCK 					PB13  spi
	SPI2_MISO					PB14	spi
	SPI2_MOSI 				PB15	spi 
	CONF_DONE 				PC6		input
	nSTATUS 					PC7		input
	nCONFIG 					PB11	output 	nCONFIG_L/H
	Reserv=LED_TEST 	PB10	output 	FPGA_Reserv_L/H
	FPGA_START 				PA8		output	FPGA_START_L/H

If your system exceeds the fast or standard POR time, you must hold nCONFIG low
until all the power supplies are stable.
MSEL[2:0] Pins 000 Standard POR Delay

nSTATUS and CONF_DONE driven low
• All I/Os pins are tied to an internal weak pull-up
• Clears configuration RAM bits
      ->
nSTATUS and nCONFIG released high
CONF_DONE pulled low
      ->
Writes configuration data to
FPGA
      ->
• nSTATUS pulled low
• CONF_DONE remains low
• Restarts configuration if option
enabled		
      ->
CONF_DONE released high
      ->
• Initializes internal logic and
registers
• Enables I/O buffers
      ->
INIT_DONE released high
(if option enabled)
      ->
Executes your design
*/
#define CONF_FILE_SIZE  718569
#define CONF_BUF_SIZE 64
void fpgaConfig(void)											//
{
	s32_t fpga_file;
	int32_t read_res;
	
	fpga_file=SPIFFS_open(&fs, "FPGA.rbf", SPIFFS_O_RDONLY, 0);
	
	uint32_t bytesCnt=0;
	uint8_t byteBuff[CONF_BUF_SIZE];
	uint8_t bytesToRead=CONF_BUF_SIZE;
	uint32_t bytesRemain=CONF_FILE_SIZE;
//	byteBuff=0;
//	spi2Transmit(&byteBuff, 1);
	
	SPI2->CR1 &= ~SPI_CR1_SPE;
	SPI2->CR1 |= SPI_CR1_LSBFIRST;
	SPI2->CR1 |= SPI_CR1_SPE;
	
	switchOUTStageInterfacePinsToPwr(DISABLE);//RDD DEBUG
	delay_ms(100);// test -> ok
	switchOUTStageInterfacePinsToPwr(ENABLE);
	delay_ms(10);
///rdd debug	spi1FifoClr();
//	GPIOB->BSRR=GPIO_BSRR_BR0;							//FPGA 1.2 V on
	nCONFIG_H;
	while(!(GPIOC->IDR & GPIO_IDR_ID7)){/** \todo timeout */}
	delay_ms(10);
	FPGA_CS_L;															//for logger
	do{
//	for(bytesCnt=0;bytesCnt<CONF_FILE_SIZE;bytesCnt++){
		read_res=SPIFFS_read(&fs, fpga_file, &byteBuff, bytesToRead);
		if (read_res<bytesToRead)
		{	break;
		};
		spi2Transmit(byteBuff, bytesToRead);
		if(GPIOC->IDR & GPIO_IDR_ID6)
			{byteBuff[0]=0;
			spi2Transmit(byteBuff, 1);
			fpgaFlags.fpgaConfigComplete=1;
			FPGA_CS_H;
			SPI2->CR1 &= ~SPI_CR1_SPE;
			SPI2->CR1 &= ~SPI_CR1_LSBFIRST;
			SPI2->CR1 |= SPI_CR1_SPE;
			
      SPIFFS_close(&fs, fpga_file);				
			return;
		}
//	}
		bytesRemain-=bytesToRead;
		if(bytesRemain>=CONF_BUF_SIZE)
			bytesToRead=CONF_BUF_SIZE;
		else
			bytesToRead=bytesRemain;
	}while(bytesToRead!=0);
//	byteBuff=0;
//	spi2Transmit(&byteBuff, 1);
	FPGA_CS_H;
//	confFailed();
	SPI2->CR1 &= ~SPI_CR1_SPE;
	SPI2->CR1 &= ~SPI_CR1_LSBFIRST;
	SPI2->CR1 |= SPI_CR1_SPE;

	fpgaFlags.fpgaConfigComplete=0;
	SPIFFS_close(&fs, fpga_file);
}

//extern uint8_t fileName[50];
uint16_t fileSect=0;
uint16_t playFileSector;

void timeToString(uint8_t* timeArr)
{
	for (int i = 0; i < 8; i++)
	{
        if( timeArr[i] < 10) 
        {
            timeArr[i] = timeArr[i] + '0';
        }
        else
        {
			timeArr[i] = ':';
        }
	}
    timeArr[9] = 0;
}

//----------------------------------BEGIN LOAD FROM FILES---------------------------------

#define D_ParamStringLength 40
s32_t freq_file;
static	uint8_t n_for_CR;
static	char tempArrOld[D_ParamStringLength+1];
static  char SLPl_filename[D_FileNameLength+1];
        char SLPl_filenameWOE[D_FileNameLength+1];
static int8_t bytesCount;

e_FunctionReturnState getFileName(uint16_t fileSectl)
{
	uint32_t startAddr;
	e_FunctionReturnState rstate=e_FRS_Done;
  //int8_t bytesCount;
	char byteBuff[D_FileNameLength+1];
	
	startAddr=fileSectl*D_FileNameLengthInFL;
	
//	File_List=SPIFFS_open(&fs, "freq.pls", SPIFFS_O_RDONLY, 0);/// \todo one time open
    SPIFFS_lseek(&fs, File_List,startAddr,SPIFFS_SEEK_SET);
	bytesCount=SPIFFS_read(&fs, File_List, &byteBuff, D_FileNameLengthInFL);
	if (bytesCount<1)
	{	rstate=e_FRS_DoneError;
	}
	byteBuff[bytesCount]=0;
	_sscanf( byteBuff,FN_Read_Template,SLPl_filename);
	SLPl_filename[D_FN_Max_Length]=0;
	strncpy(SLPl_filenameWOE,SLPl_filename,28);
	strncat(SLPl_filename,".txt",4);
	//SPIFFS_close(&fs, File_List); /// \todo one time close
	return rstate;
}

e_FunctionReturnState getControlParam(void)
{
	e_FunctionReturnState rstate=e_FRS_Done;
	uint8_t strCnt=0;
	int32_t TempParam;

	char *pch; 

	char byteBuff[D_ParamStringLength+1];
	
	

	n_for_CR=0;
	tempArrOld[0]=0;
  
		
	do
	{    
    		bytesCount=SPIFFS_read(&fs, freq_file, &byteBuff, D_ParamStringLength-n_for_CR);
				if (bytesCount<0)
				{	rstate=e_FRS_DoneError;
					break;
				}
				byteBuff[bytesCount]=0;
				strcat(tempArrOld,byteBuff);
				if (strCnt>0)
				{
					pch = strchr(tempArrOld,',');	
					if (NULL==pch)
	  			{	rstate=e_FRS_DoneError;
		  			break;
			  	}
					_sscanf( pch+1,"%i",&TempParam);
					playParamArr[strCnt-1]=TempParam;
				};	
        pch = strchr(tempArrOld,10);
				if (NULL==pch)
				{	rstate=e_FRS_DoneError;
					break;
				}	
				strcpy(tempArrOld,pch+1);	
				n_for_CR= strlen(tempArrOld);
				strCnt++;
	}				
	while (strCnt<=playParamArr_size);	
return rstate;
}

e_FunctionReturnState getFreq()
{
	int32_t TempParam;
	e_FunctionReturnState rstate=e_FRS_DoneError;
	char *pch; 
	int8_t bytesCount;
	char byteBuff[D_ParamStringLength+1];
	uint8_t index=0;
	for(int i=0;i<100;i++){
		playFreqArr_1[i]=0;
		playFreqArr_2[i]=0;
	}
	
	
		do
	{    
    		bytesCount=SPIFFS_read(&fs, freq_file, &byteBuff, D_ParamStringLength-n_for_CR);
				if (bytesCount<0)
				{	rstate=e_FRS_DoneError;
					break;
				}
				byteBuff[bytesCount]=0;
				strcat(tempArrOld,byteBuff);
				bytesCount=_sscanf( tempArrOld,"%i",&TempParam);
				if (bytesCount<1)
				{	break;
				}
				
				
				rstate=e_FRS_Done;
				
				if (0==(index&1))
					playFreqArr_1[index>>1]=TempParam;
				else
					playFreqArr_2[index>>1]=TempParam;
				
				index++;
				
        pch = strchr(tempArrOld,10);	//VV 2.02.21
					if (NULL==pch)
					{rstate=e_FRS_DoneError;
						 break;
					}	
				strcpy(tempArrOld,pch+1);		
				n_for_CR= strlen(tempArrOld);
				
	}				
	while (index<200);	

  frstChMult=((float)playParamArr[7]/10)*FPGA_GAIN/((index>>1)+(index&1));
	scndChMult=((float)playParamArr[7]/10)*FPGA_GAIN/((index>>1));
	
return rstate;
}

int verifyControlParam(void)
{
	//todo verify code
	return 0;
}

e_FunctionReturnState LoadParam(uint16_t fileSectl)
{  
	e_FunctionReturnState rstate;
	getFileName(fileSectl);
	freq_file=SPIFFS_open(&fs, SLPl_filename, SPIFFS_O_RDONLY, 0);
	rstate=getControlParam();
  SPIFFS_close(&fs, freq_file);
	return rstate;
};

e_FunctionReturnState LoadParmFreq(uint16_t fileSectl)
{ 
	e_FunctionReturnState rstate;
	getFileName(fileSectl);
	freq_file=SPIFFS_open(&fs, SLPl_filename, SPIFFS_O_RDONLY, 0);
	rstate=getControlParam();
	if (!rstate)
	   rstate=getFreq();
	SPIFFS_close(&fs, freq_file);
	return rstate;
};

//------------------------END LOAD FROM FILES-------------------------------------------------



uint32_t freqInverse(uint32_t freq)
{
	uint8_t tempArr[7] = {0};
	uint32_t remain;
	
	tempArr[6]=freq/1000000U;
	remain=freq%1000000U;
	tempArr[5]=freq/100000U;
	remain=freq%100000U;
	tempArr[4]=remain/10000U;
	remain=remain%10000U;
	tempArr[3]=remain/1000U;
	remain=remain%1000U;
	tempArr[2]=remain/100U;
	remain=remain%100U;
	tempArr[1]=remain/10U;
	remain=remain%10U;
	tempArr[0]=remain;
	for(int i=0;i<6;i++){
		if(tempArr[i]!=0)
			tempArr[i]=10-tempArr[i];
	}
	return tempArr[5]*100000U+tempArr[4]*10000U+tempArr[3]*1000U+tempArr[2]*100U+tempArr[1]*10U+tempArr[0];
}

void setInitFreq(void)
{
	if(playParamArr[4]==0 && playParamArr[5]==0){	//negative==0 and up==0
		for(int i=0;i<100;i++){
			if(playFreqArr_1[i]!=0){
				playFreqArr_1[i]+=playParamArr[1]+playParamArr[2];
			}
			if(playFreqArr_2[i]!=0){
				playFreqArr_2[i]+=playParamArr[1]+playParamArr[2];
			}
		}
	}
	if(playParamArr[4]==0 && playParamArr[5]==1){	//negative==0 and up==1
		for(int i=0;i<100;i++){
			if(playFreqArr_1[i]!=0){
				playFreqArr_1[i]+=playParamArr[2];
			}
			if(playFreqArr_2[i]!=0){
				playFreqArr_2[i]+=playParamArr[2];
			}
		}
	}
	if(playParamArr[4]==1 && playParamArr[5]==1){	//negative==1 and up==1
		for(int i=0;i<100;i++){
			if(playFreqArr_1[i]!=0){
				playFreqArr_1[i]-=playParamArr[1]-playParamArr[2];
			}
			if(playFreqArr_2[i]!=0){
				playFreqArr_2[i]-=playParamArr[1]-playParamArr[2];
			}
		}
	}
	if(playParamArr[4]==1 && playParamArr[5]==0){	//negative==1 and up==0
		for(int i=0;i<100;i++){
			if(playFreqArr_1[i]!=0){
				playFreqArr_1[i]-=playParamArr[2];
			}
			if(playFreqArr_2[i]!=0){
				playFreqArr_2[i]-=playParamArr[2];
			}
		}
	}
}

//uint32_t calcFreq(uint32_t val)
void calcFreq(void)
{
	if(playParamArr[1]==playParamArr[2]){	//is offset == onset
		fpgaFlags.endOfFile=1;
		return;
	}
	
	//Frequency change from (f0+onset+offset) to (f0+onset)
	if(playParamArr[4]==0 && playParamArr[5]==0){	//negative==0 and up==0
		for(int i=0;i<100;i++){
			if(playFreqArr_1[i]!=0){
				playFreqArr_1[i]--;
			}
			if(playFreqArr_2[i]!=0){
				playFreqArr_2[i]--;
			}
		}
		playParamArr[1]--;
	}
	
	//Frequency change from (f0+onset) to (f0+onset+offset)
	if(playParamArr[4]==0 && playParamArr[5]==1){	//negative==0 and up==1
		for(int i=0;i<100;i++){
			if(playFreqArr_1[i]!=0){
				playFreqArr_1[i]++;
			}
			if(playFreqArr_2[i]!=0){
				playFreqArr_2[i]++;
			}
		}
		playParamArr[1]--;
	}

	//Frequency change from (f0-onset-offset) to (f0-onset)
	if(playParamArr[4]==1 && playParamArr[5]==1){	//negative==1 and up==1
		for(int i=0;i<100;i++){
			if(playFreqArr_1[i]!=0){
				playFreqArr_1[i]++;
			}
			if(playFreqArr_2[i]!=0){
				playFreqArr_2[i]++;
			}
		}
		playParamArr[1]--;
	}
	
	//Frequency change from (f0-onset) to (f0-onset-offset)
	if(playParamArr[4]==1 && playParamArr[5]==0){	//negative==1 and up==0
		for(int i=0;i<100;i++){
			if(playFreqArr_1[i]!=0){
				playFreqArr_1[i]--;
			}
			if(playFreqArr_2[i]!=0){
				playFreqArr_2[i]--;
			}
		}
		playParamArr[1]--;
	}
}


void loadFreqToFpga(void)
{
	uint8_t buff[5];
	uint8_t temp;
	
  while(!(SPI2->SR & SPI_SR_TXE))
  {}
	while (SPI2->SR & SPI_SR_BSY)
  {}
	
	FPGA_CS_L;
	temp=FREQ_CW;
	//while(!(SPI2->SR & SPI_SR_TXE)){}
	spi2Transmit(&temp,1);
	for(int i=0;i<100;i++){
		if(playParamArr[6]==1){
			freq=freqInverse(playFreqArr_1[i]);
		}
		else{
			freq=playFreqArr_1[i];
		}
		buff[0]=0x00;
		buff[1]=0x00;
		buff[2]=(freq & 0x00FFFFFF)>>16;
		buff[3]=(freq & 0x00FFFFFF)>>8;
		buff[4]=(freq & 0x00FFFFFF);
		freq=0;
		while(!(SPI2->SR & SPI_SR_TXE)){}
		spi2Transmit(buff,5);
	}
	for(int i=0;i<100;i++){
		if(playParamArr[6]==1){
			freq=freqInverse(playFreqArr_2[i]);
		}
		else{
			freq=playFreqArr_2[i];
		}
		buff[0]=0x00;
		buff[1]=0x00;
		buff[2]=(freq & 0x00FFFFFF)>>16;
		buff[3]=(freq & 0x00FFFFFF)>>8;
		buff[4]=(freq & 0x00FFFFFF);
		freq=0;
		while(!(SPI2->SR & SPI_SR_TXE)){}
		spi2Transmit(buff,5);
	}
	
	
  while(!(SPI2->SR & SPI_SR_TXE))
  {}
	while (SPI2->SR & SPI_SR_BSY)
  {}

	FPGA_CS_H;
}

void loadMultToFpga(void)
{
	uint8_t buff[3];
	uint8_t temp;
	
	buff[0]=MULT_REG1_CW;
	buff[1]=frstChMult >> 8;
	buff[2]=frstChMult & 0x00FF;
  while(!(SPI2->SR & SPI_SR_TXE))
  {}
	while (SPI2->SR & SPI_SR_BSY)
  {}
	FPGA_CS_L;
	spi2Transmit(buff,3);
	while(!(SPI2->SR & SPI_SR_TXE))
  {}
	while (SPI2->SR & SPI_SR_BSY)
  {}	
	FPGA_CS_H;
		
	buff[0]=MULT_REG2_CW;
	buff[1]=scndChMult >> 8;
	buff[2]=scndChMult & 0x00FF;
  while(!(SPI2->SR & SPI_SR_TXE))
  {}
	while (SPI2->SR & SPI_SR_BSY)
  {}	
	FPGA_CS_L;
	spi2Transmit(buff,3);
  while(!(SPI2->SR & SPI_SR_TXE))
  {}
	while (SPI2->SR & SPI_SR_BSY)
  {}	
	FPGA_CS_H;
}
void startFpga(void)
{
	FPGA_START_H;
	durTimeSLast=playParamArr[3];
	delay_ms(2);
	FPGA_START_L;
}

void getCrc(void)
{
	uint8_t buff[2];
	buff[0]=CRC_CW;
	buff[1]=0x00;
	FPGA_CS_L;
	spi2Transmit(buff,2);
//	crc=SPI2->DR;
	FPGA_CS_H;
}



void SecToHhMmSs(uint32_t timeInSec)
{	
	timeArr[0]=timeInSec/3600;
	timeArr[1]=(timeInSec/60)%60;
	timeArr[2]=timeInSec%60;
}

void setFileTimer(void)
{
	SecToHhMmSs((playParamArr[1]-playParamArr[2]+1)*playParamArr[3]);
	fileHour=timeArr[0];
	fileMin=timeArr[1];
	fileSec=timeArr[2];
}

static uint32_t TotalTime;

void InitTotalTime(void)
{
	uint32_t time=0;
	
	playParamArr[1]=0;
	playParamArr[2]=0;
	playParamArr[3]=0;
	
	for(int i=0;i<SLPl_ui16_NumOffiles;i++){
			{
			LoadParam(i);
			time+=(playParamArr[1]-playParamArr[2]+1)*playParamArr[3];
			playParamArr[1]=0;playParamArr[2]=0;
			playParamArr[3]=0;
		  }
	}
TotalTime=time;
	
};

void InitFileNum(void)
{
s32_t res;	
	spiffs_stat file_stat;
//			File_List=SPIFFS_open(&fs,"freq.pls",SPIFFS_O_RDONLY,0);
			res=SPIFFS_fstat(&fs,File_List,&file_stat);
	    if (SPIFFS_OK==res)
				SLPl_ui16_NumOffiles=file_stat.size/D_FileNameLengthInFL;
};

void SLPl_InitFiles(void)
{
	InitFileNum();	
	InitTotalTime();
};

void setTotalTimer(void)
{	SecToHhMmSs(TotalTime);
	totalHour=timeArr[0];
	totalMin=timeArr[1];
	totalSec=timeArr[2];
}

//void getTimers(void)
//{
//	if(playClk>=999){
//		playClk=0;
//		
//		//Program timer
//		if(fileSec==0){
//			fileSec=59;
//			if(fileMin==0){
//				fileMin=59;
//				if(fileHour==0){
//					fileHour=99;
//				}
//				else{
//					fileHour--;
//				}
//			}
//			else{
//				fileMin--;
//			}
//		}
//		else{
//			fileSec--;
//		}
//		fileTimeArr[0]=fileHour/10;
//		fileTimeArr[1]=fileHour%10;
//		fileTimeArr[3]=fileMin/10;
//		fileTimeArr[4]=fileMin%10;
//		fileTimeArr[6]=fileSec/10;
//		fileTimeArr[7]=fileSec%10;
//		timeToString(fileTimeArr);
//	
//		//Total timer
//		if(totalSec==0){
//			totalSec=59;
//			if(totalMin==0){
//				totalMin=59;
//				if(totalHour==0){
//					totalHour=99;
//				}
//				else{
//					totalHour--;
//				}
//			}
//			else{
//				totalMin--;
//			}
//		}
//		else{
//			totalSec--;
//		}
//		totalTimeArr[0]=totalHour/10;
//		totalTimeArr[1]=totalHour%10;
//		totalTimeArr[3]=totalMin/10;
//		totalTimeArr[4]=totalMin%10;
//		totalTimeArr[6]=totalSec/10;
//		totalTimeArr[7]=totalSec%10;
//		timeToString(totalTimeArr);
//		
////		fpgaFlags.timeUpdate=1;
//	}
//	
//}

//-----------------------------------Hi level function---------------------------------------!!!!!!!!!!!!!!!!!!!!!!!!
uint8_t curState;



/**
\brief Map e_SLD_FSM onto e_PowerState

e_PS_Work,e_PS_DontMindSleep,e_PS_ReadySleep
*/
const e_PowerState SLPl_Encoder[SLPl_FSM_NumOfElements]=
{e_PS_Work      						//SLPl_FSM_InitialWait
,e_PS_ReadySleep						//SLPl_FSM_off
,e_PS_Work									//SLPl_FSM_OnTransition
,e_PS_Work									//SLPl_FSM_On
,e_PS_Work									//SLPl_FSM_OffTransition
};

const bool SLPl_FFSFree_Encoder[SLPl_FSM_NumOfElements]=
{true      						//SLPl_FSM_InitialWait
,true						//SLPl_FSM_off
,false									//SLPl_FSM_OnTransition
,false									//SLPl_FSM_On
,false									//SLPl_FSM_OffTransition
};



//---------------------------------for power sleep---------------------------------------------
//static e_PowerState SLPl_PowerState; 
//static bool SLPl_GoToSleep;

__inline e_PowerState SLPl_GetPowerState(void)
{
	 return SLPl_Encoder[curState];
};

__inline e_PowerState SLPl_SetSleepState(bool state)
{
	//SLPl_GoToSleep=state;
//	return SLPl_PowerState;
	return SLPl_Encoder[curState];
};

//---------------------------------- for comm ------------------------------------------

__inline bool SLPl_FFSFree(void)
{
	return SLPl_FFSFree_Encoder[curState];
};
//---------------------------------- For display-----------------------------------------------
void SLPl_Start(uint32_t nof)
{
	fpgaFlags.playStart=1;
	playFileSector=nof;
};
void SLPl_Stop()
{
	fpgaFlags.playStop=1;
};


//-------------------------for main------------------------------------------------------------

__inline  e_SLPl_FSM Get_SLPl_FSM_State(void)
{
  return curState;
}	

void SLP_init(void)
{
	initSpi_2();
	curState=0;
	//SLPl_PowerState=e_PS_ReadySleep;//RDD for pwr
}

void CalcTimers(void);

void SLP(void)
{
	if(spiDispCapture==1){
		return;
	}
	switch(curState){
		//file list initialization
		case SLPl_FSM_InitialWait:
//			if(fpgaFlags.fileListUpdate==1){
//				//if(!W25qxx_IsEmptySector(fileSect,0))
//					{
/////rdd debug					spi1FifoClr();
//					//W25qxx_ReadSector((uint8_t*)fileName,fileSect,FILE_NAME_SHIFT,FILE_NAME_BYTES);
//					fpgaFlags.addListItem=1;
//				}
//				if(fileSect>=MAX_FILES_NUM){
//					fileSect=0;
//					fpgaFlags.fileListUpdate=0;
//					fpgaFlags.addListItem=0;
//					curState=1;
//				}
//				else{
//					fileSect++;
//				}
//			}
//			
			curState++;
			break;
		
		//waiting for start 
		case SLPl_FSM_off:
			if(fpgaFlags.playStart==1)
			{
				fpgaFlags.playStart=0;
				if (!SLC_SPIFFS_State())
				{
					SetStatusString("Сan't play when comm");
				}
				else
				{
					curState=SLPl_FSM_OnTransition;
				};
			};
			break;
		
		case SLPl_FSM_OnTransition://preparation for the generation process
			PM_OnOffPWR(PM_Player,true );//RDD ON POWER
		  initSpi_2();
			spi2FifoClr();
//			fpgaFlags.fpgaConfig=1;
////			fpgaFlags.progBarClkStart=1;
			fpgaConfig();
////			fpgaFlags.progBarClkStart=0;
////			fpgaFlags.fpgaConfigComplete=1;	//for debug
			fpgaFlags.labelsUpdate=1;
			if(fpgaFlags.fpgaConfigComplete==1)
			{
//				playFileSector=playFileInList;
				playFileSectorBegin=playFileSector;
				setTotalTimer();
				LoadParmFreq(playFileSector);
				setFileTimer();
				setInitFreq();
				loadFreqToFpga();
				loadMultToFpga();
				startFpga();
				fpgaFlags.clockStart=1;
				fpgaFlags.playBegin=1;
				fpgaFlags.labelsUpdate=1;
				curState=SLPl_FSM_On;
				SetStatusString("Config OK");
			}
			else
			{
				SetStatusString("Config failed");
				curState=SLPl_FSM_off;
			}
			break;
			
		//generation process	
		case SLPl_FSM_On:
//			getTimers();
			if(durTimeMs>=999)
			{
				durTimeS++;
				durTimeMs=0;
			}
			CalcTimers();
			if(durTimeS>=durTimeSLast)
			{
				durTimeS=0;
        calcFreq(); 
///rdd debug				spi1FifoClr();
				spi2FifoClr();
				
				if(fpgaFlags.endOfFile==1)
					{
						playFileSector++;
						if(playFileSector>=SLPl_ui16_NumOffiles) 
						{	
							playFileSector=0;
						};	
					if(playFileSector==playFileSectorBegin)
							setTotalTimer();
//					playFileInList=playFileSector;
					LoadParmFreq(playFileSector);
					setFileTimer();
					setInitFreq();
					loadMultToFpga();
					
			    }
				loadFreqToFpga();
				startFpga();
		  }	
			if ((fpgaFlags.playStop==1)||(!SLC_SPIFFS_State()))
			{
				fpgaFlags.playStop=0;
				fpgaFlags.fpgaConfigComplete=0;
				fpgaFlags.playBegin=0;
				fpgaFlags.clockStart=0;
				PM_OnOffPWR(PM_Player,false );//RDD OFF POWER
				
			  totalSec=0;
		    totalMin=0;
		    totalHour=0;
		    fileSec=0;
		    fileMin=0;
		    fileHour=0;
				
				curState=SLPl_FSM_off;
			}
			break;
		default:curState=SLPl_FSM_InitialWait;
			break;
	}
}

void CalcTimers(void)
{
		if(playClk>=999)
		{
		playClk=0;
		
		//Program timer
		if(fileSec==0){
			fileSec=59;
			if(fileMin==0){
				fileMin=59;
				if(fileHour==0){
					fileHour=99;
				}
				else{
					fileHour--;
				}
			}
			else{
				fileMin--;
			}
		}
		else{
			fileSec--;
		}
	
		//Total timer
		if(totalSec==0){
			totalSec=59;
			if(totalMin==0){
				totalMin=59;
				if(totalHour==0){
					totalHour=99;
				}
				else{
					totalHour--;
				}
			}
			else{
				totalMin--;
			}
		}
		else{
			totalSec--;
		}
		fpgaFlags.timeUpdate=1;
	}
};

