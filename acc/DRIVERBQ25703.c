/*
 * DRIVERBQ25703.c
 *
 *  Created on: Feb 13, 2019
 *      Author: 2382
 */
#include "DriverBQ25703.h"
#include "i2c_API.h"
#include "regBQ25703A.h"

#define cWriteTryCount 6


//typedef enum {eWaitBus,eDR25703Processing,eDONE} t_eDriverFunctionInternalState;


static uint8_t internalstate;
e_FunctionReturnState returnstate;
uint8_t BQ25703_Wr_Check_state;


//------------------------------------------------------------------------------------------------------------------------------
//Middle level function
//------------------------------------------------------------------------------------------------------------------------------


void BQ25703_DriverReset(void)
{
	internalstate=0;//ChargeOption2;//TODO debug
	returnstate=e_FRS_Idle;
	I2C_API_Reset();

};
/*
e_FunctionReturnState BQ25703_Init()
{

	e_FunctionReturnState returnstateL;
    uint16_t data;
	//if (eDone==returnstate) {internalstate=0;returnstateL=eProcessing;returnstate=eProcessing;}
	switch (internalstate)
	{
//I2C_API_Exchange( e_I2C_API_Devices d,	t_I2cRecord i2cRecord, uint8_t *buf, unsigned char priority,void (*fun)(uint8_t) )

	case ChargeOption3://eBQ25703_NumOfReg
	   { returnstateL=e_FRS_Done;internalstate=0;break;}
	default:
	        { if (I2C_OP_READ!=bq25703InitData[internalstate].I2cRecord.op_type)
	        	{
	        	data=bq25703InitData[internalstate].data;
	        	if (e_FRS_Done==I2C_API_Exchange(       bq25703,
												bq25703InitData[internalstate].I2cRecord,
												(uint8_t*)&data,
												cPriorityDefault,
												voidfun8
	        									)
	        		)
		                                      {internalstate++;};
	        	}
	        	else
	        	{
	        		internalstate++;
	        	};
	         returnstateL=e_FRS_Processing;
	         break;
	         };

	};
	return returnstateL;
}
*/
e_FunctionReturnState BQ25703_Init_Check(void)
{
	e_FunctionReturnState returnstateL;
	e_FunctionReturnState wrstate;
    static  uint16_t data;
	//if (eDone==returnstate) {internalstate=0;returnstateL=eProcessing;returnstate=eProcessing;}
	switch (internalstate)
	{
//I2C_API_Exchange( e_I2C_API_Devices d,	t_I2cRecord i2cRecord, uint8_t *buf, unsigned char priority,void (*fun)(uint8_t) )

	case eBQ25703_NumOfReg://ChargeCurrent://eBQ25703_NumOfReg://ChargeOption3://
	{ returnstateL=e_FRS_Done;internalstate=0;break;}
	case 0xff:{ returnstateL=e_FRS_DoneError;internalstate=0;break;}
//	case ChargeCurrent:
//		data=bq25703InitData[internalstate].data;
//		if (e_FRS_Done==I2C_API_Exchange(       bq25703,
//														bq25703InitData[internalstate].I2cRecord,
//														(uint8_t*)&data,
//														cPriorityDefault,
//														voidfun8
//			        									)
//			        		)
//				                                      {internalstate++;}
//            break;
	default:
	        { if (I2C_OP_READ!=bq25703InitData[internalstate].I2cRecord.op_type)
	        	{
	        	data=bq25703InitData[internalstate].data;
	        	wrstate=BQ25703_Wr_Check(       bq25703,
												bq25703InitData[internalstate].I2cRecord,
												data,
												cPriorityDefault,
												voidfun8
	        									);
	        	if (e_FRS_Done==wrstate)
	        	                         {internalstate++;};
	        	if (e_FRS_DoneError==wrstate) 
                                         {internalstate=0xff;};
	        	}
	        	else
	        	{
	        		internalstate++;
	        	};
	         returnstateL=e_FRS_Processing;
	         break;
	         };

	};
	return returnstateL;

}


e_FunctionReturnState BQ25703_Write_Check(bq25703Registers reg, uint16_t data)
{
e_FunctionReturnState wrstate;
uint16_t data16=data;
wrstate=BQ25703_Wr_Check(       bq25703,
												bq25703InitData[reg].I2cRecord,
												data16,
												cPriorityDefault,
												voidfun8
	        			);
return  wrstate;

}


e_FunctionReturnState BQ25703_Charge_Check(uint16_t I)
{
e_FunctionReturnState wrstate;
uint16_t data16=BQ25703_ChargeCurrent_Eval(I);
wrstate=BQ25703_Wr_Check(       bq25703,
												bq25703InitData[ChargeCurrent].I2cRecord,
												data16,
												cPriorityDefault,
												voidfun8
	        			);
return  wrstate;

}

e_FunctionReturnState BQ25703_IIN_Check(uint16_t I)
{
	e_FunctionReturnState wrstate;
	uint16_t data16=BQ25703_IIN_HOST_Eval(I);
	wrstate=BQ25703_Wr_Check(       bq25703,
													bq25703InitData[IIN_HOST].I2cRecord,
													data16,
													cPriorityDefault,
													voidfun8
		        			);
	return  wrstate;
	 ;//charge, mA
}


e_FunctionReturnState BQ25703_SetBits_Check(bq25703Registers reg, uint16_t set, uint16_t reset)
{
	e_FunctionReturnState wrstate;
	static uint16_t data16;
    data16=(bq25703InitData[reg].data)|(set);
    data16&=(~(reset));
	wrstate=BQ25703_Wr_Check(       bq25703,
									bq25703InitData[reg].I2cRecord,
									data16,
									cPriorityDefault,
									voidfun8
			        			);
		return  wrstate;


}


