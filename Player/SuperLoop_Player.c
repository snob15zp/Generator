#include "Spi1.h"
#include "SuperLoop_Player.h"

uint16_t freqStartByte;
uint32_t freq;

volatile uint32_t tim3TickCounter;
extern volatile uint32_t playClk;
volatile uint32_t durTimeMs;
volatile uint32_t durTimeS;
uint16_t steps;

extern volatile int playFileInList;
uint16_t playFileSector;
uint8_t i = 0;


#define  playParamArr_size 7

uint32_t playParamArr[playParamArr_size];
/*************************************
playParamArr[0] - frequencies
playParamArr[1] - offset
playParamArr[2] - onset
playParamArr[3] - duration
playParamArr[4] - negative
playParamArr[5] - up
playParamArr[6] - inverse
**************************************/

uint8_t timeArr[3];
/*******************************
[0] - hours
[1] - min
[2] - sec
*******************************/
uint8_t totalSec = 0;
uint8_t totalMin = 0;
uint8_t totalHour = 0;
uint8_t fileSec = 0;
uint8_t fileMin = 0;
uint8_t fileHour = 0;

//------------------------ for power---------------------------------------------
e_FSMState_SuperLoopPlayer SLPl_FSM_State;

__inline e_FSMState_SuperLoopPlayer SLPl_FSMState(void)
{
	return SLPl_FSM_State;
};

bool SuperLoop_Player_SleepIn(void)
{
	return true;
};
bool SuperLoop_Player_SleepOut(void)
{
	return true;
};

//-------------------------for SPI2-----------------------------------------------
void initSpi_2(void)
{
	RCC->APBENR1 |= RCC_APBENR1_SPI2EN; // enable SPI2 clk
	SPI2->CR1 &= ~SPI_CR1_SPE;			// disable SPI2 perif
	SPI2->CR1 |= /*SPI_CR1_BR_2 | SPI_CR1_BR_1 |*/ SPI_CR1_BR_0 |
				 SPI_CR1_SSM | // SPI_NSS_Soft
				 SPI_CR1_SSI |
				 SPI_CR1_LSBFIRST |
				 //               SPI_CR1_CPOL |															// SPI_CPOL_High
				 //               SPI_CR1_CPHA |															// SPI_CPHA_2Edge
				 SPI_CR1_MSTR;	// master mode select
	SPI2->CR2 |= SPI_CR2_FRXTH; // RXNE event is generated if the FIFO level is greater than or equal to 1/4 (8-bit)
								//							 SPI_CR2_RXNEIE;														// enable SPI2 RXNE interrupt
	SPI2->CR1 |= SPI_CR1_SPE;	// enable SPI2 perif
}

void spi2Transmit(uint8_t *pData, uint16_t Size)
{
	uint16_t TxXferCount = Size;
	uint16_t Temp;
	/* Transmit data in 8 Bit mode */
	while (TxXferCount > 0U)
	{
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
			else
			{
				*(__IO uint8_t *)&SPI2->DR = *pData;
				pData++;
				TxXferCount--;
			}
		}
	}
	while (SPI2->SR & SPI_SR_BSY)
	{
	}

	for (int i = 0; i < 2; i++)
	{
		Temp = SPI2->DR;
	}
}

void spi2FifoClr(void)
{
	uint16_t Temp;

	for (int i = 0; i < 4; i++)
	{
		Temp = SPI2->DR;
	}
}

//-----------------------------------for TIM3---------------------------------------

void tim3Init(void)
{
	RCC->APBENR1 |= RCC_APBENR1_TIM3EN;
	TIM3->PSC = 63;	  //sets TIM3 clk 1 MHz
	TIM3->ARR = 1000; //sets TIM3 count interval 1 ms
	TIM3->DIER = TIM_DIER_UIE;
	NVIC_SetPriority(TIM3_IRQn, 15);
	NVIC_EnableIRQ(TIM3_IRQn);
	TIM3->CR1 = TIM_CR1_CEN;
}

void delay_ms(uint32_t delayTime)
{
	tim3TickCounter = delayTime;
	while (tim3TickCounter)
	{
	}
}

