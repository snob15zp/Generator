#include "I2C1.h"


static i2cState_e i2cMode;
static i2cPacket_t i2cPacket;
static uint16_t i2cErrorCode;
uint8_t*	pI2cBuff;

static uint8_t ErrorI2C_ISR_NACKF;
static uint8_t ErrorI2C_ICR_OVRCF; 
static uint8_t ErrorI2C_ICR_ARLOCF; 
static uint8_t ErrorI2C_ICR_BERRCF;

/*************************************************************************************************************************
*
*
**************************************************************************************************************************/
	/**
	 * @brief   do i2c transfer, read or write.
	 * @pre		error when read (write) 1 byte
	 *
	 * @param[in] p1
	 * @param[in] p2
	 * @param[in] p3
	 * @param[in] p4
	 * @param[in] p5
	 *
	 * @api
	 */

void I2C1_IRQHandler(void){
	
	uint16_t isrVal = I2C1->ISR;
  
	if(isrVal & I2C_ISR_TXIS){
 		
		if(i2cPacket.numOfSubAddrBytes > 0){
			
			i2cPacket.numOfSubAddrBytes--;				
			I2C1->TXDR = i2cPacket.subAddr[i2cPacket.numOfSubAddrBytes];			
     // if((i2cPacket.numOfSubAddrBytes == 0) && (i2cPacket.direct == TRANSFER_READ))
     //                                                                               { I2C1->CR1 &= ~I2C_CR1_TXIE;}; 
				
		} else if ((i2cPacket.numOfDataBytes > 0) && (i2cPacket.direct == TRANSFER_WRITE)){	
			
       I2C1->TXDR = *pI2cBuff++;
       i2cPacket.numOfDataBytes--;
      // if(i2cPacket.numOfDataBytes == 0){I2C1->CR1 &= ~I2C_CR1_TXIE;}
			 
		 } 		
	}
	 
  if(isrVal & I2C_ISR_TC) 
		{ 
		
   // I2C1->CR1 &= ~I2C_CR1_TCIE;                         	
   // i2cMode = (i2cPacket.direct == TRANSFER_READ) ? I2C_SUBADDR_TRANSFERED : I2C_TRANSACTION_OK;
   	 if (i2cPacket.direct == TRANSFER_WRITE) 	
		 {
			I2C1->CR2 |= I2C_CR2_STOP;
		 }
     else
     {if 	(I2C1->CR2 & I2C_CR2_RD_WRN)
        {			 
	        I2C1->CR2 |= I2C_CR2_STOP;		
				}
  		else
			  { 				
          I2C1->CR2 &= ~I2C_CR2_NBYTES_Msk;  																							// clear NBYTES area 
          I2C1->CR2 |= (1 << I2C_CR2_NBYTES_Pos) | I2C_CR2_RD_WRN|I2C_CR2_RELOAD;	// set transfer bytes count and RD_WRN bit                            
          I2C1->CR2 |= I2C_CR2_START;                                   									// repeated start 
		    } 
			}
     }
  
  if(isrVal & I2C_ISR_RXNE)
		{
		
     *pI2cBuff++ = I2C1->RXDR;
     i2cPacket.numOfDataBytes--;
		};	
		
  if(isrVal & I2C_ISR_TCR){
	
    if(i2cPacket.numOfDataBytes == 1)
			{
			 I2C1->CR2 &= ~(I2C_CR2_RELOAD_Msk);	
			 //I2C1->CR2 &= ~I2C_CR2_NBYTES_Msk; 	
			 I2C1->CR2 |= (1 << I2C_CR2_NBYTES_Pos);	
 //     I2C1->CR1 &= ~I2C_CR1_RXIE;
 //     i2cMode = I2C_TRANSACTION_OK;
			
      }
			else
			{	
				I2C1->CR2 &= ~I2C_CR2_NBYTES_Msk;  																							// clear NBYTES area 
        I2C1->CR2 |= (1 << I2C_CR2_NBYTES_Pos);
			}
  }
   
	 
 if(isrVal & I2C_ISR_STOPF)
	 {
	  I2C1->ICR = I2C_ICR_STOPCF;	
    i2cMode = I2C_IDLE;
	//I2C1->CR1 &= ~(I2C_CR1_NACKIE | I2C_CR1_RXIE | I2C_CR1_TCIE | I2C_CR1_TXIE);    // disable interrupts
   } 	
	 
	 
 if(isrVal & I2C_ISR_NACKF)
	 {
	  I2C1->ICR = I2C_ICR_NACKCF;	
    i2cMode = I2C_TRANSACTION_ERROR;
	//I2C1->CR1 &= ~(I2C_CR1_NACKIE | I2C_CR1_RXIE | I2C_CR1_TCIE | I2C_CR1_TXIE);    // disable interrupts
		 ErrorI2C_ISR_NACKF++;
   } 	
	 
 if ( (isrVal & I2C_ICR_OVRCF) )
   {
		 I2C1->ICR = I2C_ICR_OVRCF ;
	   i2cMode = I2C_TRANSACTION_ERROR;
		 i2cErrorCode=1;
		 ErrorI2C_ICR_OVRCF++;
	 };	 
	if (  (isrVal & I2C_ICR_ARLOCF)  )
   {
		 I2C1->ICR =  I2C_ICR_ARLOCF ;
	   i2cMode = I2C_TRANSACTION_ERROR;
		 i2cErrorCode=1;
		 ErrorI2C_ICR_ARLOCF++;
	 };	 
if (  (isrVal & I2C_ICR_BERRCF) )
   {
		 I2C1->ICR =  I2C_ICR_BERRCF;
	   i2cMode = I2C_TRANSACTION_ERROR;
		 i2cErrorCode=1;
		 ErrorI2C_ICR_BERRCF++;
	 };	 
	 
}

