#include <string.h>
#include "SL_CommModbus.h"
#include "SuperLoop_Comm2.h"

#include "mb.h"
#include "mbport.h"
#include "tim1.h"
#include "version.h"
#include "fs.h"

#define VERSION_MAJOR_REG 0x10
#define VERSION_MINOR_REG 0x11
#define VERSION_BUILD_REG 0x12

#define SERIAL_REG 0x30
#define ERASE_FN_EXT_REG0 0x38
#define ERASE_FN_EXT_REG1 0x39

#define ERASE_ALL_START_COIL 0x80
#define ERASE_FN_EXT_START_COIL 0x81


#define REG_HOLDING_START   1000
#define REG_HOLDING_NREGS   5


systemticks_t USBcommLastTime=-USBcommPause;

static uint8_t erase_fn_ext_reg[4]; /**< Erase filename extension */


eMBErrorCode eStatus;	
	
	
int SL_CommModbus(void)
{
    eMBPoll();
    task_modbus();		
	return 0;
}

int SL_CommModbusInit(void)
{
    eStatus = eMBInit( MB_RTU, 0x0A, 0, 15200UL, MB_PAR_NONE );
    eStatus = eMBEnable();	
    return 0;
}
 

eMBErrorCode eMBRegCoilsCB( UCHAR * pucRegBuffer, USHORT usAddress,
                            USHORT usNCoils, eMBRegisterMode eMode )
{

    
    if (eMode == MB_REG_WRITE)
    {
			 USBcommLastTime=SystemTicks-USBcommPause+USBcommPauseErase;
			 if (!SLC_FFSEnable())   //get error if FFS is busy
			 {   
					 return MB_ENORES;
			 };

			 if (usAddress == (ERASE_ALL_START_COIL+1))
        {
            return spiffs_erase_all();
        }
        
        if (usAddress == (ERASE_FN_EXT_START_COIL+1) &&
            erase_fn_ext_reg[0] != 0)
        {
            int res = spiffs_erase_by_ext((const char*)erase_fn_ext_reg);
            memset(erase_fn_ext_reg,0,sizeof(erase_fn_ext_reg));
            
            return res;
        }
    }        
    return MB_ENOREG;
}

eMBErrorCode eMBRegDiscreteCB( UCHAR * pucRegBuffer, USHORT usAddress, USHORT usNDiscrete )
{
	//USBcommLastTime=SystemTicks;
 //   USBcommLastTimeSet=true;
	
    return MB_ENOREG;
}

eMBErrorCode eMBRegInputCB( UCHAR * pucRegBuffer, USHORT usAddress, USHORT usNRegs )
{
	
//	 USBcommLastTime=SystemTicks;
//   USBcommLastTimeSet=true;
	
    uint8_t reg = pucRegBuffer[0];
    
    if(reg >= VERSION_MAJOR_REG && reg <= VERSION_BUILD_REG)
    {
        for(uint16_t i = 0; i<  usNRegs; i++)
        {
            switch(reg + i)
            {
                case VERSION_MAJOR_REG: 
                      pucRegBuffer[i << 1] = VERSION_MAJOR >> 8;
                      pucRegBuffer[(i << 1)+1] = VERSION_MAJOR & 0xff;
                break;
                
                case VERSION_MINOR_REG:
                      pucRegBuffer[i << 1] = VERSION_MINOR >> 8;
                      pucRegBuffer[(i << 1)+1] = VERSION_MINOR & 0xff;
                break;
                
                case VERSION_BUILD_REG:
                     pucRegBuffer[i << 1] = VERSION_BUILD >> 8;
                     pucRegBuffer[(i << 1)+1] = VERSION_BUILD & 0xff;
                break;
                
                default:  return MB_ENOREG;
            }
        }
        
      return MB_ENOERR;
    }
    
        
    if(reg >= SERIAL_REG && reg < (SERIAL_REG + 6))
    {     

        memcpy((void*)pucRegBuffer,(const void*)SERIAL, 2+2+4+4);
        return MB_ENOERR;
    }
    
    return MB_ENOREG;
}


eMBErrorCode eMBRegHoldingCB( UCHAR * pucRegBuffer, USHORT usAddress,
                              USHORT usNRegs, eMBRegisterMode eMode )
{
    
    
    if (eMode == MB_REG_WRITE && 
        (usAddress == (ERASE_FN_EXT_REG0+1) || usAddress == (ERASE_FN_EXT_REG1+1)))
    {
			USBcommLastTime=SystemTicks-USBcommPause+USBcommPauseErase; 
			if (!SLC_FFSEnable())  //get error if FFS is busy
			 {   
					 return MB_ENORES;
			 }; 
			
        uint8_t offset = (usAddress - (ERASE_FN_EXT_REG0+1))<<1;
        if ((offset + (usNRegs<<1)) > sizeof(erase_fn_ext_reg))
        {
            return MB_EINVAL;
        }
        
        for(uint8_t iter = 0; iter < usNRegs; iter++)
        {
            erase_fn_ext_reg[offset+ (iter<<1) ] = pucRegBuffer[1 + (iter << 1)];
            erase_fn_ext_reg[offset + (iter<<1) + 1 ] = pucRegBuffer[iter << 1];
        }
        
        return MB_ENOERR;
    }   
    
    return MB_ENOREG;
}