void TIM3_IRQHandler(void)
{
	if (TIM3->SR & TIM_SR_UIF)
	{
		TIM3->SR = ~TIM_SR_UIF;
		tim3TickCounter--;
		if (fpgaFlags.clockStart == 1)
		{
			playClk++;
			durTimeMs++;
		}
		else
		{
			playClk = 0;
			durTimeMs = 0;
		}
	}
}

//-----------------------------------for player---------------------------------------

// void fpgaConfig(void)
// {
// 	uint8_t bytesConfBuff[CONF_BUFF_SIZE];
// 	uint32_t bytesCnt;
// 	uint32_t bytesRemain = CONF_FILE_SIZE;
// 	uint16_t bytesToRead;

// 	SPI2->CR1 &= ~SPI_CR1_SPE;
// 	SPI2->CR1 |= SPI_CR1_LSBFIRST;
// 	SPI2->CR1 |= SPI_CR1_SPE;
// 	GPIOB->BSRR = GPIO_BSRR_BR0; //FPGA 1.2 V on
// 	nCONFIG_H;
// 	while (!(GPIOC->IDR & GPIO_IDR_ID7))
// 	{
// 	}
// 	delay_ms(1);
// 	FPGA_CS_L; //for logger
// 	while (bytesRemain > 0)
// 	{
// 		bytesCnt += bytesToRead;
// 		bytesRemain = CONF_FILE_SIZE - bytesCnt;
// 		if (bytesRemain >= CONF_BUFF_SIZE)
// 		{
// 			bytesToRead = CONF_BUFF_SIZE;
// 		}
// 		else
// 		{
// 			bytesToRead = bytesRemain;
// 		}
// 		W25qxx_ReadBytes(bytesConfBuff, FIRST_CONF_BYTE + 10 + bytesCnt, bytesToRead);
// 		spi2Transmit(bytesConfBuff, bytesToRead);
// 		if (GPIOC->IDR & GPIO_IDR_ID6)
// 		{
// 			spi2Transmit(0x00, 1);
// 			confComplete();
// 			fpgaFlags.fpgaConfigComplete = 1;
// 			fpgaFlags.fpgaConfigCmd = 0;
// 			FPGA_CS_H;
// 			SPI2->CR1 &= ~SPI_CR1_SPE;
// 			SPI2->CR1 &= ~SPI_CR1_LSBFIRST;
// 			SPI2->CR1 |= SPI_CR1_SPE;
// 			return;
// 		}
// 	}
// 	FPGA_CS_H;
// 	confFailed();
// 	fpgaFlags.fpgaConfigComplete = 0;
// 	fpgaFlags.fpgaConfigCmd = 0;
// }

//As in VV2
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
� All I/Os pins are tied to an internal weak pull-up
� Clears configuration RAM bits
      ->
nSTATUS and nCONFIG released high
CONF_DONE pulled low
      ->
Writes configuration data to
FPGA
      ->
� nSTATUS pulled low
� CONF_DONE remains low
� Restarts configuration if option
enabled		
      ->
CONF_DONE released high
      ->
� Initializes internal logic and
registers
� Enables I/O buffers
      ->
INIT_DONE released high
(if option enabled)
      ->
Executes your design
*/
void fpgaConfig(void)											//
{
	uint32_t bytesCnt = 0;
	uint8_t byteBuff;
//	byteBuff=0;
//	spi2Transmit(&byteBuff, 1);
	
	SPI2->CR1 &= ~SPI_CR1_SPE;
	SPI2->CR1 |= SPI_CR1_LSBFIRST;
	SPI2->CR1 |= SPI_CR1_SPE;
	
	switchOUTStageInterfacePinsToPwr(DISABLE);//RDD DEBUG
	delay_ms(100);// test -> ok
	switchOUTStageInterfacePinsToPwr(ENABLE);
	delay_ms(10);
	spi1FifoClr();
//	GPIOB->BSRR=GPIO_BSRR_BR0;							//FPGA 1.2 V on
	nCONFIG_H;
	while(!(GPIOC->IDR & GPIO_IDR_ID7)){/** \todo timeout */}
	delay_ms(10);
	FPGA_CS_L;															//for logger
	for(bytesCnt=0;bytesCnt<CONF_FILE_SIZE;bytesCnt++){
		W25qxx_ReadByte(&byteBuff,FIRST_CONF_BYTE+bytesCnt);
		spi2Transmit(&byteBuff, 1);
		if(GPIOC->IDR & GPIO_IDR_ID6)
			{byteBuff=0;
			spi2Transmit(&byteBuff, 1);
//			confComplete();
			fpgaFlags.fpgaConfigComplete=1;
			FPGA_CS_H;
			SPI2->CR1 &= ~SPI_CR1_SPE;
			SPI2->CR1 &= ~SPI_CR1_LSBFIRST;
			SPI2->CR1 |= SPI_CR1_SPE;
			return;
		}
	}
//	byteBuff=0;
//	spi2Transmit(&byteBuff, 1);
	FPGA_CS_H;
	//	confFailed();
	fpgaFlags.fpgaConfigComplete = 0;
}

