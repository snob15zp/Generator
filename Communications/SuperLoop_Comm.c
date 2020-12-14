#include "GlobalKey.h"
#include "SuperLoop_Comm.h"

uint8_t usbCmd;
uint16_t startPageAddr;
uint32_t rxIrqCnt;
uint16_t startPageAddr;
uint16_t filePagesCnt;
uint8_t lastPageBytesNum;
uint16_t filePagesNum;
uint32_t fileBytesNum;
uint32_t nonEmptyBytes;
uint8_t usbRxBuff0[PAGE_SIZE];
uint8_t usbRxBuff1[PAGE_SIZE];
uint8_t usbBuffBytesCnt;
extern uint16_t filePagesCnt;
extern uint8_t lastPageBytesNum;
extern uint16_t filePagesNum;
extern uint32_t fileBytesNum;
extern uint32_t rxIrqCnt;
uint8_t usbCmdBuff[3];

uint8_t debArr1[50];
uint8_t debArr2[206];

volatile struct{
	uint16_t buffSel					:1;
	uint16_t getStartAddr			:1;
	uint16_t buff0DataRdy			:1;
	uint16_t buff1DataRdy			:1;
	uint16_t firstPage				:1;
	uint16_t lastPage					:1;
	uint16_t stopWrite				:1;
}usbFlags;

void SLC_init(void)
{
	uart1Init();
	__flashInit();
}

void SLC(void)
{
	if(spiDispCapture==1){
		return;
	}
	
	procCmdFromUsb();
}

void uart1Init(void)
{
	RCC->APBENR2 |= RCC_APBENR2_USART1EN;
	USART1->CR1 &= ~USART_CR1_UE;
	USART1->BRR = USART1_DIV;		//sets UART1 baudrate 115200 baud
	USART1->CR3 |= USART_CR3_ONEBIT;
	USART1->CR1 |= //USART_CR1_FIFOEN |
									USART_CR1_RXNEIE_RXFNEIE |
									USART_CR1_TE |
									USART_CR1_RE;
	NVIC_SetPriority(USART1_IRQn,1);
	NVIC_EnableIRQ(USART1_IRQn);
	USART1->CR1 |= USART_CR1_UE;
	
	usbFlags.buffSel=0;
	usbBuffBytesCnt=0;
}

void procCmdFromUsb(void)
{
//	usbCmdDetect();
	switch(usbCmd){
		case WR_CONF_FILE:
			wrConfFile();
			return;
		case WR_PLAY_FILES:
			wrPlayFiles();
//			NVIC_SystemReset();
//			fpgaFlags.fileListUpdate=1;
			return;
		case RD_CONF_FILE:
//			rdConfFile();
			rdFlash();
			return;
		case RD_PLAY_FILES:
			rdPlayFiles();
//			rdFlash();
			return;
		case ER_CONF_FILE:
			erFlash(FIRST_CONF_SECT,LAST_CONF_SECT);
			confSectorsStatus();
			usbCmd=0;
		  rxIrqCnt=0;
			//NVIC_SystemReset();
			return;
		case ER_PLAY_FILES:
			erFlash(FIRST_PLAY_SECT,LAST_PLAY_SECT);
			playSectorsStatus();
			usbCmd=0;
		  rxIrqCnt=0;
			//NVIC_SystemReset();
//			fpgaFlags.fileListUpdate=1;
			return;
		case ER_ALL_FILES:
			eraseFlash();
			usbCmd=0;
		  rxIrqCnt=0;
			fpgaFlags.fileListUpdate=1;
			return;
		default:
			return;
	}
}

void wrPlayFiles(void)
{
	if(usbFlags.getStartAddr==0){
		startPageAddr=W25qxx_SectorToPage(findEmptySector());
		usbFlags.getStartAddr=1;
	}
	wrPage();
	if(usbFlags.stopWrite==1){
		usbFlags.stopWrite=0;
		usbCmd=0x00;
		rxIrqCnt=0;
		usbBuffBytesCnt=0;
		filePagesCnt=0;
		usbFlags.lastPage=0;
		usbFlags.buff0DataRdy=0;
		usbFlags.buff1DataRdy=0;
		usbFlags.getStartAddr=0;
		fpgaFlags.fileListUpdate=1;
	}
}

void wrConfFile(void)
{
	if(usbFlags.getStartAddr==0){
		startPageAddr=W25qxx_SectorToPage(FIRST_CONF_SECT);
		usbFlags.getStartAddr=1;
	}
	wrPage();
	if(usbFlags.stopWrite==1){
		usbFlags.stopWrite=0;
		usbCmd=0x00;
		rxIrqCnt=0;
		usbBuffBytesCnt=0;
		filePagesCnt=0;
		usbFlags.lastPage=0;
		usbFlags.buff0DataRdy=0;
		usbFlags.buff1DataRdy=0;
		usbFlags.getStartAddr=0;
		fpgaFlags.fileListUpdate=1;
	}
}

void wrPage(void)
{
	if(usbFlags.lastPage==1){
		if(usbFlags.buffSel==0){
			spi1FifoClr();
			W25qxx_WritePage(usbRxBuff0,startPageAddr+filePagesNum-1,0,lastPageBytesNum);
		}
		else{
			spi1FifoClr();
			W25qxx_WritePage(usbRxBuff1,startPageAddr+filePagesNum-1,0,lastPageBytesNum);
		}
		return;
	}
	if(usbFlags.buff0DataRdy==1){
		spi1FifoClr();
		W25qxx_WritePage(usbRxBuff0,startPageAddr+filePagesCnt-1,0,PAGE_SIZE);
		usbFlags.buff0DataRdy=0;
		return;
	}
	if(usbFlags.buff1DataRdy==1){
		spi1FifoClr();
		W25qxx_WritePage(usbRxBuff1,startPageAddr+filePagesCnt-1,0,PAGE_SIZE);
		usbFlags.buff1DataRdy=0;
		return;
	}
}