/*************************************************************************************************************************
*
*
**************************************************************************************************************************/
void I2c1InSleep(void)
{
	RCC->APBENR1 |= RCC_APBENR1_I2C1EN;											// I2C1 clock enable
  I2C1->CR1 &= ~I2C_CR1_PE;
	RCC->APBENR1 &= ~RCC_APBENR1_I2C1EN;
};

void I2c1OutSleep(void)	
{
	initI2c1();
};

void initI2c1(void){
      
	RCC->APBENR1 |= RCC_APBENR1_I2C1EN;											// I2C1 clock enable
  I2C1->CR1 &= ~I2C_CR1_PE;																// I2C1 peripheral disable
	I2C1->CR1 &= ~I2C_CR1_ANFOFF_Msk;												// Analog noise filter Enable
	
	I2C1->TIMINGR |=  0xF0000000 |							            // TIMINGR register configuration for Standard-mode 100 kHz 
									 (0x26 << I2C_TIMINGR_SCLL_Pos) |//(0x13 << I2C_TIMINGR_SCLL_Pos) |
									 (0x1e << I2C_TIMINGR_SCLH_Pos) |//(0x0F << I2C_TIMINGR_SCLH_Pos) |
									 (0x04 << I2C_TIMINGR_SDADEL_Pos) |//(0x02 << I2C_TIMINGR_SDADEL_Pos) |
									 (0x08 << I2C_TIMINGR_SCLDEL_Pos) ;//(0x04 << I2C_TIMINGR_SCLDEL_Pos);    
	I2C1->CR1	&= ~I2C_CR1_NOSTRETCH; 												// Clock stretching disable. It must be kept cleared in master mode
  I2C1->CR2 &= ~(I2C_CR2_ADD10 |                          // master operates in 7-bit addressing mode
                 I2C_CR2_AUTOEND |                        // disable autoend mode 
                 I2C_CR2_RELOAD);                         // TCR flag is set when NBYTES data are transferred, stretching SCL low.
	I2C1->CR1 |= I2C_CR1_PE;														    // I2C peripheral enable
	
	I2C1->CR1 |=((uint32_t)0xf)<<I2C_CR1_DNF_Pos;
	
	
	NVIC_SetPriority(I2C1_IRQn,2);
  NVIC_EnableIRQ(I2C1_IRQn);
}


/***********************************************************************************************


************************************************************************************************/
static void i2cStart(void){
	
	i2cMode = I2C_BUSY; 
	i2cErrorCode = 0;
 	
  I2C1->CR2 &= ~(I2C_CR2_NBYTES_Msk | I2C_CR2_SADD_Msk | I2C_CR2_RD_WRN_Msk |I2C_CR2_RELOAD_Msk);	// clear NBYTES, SADD data and RD_WRN bit 
	
	uint8_t transactionLen = i2cPacket.numOfSubAddrBytes;
	if (i2cPacket.direct == TRANSFER_WRITE) { transactionLen += i2cPacket.numOfDataBytes;}
	
  I2C1->CR2 |= i2cPacket.slaveAddr | (transactionLen << I2C_CR2_NBYTES_Pos);	// set slave device address and transfer bytes count                          
  I2C1->CR1 |= I2C_CR1_NACKIE | I2C_CR1_TCIE | I2C_CR1_TXIE | I2C_CR1_RXIE
            	| I2C_CR1_ERRIE | I2C_CR1_STOPIE ;	                         	// enable all interrupts	
  I2C1->CR2 |= I2C_CR2_START;                                   							// start transaction
 
}