extern uint8_t fileName[50];
extern uint8_t fileSect;

// void getFileList(void)
// {
// 	uint8_t sect;
// 	for (sect = 0; sect < MAX_FILES_NUM; sect++)
// 	{

// 		spi1FifoClr();
// 		W25qxx_ReadSector((uint8_t *)fileName, sect, FILE_NAME_SHIFT, FILE_NAME_BYTES);
// 		spi1FifoClr();
// 		gwinListAddItem(ghList1, (char *)fileName, gTrue);
// 	}
// }
// }

//
void timeToString(uint8_t *timeArr)
{
	for (int i = 0; i < 8; i++)
	{
        if( timeArr[i] < 10) 
        {
            timeArr[i] = timeArr[i] + '0';
        }
        else
        {
            timeArr[i] = ' ';
        }
	}
    timeArr[9] = 0;
}

uint16_t getPlayFileSector(int fileInList)
{
	uint16_t sect = 0;

	for (sect = 0; sect < MAX_FILES_NUM; sect++)
	{
		if (!W25qxx_IsEmptySector(sect, 0))
		{
			if (sect == fileInList)
			{
				return sect;
			}
		}
	} /** \todo no return, no error detect */
}

void getControlParam(uint16_t fileSect)
{
	uint8_t temp;
	uint8_t tempArr[6];
	uint16_t byteCnt = 0;
	uint8_t strCnt = 0;
	uint8_t chrCnt = 0;
	uint32_t startAddr = fileSect * SECTOR_SIZE;

	do
	{ //skip first line
		W25qxx_ReadByte(&temp, startAddr + byteCnt);
		byteCnt++;
	}while(temp!='\n');
	
	for(int i=0;i<playParamArr_size;i++) {playParamArr[i]=0;}
	
	while(strCnt<7){																//fill an array of parameters
		W25qxx_ReadByte(&temp,startAddr+byteCnt);
		byteCnt++;
		if((temp>='0')&&(temp<='9')){
			tempArr[chrCnt]=temp; /** \todo check array overflow */
			chrCnt++;
			continue;
		}
		if (temp == '\n')
		{
			for (int i = 0; i < chrCnt; i++)
			{
				playParamArr[strCnt] += (uint32_t)(tempArr[i] & 0x0F) * (uint32_t)powf(10, chrCnt - 1 - i);
				tempArr[i] = 0;
			}
			chrCnt = 0;
			strCnt++;
			continue;
		}
	}
	freqStartByte = startAddr + byteCnt;
}

uint32_t freqInverse(uint32_t freq)
{
	uint8_t tempArr[6];
	uint32_t remain;

	tempArr[5] = freq / 100000U;
	remain = freq % 100000U;
	tempArr[4] = remain / 10000U;
	remain = remain % 10000U;
	tempArr[3] = remain / 1000U;
	remain = remain % 1000U;
	tempArr[2] = remain / 100U;
	remain = remain % 100U;
	tempArr[1] = remain / 10U;
	remain = remain % 10U;
	tempArr[0] = remain;
	for (int i = 0; i < 6; i++)
	{
		if (tempArr[i] != 0)
			tempArr[i] = 10 - tempArr[i];
	}
	return tempArr[5] * 100000U + tempArr[4] * 10000U + tempArr[3] * 1000U + tempArr[2] * 100U + tempArr[1] * 10U + tempArr[0];
}

//uint8_t steps;