//uint32_t isFlashClear(void)
//{
//	uint32_t bugs=0;
//	uint8_t byte;
//	
//	for(int i=0;i<2097152;i++){
//		W25qxx_ReadByte(&byte,i);
//		if(byte!=0xFF){
//			bugs++;
//		}
//	}
//	return bugs;
//}

void eraseFlash(void)
{
	W25qxx_EraseChip();
	fpgaFlags.fileListUpdate=1;
}

uint16_t findEmptySector(void)
{
	uint32_t sectAddr=0;
	while(!(W25qxx_IsEmptySector(sectAddr,0))){
		sectAddr++;
	}
	return sectAddr;
}

void erFlash(uint8_t firstSectAddr, uint8_t lastSectAddr)
{
	if(firstSectAddr<lastSectAddr){
		for(int i=firstSectAddr;i<=lastSectAddr;i++){
			spi1FifoClr();
			W25qxx_EraseSector(i);
		}
	}
	if(firstSectAddr==lastSectAddr){
		spi1FifoClr();
		W25qxx_EraseSector(firstSectAddr);
	}
}

void rdFlash(void)
{
	uint8_t byte;
	
	for(int i=0;i<SECTOR_SIZE*SECTORS_NUM;i++)
	{
		spi1FifoClr();
		W25qxx_ReadSector(&byte,(uint32_t)i,(uint32_t)i,1);
		USART1->TDR = byte;
		while(!(USART1->ISR & USART_ISR_TC)){}
	}
	usbCmd=0;
}

#ifdef COMMS
void USART1_IRQHandler(void)
{
	if(USART1->ISR & USART_ISR_RXNE_RXFNE)
	{
		USART1->ICR |= USART_ICR_ORECF;
		USART1->RQR |= USART_RQR_RXFRQ;
		if(rxIrqCnt<3){
			usbCmdBuff[rxIrqCnt]=USART1->RDR;
			if(rxIrqCnt==2){
				usbCmd=usbCmdBuff[0];
			}
			rxIrqCnt++;
			return;
		}
		USART1->ICR |= USART_ICR_IDLECF;
		USART1->CR1 |= USART_CR1_IDLEIE;
		rxIrqCnt++;
		if(usbFlags.buffSel==0){
			usbRxBuff0[usbBuffBytesCnt]=USART1->RDR;
			if((usbBuffBytesCnt==PAGE_SIZE-1)/*||((usbBuffBytesCnt==PAGE_SIZE-11)&&(usbFlags.firstPage==1))*/){
				filePagesCnt++;
				usbBuffBytesCnt=0;
				usbFlags.buff0DataRdy=1;
				usbFlags.buff1DataRdy=0;
				usbFlags.buffSel=1;
			}
			else{
				usbBuffBytesCnt++;
			}
		}
		else{
			usbRxBuff1[usbBuffBytesCnt]=USART1->RDR;
			if((usbBuffBytesCnt==PAGE_SIZE-1)/*||((usbBuffBytesCnt==PAGE_SIZE-11)&&(usbFlags.firstPage==1))*/){
				filePagesCnt++;
				usbBuffBytesCnt=0;
				usbFlags.buff1DataRdy=1;
				usbFlags.buff0DataRdy=0;
				usbFlags.buffSel=0;
			}
			else{
				usbBuffBytesCnt++;
			}
		}
	}
	if(USART1->ISR & USART_ISR_IDLE){
		USART1->ICR |= USART_ICR_IDLECF;
		USART1->CR1 &= ~USART_CR1_IDLEIE;
		filePagesCnt++;
		usbFlags.lastPage=1;
		if(usbFlags.buffSel==0){
			usbFlags.buff0DataRdy=1;
			usbFlags.buff1DataRdy=0;
		}
		else{
			usbFlags.buff1DataRdy=1;
			usbFlags.buff0DataRdy=0;
		}
		lastPageBytesNum=usbBuffBytesCnt;
		filePagesNum=filePagesCnt;
		fileBytesNum=(filePagesNum-1)*PAGE_SIZE+lastPageBytesNum;
//		usbBuffBytesCnt=0;
//		filePagesCnt=0;
		usbFlags.stopWrite=1;
	}
}
#endif


void playSectorsStatus(void)
{
	for(int i=0;i<MAX_FILES_NUM;i++){
		if(W25qxx_IsEmptySector(i,0))
			debArr1[i]=1;
		else
			debArr1[i]=0;
	}
}

void confSectorsStatus(void)
{
	for(int i=FIRST_CONF_SECT;i<LAST_CONF_SECT+1;i++){
		if(W25qxx_IsEmptySector(i,0))
			debArr2[i]=1;
		else
			debArr2[i]=0;
	}
}

//-----------------------------------for debug----------------------------------
void rdPlayFiles(void)
{
	uint16_t shift;
	uint8_t byte;
	uint32_t fileSize;
	for(uint8_t i=0;i<MAX_FILES_NUM;i++){
		if(!W25qxx_IsEmptySector(i,0)){
			for(shift=0;shift<fileSize;shift++){
				spi1FifoClr();
				W25qxx_ReadSector(&byte,(uint32_t)i,(uint32_t)shift,1);
				USART1->TDR = byte;
				while(!(USART1->ISR & USART_ISR_TC)){}
			}
		}
	}
	rxIrqCnt=0;
	usbCmd=0;
}
