#include "i2c_API.h"
#include "I2C1.h"
//#include "BoardSetup.h"

#define	TPS65987_ADDR								0x22		// I2C device address
#define	BQ25703_ADDR									0x6B		// I2C device address
#define	BQ28Z610_ADDR										0x55		// I2C device address	

#define MaxI2CExchangeTime              ((systemticks_t)30) //ms
#define InitPauseI2CExchangeTime        ((systemticks_t)2) //ms

//typedef enum  {TPS87,bq25703,bq28z610,NumOfDevices} e_I2C_API_Devices;
const slaveChip_e i2c_Devices[NumOfDevices]={TPS65987_CHIP,		BQ25703_CHIP,    BQ28Z610_CHIP };
const uint8_t i2c_DevicesAddr[NumOfDevices]={ TPS65987_ADDR, BQ25703_ADDR ,BQ28Z610_ADDR };
//#define I2C_OP_READ         ((unsigned short)(0))
//#define I2C_OP_WRITE        ((unsigned short)(1))
const transferMode_e optype[]={TRANSFER_READ,TRANSFER_WRITE}; 

// t_I2C_API_DATA I2C_API_DATA[NumOfDevices];
//typedef enum {TPS82,TPS86,bq25703,bq28z610,NumOfDevices} e_I2C_API_Devices;
// const uint8_t devices_addr[NumOfDevices]= {0x38<<1, 0x38<<1, 0xD6,0xAA};
 
//int i2c_HANDLES[eI2CNumOfBusses];
//t_i2c_node* i2c_NODES[eI2CNumOfBusses];
//e_I2C_API_Buses BusOfDevice[NumOfDevices]={eI2CofTPS82,eI2CofTPS86,eI2CofPB,eI2CofPB};

uint8_t I2C_API_Wr_Check_state;

//unsigned char state_of_I2Cpower_bus(e_I2C_API_Buses bus)
//{
//	return 0;// state_of_I2Cpower(i2c_NODES[bus]);
//}

void I2C_API_INIT(void)
{
	//eI2CofPB,eI2CofTPS82,eI2CofTPS86,eI2CNumOfBusses
//	i2c_HANDLES[eI2CofPB]=i2c_PB_HANDLE;
//	i2c_HANDLES[eI2CofTPS82]=i2c_82_HANDLE;
//	i2c_HANDLES[eI2CofTPS86]=i2c_86_HANDLE;
//	i2c_NODES[eI2CofPB]=get_i2c_ptr_node(i2c_HANDLES[eI2CofPB]);
//	i2c_NODES[eI2CofTPS82]=get_i2c_ptr_node(i2c_HANDLES[eI2CofTPS82]);
//	i2c_NODES[eI2CofTPS86]=get_i2c_ptr_node(i2c_HANDLES[eI2CofTPS86]);

	I2C_API_Reset();
};

void I2C_API_Reset(void)
{
	I2C_API_Wr_Check_state=0;
}

//extern uint16_t 
//i2cDataRW(slaveChip_e chipID, transferMode_e tMode, uint8_t slaveAddr, uint16_t subAddr,  uint8_t subAddrLen, uint8_t* pDataBuff, uint8_t len );