uint32_t calcFreq(uint32_t val)
{
	//	static uint8_t steps;
	//Frequency change from (f0+offset) to f0
	if (playParamArr[4] == 0 && playParamArr[5] == 0)
	{ //negative==0 and up==0

		if (playParamArr[1] == 0)
		{
			fpgaFlags.endOfFile = 1;
		}
		else
		{
			val += playParamArr[1];
		}
		playParamArr[1]--;
	}
	//Frequency change from f0 to (f0-offset)
	if (playParamArr[4] == 1 && playParamArr[5] == 0)
	{ //negative==1 and up==0

		if (steps == playParamArr[1])
		{
			fpgaFlags.endOfFile = 1;
			steps = 0;
		}
		else
		{
			val -= steps;
		}
		//		steps++;
	}
	//Frequency change from f0 to (f0+offset)
	if (playParamArr[4] == 0 && playParamArr[5] == 1)
	{ //negative==0 and up==1

		if (steps == playParamArr[1])
		{
			fpgaFlags.endOfFile = 1;
			steps = 0;
		}
		else
		{
			val += steps;
		}
		//		steps++;
	}
	//Validation value inverse
	if (playParamArr[6] == 1)
	{
		val = freqInverse(val);
	}
	return val;
}

void loadFreqToFpga(uint16_t addr)
{
	uint16_t byteCnt = 0;
	uint8_t strCnt = 0;
	uint8_t chrCnt = 0;
	//	uint32_t c;
	uint8_t tempArr[] = {'0', '0', '0', '0', '0', '0'};
	uint8_t buff[5];
	uint8_t temp;

	FPGA_CS_L;
	temp = FREQ_CW;
	while (!(SPI2->SR & SPI_SR_TXE))
	{
	}
	spi2Transmit(&temp, 1);
	//	freq=calcFreq(freq);
	while (strCnt < playParamArr[0])
	{
		W25qxx_ReadByte(&temp, addr + byteCnt);
		byteCnt++;
		if((temp>='0')&&(temp<='9')){
			tempArr[chrCnt]=temp;/** \todo check overflow array */
			chrCnt++;
			continue;
		}
		if (temp == '\n')
		{
			for (int i = 0; i < chrCnt; i++)
			{
				freq += (uint32_t)(tempArr[i] & 0x0F) * (uint32_t)powf(10, chrCnt - 1 - i);
				tempArr[i] = 0;
			}
			chrCnt = 0;
			strCnt++;
			freq = calcFreq(freq);
			buff[0] = 0x00;
			buff[1] = 0x00;
			buff[2] = (freq & 0x00FFFFFF) >> 16;
			buff[3] = (freq & 0x00FFFFFF) >> 8;
			buff[4] = (freq & 0x00FFFFFF);
			freq = 0;
			while (!(SPI2->SR & SPI_SR_TXE))
			{
			}
			spi2Transmit(buff, 5);
		}
	}
	FPGA_CS_H;
	steps = steps + 1000;
}

void loadMultToFpga(void)
{
	uint8_t buff[3];
	//load mult
	buff[0] = MULT_REG1_CW;
	buff[1] = MULT_VAL_1 >> 8;
	buff[2] = MULT_VAL_1 & 0x00FF;
	while (!(SPI2->SR & SPI_SR_TXE))
	{
	}
	FPGA_CS_L;
	spi2Transmit(buff, 3);
	FPGA_CS_H;
	buff[0] = MULT_REG2_CW;
	buff[1] = MULT_VAL_2 >> 8;
	buff[2] = MULT_VAL_2 & 0x00FF;
	while (!(SPI2->SR & SPI_SR_TXE))
	{
	}
	FPGA_CS_L;
	spi2Transmit(buff, 3);
	FPGA_CS_H;
}
void startFpga(void)
{
	FPGA_START_H;
	delay_ms(1);
	FPGA_START_L;
}

void getCrc(void)
{
	uint8_t buff[2];
	buff[0] = CRC_CW;
	buff[1] = 0x00;
	FPGA_CS_L;
	spi2Transmit(buff, 2);
	//	crc=SPI2->DR;
	FPGA_CS_H;
}

void fileListInit(void)
{
	fpgaFlags.fileListUpdate = 1;
	//	fpgaFlags.labelsUpdate=1;
	fileSect = 0;
}

void SecToHhMmSs(uint32_t timeInSec)
{
	timeArr[0] = timeInSec / 3600;
	timeArr[1] = (timeInSec / 60) % 60;
	timeArr[2] = timeInSec % 60;
}