e_FunctionReturnState BQ25703_SetMode_Check(t_eDR703_mode mode)
{
	e_FunctionReturnState wrstate;
	static uint16_t data16;
	switch (mode)
	{
	case e_DR703_Sleep:
		 data16=(bq25703InitData[ChargeOption0].data)|(0x8001);
		 break;
	case e_DR703_HiZ:
		 data16=(bq25703InitData[ChargeOption0].data)|(0x8001);// the same as sleep
		 break;
	case e_DR703_Inhibit:
		 data16=((bq25703InitData[ChargeOption0].data)|(0x0001))&(~(0x8000));
		  break;
	case e_DR703_Work:
		 data16=(bq25703InitData[ChargeOption0].data)&(~(0x8001));
		 break;
    case e_DR703_NoS:
        return e_FRS_Idle;
	}
	wrstate=BQ25703_Wr_Check(       bq25703,
								bq25703InitData[ChargeOption0].I2cRecord,
								data16,
								cPriorityDefault,
								voidfun8
		        			);
	return  wrstate;
	;//sleep
}


e_FunctionReturnState BQ25703_ADCIBAT_Read(uint16_t *Ich,uint16_t *Idch)	//I, mA
{
	e_FunctionReturnState wrstate;
	static uint16_t data16;
	wrstate= BQ25703_Read(ADCIBAT, &data16);
	if (e_FRS_Done==wrstate)
	{
		*Ich=BQ25703_IBAT_CH(data16);
		*Idch=BQ25703_IBAT_DCH(data16);
	}
	return  wrstate;
}

e_FunctionReturnState BQ25703_ADCVSYSVBAT_Read(uint16_t *Vsys,uint16_t *Vbat)//V. mV
{
	e_FunctionReturnState wrstate;
	static uint16_t data16;
	wrstate= BQ25703_Read(ADCVSYSVBAT, &data16);
	if (e_FRS_Done==wrstate)
	{
		*Vsys=BQ25703_VSYS(data16);
		*Vbat=BQ25703_VBAT(data16);
	}
	return  wrstate;
};

//------------------------------------------------------------------------------------------------------------------------------
//Low level function
//------------------------------------------------------------------------------------------------------------------------------

e_FunctionReturnState BQ25703_Read(bq25703Registers reg, uint16_t *data)
{   static uint16_t dataRead;
	e_FunctionReturnState wrstate;
	t_I2cRecord I2cRecordRead;
	I2cRecordRead=bq25703InitData[reg].I2cRecord;
	I2cRecordRead.op_type=I2C_OP_READ;
	wrstate=I2C_API_Exchange(   bq25703,
   	   	   	   	   				I2cRecordRead,
				                (uint8_t*)&dataRead,
								cPriorityDefault,
								voidfun8
							);

	if (e_FRS_Done==wrstate)
	                         {*data=dataRead;};
	return  wrstate;
	;//sleep
}

e_FunctionReturnState BQ25703_Wr_Check( e_I2C_API_Devices d,	t_I2cRecord i2cRecord, uint16_t data, unsigned char priority,void (*fun)(uint8_t))
{
	  static uint16_t dataRead;
	  static uint16_t dataWrite;
	  dataWrite=data;
	  t_I2cRecord I2cRecordRead;
	  uint8_t k,i;
	  e_FunctionReturnState frsl;
	  switch (BQ25703_Wr_Check_state&1)
	  {
	  case 0:{
		  	     frsl=I2C_API_Exchange(       d,
		  			  	  	  	  	  	  	  	  	i2cRecord,
	  												(uint8_t*)&dataWrite,
	  												cPriorityDefault,
	  												voidfun8
	  	        									);
			  	  
					if (e_FRS_Done==frsl)
		  	  	  {BQ25703_Wr_Check_state++ ;};
          if (e_FRS_DoneError==frsl)
						  {BQ25703_Wr_Check_state=0; return e_FRS_DoneError;};
	  	  	  break;
	  	  	  };
	  case 1:{ I2cRecordRead=i2cRecord;I2cRecordRead.op_type=I2C_OP_READ;
	  	       frsl=I2C_API_Exchange(       d,
	  	    		   	   	   	   	   	   	   	I2cRecordRead,
  												(uint8_t*)&dataRead,
												priority,
  												fun
  	        									);
		  	  
			      if (e_FRS_Done==frsl)
	  	       {  k=I2cRecordRead.bufRW_qntByte;
	  	    	   for(i=0;i<I2cRecordRead.bufRW_qntByte;i++)
	  	    	      { k-=(uint8_t)(((uint8_t*)&dataRead)[i]==((uint8_t*)&data)[i]);};
	  	    	   if (0!=k) {BQ25703_Wr_Check_state++  ;}
	  	             else {BQ25703_Wr_Check_state=0xff;};
	  	       };
            if (e_FRS_DoneError==frsl)
						{
							BQ25703_Wr_Check_state=0; return e_FRS_DoneError;
						};
	  	     break;
	  	  	 };
	  };
	         if (BQ25703_Wr_Check_state==0xff)
	                                {BQ25703_Wr_Check_state=0; return e_FRS_Done;};
	         if (BQ25703_Wr_Check_state>=cWriteTryCount)
	                                {BQ25703_Wr_Check_state=0; return e_FRS_DoneError;};

      return  e_FRS_Processing;
}
