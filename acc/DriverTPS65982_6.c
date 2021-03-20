/*
 * DriverTPS65982_6.c
 *
 *  Created on: Mar 8, 2019
 *      Author: RD
 */

#include "DriverTPS65982_6.h"
#include <stdint.h>

#define MaxTXSourceCapabilities 1 //must be agreed with tps65986.PRJ !!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!
#define MaxTXSinkCapabilities 3   //must be agreed with tps65986.PRJ !!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!

#define VbusStatus 20
#define PP_EXTswitch 12
#define PP_HVswitch 10
#define StatusRegister_Maska ((3<<PP_EXTswitch)|(3<<VbusStatus))
#define StatusRegister_MaskaHV ((3<<PP_HVswitch)|(3<<VbusStatus))

#define StatusRegister_Maska_neg   ((3<<VbusStatus))


#define VBUS_is_at_other_PD_negotiated_power_level 2
#define VBUS_is_a_vSafe5V 1
#define PP_EXT_switch_enabled 3
#define StatusRegister_5VIn    ((PP_EXT_switch_enabled<<PP_EXTswitch)|(VBUS_is_a_vSafe5V<<VbusStatus))
#define StatusRegister_OtherIn ((PP_EXT_switch_enabled<<PP_EXTswitch)|( VBUS_is_at_other_PD_negotiated_power_level<<VbusStatus))
#define StatusRegister_5VInHV    ((PP_EXT_switch_enabled<<PP_HVswitch)|(VBUS_is_a_vSafe5V<<VbusStatus))
#define StatusRegister_OtherInHV ((PP_EXT_switch_enabled<<PP_HVswitch)|( VBUS_is_at_other_PD_negotiated_power_level<<VbusStatus))

#define StatusRegister_5V_neg    ((VBUS_is_a_vSafe5V<<VbusStatus))
#define StatusRegister_Other_neg (( VBUS_is_at_other_PD_negotiated_power_level<<VbusStatus))




const t_I2c16InitData TPS65982_6InitData[e_TPS65982_6_NumOfReg]=
{
		{{I2C_OP_READ,0x35,		1,5},0}, // e_TPS65982_6_ActiveRDO                  TPS659827       //4
		{{I2C_OP_READ,0x1A,		1,8+1},0}, // e_TPS65982_6_StatusRegister             TPS659827       //8
		{{I2C_OP_READ,0x3F,		1,3},0}, //  e_TPS65982_6_PowerStatusRegister       TPS659827       //2
		{{I2C_OP_READ,0x34,		1,7},0}, // e_TPS65982_6_ActivePDO                  TPS659827       //6
		{{I2C_OP_WRITE,0x8,		1,4+1},0}, // e_TPS65982_6_Cmd1                     TPS659827       //4cc
		{{I2C_OP_WRITE,0x9,		1,64+1},0}, // e_TPS65982_6_Data1                   TPS659827       //64
		{{I2C_OP_WRITE,0x10,		1,4+1},0}, // e_TPS65982_6_Cmd2                   TPS659827       //4cc
		{{I2C_OP_WRITE,0x11,		1,64+1},0}, // e_TPS65982_6_Data2                 TPS659827       //64
		{{I2C_OP_WRITE,0x32,		1,31+1},0}, // e_TPS65982_6_TXSourceCapabilities  TPS659827       //64  
		{{I2C_OP_WRITE,0x33,		1,57+1},0}, // e_TPS65982_6_TXSinkCapabilities    TPS659827       //57
		{{I2C_OP_WRITE,0x03,		1,4+1},0},//e_TPS65982_6_Mode,                    TPS659827       //4cc
		//{{I2C_OP_WRITE,0x28,		1,17+1},0},//e_TPS65982_6_SystemConfigurationRegister   TPS659827 // 8 bytes
  	{{I2C_OP_WRITE,0x28,		1,8+1},0},//e_TPS65982_6_PortConfigurationRegister   TPS659827 // 8 bytes
		{{I2C_OP_WRITE,0x18,		1,11+1},0},//e_TPS65987_IntClear1   TPS659827 // 11 bytes
		{{I2C_OP_WRITE,0x14,		1,11+1},0},//e_TPS65987_IntEvent1   TPS659827 // 11 bytes
		{{I2C_OP_READ, 0x3f,		1,2+1},0},//e_TPS65987_PowerStatusRegister
		{{I2C_OP_READ, 0x2D,		1,12+1},0},//e_TPS65987_BootFlagsRegister
	
};