/***********************************************************************************************


************************************************************************************************/
//static void i2cRepStart(void){
//	
//  I2C1->CR2 &= ~I2C_CR2_NBYTES_Msk;  																							// clear NBYTES area 	
////  I2C1->CR2 |= (i2cPacket.numOfDataBytes << I2C_CR2_NBYTES_Pos) | I2C_CR2_RD_WRN;	// set transfer bytes count and RD_WRN bit   
//  I2C1->CR2 |= (1 << I2C_CR2_NBYTES_Pos) | I2C_CR2_RD_WRN|I2C_CR2_RELOAD;	// set transfer bytes count and RD_WRN bit   	
//  //I2C1->CR1 |= I2C_CR1_RXIE;                           														// enable RX interrupt
//  I2C1->CR2 |= I2C_CR2_START;                                   									// repeated start 
//  
//}

/***********************************************************************************************


************************************************************************************************/
uint16_t i2cDataRW(slaveChip_e chipID, transferMode_e tMode, uint8_t slaveAddr, uint16_t subAddr,  uint8_t subAddrLen, uint8_t* pDataBuff, uint8_t len ){

	uint16_t result = 1;

	I2C1->CR1 &= ~I2C_CR1_PE;														    								// I2C peripheral disable
	
	GPIOB->MODER |= GPIO_MODER_MODE6_Msk | GPIO_MODER_MODE7_Msk |         	// set all i2c1 pins as analog inputs  
                  GPIO_MODER_MODE8_Msk | GPIO_MODER_MODE9_Msk;
  
  switch (chipID){    
  	case TPS65987_CHIP:
				GPIOB->MODER &= ~(GPIO_MODER_MODE8_0 | GPIO_MODER_MODE9_0);			  // set PB8 as I2C1_SCL pin and PB9 as I2C1_SDA pin
  		break;    
  	case BQ25703_CHIP:
				GPIOB->MODER &= ~(GPIO_MODER_MODE8_0 | GPIO_MODER_MODE9_0);			  // set PB8 as I2C1_SCL pin and PB9 as I2C1_SDA pin
  		break;  
  	case BQ28Z610_CHIP:
				GPIOB->MODER &= ~(GPIO_MODER_MODE6_0 | GPIO_MODER_MODE7_0);			  // set PB6 as I2C1_SCL pin and PB7 as I2C1_SDA pin
  		break; 
		default:
				result = 0;
			break;		
  }
	
	if (!result) {return result;}
	
	I2C1->CR1 |= I2C_CR1_PE;														    								// I2C peripheral enable
	

	i2cPacket.direct = tMode;
  i2cPacket.slaveAddr = slaveAddr << 1;
	i2cPacket.subAddr[0] = *(uint8_t*)&subAddr;
	i2cPacket.subAddr[1] = *((uint8_t*)&subAddr + 1);
	i2cPacket.numOfSubAddrBytes = subAddrLen;
	i2cPacket.numOfDataBytes = len;
	pI2cBuff = pDataBuff;  
  i2cStart();
	
	return result;
  
}


/***********************************************************************************************


************************************************************************************************/
//void i2cStateMachine(void){
//  
//  switch (i2cMode){
//		
//    case I2C_IDLE:
//      break;	
//		
//     case I2C_BUSY:
//      break;
//		 
//    case I2C_SUBADDR_TRANSFERED:
//				i2cRepStart();
//				i2cMode = I2C_BUSY;
//      break;
//		
//    case I2C_TRANSACTION_OK:
//				I2C1->CR2 |= I2C_CR2_STOP;
//				i2cMode = I2C_IDLE;
//      break;
//		
//    case I2C_TRANSACTION_ERROR:
//			  I2C1->CR2 |= I2C_CR2_STOP;
//				i2cErrorCode = 1;
//				i2cMode = I2C_IDLE;
//      break;
//   }

//}

/***********************************************************************************************


************************************************************************************************/
i2cState_e getI2cStatus(void) {
		return i2cMode;
}

void getI2cReset(void) {
	I2C1->CR1 &= ~I2C_CR1_PE;														    								// I2C peripheral disable
	i2cMode = I2C_IDLE;
	I2C1->CR1 |= I2C_CR1_PE;	
}

/***********************************************************************************************


************************************************************************************************/
uint16_t getI2cError(void) {
	return i2cErrorCode;
}