e_FunctionReturnState
I2C_API_Exchange( e_I2C_API_Devices d,	t_I2cRecord i2cRecord, uint8_t *buf, unsigned char priority,void (*fun)(uint8_t) )
{
  static unsigned int ReturnAddress=0;
	int ReturnAddressl=0;//in register
  static	uint8_t stateIN=0;
  static	uint8_t trcounter=0;
  static  systemticks_t systickl;
        systemticks_t	systickl1;
	e_FunctionReturnState rs;
	i2cState_e rsi2c;
	
	
// __asm 
//  (
//    "MOV %[result], lr \t\n"
//    : [result] "=r" (ReturnAddressl)
//	  : 
//	  :	
//  );
//	
//	if (0==ReturnAddress )
//	{ReturnAddress=ReturnAddressl;
//	};

//  if 	(ReturnAddress!=ReturnAddressl)
//	{ return e_FRS_Busy;
//	};

	  rs=e_FRS_Processing;
#define stateerror 6	  
    switch  (stateIN)
    {
    	case 0:systickl=SystemTicks;stateIN++;break;                  // pause
      case 1: systickl1=SystemTicks-systickl;
				      if ((systickl1)>InitPauseI2CExchangeTime) 
                            {stateIN++;systickl=SystemTicks;};	
							break; 													
			case 2:rsi2c=getI2cStatus();                                  // white idle state
				      if ((I2C_IDLE==rsi2c)||(I2C_TRANSACTION_OK==rsi2c)||(I2C_TRANSACTION_ERROR==rsi2c))
			          {stateIN++;}
							else
								{	
                  systickl1=SystemTicks-systickl;
				          if ((systickl1)>MaxI2CExchangeTime) 
                            {getI2cReset();
														 stateIN++;  
														};	
								};
							break; 										
			case 3: 	
             trcounter=2;			                                      // num of try
             stateIN++;								
						 break;		
			case 4:	systickl=SystemTicks;                                 // setup exchange
    		{i2cDataRW(i2c_Devices[d],
					           optype[i2cRecord.op_type], 
					           i2c_DevicesAddr[d],
    		             i2cRecord.addrReg,
					           i2cRecord.addrReg_qntByte,
    		             buf,
				             i2cRecord.bufRW_qntByte
										);
					stateIN++;
    		};
    		break;
    		
    	case 5:rsi2c=getI2cStatus();                                 //wait result
				  if (I2C_IDLE==rsi2c) 
            { if (0==getI2cError())
							 {rs=e_FRS_Done;
								ReturnAddress=0;
                stateIN+=2;
							 }
							 else
							 { if (--trcounter==0)
							   { 
                  stateIN++;
								 } else 
								 { stateIN=4;
									 
								 }
							 }	 
						}	 
						else 
							{
						    rs=e_FRS_Processing;
					      if (I2C_TRANSACTION_ERROR==rsi2c)	
						    { 
						      stateIN=stateerror;
						    }
								else
								{
                  systickl1=SystemTicks-systickl;
				          if ((systickl1)>MaxI2CExchangeTime) 
                            {stateIN=stateerror;};	
								};						
							};	
    			break;
    	       
			
			case 6:                                                     //error
				      rs=e_FRS_DoneError;
              ReturnAddress=0;			
						  stateIN++;
			        break;
    	default: { stateIN=0;   
                 ReturnAddress=0;				// goto idle state
				         rs=e_FRS_Idle;
				         getI2cReset();
    	           break;
    	         }
    }
		  return rs;
}

e_FunctionReturnState I2C_API_Wr_Check( e_I2C_API_Devices d,	t_I2cRecord i2cRecord, uint16_t data, unsigned char priority,void (*fun)(uint8_t))
{
	  static uint16_t dataRead;
	  static uint16_t dataWrite;
	  dataWrite=data;
	  t_I2cRecord I2cRecordRead;
	  uint8_t k,i;
	  switch (I2C_API_Wr_Check_state&1)
	  {
			case 0:                                                           // maybe this is superfluous
		  	  if (e_FRS_Done==I2C_API_Exchange(       d,
		  			  	  	  	  	  	  	  	  	i2cRecord,
	  												(uint8_t*)&dataWrite,
	  												cPriorityDefault,
													fun
	  	        									)
			  	  )
		  	  	  {I2C_API_Wr_Check_state++ ;};

	  	  	  break;
	  	  	  
	  case 1:{ I2cRecordRead=i2cRecord;I2cRecordRead.op_type=I2C_OP_READ;
	  	       if (e_FRS_Done==I2C_API_Exchange(       d,
	  	    		   	   	   	   	   	   	   	I2cRecordRead,
  												(uint8_t*)&dataRead,
												priority,
  												fun
  	        									)
		  	  )
	  	       {  k=I2cRecordRead.bufRW_qntByte;
	  	    	   for(i=0;i<I2cRecordRead.bufRW_qntByte;i++)
	  	    	      { k-=(uint8_t)(((uint8_t*)&dataRead)[i]==((uint8_t*)&data)[i]);};
	  	    	   if (0!=k) {I2C_API_Wr_Check_state++  ;}
	  	             else {I2C_API_Wr_Check_state=0xff;};
	  	       }

	  	     break;
	  	  	 };
	  };
	         if (I2C_API_Wr_Check_state==0xff) {I2C_API_Wr_Check_state=0; return e_FRS_Done;};
	         if (I2C_API_Wr_Check_state>=cWriteTryCount)  {I2C_API_Wr_Check_state=0; return e_FRS_DoneError;};

      return  e_FRS_Processing;
}