const t_TPS_CMD TPS_CMD[e_TPS_CMD_NumOfEl]=
{
	{"SRDY",2,1},//e_TPS_CMD_SRDY   //Data_qntByte only 0 or 1
	{"SRYR",0,0},//e_TPS_CMD_SRYR
	{"SWSr",0,0},//e_TPS_CMD_SWSr
	{"SWSk",0,0},//e_TPS_CMD_SWSk
	{"DBfg",0,0},//e_TPS_CMD_DBfg
	{"FLrr",0,0},//e_TPS_CMD_DBfg
	{"FLem",0,0},//e_TPS_CMD_DBfg
	{"FLad",0,0},//e_TPS_CMD_DBfg
	{"FLwd",0,0},//e_TPS_CMD_DBfg
	{"FLvy",0,0},//e_TPS_CMD_DBfg
	{"GAID",0,0},//e_TPS_CMD_DBfg
};


static void* Driver6598x_KEY; // for multi task


static uint8_t internalstate;
static uint8_t internalstate1;

uint8_t data[65];
uint32_t data32;

e_FunctionReturnState TPS65982_6_CMD(e_I2C_API_Devices device,e_TPS65982_6_CMD CMD, void* key)
{
	//standard response assumed!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!
	e_FunctionReturnState returnstateL;
	//static uint8_t d;
    returnstateL=e_FRS_Processing;
    uint8_t k;
    //d=0x0f;
	
		if (0==Driver6598x_KEY)  // for multi task
	  {Driver6598x_KEY = key;}
	  else if (Driver6598x_KEY!=key) 
		       return e_FRS_Busy;

    switch (internalstate1)
	{
	case 0:if (0!=TPS_CMD[CMD].Data_qntByte)
	         {
		     data[0]=TPS_CMD[CMD].Data_qntByte;data[1]=TPS_CMD[CMD].Data;
	         if (e_FRS_Done==TPS65982_6_RW(device,e_TPS65982_6_Data1,data,2,I2C_OP_WRITE,key))
	            {
	        	 internalstate1++;
	            };
	         }
	        else {internalstate1++;};
	         break;
	case 1:data[0]=4;for(k=0;k<4;k++) {data[k+1]=TPS_CMD[CMD].CMD[k];};
	         if (e_FRS_Done==TPS65982_6_RW(device,e_TPS65982_6_Cmd1,data,5,I2C_OP_WRITE,key))
	         {
	        	 internalstate1++;
	         };
	         break;
	case 2:
            if (e_FRS_Done==TPS65982_6_RW(device,e_TPS65982_6_Cmd1,data,5,I2C_OP_READ,key))
			         {
			        	if ((0==data[1])&&(0==data[2])&&(0==data[3])&&(0==data[4]))
			        		{
			        		internalstate1++;
			        		};
			        	if (('!'==data[1])&&('C'==data[2])&&('M'==data[3])&&('D'==data[4]))
			        		{
			        		 returnstateL=e_FRS_DoneError;internalstate1=0;Driver6598x_KEY=0;
			        	    };

			         };
			         break;
	case 3:
            if (e_FRS_Done==TPS65982_6_RW(device,e_TPS65982_6_Data1,data,2,I2C_OP_READ,key))
			         {
			        	if ((0==data[1]))
			        		{
			        		internalstate1++;
			        		}
			        	else {returnstateL=e_FRS_DoneError;internalstate1=0;Driver6598x_KEY=0;}
			         };
			         break;

	 default: internalstate1=0; returnstateL=e_FRS_Done;Driver6598x_KEY=0;
	};
	return returnstateL;

}

