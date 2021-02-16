/**
 * DriverBQ28z610.c
 *
 *  Created on: Feb 21, 2019
 *      Author: 2382
 STM32 it is little endian only.
 */


#include "DriverBQ28z610.h"
#include "i2c_API.h"

const t_I2c16InitData BQ28z610InitData[e_BQ28z610_NumOfReg]=
{
		{{I2C_OP_READ,0x06,		1,2},0}  // e_BQ28z610_Temperature
	 ,{{I2C_OP_READ,0x08,		1,2},0}  //e_BQ28z610_Voltage
	 ,{{I2C_OP_READ,0x2c,		1,2},0}  //e_BQ28z610_RelativeStateOfCharge
	 ,{{I2C_OP_WRITE,0x3e,		1,2},0}  //e_BQ28z610_AltManufacturerAccess
	 ,{{I2C_OP_WRITE,0x60,		1,2},0}  //MACDataSum
	 ,{{I2C_OP_READ,0x0A,		1,2},0}   //e_BQ28z610_BatteryStatus
};

void* DriverBQ28z610_KEY; // for multi task

	static uint8_t DriverBQ28z610_FSMs_state=0;
	static t_I2cRecord DriverBQ28z610_I2cRecord;//	


static uint8_t BQ28z610_ExchangeData[36];

e_FunctionReturnState BQ28z610_AltManufacturerAccessCommand(uint16_t command, void* key)
{
//	static uint16_t commandl;
	e_FunctionReturnState wrstate;

	
	if (0==DriverBQ28z610_KEY)  // for multi task
	  {DriverBQ28z610_KEY = key;}
	else if (DriverBQ28z610_KEY!=key) 
		  return e_FRS_Busy;
	
	
	DriverBQ28z610_I2cRecord=BQ28z610InitData[e_BQ28z610_AltManufacturerAccess].I2cRecord;
	DriverBQ28z610_I2cRecord.op_type=I2C_OP_WRITE;
	*((uint16_t*)BQ28z610_ExchangeData)=command;
		wrstate=I2C_API_Exchange(   bq28z610,
	   	   	   	   	   				DriverBQ28z610_I2cRecord,
					                BQ28z610_ExchangeData,
									cPriorityDefault,
									voidfun8
								);
	  
	if ((wrstate==e_FRS_Done) || (wrstate==e_FRS_DoneError))  // for multi task
		{DriverBQ28z610_KEY=0;
		};
		
		return  wrstate;	
};



/**

see https://e2e.ti.com/support/power-management/f/196/t/632332?BQ28Z610-I-can-t-write-Data-Flash
*/
e_FunctionReturnState BQ28z610_AltManufacturerAccessDFWrite(uint16_t address, uint8_t * data, uint8_t size,void* key)
{

	e_FunctionReturnState rstatel,wrstate;
	uint8_t sum;	
	
	
	if (0==DriverBQ28z610_KEY)  // for multi task
	  {DriverBQ28z610_KEY = key;}
	else if (DriverBQ28z610_KEY!=key) 
		  return e_FRS_Busy;
	
	
	
  rstatel=e_FRS_Processing;
	
#define stError 5
  switch(DriverBQ28z610_FSMs_state)
  {
   case 0:
			if (size>32)
			{
				size=32;
				//strcopy(GlobalErrorString,"BQ28z610_AltManufacturerAccessDF, size>32");
			};
		 
			DriverBQ28z610_I2cRecord=BQ28z610InitData[e_BQ28z610_AltManufacturerAccess].I2cRecord;
			DriverBQ28z610_I2cRecord.op_type=I2C_OP_WRITE;
			DriverBQ28z610_I2cRecord.bufRW_qntByte=size+2;
			sum=0;
			for (uint8_t i=0;i<size;i++)
			{ 
				BQ28z610_ExchangeData[i+2]=data[i];
			};		 
			*((uint16_t*)BQ28z610_ExchangeData)=address;
			DriverBQ28z610_FSMs_state++;
			//break;
	 case 1:
  		wrstate=I2C_API_Exchange(   bq28z610,
	   	   	   	   	   				DriverBQ28z610_I2cRecord,
					                BQ28z610_ExchangeData,
									cPriorityDefault,
									voidfun8
								);
	 if (e_FRS_Done==wrstate)
		 {DriverBQ28z610_FSMs_state++;
		 };
	 if (e_FRS_DoneError==wrstate)
		 {DriverBQ28z610_FSMs_state=stError;
		 };
		 break;
	 case 2:	
      sum=0;		 
			for (uint8_t i=0;i<size+2;i++)
			{ 
				sum+=BQ28z610_ExchangeData[i];
			};		 
	    BQ28z610_ExchangeData[0]=0xff-sum;
			BQ28z610_ExchangeData[1]=size+4;
			DriverBQ28z610_FSMs_state++;
	    DriverBQ28z610_I2cRecord=BQ28z610InitData[e_BQ28z610_MACDataSum].I2cRecord;
			// break;
	 case 3:
			wrstate=I2C_API_Exchange(   bq28z610,
	   	   	   	   	   				DriverBQ28z610_I2cRecord,
					                BQ28z610_ExchangeData,
									cPriorityDefault,
									voidfun8
								);
	   if (e_FRS_Done==wrstate)
		  {DriverBQ28z610_FSMs_state++;
		  };
	 	 if (e_FRS_DoneError==wrstate)
			{DriverBQ28z610_FSMs_state=stError;
			};
		 break;
	 case 4:
		 DriverBQ28z610_KEY=0; // for multi task
	   rstatel=e_FRS_Done;
	   DriverBQ28z610_FSMs_state=0;
	   break;
	 case 5://stError
		 DriverBQ28z610_KEY=0; // for multi task
	   rstatel=e_FRS_DoneError;
	   DriverBQ28z610_FSMs_state=0;
	   break;
   default: DriverBQ28z610_FSMs_state=0;
	};
	
  return rstatel;	


	
	

	
	

		return  wrstate;	
};


e_FunctionReturnState BQ28z610_Read(e_BQ28z610_Registers reg, uint16_t *data,void* key)
{
	static uint16_t dataRead;
	e_FunctionReturnState wrstate;

	
		
	if (0==DriverBQ28z610_KEY)  // for multi task
	  {DriverBQ28z610_KEY = key;}
	else if (DriverBQ28z610_KEY!=key) 
		  return e_FRS_Busy;
	

	
	DriverBQ28z610_I2cRecord=BQ28z610InitData[reg].I2cRecord;
	DriverBQ28z610_I2cRecord.op_type=I2C_OP_READ;
		wrstate=I2C_API_Exchange(   bq28z610,
	   	   	   	   	   				DriverBQ28z610_I2cRecord,
					                (uint8_t*)&dataRead,
									cPriorityDefault,
									voidfun8
								);

		if (e_FRS_Done==wrstate)
		                         {*data=dataRead;
														  DriverBQ28z610_KEY=0; // for multi task
														 };
		if (e_FRS_DoneError==wrstate)
		                         {*data=0xfffe;
															DriverBQ28z610_KEY=0; // for multi task
														 };		
 //   if ((dataRead>4000)&&(reg==0))
	//	                         {*data=0xffff;};														 
		return  wrstate;
}

void BQ28z610_DriverReset(void)
{
	DriverBQ28z610_KEY=0;
	DriverBQ28z610_FSMs_state=0;
}