void setFileTimer(void)
{
	SecToHhMmSs((playParamArr[1]+1)*playParamArr[3]);
	fileHour=timeArr[0];
	fileMin=timeArr[1];
	fileSec=timeArr[2];
}

void setTotalTimer(void)
{
	uint32_t time = 0;

	playParamArr[1] = 0;
	playParamArr[3] = 0;
	for (int i = 0; i < 50; i++)
	{
		if (!W25qxx_IsEmptySector(i, 0))
		{
			getControlParam(i);
			time+=(playParamArr[1]+1)*playParamArr[3];
			playParamArr[1]=0;
			playParamArr[3]=0;
		}
	}
	SecToHhMmSs(time);
	totalHour = timeArr[0];
	totalMin = timeArr[1];
	totalSec = timeArr[2];
}

//-----------------------------------for main---------------------------------------
void SLP_init(void)
{
	initSpi_2();
	//	fpgaFlags.fileListUpdate=1;
}

void SLP(void)
{
	if (spiDispCapture == 1)
	{
		return;
	}
	//start of the generation process
	if (fpgaFlags.playStart == 1)
	{
		if (fpgaFlags.fpgaConfig == 1)
		{
			fpgaFlags.fpgaConfig = 0;
			spi1FifoClr();
			spi2FifoClr();
			//fpgaConfig();
			//GPIOB->BSRR=GPIO_BSRR_BS0;	//FPGA 1.2 V off
			//delay_ms(20);
			//spi1FifoClr();
			//spi2FifoClr();
			fpgaConfig();
			fpgaFlags.labelsUpdate = 1;
			//******************************************
			//			fpgaFlags.fpgaConfigComplete=1;	//for debug
			//******************************************
			//			fpgaFlags.labelsUpdate=1;
		}
		if(fpgaFlags.fpgaConfigComplete==1){
			playFileSector=getPlayFileSector(playFileInList);
			setTotalTimer();
			getControlParam(playFileSector);
			setFileTimer();
			//********************************************
			//for debug
			//			SPI2->CR1 &= ~SPI_CR1_SPE;
			//			SPI2->CR1 &= ~SPI_CR1_LSBFIRST;
			//			SPI2->CR1 |= SPI_CR1_SPE;
			//********************************************
			//			freq=calcFreq(freq);
			loadFreqToFpga(freqStartByte);
			loadMultToFpga();
			startFpga();
			//			getCrc();

			//begin generation
			fpgaFlags.playStart = 0;
			fpgaFlags.clockStart = 1;
			fpgaFlags.playBegin = 1;
			fpgaFlags.labelsUpdate = 1;
		}
	}

	//generation process
	if (fpgaFlags.playBegin == 1)
	{
		if (durTimeMs >= 999)
		{
			durTimeS++;
			durTimeMs = 0;
			//			steps=steps+1000;
		}
		if (durTimeS >= playParamArr[3])
		{
			FPGA_START_H;
			delay_ms(1);
			FPGA_START_L;
			durTimeS = 0;
			spi1FifoClr();
			spi2FifoClr();
			//			steps=steps+1000;
			//			freq=calcFreq(freq);
			loadFreqToFpga(freqStartByte);
		}
	}

	//end of generation process
	if(fpgaFlags.playStop==1){
		GPIOB->BSRR=GPIO_BSRR_BS0;	//FPGA 1.2 V off
		fpgaFlags.fpgaConfigComplete=0;
		fpgaFlags.playBegin=0;
		fpgaFlags.clockStart=0;
		fpgaFlags.labelsUpdate=1;
		//NVIC_SystemReset();
	}

	//get list of files
	if (fpgaFlags.fileListUpdate == 1)
	{
		if (!W25qxx_IsEmptySector(fileSect, 0))
		{
			spi1FifoClr();
			W25qxx_ReadSector((uint8_t *)fileName, fileSect, FILE_NAME_SHIFT, FILE_NAME_BYTES);
			fpgaFlags.addListItem = 1;
		}
		if (fileSect >= MAX_FILES_NUM)
		{
			fileSect = 0;
			fpgaFlags.fileListUpdate = 0;
			fpgaFlags.addListItem = 0;
		}
		else
		{
			fileSect++;
		}
	}
}