extern  e_FunctionReturnState 
TPS65982_6_CMD_U(e_I2C_API_Devices device,e_TPS65982_6_CMD CMD, uint8_t *dataWR, 
                                    uint8_t qntByteWR,uint8_t *dataRD, uint8_t qntByteRD, void* key)
{
	//standard response assumed!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!
	e_FunctionReturnState returnstateL,returnstateL1 ;
	//static uint8_t d;
    returnstateL=e_FRS_Processing;
    uint8_t k;
    //d=0x0f;
	
	
		if (0==Driver6598x_KEY)  // for multi task
	  {Driver6598x_KEY = key;}
	  else if (Driver6598x_KEY!=key) 
		       return e_FRS_Busy;
	
	
    switch (internalstate1)
	{
	case 0:if (0!=qntByteWR)
	       {
//		     	data[0]=TPS_CMD[CMD].Data_qntByte;data[1]=TPS_CMD[CMD].Data;
						returnstateL1=TPS65982_6_RW(device,e_TPS65982_6_Data1,dataWR,qntByteWR,I2C_OP_WRITE,key);	 
						if (e_FRS_Done==returnstateL1)
						{
							internalstate1++;
						};
						if (e_FRS_DoneError==returnstateL1)
						{
							internalstate1=101;
						};
	       }
	       else {internalstate1++;};
		break;
	case 1:data[0]=4;for(k=0;k<4;k++) {data[k+1]=TPS_CMD[CMD].CMD[k];};
	       returnstateL1=TPS65982_6_RW(device,e_TPS65982_6_Cmd1,data,5,I2C_OP_WRITE,key);
	         if (e_FRS_Done==returnstateL1)
	         {
	        	 internalstate1++;
	         };
	         if (e_FRS_DoneError==returnstateL1)
	         {
	        	 internalstate1=101;
	         };
	         break;
	case 2:returnstateL1=TPS65982_6_RW(device,e_TPS65982_6_Cmd1,data,5,I2C_OP_READ,key);
            if (e_FRS_Done==returnstateL1)
			         {
			        	if ((0==data[1])&&(0==data[2])&&(0==data[3])&&(0==data[4]))
			        		{
			        		internalstate1++;
			        		};
			        	if (('!'==data[1])&&('C'==data[2])&&('M'==data[3])&&('D'==data[4]))
			        		{
			        		 returnstateL=e_FRS_DoneError;internalstate1=0;
			        	    };

			         };
						if (e_FRS_DoneError==returnstateL1)
							internalstate1=101;
			    break;
	case 3: returnstateL1=TPS65982_6_RW(device,e_TPS65982_6_Data1,dataRD,qntByteRD,I2C_OP_READ,key);
            if (e_FRS_Done==returnstateL1)
			         {
			       		internalstate1=100;
			         };
						if (e_FRS_DoneError==returnstateL1)
							internalstate1=101;
			    break;
  case 100://done
			internalstate1=0; 
	    returnstateL=e_FRS_Done;
	    Driver6598x_KEY=0;
		break;		
  case 101://error
			internalstate1=0; 
	    returnstateL=e_FRS_DoneError;
	    Driver6598x_KEY=0;
		break;		
	 default: internalstate1=0; returnstateL=e_FRS_Done;
	};
	return returnstateL;
	
};


e_FunctionReturnState
TPS65982_6_RW(e_I2C_API_Devices device, e_TPS65982_6_Registers reg, uint8_t *data, 
                                                 uint8_t qntByte, uint8_t RW, void* key)
{
	//static uint8_t dataRead[6];
	e_FunctionReturnState wrstate;
	t_I2cRecord I2cRecordRead;
	
//	if (0==Driver6598x_KEY)  // for multi task
//	  {Driver6598x_KEY = key;}
//	  else if (Driver6598x_KEY!=key) 
//		       return e_FRS_Busy;

	
	
	I2cRecordRead=TPS65982_6InitData[reg].I2cRecord;
	if (255!=qntByte) {I2cRecordRead.bufRW_qntByte=qntByte;};
	I2cRecordRead.op_type=RW;
		wrstate=I2C_API_Exchange(   device,
	   	   	   	   	   				I2cRecordRead,
					                data,
									cPriorityDefault,
									voidfun8
								);

//		if ((e_FRS_Done==wrstate)||(e_FRS_DoneError==wrstate))
//		                         {Driver6598x_KEY=0;};
		return  wrstate;
};



const uint16_t USBCurrents[4]={500,1500,3000,900};

e_FunctionReturnState TPS65982_6_RDO_R(e_I2C_API_Devices device,  uint16_t* I, uint16_t* V, void* key) // mA, mV
{
	e_FunctionReturnState returnstateL,returnstateL1;

  if (0==Driver6598x_KEY)  // for multi task
	  {Driver6598x_KEY = key;}
	  else if (Driver6598x_KEY!=key) 
		       return e_FRS_Busy;

	
	
    returnstateL=e_FRS_Processing;
	switch (internalstate)
	{
	  case 0: data[0]=0;internalstate++; break;
		case 1:returnstateL1=TPS65982_6_RW(device,e_TPS65982_6_StatusRegister,data,255,I2C_OP_READ,key);
	        	if (e_FRS_Done==returnstateL1)
    			{data32=(data[1])+((data[2])<<8)+((data[3])<<16)+((data[4])<<24);
	        	 if (
	        	    (StatusRegister_5V_neg==(data32&StatusRegister_Maska_neg))||
	        	    (StatusRegister_Other_neg==(data32&StatusRegister_Maska_neg))
					)
    				{
    			    	internalstate++;
    				}
    			else
    				{
    				*I=0;
    				*V=0;
    				returnstateL=e_FRS_Done;
						Driver6598x_KEY=0;
    				internalstate=0;
    				};
			    };
					if (e_FRS_DoneError==returnstateL1)
						{
    				*I=0;
    				*V=0;
    				returnstateL=e_FRS_Done;
						Driver6598x_KEY=0;	
    				internalstate=0;
    				};
			    
      		  break;
	        
	case 2: returnstateL1=TPS65982_6_RW(device,e_TPS65982_6_PowerStatusRegister,data,255,I2C_OP_READ,key);
		      if (e_FRS_Done==returnstateL1)
	           {data32=(data[1])+((data[2])<<8);
	            if (3==(data32&3))
	              {if (3==((data32>>2)&3))
	              	  {
	            	  	  internalstate++;
	              	  }
	               else
	              	  {
	            	  *I=USBCurrents[(data32>>2)&3];
	            	  *V=5000;
	            	  returnstateL=e_FRS_Done;
									Driver6598x_KEY=0;
	            	  internalstate=0;
	              	  }
	              }
	            else
	            {
    				*I=0;
    				*V=0;
    				returnstateL=e_FRS_Done;
						Driver6598x_KEY=0;
    				internalstate=0;
	            }
	           };
				if (e_FRS_DoneError==returnstateL1)
						{
    				*I=0;
    				*V=0;
    				returnstateL=e_FRS_Done;
						Driver6598x_KEY=0;
    				internalstate=0;
    				};		 
 			   break;

	case 3:   returnstateL1=TPS65982_6_RW(device,e_TPS65982_6_ActivePDO,data,255,I2C_OP_READ,key); 
            if (e_FRS_Done==returnstateL1)
	        	{data32=(data[1])+((data[2])<<8)+((data[3])<<16)+((data[4])<<24);
            	 *I=(data32&0x3ff)*10;
            	 *V=((data32>>10)&0x3ff)*50;
            	 returnstateL=e_FRS_Done;
							 Driver6598x_KEY=0;
            	 internalstate=0;
	        	};
						if (e_FRS_DoneError==returnstateL1)
						{
    				*I=0;
    				*V=0;
    				returnstateL=e_FRS_Done;
						Driver6598x_KEY=0;
    				internalstate=0;
    				};		 
 			   	  break;
	default:
		      internalstate=0;Driver6598x_KEY=0;

	};
	return returnstateL;
}


/*
e_FunctionReturnState TPS65982_6_RDO(e_I2C_API_Devices device,  uint16_t* I, uint16_t* V) // mA, mV
{
	e_FunctionReturnState returnstateL;

    returnstateL=e_FRS_Processing;
	switch (internalstate)
	{
	case 0:
	        { data[0]=0;
	        	if (e_FRS_Done==TPS65982_6_RW(device,e_TPS65982_6_StatusRegister,data,255,I2C_OP_READ))
    			{data32=(data[1])+((data[2])<<8)+((data[3])<<16)+((data[4])<<24);
	        	 if (
	        	    (StatusRegister_5VIn==(data32&StatusRegister_Maska))||
	        	    (StatusRegister_OtherIn==(data32&StatusRegister_Maska))||
	        	    (StatusRegister_5VInHV==(data32&StatusRegister_MaskaHV))||
	        	    (StatusRegister_OtherInHV==(data32&StatusRegister_MaskaHV))

					)
    				{
    			    	internalstate++;
    				}
    			else
    				{
    				*I=0;
    				*V=0;
    				returnstateL=e_FRS_Done;
    				internalstate=0;
    				};
			    };
      		  break;
	        };
	case 1: if (e_FRS_Done==TPS65982_6_RW(device,e_TPS65982_6_PowerStatusRegister,data,255,I2C_OP_READ))
	           {data32=(data[1])+((data[2])<<8);
	            if (3==(data32&3))
	              {if (3==((data32>>2)&3))
	              	  {
	            	  	  internalstate++;
	              	  }
	               else
	              	  {
	            	  *I=USBCurrents[(data32>>2)&3];
	            	  *V=5000;
	            	  returnstateL=e_FRS_Done;
	            	  internalstate=0;
	              	  }
	              }
	            else
	            {
    				*I=0;
    				*V=0;
    				returnstateL=e_FRS_Done;
    				internalstate=0;
	            }
	           };
 			   break;

	case 2:
            { if (e_FRS_Done==TPS65982_6_RW(device,e_TPS65982_6_ActivePDO,data,255,I2C_OP_READ))
	        	{data32=(data[1])+((data[2])<<8)+((data[3])<<16)+((data[4])<<24);
            	 *I=(data32&0x3ff)*10;
            	 *V=((data32>>10)&0x3ff)*50;
            	 returnstateL=e_FRS_Done;
            	 internalstate=0;
	        	};
        	  break;
         	};

	default:
		      internalstate=0;

	};
	return returnstateL;
}
*/

e_FunctionReturnState TPS65982_6_DISC(e_I2C_API_Devices device,uint8_t d, void* key)
{
	e_FunctionReturnState returnstateL;
	
	if (0==Driver6598x_KEY)  // for multi task
	  {Driver6598x_KEY = key;}
	  else if (Driver6598x_KEY!=key) 
		       return e_FRS_Busy;
	
	
	
    returnstateL=e_FRS_Processing;
	switch (internalstate)
	{
	 case 0: data[0]=1;data[1]=0x0f;
//		     if (e_FRS_Done==TPS65982_6_RW(device,e_TPS65982_6_SystemConfigurationRegister,data,2,I2C_OP_WRITE))
             {
//    	         if ((0==data[1])&&(0==data[2])&&(0==data[3])&&(0==data[4]))
    	        	 {
    	        	    	internalstate++;

    	        	 };
             };
            break;
	 case 1: data[0]=1;data[1]=d;
//		     if (e_FRS_Done==TPS65982_6_RW(device,e_TPS65982_6_SystemConfigurationRegister,data,2,I2C_OP_WRITE))
             {
//    	         if ((0==data[1])&&(0==data[2])&&(0==data[3])&&(0==data[4]))
    	        	 {
    	        	    	internalstate++;
    	        	 };
             };
            break;
	 default: internalstate=0; returnstateL=e_FRS_Done; Driver6598x_KEY=0;
	};
	return returnstateL;
}

e_FunctionReturnState TPS65982_6_PSwap(e_I2C_API_Devices device,uint8_t sink,uint8_t sourse,uint16_t I, void* key)//0 - absent, else - present
{
	e_FunctionReturnState returnstateL;
	static uint8_t d;
	
	if (0==Driver6598x_KEY)  // for multi task
	  {Driver6598x_KEY = key;}
	  else if (Driver6598x_KEY!=key) 
		       return e_FRS_Busy;

	
    returnstateL=e_FRS_Processing;
    //d=0x0f;
    switch (internalstate1)
	{
	case 0://for debug in debugger
	         if (e_FRS_Done==TPS65982_6_RW(device,e_TPS65982_6_TXSourceCapabilities,data,255,I2C_OP_READ,key))
	         {
	        	 internalstate1++;
	         };
	         break;
    case 1:  data[0]=5;
             if (0==sourse) {data[1]=0;}
             else {
            	   data[1]=MaxTXSourceCapabilities;
                   };
            data[2]=0xfc;
            data[3]=0;
            data[4]=(I/10)&0xff;
            data[5]=0x90|(((I/10)>>8)&0x03);

	         if (e_FRS_Done==TPS65982_6_RW(device,e_TPS65982_6_TXSourceCapabilities,data,6,I2C_OP_WRITE,key))
	         {
	        	 internalstate1++;
	         };
	         break;
	case 2:data[0]=1;if (0==sink) {data[1]=0;d=0x0a;} else {data[1]=MaxTXSinkCapabilities;d=0x08;};
	         if (e_FRS_Done==TPS65982_6_RW(device,e_TPS65982_6_TXSinkCapabilities,data,2,I2C_OP_WRITE,key))
	         {
	        	 internalstate1++;
	         };
	         break;
/*	case 2:  data[0]=1;if (0==sink) {data[1]=0;d=0x0a;} else {data[1]=MaxTXSinkCapabilities;d=0x08;};
	         if (e_FRS_Done==TPS65982_6_RW(device,e_TPS65982_6_TXSinkCapabilities,data,2,I2C_OP_WRITE))
	         {
	        	 internalstate1++;
	         };
	         break;
*/
    case 3:
	         if (e_FRS_Done==TPS65982_6_DISC(device,d,key))
	         { Driver6598x_KEY=key;
	        	 internalstate1++;
	         };
	         break;

	 default: internalstate1=0; returnstateL=e_FRS_Done; Driver6598x_KEY=0;
	};
	return returnstateL;
};






void TPS65982_6_DriverReset(void)
{
	internalstate=0;
	internalstate1=0;
	Driver6598x_KEY=0;
}


e_FunctionReturnState TPS6598x_DriverState(void)
{
	if (Driver6598x_KEY)
	{	return e_FRS_Busy;}
	else
	{	return e_FRS_Done;};
};

